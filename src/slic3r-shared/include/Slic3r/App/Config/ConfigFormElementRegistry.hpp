#pragma once

#include "Slic3r/App/Config/ConfigFormElement.hpp"
#include "Slic3r/Domain/ConfigDef.hpp"

#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace Slic3r::App {

/**
 * @brief The controls that stand in for the default one-row-per-setting form.
 *
 * This is the seam between the config and what the user sees. The form is
 * otherwise generated straight from the config -- one row per setting, in the
 * group its definition names -- which is a faithful view of the data and often
 * a poor view of the decision.
 *
 * Everything registered here is declared by a plugin, not compiled in: see
 * install_plugin_form_specs(), which is the only way anything gets in. That is
 * deliberate. A choice about how eighteen infill patterns are best presented is
 * a judgement that will change, that different people will make differently,
 * and that nobody should have to rebuild the slicer to revisit.
 *
 * Note the factory returns a widget rather than taking a draw callback: this is
 * an immediate-mode UI whose render runs every frame alongside the 3D scene, so
 * an element is built once and updated on change, never redrawn by a
 * caller-supplied function -- and certainly not by a script.
 */
class ConfigFormElementRegistry
{
public:
    using Factory = std::function<std::unique_ptr<ConfigFormElement>(const ConfigFormContext&)>;

    struct Entry
    {
        Domain::ConfigItemDef::Category category{Domain::ConfigItemDef::Category::Unknown};
        Domain::ConfigItemDef::OptionGroup option_group{
            Domain::ConfigItemDef::OptionGroup::Unknown
        };
        /// Settings this element renders. They get no default row of their own.
        std::set<std::string> claimed_keys;
        Factory factory;
    };

    /**
     * @brief A boolean that turns a whole option group on and off.
     *
     * Registered where a group is a feature plus its parameters: the switch
     * moves out of the rows and into the group's heading, and the rest of the
     * group collapses while it is off.
     */
    struct SectionToggle
    {
        Domain::ConfigItemDef::Category category{Domain::ConfigItemDef::Category::Unknown};
        Domain::ConfigItemDef::OptionGroup option_group{
            Domain::ConfigItemDef::OptionGroup::Unknown
        };
        std::string key;
    };

    static ConfigFormElementRegistry& instance();

    /**
     * @brief Replace every registered control.
     *
     * Rescanning plugins is a fresh start -- one may have been installed,
     * removed, or edited on disk -- and rebuilding from what is there now is
     * simpler to be sure of than unpicking what each plugin contributed.
     */
    void set_form_controls(std::vector<Entry> entries, std::vector<SectionToggle> toggles);

    /// Elements standing in for part of this group, in registration order.
    std::vector<const Entry*> elements_for(
        Domain::ConfigItemDef::Category category,
        Domain::ConfigItemDef::OptionGroup option_group
    ) const;

    /// The gate for this group, or nullptr when it has none.
    const SectionToggle* section_toggle_for(
        Domain::ConfigItemDef::Category category,
        Domain::ConfigItemDef::OptionGroup option_group
    ) const;

private:
    ConfigFormElementRegistry() = default;

    std::vector<Entry> m_entries;
    std::vector<SectionToggle> m_section_toggles;
};

} // namespace Slic3r::App
