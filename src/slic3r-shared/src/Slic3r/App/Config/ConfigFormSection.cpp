#include "Slic3r/App/Config/ConfigFormSection.hpp"

#include "Slic3r/App/Config/ConfigFormElementRegistry.hpp"
#include "Slic3r/App/Config/SectionToggleElement.hpp"

#include <algorithm>
#include <utility>

namespace Slic3r::App {

ConfigFormSection::ConfigFormSection(Yoga::Item& heading, Yoga::Item& elements) :
    m_heading(heading), m_elements(elements)
{}

void ConfigFormSection::rebuild(
    const Domain::ConfigItemDef::Category category,
    const Domain::ConfigItemDef::OptionGroup option_group,
    const ConfigFormContext& context,
    const ClaimAllowedFn& claim_allowed
)
{
    while (m_elements.object_count() > 0)
        m_elements.remove(m_elements.get_item(0));

    if (m_toggle != nullptr) {
        m_heading.remove(m_toggle);
        m_toggle = nullptr;
    }
    m_claimed.clear();
    m_force_expanded = false;

    const auto allowed = [&claim_allowed](const std::set<std::string>& keys)
    {
        return !claim_allowed
            || std::ranges::all_of(keys, [&claim_allowed](const std::string& key)
                                   { return claim_allowed(key); });
    };

    const ConfigFormElementRegistry& registry = ConfigFormElementRegistry::instance();

    if (const ConfigFormElementRegistry::SectionToggle* toggle =
            registry.section_toggle_for(category, option_group))
    {
        const std::set<std::string> keys{toggle->key};
        if (allowed(keys)) {
            ConfigFormContext toggle_context = context;
            toggle_context.claimed_keys      = keys;
            auto* element =
                m_heading.emplace_back<SectionToggleElement>(toggle_context, toggle->key);
            // A config box that does not carry the gate gets its group back
            // unchanged, rather than an empty switch and a group that can
            // never be opened.
            if (element->valid()) {
                m_toggle = element;
                m_claimed.insert(toggle->key);
            } else {
                m_heading.remove(element);
            }
        }
    }

    for (const ConfigFormElementRegistry::Entry* entry :
         registry.elements_for(category, option_group))
    {
        if (!allowed(entry->claimed_keys))
            continue;

        // The element is told which settings it stands in for, so it can honour
        // their enable_if and requirements. Those settings get no default row,
        // so nothing else is left to enforce them.
        ConfigFormContext element_context = context;
        element_context.claimed_keys      = entry->claimed_keys;
        if (std::unique_ptr<ConfigFormElement> element = entry->factory(element_context)) {
            m_elements.append(std::move(element));
            m_claimed.insert(entry->claimed_keys.begin(), entry->claimed_keys.end());
        }
    }
}

bool ConfigFormSection::expanded() const
{
    return m_toggle == nullptr || m_force_expanded || m_toggle->is_on();
}

} // namespace Slic3r::App
