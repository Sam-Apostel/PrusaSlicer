#include "Slic3r/App/Config/ChoiceElement.hpp"

#include "Slic3r/App/Config/ConfigItemControl.hpp"
#include "Slic3r/App/Yoga/RadioButton.hpp"
#include "Slic3r/App/Yoga/Text.hpp"

#include "Slic3r/Biz/I18N/I18N.hpp"

#include <algorithm>
#include <utility>

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ChoiceElement::ChoiceElement(
    const ConfigFormContext& context,
    std::string label,
    std::vector<ChoiceOptionSpec> options
) :
    ConfigFormElement(context), m_options(std::move(options))
{
    set_object_name("ChoiceElement");
    set_orientation(Orientation::Vertical);
    set_gap(5);

    if (m_options.empty())
        return;

    for (const ChoiceOptionSpec& option : m_options) {
        for (const auto& [key, value] : option.flags) {
            if (std::ranges::find(m_all_flags, key) == m_all_flags.end())
                m_all_flags.push_back(key);
        }
    }

    if (!label.empty())
        m_label = emplace_back<Text>(Biz::_u8(label), Render::ImguiFontType::Bold);

    for (const ChoiceOptionSpec& option : m_options) {
        m_buttons.push_back(
            emplace_back<RadioButton>(Biz::_u8(option.label), Biz::_u8(option.description))
        );
        m_group.insert_button(m_buttons.back());
    }
    m_group.set_always_checked(true);

    m_group.callbacks().checked_changed = [this](AbstractButton* current, AbstractButton*)
    {
        // set_checked() fires this too, so a refresh from the config would
        // otherwise write straight back to the config it just read.
        if (m_applying_from_config)
            return;
        for (size_t i = 0; i < m_buttons.size(); ++i) {
            if (m_buttons[i] == current) {
                apply_option(i);
                return;
            }
        }
    };

    // Disclosed settings keep their standard control, so they inherit the unit,
    // the bounds and the formatting from their own definition rather than
    // having them restated here.
    for (size_t i = 0; i < m_options.size(); ++i) {
        for (const std::string& key : m_options[i].reveals) {
            const Domain::ConfigItem* item = config_item(key);
            if (item == nullptr || context.setter == nullptr)
                continue;

            auto* row = emplace_back<Item>();
            row->set_orientation(Orientation::Horizontal);
            row->set_gap(5);
            row->set_align_items(YGAlignCenter);
            row->set_padding(Paddings(0.f, 0.f, 20.f, 0.f));

            auto* text = row->emplace_back<Text>(Biz::_u8(item->def().label));
            text->set_width(150);
            text->set_wrap_mode(Text::WrapMode::WrapElide);

            ConfigItemControl* control = ConfigItemControl::config_item_control_factory(
                row,
                row->object_count(),
                0,
                *item,
                *context.setter,
                {context.cbi_index}
            );
            if (auto* input = dynamic_cast<Item*>(control)) {
                input->set_width(150_fpx);
                input->set_max_width(200_fpx);
            }
            m_revealed.push_back(Revealed{.option_index = i, .key = key, .row = row,
                                          .control = control});
        }
    }

    refresh_from_config();
}

size_t ChoiceElement::option_from_config() const
{
    // An option matches when every flag it names holds that value. Flags it
    // does not name are don't-care here, and cleared when it is chosen -- so
    // "around perimeters" can say only that its own flag is set, and still be
    // recognised in a profile written before this control existed, where both
    // exclusive flags are on. First match wins, so which option that resolves
    // to is a property of the declared order rather than of the config.
    for (size_t i = 0; i < m_options.size(); ++i) {
        if (m_options[i].flags.empty())
            continue;
        const bool matches = std::ranges::all_of(
            m_options[i].flags,
            [this](const auto& flag) { return flag_of(flag.first) == flag.second; }
        );
        if (matches)
            return i;
    }

    // An option naming no flags matches anything, so it cannot take part in the
    // pass above without swallowing every state. It is the fallback instead:
    // "none of the above", wherever the author chose to put it.
    for (size_t i = 0; i < m_options.size(); ++i) {
        if (m_options[i].flags.empty())
            return i;
    }
    return 0;
}

void ChoiceElement::apply_option(const size_t index)
{
    if (index >= m_options.size())
        return;

    // Write every flag any option names, not just the chosen option's. Setting
    // only the ones it mentions would leave a flag from the previous choice set.
    const std::map<std::string, bool>& flags = m_options[index].flags;
    for (const std::string& key : m_all_flags) {
        const auto it = flags.find(key);
        set_flag(key, it != flags.end() && it->second);
    }
    update_disclosure(index);
}

void ChoiceElement::update_disclosure(const size_t index)
{
    for (const Revealed& revealed : m_revealed)
        revealed.row->set_visible(revealed.option_index == index);
}

void ChoiceElement::refresh_revealed()
{
    for (Revealed& revealed : m_revealed) {
        if (revealed.control == nullptr)
            continue;
        const Domain::ConfigItem* item = config_item(revealed.key);
        if (item == nullptr)
            continue;
        if (revealed.shown_value.has_value() && *revealed.shown_value == item->value())
            continue;
        revealed.shown_value = item->value();
        revealed.control->set_state(*item);
    }
}

void ChoiceElement::refresh_from_config()
{
    if (m_buttons.empty())
        return;

    refresh_revealed();

    const size_t option = option_from_config();
    if (m_shown_option == option)
        return;
    m_shown_option = option;

    m_applying_from_config = true;
    for (size_t i = 0; i < m_buttons.size(); ++i)
        m_buttons[i]->set_checked(i == option);
    m_applying_from_config = false;

    update_disclosure(option);
}

void ChoiceElement::render(const Yoga::Vec2f& pos, const Yoga::Vec2f& size)
{
    // Re-read every frame, like the rest of this UI: the settings can change
    // from an undo, a preset switch or a write elsewhere, and none of those
    // notify this element. The work is a comparison unless something changed.
    refresh_element();
    Item::render(pos, size);
}

} // namespace Slic3r::App
