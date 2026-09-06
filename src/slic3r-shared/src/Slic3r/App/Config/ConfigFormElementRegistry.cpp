#include "Slic3r/App/Config/ConfigFormElementRegistry.hpp"

#include <utility>

namespace Slic3r::App {

ConfigFormElementRegistry& ConfigFormElementRegistry::instance()
{
    static ConfigFormElementRegistry registry;
    return registry;
}

void ConfigFormElementRegistry::set_form_controls(
    std::vector<Entry> entries,
    std::vector<SectionToggle> toggles
)
{
    m_entries         = std::move(entries);
    m_section_toggles = std::move(toggles);
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

const ConfigFormElementRegistry::SectionToggle* ConfigFormElementRegistry::section_toggle_for(
    const Domain::ConfigItemDef::Category category,
    const Domain::ConfigItemDef::OptionGroup option_group
) const
{
    // A group has one switch. Two declarations for the same group cannot both
    // be honoured, and the installer rejects the second rather than leaving
    // which one wins to the order this vector happens to be in.
    for (const SectionToggle& toggle : m_section_toggles) {
        if (toggle.category == category && toggle.option_group == option_group)
            return &toggle;
    }
    return nullptr;
}

} // namespace Slic3r::App
