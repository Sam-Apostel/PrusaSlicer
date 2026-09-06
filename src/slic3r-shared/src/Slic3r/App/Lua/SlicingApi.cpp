#include "Slic3r/App/Lua/SlicingApi.hpp"

#include "Slic3r/Domain/PrintStatistics.hpp"
#include "Slic3r/Log.hpp"

#include <algorithm>
#include <variant>

namespace Slic3r::App::Lua {

namespace {

/**
 * @brief Put a vector of numbers into a Lua array.
 *
 * One-based, because that is what `#t` and `ipairs` agree with. Every array in
 * the view is built this way, so a plugin author has one convention to learn
 * rather than one per field.
 */
template <class T>
sol::table numbers(sol::state& state, const std::vector<T>& values)
{
    sol::table t = state.create_table(static_cast<int>(values.size()), 0);
    for (size_t i = 0; i < values.size(); ++i)
        t[i + 1] = static_cast<double>(values.at(i));
    return t;
}

/// The same, for strings.
sol::table strings(sol::state& state, const std::vector<std::string>& values)
{
    sol::table t = state.create_table(static_cast<int>(values.size()), 0);
    for (size_t i = 0; i < values.size(); ++i)
        t[i + 1] = values.at(i);
    return t;
}

/// The value at `index`, or 0 when the vector is shorter than the extruder count.
double at_or_zero(const std::vector<float>& values, const size_t index)
{
    return index < values.size() ? static_cast<double>(values.at(index)) : 0.0;
}

sol::table time_table(sol::state& state, const Domain::TimeStatistics& time)
{
    sol::table t = state.create_table();
    t["total"]       = static_cast<double>(time.time);
    t["first_layer"] = static_cast<double>(time.first_layer_time);
    return t;
}

sol::table filament_table(sol::state& state, const Domain::FullPrintStatistics& stats)
{
    sol::table filament = state.create_table();
    filament["total_mm"]   = static_cast<double>(stats.total_used_filament_mm);
    filament["total_cm3"]  = static_cast<double>(stats.total_used_filament_cm3);
    filament["total_g"]    = static_cast<double>(stats.total_used_filament_g);
    filament["total_cost"] = static_cast<double>(stats.total_filament_cost);

    // Per extruder, one entry per extruder that has any of the four numbers,
    // so `#slice.filament.per_extruder` is the count a plugin can loop over
    // rather than four vectors it has to reconcile. They are filled from the
    // same source and are the same length in practice; a short one reads as
    // zero rather than as a gap in the array.
    const size_t extruders = std::max(
        {stats.used_filament_per_extruder_mm.size(),
         stats.used_filament_per_extruder_cm3.size(),
         stats.used_filament_per_extruder_g.size(),
         stats.filament_cost_per_extruder.size()}
    );
    sol::table per_extruder = state.create_table(static_cast<int>(extruders), 0);
    for (size_t i = 0; i < extruders; ++i) {
        sol::table one = state.create_table();
        one["mm"]   = at_or_zero(stats.used_filament_per_extruder_mm, i);
        one["cm3"]  = at_or_zero(stats.used_filament_per_extruder_cm3, i);
        one["g"]    = at_or_zero(stats.used_filament_per_extruder_g, i);
        one["cost"] = at_or_zero(stats.filament_cost_per_extruder, i);
        per_extruder[i + 1] = one;
    }
    filament["per_extruder"] = per_extruder;

    // Keyed by role rather than an array, because the roles present depend on
    // the model -- an array would make position mean nothing.
    sol::table per_role = state.create_table();
    for (const auto& [role, used] : stats.used_filaments_per_role) {
        sol::table one = state.create_table();
        one["mm"] = static_cast<double>(used.first);
        one["g"]  = static_cast<double>(used.second);
        per_role[std::string{extrusion_role_key(role)}] = one;
    }
    filament["per_role"] = per_role;

    sol::table wipe_tower = state.create_table();
    wipe_tower["mm"]   = static_cast<double>(stats.total_used_filament_for_wipe_tower_mm);
    wipe_tower["cm3"]  = static_cast<double>(stats.total_used_filament_for_wipe_tower_cm3);
    wipe_tower["g"]    = static_cast<double>(stats.total_used_filament_for_wipe_tower_g);
    wipe_tower["cost"] = static_cast<double>(stats.total_wipe_tower_cost);
    filament["wipe_tower"] = wipe_tower;

    return filament;
}

} // namespace

std::string_view extrusion_role_key(const Domain::GCodeExtrusionRole role)
{
    switch (role) {
    case Domain::GCodeExtrusionRole::None:                     return "none";
    case Domain::GCodeExtrusionRole::Perimeter:                return "perimeter";
    case Domain::GCodeExtrusionRole::ExternalPerimeter:        return "external_perimeter";
    case Domain::GCodeExtrusionRole::OverhangPerimeter:        return "overhang_perimeter";
    case Domain::GCodeExtrusionRole::InternalInfill:           return "internal_infill";
    case Domain::GCodeExtrusionRole::SolidInfill:              return "solid_infill";
    case Domain::GCodeExtrusionRole::TopSolidInfill:           return "top_solid_infill";
    case Domain::GCodeExtrusionRole::Ironing:                  return "ironing";
    case Domain::GCodeExtrusionRole::BridgeInfill:             return "bridge_infill";
    case Domain::GCodeExtrusionRole::GapFill:                  return "gap_fill";
    case Domain::GCodeExtrusionRole::Skirt:                    return "skirt_brim";
    case Domain::GCodeExtrusionRole::SupportMaterial:          return "support_material";
    case Domain::GCodeExtrusionRole::SupportMaterialInterface: return "support_material_interface";
    case Domain::GCodeExtrusionRole::WipeTower:                return "wipe_tower";
    case Domain::GCodeExtrusionRole::Custom:                   return "custom";
    case Domain::GCodeExtrusionRole::Count:                    break;
    }
    return "unknown";
}

std::string_view warning_code_key(const Biz::Slicing::WarningCode code)
{
    switch (code) {
    case Biz::Slicing::WarningCode::None: return "none";
    case Biz::Slicing::WarningCode::BedTempsDiffer: return "bed_temps_differ";
    case Biz::Slicing::WarningCode::BedTempsChanged: return "bed_temps_changed";
    case Biz::Slicing::WarningCode::FilamentShrinkageDiffer: return "filament_shrinkage_differ";
    case Biz::Slicing::WarningCode::WipeTowerNozzleDiameterDiffer:
        return "wipe_tower_nozzle_diameter_differ";
    case Biz::Slicing::WarningCode::SupportNozzleDiameterDiffer:
        return "support_nozzle_diameter_differ";
    case Biz::Slicing::WarningCode::SupportsTurnedOff: return "supports_turned_off";
    case Biz::Slicing::WarningCode::StabilityIssues: return "stability_issues";
    case Biz::Slicing::WarningCode::EmptyLayers: return "empty_layers";
    case Biz::Slicing::WarningCode::CustomGCodeReservedKeywords:
        return "custom_gcode_reserved_keywords";
    case Biz::Slicing::WarningCode::InvalidToolchange: return "invalid_toolchange";
    case Biz::Slicing::WarningCode::CloseToPrimingRegions: return "close_to_priming_regions";
    case Biz::Slicing::WarningCode::ToolpathOutsideBuildVolume:
        return "toolpath_outside_build_volume";
    case Biz::Slicing::WarningCode::GCodeConflict: return "gcode_conflict";
    case Biz::Slicing::WarningCode::XYSizeCompensationIgnoredMultiMaterialPainting:
        return "xy_size_compensation_ignored_multi_material_painting";
    case Biz::Slicing::WarningCode::XYSizeCompensationIgnoredFuzzySkinPainting:
        return "xy_size_compensation_ignored_fuzzy_skin_painting";
    }
    return "unknown";
}

std::string_view warning_severity_key(const Biz::Slicing::WarningSeverity severity)
{
    switch (severity) {
    case Biz::Slicing::WarningSeverity::LOW: return "low";
    case Biz::Slicing::WarningSeverity::HIGH: return "high";
    }
    return "unknown";
}

bool has_slice_view(const Biz::Slicing::FDMResult& result)
{
    return std::holds_alternative<Domain::FullPrintStatistics>(result.print_statistics);
}

std::optional<sol::table> build_slice_view(
    sol::state& state,
    const Domain::SlicingId& id,
    const Biz::Slicing::FDMResult& result,
    const std::vector<Biz::Slicing::Warning>& warnings
)
{
    const auto* stats = std::get_if<Domain::FullPrintStatistics>(&result.print_statistics);
    if (stats == nullptr) {
        return std::nullopt;
    }

    sol::table slice = state.create_table();
    slice["bed"]         = static_cast<double>(id.bed_instance_id);
    slice["extruders"]   = static_cast<double>(result.extruders_count);
    slice["spiral_vase"] = result.spiral_vase_enabled;
    slice["sequential"]  = result.sequential_print;
    slice["toolchanges"] = static_cast<double>(stats->total_toolchanges);

    sol::table time = state.create_table();
    time["normal"] = time_table(state, stats->normal_mode_time);
    if (stats->silent_mode_time.has_value())
        time["silent"] = time_table(state, *stats->silent_mode_time);
    slice["time"] = time;

    slice["filament"] = filament_table(state, *stats);

    sol::table filaments = state.create_table();
    filaments["initial"]  = stats->initial_filament_type;
    filaments["printing"] = strings(state, stats->printing_filament_types);
    slice["filament_types"] = filaments;

    // Extruder ids as the config numbers them, unshifted, so they can be
    // compared against a setting. The per-extruder arrays above are the ones
    // that are one-based; mixing the two conventions in one table would be a
    // trap, so they are kept under names that say which is which.
    slice["extruder_ids"] = numbers(state, stats->printing_extruders);
    slice["initial_extruder_id"] = static_cast<double>(stats->initial_extruder_id);

    sol::table warning_list = state.create_table(static_cast<int>(warnings.size()), 0);
    for (size_t i = 0; i < warnings.size(); ++i) {
        const Biz::Slicing::Warning& warning = warnings.at(i);
        sol::table one = state.create_table();
        one["code"]     = std::string{warning_code_key(warning.code)};
        one["severity"] = std::string{warning_severity_key(warning.severity)};
        warning_list[i + 1] = one;
    }
    slice["warnings"] = warning_list;

    return slice;
}

SlicingApi::SlicingApi(std::string plugin_id, ReportFn report) :
    m_plugin_id(std::move(plugin_id)),
    m_report(std::move(report))
{}

void SlicingApi::register_api(Biz::Lua::LuaEngine& lua)
{
    sol::state& state = lua.state();

    // Lua's own print writes to stdout, which on a windowed build is nowhere a
    // user will look. Routed to the log instead, tagged with the plugin, so a
    // plugin author debugging one has somewhere to read.
    state["print"] = [id = m_plugin_id](sol::this_state ts, sol::variadic_args args)
    {
        sol::state_view view(ts);
        const sol::function to_string = view["tostring"];
        std::string line;
        for (const auto arg : args) {
            if (!line.empty())
                line += '\t';
            const sol::object value = arg;
            const std::string text = to_string(value);
            line += text;
        }
        SPDLOG_INFO("Plugin {}: {}", id, line);
    };

    state["report"] = [this](std::string first, sol::optional<std::string> second)
    {
        // report(text) and report(title, text). A title on its own would be a
        // notification with nothing in it, so a single argument is the body.
        if (second.has_value())
            m_report(first, *second);
        else
            m_report({}, first);
    };
}

} // namespace Slic3r::App::Lua
