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
 * @brief Custom controls that stand in for the default one-row-per-setting form.
 *
 * This is the seam between the config and what the user sees. Anything that
 * wants to present a group of settings as something other than a list of rows
 * registers here; everything unregistered keeps rendering exactly as it does
 * today, so adopting this is per-group and reversible.
 *
 * It is also where a plugin would eventually attach. Note the factory returns a
 * widget rather than taking a draw callback: this is an immediate-mode UI whose
 * render runs every frame alongside the 3D scene, so an element is built once
 * and updated on change, never redrawn by a caller-supplied function.
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
     * Register one where a group is a feature plus its parameters: the switch
     * moves out of the rows and into the group's heading, and the rest of the
     * group collapses while it is off.
     *
     * Only register a key that every other setting in the group depends on.
     * Collapsing hides those settings, which is right when they are all inert
     * anyway and wrong the moment one of them still applies.
     */
    struct SectionToggle
    {
        Domain::ConfigItemDef::Category category{Domain::ConfigItemDef::Category::Unknown};
        Domain::ConfigItemDef::OptionGroup option_group{
            Domain::ConfigItemDef::OptionGroup::Unknown
        };
        std::string key;
    };

    /// The registry, with the built-in elements registered on first access.
    static ConfigFormElementRegistry& instance();

    /// Register an element. Later registrations render after earlier ones.
    void register_element(Entry entry);

    /// Promote a setting to its group's heading. One per group; later wins.
    void register_section_toggle(SectionToggle toggle);

    /// The gate for this group, or nullptr when it has none.
    const SectionToggle* section_toggle_for(
        Domain::ConfigItemDef::Category category,
        Domain::ConfigItemDef::OptionGroup option_group
    ) const;

    /// Elements standing in for part of this group, in registration order.
    std::vector<const Entry*> elements_for(
        Domain::ConfigItemDef::Category category,
        Domain::ConfigItemDef::OptionGroup option_group
    ) const;

    /**
     * @brief True when this setting is rendered elsewhere, so no row should.
     *
     * Covers both a custom element that has claimed it and a gate promoted to
     * the group's heading.
     */
    bool is_claimed(
        Domain::ConfigItemDef::Category category,
        Domain::ConfigItemDef::OptionGroup option_group,
        const std::string& key
    ) const;

private:
    ConfigFormElementRegistry() = default;

    std::vector<Entry> m_entries;
    std::vector<SectionToggle> m_section_toggles;
};

} // namespace Slic3r::App
