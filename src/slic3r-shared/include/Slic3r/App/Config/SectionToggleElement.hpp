#pragma once

#include "Slic3r/App/Config/ConfigFormElement.hpp"

#include <string>

namespace Slic3r::App::Yoga {
class Text;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class ConfigItemControl;

/**
 * @brief A section's on/off switch, rendered in the section's heading.
 *
 * Several groups are one feature and its parameters: a boolean that turns the
 * feature on, and settings that mean nothing until it is. Rendered row by row
 * that reads as a list of unrelated settings of which most happen to be greyed
 * out -- the heading says "Ironing", the first row says "Enable ironing", and
 * the rest sit there disabled explaining nothing.
 *
 * The switch belongs to the heading, because that is what it switches. Put
 * there, "Ironing" and its switch are one line, and the parameters below are
 * plainly the parameters of the thing that line names.
 *
 * The switch is the setting's own control, so it keeps the tooltip, the revert
 * arrow and the write path a row would have given it.
 */
class SectionToggleElement : public ConfigFormElement
{
public:
    SectionToggleElement(const ConfigFormContext& context, std::string key);

    /// False when this config box has no such setting, so nothing was built.
    bool valid() const { return m_control != nullptr; }

    /**
     * @brief Whether the section this gates should show its contents.
     *
     * Switched on, and able to take effect: a gate held back by an unmet
     * requirement cannot apply, so neither can the settings that depend on it,
     * whatever value it is storing. The heading still says so -- the switch and
     * the reason it is greyed stay visible either way.
     */
    bool is_on() const { return m_on && m_applies; }

    void refresh_from_config() override;

    void render(const Yoga::Vec2f& pos, const Yoga::Vec2f& size) override;

private:
    /// Show, next to the switch, why it cannot be turned on.
    void apply_reason_text();

private:
    std::string m_key;
    ConfigItemControl* m_control{nullptr};
    Yoga::Text* m_reason{nullptr};

    bool m_on{false};
    bool m_applies{true};
    /// The item last read, to tell a real change from re-reading the same value.
    const Domain::ConfigItem* m_shown_item{nullptr};
    std::string m_shown_reason;
};

} // namespace Slic3r::App
