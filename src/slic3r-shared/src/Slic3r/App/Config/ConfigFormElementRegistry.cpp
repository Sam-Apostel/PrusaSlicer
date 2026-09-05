#include "Slic3r/App/Config/ConfigFormElementRegistry.hpp"

#include "Slic3r/App/Config/EnumCardsElement.hpp"
#include "Slic3r/App/Config/PercentSliderElement.hpp"
#include "Slic3r/App/Config/TravelAvoidanceElement.hpp"

#include <algorithm>
#include <utility>

namespace Slic3r::App {

ConfigFormElementRegistry& ConfigFormElementRegistry::instance()
{
    static ConfigFormElementRegistry registry;
    // Registered on first access rather than from a start-up hook, so there is
    // no ordering to get wrong and the CLI never pays for GUI-only state. The
    // built-ins are listed here so the set is readable in one place instead of
    // being left to static initialisers and link order.
    static const bool builtins_registered = []
    {
        register_travel_avoidance_element(registry);

        // Infill density and pattern. Density is a bounded quantity tuned by
        // feel, and pattern is a choice made by comparing the alternatives —
        // neither is served by the text field and dropdown they get by default.
        using Category    = Domain::ConfigItemDef::Category;
        using OptionGroup = Domain::ConfigItemDef::OptionGroup;
        registry.register_element(Entry{
            .category     = Category::Print_Infill,
            .option_group = OptionGroup::Print_Infill_DensityPattern,
            .claimed_keys = {"fill_density"},
            .factory =
                [](const ConfigFormContext& context)
            // 1% steps: the slider snaps, and coarser steps would make a stored
            // value that is not a multiple of the step unreachable by dragging.
            { return std::make_unique<PercentSliderElement>(context, "fill_density", 1.0); }
        });
        registry.register_element(Entry{
            .category     = Category::Print_Infill,
            .option_group = OptionGroup::Print_Infill_DensityPattern,
            .claimed_keys = {"fill_pattern"},
            .factory =
                [](const ConfigFormContext& context)
            { return std::make_unique<EnumCardsElement>(context, "fill_pattern", 3); }
        });

        // Enums whose options are compared rather than looked up. Each is a
        // short list of self-describing choices, so laying them out shows the
        // decision at a glance where a dropdown shows one option and hides the
        // rest.
        //
        // Deliberately not every enum. Without illustrations, cards only beat a
        // dropdown when the options are few and their names carry their meaning;
        // a long list of bare labels is just a dropdown that takes more room. So
        // the eight-option top/bottom fill patterns stay as they are until there
        // is artwork to compare, and degree-like enums (draft shield) keep their
        // dropdown too.
        const auto cards = [](
                               Category category,
                               OptionGroup group,
                               std::string key,
                               size_t columns = 1
                           )
        {
            registry.register_element(Entry{
                .category     = category,
                .option_group = group,
                .claimed_keys = {key},
                .factory =
                    [key, columns](const ConfigFormContext& context)
                { return std::make_unique<EnumCardsElement>(context, key, columns); }
            });
        };

        cards(Category::Print_WallsPerimeters, OptionGroup::Print_WallsPerimeters_Seams,
              "seam_position");
        cards(Category::Print_WallsPerimeters, OptionGroup::Print_WallsPerimeters_FuzzySkin,
              "fuzzy_skin");
        cards(Category::Print_BedAdhesion, OptionGroup::Print_BedAdhesion_Brim, "brim_type");
        cards(Category::Print_Supports, OptionGroup::Print_Supports_Generation,
              "support_material_style");
        cards(Category::Print_Supports, OptionGroup::Print_Supports_PatternDensity,
              "support_material_pattern");
        cards(Category::Print_LayersSurfaces, OptionGroup::Print_LayerSurfaces_Ironing,
              "ironing_type");

        // Groups that are one feature and its parameters. In each, every other
        // setting in the group already carries an enable_if naming this key
        // (see apply_dependency_rules in ConfigDefsFDM.cpp), so collapsing the
        // group while the switch is off hides only settings that were inert.
        //
        // Deliberately not every group whose first setting is a boolean:
        //
        // - automatic_infill_combination and automatic_extrusion_widths pick
        //   between automatic and manual rather than switching a feature off.
        //   Their groups hold the manual values, which apply while the box is
        //   *clear*; collapsing on "off" would hide the half that is in use.
        // - travel_ramping_lift and filament_multitool_ramming do gate their
        //   groups, but nothing in the config says so yet. A collapse resting
        //   on this list alone silently hides a setting the day someone adds
        //   one to the group, so the rule comes first.
        registry.register_section_toggle(SectionToggle{
            .category     = Category::Print_LayersSurfaces,
            .option_group = OptionGroup::Print_LayerSurfaces_Ironing,
            .key          = "ironing"
        });
        registry.register_section_toggle(SectionToggle{
            .category     = Category::Print_MultiMaterial,
            .option_group = OptionGroup::Print_MultiMaterial_WipeTower,
            .key          = "wipe_tower"
        });
        registry.register_section_toggle(SectionToggle{
            .category     = Category::Print_MultiMaterial,
            .option_group = OptionGroup::Print_MultiMaterial_OozePrevention,
            .key          = "ooze_prevention"
        });

        return true;
    }();
    (void) builtins_registered;
    return registry;
}

void ConfigFormElementRegistry::register_element(Entry entry)
{
    m_entries.push_back(std::move(entry));
}

std::vector<const ConfigFormElementRegistry::Entry*> ConfigFormElementRegistry::elements_for(
    const Domain::ConfigItemDef::Category category,
    const Domain::ConfigItemDef::OptionGroup option_group
) const
{
    std::vector<const Entry*> found;
    for (const Entry& entry : m_entries) {
        if (entry.category == category && entry.option_group == option_group)
            found.push_back(&entry);
    }
    return found;
}

void ConfigFormElementRegistry::register_section_toggle(SectionToggle toggle)
{
    m_section_toggles.push_back(std::move(toggle));
}

const ConfigFormElementRegistry::SectionToggle* ConfigFormElementRegistry::section_toggle_for(
    const Domain::ConfigItemDef::Category category,
    const Domain::ConfigItemDef::OptionGroup option_group
) const
{
    // Backwards, so a later registration replaces an earlier one. A group has
    // one switch; a plugin registering a gate for a group that already has one
    // is taking it over, not adding a second.
    for (auto it = m_section_toggles.rbegin(); it != m_section_toggles.rend(); ++it) {
        if (it->category == category && it->option_group == option_group)
            return &*it;
    }
    return nullptr;
}

bool ConfigFormElementRegistry::is_claimed(
    const Domain::ConfigItemDef::Category category,
    const Domain::ConfigItemDef::OptionGroup option_group,
    const std::string& key
) const
{
    const SectionToggle* toggle = section_toggle_for(category, option_group);
    if (toggle != nullptr && toggle->key == key)
        return true;

    return std::any_of(
        m_entries.begin(),
        m_entries.end(),
        [&](const Entry& entry)
        {
            return entry.category == category && entry.option_group == option_group
                && entry.claimed_keys.contains(key);
        }
    );
}

} // namespace Slic3r::App
