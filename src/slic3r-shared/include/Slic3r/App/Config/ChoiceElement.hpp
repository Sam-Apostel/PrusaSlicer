#pragma once

#include "Slic3r/App/Config/ConfigFormElement.hpp"
#include "Slic3r/App/Config/FormSpec.hpp"
#include "Slic3r/App/Yoga/ButtonGroup.hpp"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace Slic3r::App::Yoga {
class RadioButton;
class Text;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class ConfigItemControl;

/**
 * @brief Several booleans rendered as the one choice they actually are.
 *
 * A set of mutually exclusive flags is a decision the config happens to store
 * as separate switches. Two checkboxes for "avoid crossing perimeters" and
 * "avoid crossing curled overhangs" invite turning both on, which the slicer
 * then refuses; a parameter belonging to one of them sits there whichever is
 * chosen. As one radio group with progressive disclosure, the same settings
 * read as what they are: pick a strategy, then tune the one you picked.
 *
 * Nothing about the storage changes. Each option declares the value every flag
 * takes while it is chosen, which is both how a choice is written and how one
 * is recognised in a profile that was written before this control existed.
 * Revealed settings keep their own standard control, so they keep their units,
 * bounds and formatting.
 */
class ChoiceElement : public ConfigFormElement
{
public:
    ChoiceElement(
        const ConfigFormContext& context,
        std::string label,
        std::vector<ChoiceOptionSpec> options
    );

    void refresh_from_config() override;

    void render(const Yoga::Vec2f& pos, const Yoga::Vec2f& size) override;

private:
    /// Index of the option matching the config, or 0 when none does.
    size_t option_from_config() const;

    /// Write every flag any option names, so no stale one is left set.
    void apply_option(size_t index);

    void update_disclosure(size_t index);

    /**
     * @brief Push new values into the disclosed controls.
     *
     * They are not rows in the observed list, so nothing tells them their
     * setting changed under an undo or a preset switch. Comparing the value
     * first keeps this to one comparison per control in the ordinary frame.
     */
    void refresh_revealed();

private:
    struct Revealed
    {
        size_t option_index{0};
        std::string key;
        Yoga::Item* row{nullptr};
        ConfigItemControl* control{nullptr};
        std::optional<Domain::ConfigValue> shown_value;
    };

    std::vector<ChoiceOptionSpec> m_options;
    /// Every flag named by any option, so a change can clear the others.
    std::vector<std::string> m_all_flags;

    Yoga::Text* m_label{nullptr};
    Yoga::ButtonGroup m_group;
    std::vector<Yoga::RadioButton*> m_buttons;
    std::vector<Revealed> m_revealed;

    std::optional<size_t> m_shown_option;
    bool m_applying_from_config{false};
};

} // namespace Slic3r::App
