#include "Slic3r/App/Config/SectionToggleElement.hpp"

#include "Slic3r/App/Config/ConfigItemControl.hpp"
#include "Slic3r/App/Yoga/Text.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"

#include <utility>

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

SectionToggleElement::SectionToggleElement(const ConfigFormContext& context, std::string key) :
    ConfigFormElement(context), m_key(std::move(key))
{
    set_object_name("SectionToggleElement");
    set_orientation(Orientation::Horizontal);
    set_align_items(YGAlignCenter);
    set_gap(10);
    set_flex_shrink(0);

    const Domain::ConfigItem* item = config_item(m_key);
    if (item == nullptr || context.setter == nullptr)
        return;

    // The setting's own control rather than a bare switch, so it keeps its
    // tooltip, its revert behaviour and its write path. Only the label is
    // dropped: the heading beside it already names the feature, and "Ironing"
    // followed by "Enable ironing" says the same thing twice.
    m_control = ConfigItemControl::config_item_control_factory(
        this,
        object_count(),
        0,
        *item,
        *context.setter,
        {context.cbi_index}
    );
    if (auto* input = dynamic_cast<Item*>(m_control)) {
        // The control sizes itself for a row, where it shares a fixed column
        // with every other input. Here it is one switch at the end of a title.
        input->set_width(40);
        input->set_flex_shrink(0);
    }

    refresh_from_config();
}

void SectionToggleElement::apply_reason_text()
{
    if (m_shown_reason.empty()) {
        if (m_reason != nullptr)
            m_reason->set_visible(false);
        return;
    }

    // Built on first need. Most gates have no requirements, and the ones that
    // do are usually met, so this stays absent in the ordinary case.
    if (m_reason == nullptr) {
        m_reason = emplace<Text>(0, m_shown_reason);
        m_reason->set_wrap_mode(Text::WrapMode::WrapElide);
        m_reason->set_flex_shrink(1.f);
        m_reason->set_max_width(320);
        m_reason->set_align({AlignH::Right, AlignV::Center});
        m_reason->set_text_color(
            m_theme->color_imgui(Platform::Color::Text, Platform::ColorGroup::Disabled)
        );
    } else {
        m_reason->set_text(m_shown_reason);
    }
    m_reason->set_visible(true);
}

void SectionToggleElement::refresh_from_config()
{
    if (m_control == nullptr)
        return;

    const Domain::ConfigItem* item = config_item(m_key);
    if (item == nullptr)
        return;

    const Domain::ConfigItemRequirement* unmet = unmet_requirement(m_key);
    const std::string reason = unmet == nullptr ? std::string{} : Biz::_u8(unmet->reason);
    const bool on = item->holds_alternative<bool>() && item->get<bool>();
    // refresh_element() has already applied this to the switch; kept here so
    // the group can be asked whether its contents mean anything.
    const bool applies = any_claimed_setting_applies();

    // Nothing else pushes updates into this control -- it is not a row in the
    // observed list -- so it re-reads every frame and writes only on a real
    // change. Comparing the item too catches the config box being swapped out
    // from under us for one holding the same value.
    if (item == m_shown_item && on == m_on && applies == m_applies && reason == m_shown_reason)
        return;

    m_shown_item   = item;
    m_on           = on;
    m_applies      = applies;
    m_shown_reason = reason;

    m_control->set_state(*item);
    apply_reason_text();
}

void SectionToggleElement::render(const Yoga::Vec2f& pos, const Yoga::Vec2f& size)
{
    refresh_element();
    Item::render(pos, size);
}

} // namespace Slic3r::App
