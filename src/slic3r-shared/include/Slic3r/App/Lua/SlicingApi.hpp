#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <sol/sol.hpp>

#include "Slic3r/Biz/Lua/LuaEngine.hpp"
#include "Slic3r/Biz/Slicing/SlicingInteractor.hpp"
#include "Slic3r/Domain/GCodeExtrusionRole.hpp"
#include "Slic3r/Domain/SlicingId.hpp"

namespace Slic3r::App::Lua {

/**
 * @name Stable names for values a slicing plugin can see.
 *
 * Deliberately not the strings the UI shows. Those are translated and are
 * revised whenever the wording is improved, and a plugin comparing against
 * one would break the first time either happens -- silently, and only for
 * users in one language. These are part of the API and change only when the
 * API version does.
 * @{
 */
std::string_view extrusion_role_key(Domain::GCodeExtrusionRole role);
std::string_view warning_code_key(Biz::Slicing::WarningCode code);
std::string_view warning_severity_key(Biz::Slicing::WarningSeverity severity);
/// @}

/**
 * @brief Whether there is anything worth showing a plugin in this result.
 *
 * The same condition build_slice_view() applies, asked without a Lua state so
 * a caller can decide not to build one at all.
 */
bool has_slice_view(const Biz::Slicing::FDMResult& result);

/**
 * @brief Build the read-only view of a finished slice.
 *
 * A plain Lua table of numbers, strings and nested tables, built fresh for
 * each delivery. Nothing in it holds a reference back into the slicer, so a
 * handler that writes to the table changes only its own copy and that copy is
 * dropped when the handler returns. That is the whole of "read-only" here --
 * there is nothing to write *through*.
 *
 * Returns nothing when the result carries only the basic statistics. That is a
 * partial or pre-preview result: it has no filament totals, no cost and no
 * per-extruder breakdown, which is most of what the view is for. Reporting the
 * few fields it does have under the same shape would make every plugin check
 * each one for nil.
 */
std::optional<sol::table> build_slice_view(
    sol::state& state,
    const Domain::SlicingId& id,
    const Biz::Slicing::FDMResult& result,
    const std::vector<Biz::Slicing::Warning>& warnings
);

/**
 * @brief What a slicing plugin's script is allowed to call.
 *
 * Small on purpose. A slicing plugin observes; the only thing it can do with
 * what it observed is say it. There is no file access beyond `require` (which
 * the sandbox already confines to the plugin's own directory) and nothing that
 * reaches back into the slicer, the config or the project.
 */
class SlicingApi
{
public:
    /// Called with whatever the plugin passed to report(). Title may be empty.
    using ReportFn = std::function<void(const std::string& title, const std::string& text)>;

    SlicingApi(std::string plugin_id, ReportFn report);

    void register_api(Biz::Lua::LuaEngine& lua);

private:
    std::string m_plugin_id;
    ReportFn m_report;
};

} // namespace Slic3r::App::Lua
