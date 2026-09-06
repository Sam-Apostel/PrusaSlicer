#include "Slic3r/App/Lua/SlicingPluginRunner.hpp"

#include "Slic3r/App/Lua/PackageRegistry.hpp"
#include "Slic3r/App/Lua/SlicingApi.hpp"
#include "Slic3r/Biz/FDMResultCache.hpp"
#include "Slic3r/Biz/Lua/LuaEngine.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Log.hpp"

#include <ranges>
#include <vector>

namespace Slic3r::App::Lua {

SlicingPluginRunner::SlicingPluginRunner(
    const PluginRegistry& registry,
    Biz::ProjectInteractor& project_interactor,
    ReportFn report
) :
    m_registry(registry),
    m_project_interactor(project_interactor),
    m_report(std::move(report))
{}

void SlicingPluginRunner::on_status_changed(
    const Biz::Slicing::StatusUpdate status,
    const Domain::SlicingId id
)
{
    // Only a completed slice. A cancelled one ends in Modified, a failed one
    // in InvalidData, and one stopped at a step stays Modified -- none of them
    // reaches here, which is how "does not run on a cancelled slice" is
    // arranged rather than checked for.
    if (status.code != Biz::Slicing::StatusCode::Finished) {
        return;
    }
    deliver_sliced(id);
}

void SlicingPluginRunner::deliver_sliced(const Domain::SlicingId id)
{
    std::vector<const Plugin*> watching;
    for (const Plugin& plugin : m_registry.plugins() | std::views::values) {
        if (plugin.meta().type != PluginType::SlicingPlugin)
            continue;
        const auto& events = plugin.meta().slicing_events;
        if (std::ranges::find(events, SlicingEvent::Sliced) != events.end())
            watching.push_back(&plugin);
    }
    // The common case, and the one that has to cost nothing: no slicing plugin
    // is installed, so a finished slice does no work beyond this loop.
    if (watching.empty()) {
        return;
    }

    const std::optional<Biz::FDMResultRef> result{
        m_project_interactor.fdm_result_cache().get_result(id)
    };
    if (!result.has_value()) {
        // The result is dispatched to the main thread before the status is, so
        // it is normally already cached by now. Not worth an error if it is
        // not: nothing is broken, there is simply nothing to report on.
        SPDLOG_DEBUG("Slicing plugins: no result cached for the finished slice");
        return;
    }

    if (!has_slice_view(result->get())) {
        // Basic statistics only -- a pre-preview or partial result, with no
        // filament totals in it. Asked before an engine is built rather than
        // discovered once per plugin.
        SPDLOG_DEBUG("Slicing plugins: the finished slice has no full statistics");
        return;
    }

    std::vector<Biz::Slicing::Warning> warnings;
    if (const auto status = m_project_interactor.status_cache().get_status(id)) {
        warnings = status->warrnings;
    }

    for (const Plugin* plugin : watching) {
        const std::string& plugin_id = plugin->meta().id;

        // An engine per plugin, not one shared between them. Two plugins that
        // saw each other's globals would be able to break each other in ways
        // neither author could reproduce, and the order they run in is an
        // implementation detail of a std::map.
        //
        // The engine is declared last so it is destroyed first: both objects
        // above it are bound into the state, and must outlive it rather than
        // the other way round.
        SlicingApi api{
            plugin_id,
            [this, &plugin_id](const std::string& title, const std::string& text)
            { m_report(plugin_id, title, text); }
        };
        PackageRegistry package_registry;
        Biz::Lua::LuaEngine lua;
        lua.open_registry([&api](auto& engine) { api.register_api(engine); });
        lua.open_registry([&package_registry](auto& engine)
                          { package_registry.register_api(engine); });

        const std::optional<sol::table> slice =
            build_slice_view(lua.state(), id, result->get(), warnings);
        ASSERT(slice.has_value(), "has_slice_view() said there was one");

        if (const auto delivered = plugin->deliver(lua, SlicingEvent::Sliced, *slice);
            !delivered.has_value())
        {
            // Logged against the plugin and then dropped. The slice is already
            // done and the G-code already exists; there is nothing here that
            // could be rolled back, and the next plugin still gets its turn.
            SPDLOG_ERROR("Slicing plugin {} failed on 'sliced': {}", plugin_id, delivered.error());
        }
    }
}

} // namespace Slic3r::App::Lua
