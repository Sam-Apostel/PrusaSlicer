#include "Slic3r/App/Lua/PluginSystem.hpp"
#include "Slic3r/App/Lua/PackageRegistry.hpp"
#include "Slic3r/App/Lua/PluginDialog.hpp"
#include "Slic3r/App/Lua/ProjectApi.hpp"
#include "Slic3r/App/Config/ConfigFormElementRegistry.hpp"
#include "Slic3r/App/Config/FormSpecInstaller.hpp"
#include "Slic3r/Biz/Lua/LuaException.hpp"

#include <ranges>
#include <utility>
#include <vector>
#include <imgui/imgui.h>

namespace Slic3r::App::Lua {

PluginSystem::PluginSystem(
    std::initializer_list<std::string> plugin_paths,
    Biz::ProjectInteractor& project_interactor,
    Biz::Emboss::IFontManager& font_manager
) :
    m_plugin_paths(plugin_paths),
    m_project_interactor(project_interactor),
    m_font_manager(font_manager),
    m_slicing_runner(
        m_registry,
        project_interactor,
        [this](const std::string& id, const std::string& title, const std::string& text)
        {
            invoke_listeners<ISlicingPluginReportListener>(
                [&](auto* listener) { listener->on_slicing_plugin_report(id, title, text); }
            );
        }
    ),
    m_slicing_runner_scope(project_interactor.slicing_interactor(), m_slicing_runner)
{}

void PluginSystem::install(const std::string& zip_file_path)
{
    auto zip_source = Biz::Crypto::create_zip_source(zip_file_path);
    if (zip_source == nullptr) {
        std::string error_message = fmt::format(
            fmt::runtime(
                // TRN {} Is a path to file that can't be opened
                Biz::_u8L("Cannot open file: {}")
            ),
            zip_file_path
        );
        invoke_listeners<IPluginInstallationListener>(
            [&error_message](auto* listener)
            { listener->on_plugin_installation_error(error_message); }
        );
        return;

    }
    PluginBundle plugin_bundle{std::move(zip_source)};
    if (auto load_meta_result = plugin_bundle.load_meta(); !load_meta_result.has_value()) {
        invoke_listeners<IPluginInstallationListener>(
            [&load_meta_result](auto* listener)
            { listener->on_plugin_installation_error(load_meta_result.error()); }
        );
        return;
    }

    if (auto install_result = m_registry.install(plugin_bundle); !install_result.has_value()) {
        invoke_listeners<IPluginInstallationListener>(
            [&install_result](auto* listener)
            { listener->on_plugin_installation_error(install_result.error()); }
        );
        return;
    }

    rescan();

    invoke_listeners<IPluginInstallationListener>(
        [&plugin_bundle](auto* listener)
        { listener->on_plugin_installation_succeeded(plugin_bundle.meta()); }
    );

}

Yoga::Passthrough<PluginDialog>& PluginSystem::init_dialog()
{
    m_dialog = std::make_unique<PluginDialog>([this](const auto& meta, const auto& params)
    {
        m_current_plugin_data = std::make_optional<PluginData>(meta, params);
        finalize_run();
    });
    return m_dialog;
}

void PluginSystem::execute_plugin(const std::string& id)
{
    auto it = m_registry.plugins().find(id);
    if (it == m_registry.plugins().end()) {
        SPDLOG_ERROR("Cannot execute missing plugin id: {}", id);
        return;
    }
    const auto& plugin = it->second;
    ASSERT(m_dialog.get() != nullptr);
    m_dialog->show_plugin(
        plugin.meta(),
        m_last_plugin_data.has_value() && m_last_plugin_data->meta.id == id ?
            m_last_plugin_data->param_values :
            PluginParamValueMap{}
    );
    Biz::Platform::PlatformServices::instance().render_request_handler().request_render();
}

void PluginSystem::finalize_run()
{
    ASSERT(m_current_plugin_data.has_value());

    ProjectApi project_api(m_project_interactor, m_font_manager);
    Biz::Lua::LuaEngine lua;
    lua.open_registry([&project_api](auto& lua) { project_api.register_api(lua); });
    PackageRegistry package_registry;
    lua.open_registry([&package_registry](auto& lua) { package_registry.register_api(lua); });

    const auto& plugin = m_registry.plugins().at(m_current_plugin_data->meta.id);

    try {
        plugin.execute(lua, m_current_plugin_data->param_values);
    } catch (Biz::Lua::LuaException& e) {
        SPDLOG_ERROR(
            "Running plugin {} failed in script {}\n{}",
            plugin.meta().id,
            e.script_path(),
            e.what()
        );
    } catch (std::exception& e) {
        SPDLOG_ERROR("Running plugin {} failed\n{}", plugin.meta().id, e.what());
    }

    m_last_plugin_data = m_current_plugin_data;
    m_current_plugin_data = std::nullopt;

    m_project_interactor.undo_provider().take_snapshot(Biz::UndoSnapshotType::ExecutePlugin);
}

void PluginSystem::clear()
{
    m_registry.clear();
    m_last_plugin_data = std::nullopt;
    m_current_plugin_data = std::nullopt;
}

void PluginSystem::scan(const std::string& path)
{
    m_registry.scan(path);
}

void PluginSystem::install_form_elements()
{
    // Gathered from every form plugin and installed in one go, replacing the
    // previous set. A rescan is a fresh start -- a plugin may have been
    // removed, or edited on disk -- and rebuilding from what is there now is
    // simpler to be sure of than unpicking what each plugin contributed.
    std::vector<FormElementSpec> specs;
    for (const auto& plugin : m_registry.plugins() | std::views::values) {
        if (plugin.meta().type != PluginType::FormPlugin)
            continue;
        specs.insert(
            specs.end(), plugin.meta().form_elements.begin(), plugin.meta().form_elements.end()
        );
    }

    const FormSpecInstallReport report =
        install_plugin_form_specs(specs, ConfigFormElementRegistry::instance());
    for (const std::string& rejected : report.rejected)
        SPDLOG_ERROR("Form element not installed -- {}", rejected);
    if (report.installed > 0)
        SPDLOG_INFO("Plugins provide {} settings form control(s)", report.installed);
}

void PluginSystem::rescan()
{
    clear();
    for (const auto& path : m_plugin_paths) {
        scan(path);
    }
    install_form_elements();
    invoke_listeners<IPluginRescanListener>([this](auto* l) { l->on_plugins_scanned(m_registry); });
}

} // namespace Slic3r::App::Lua
