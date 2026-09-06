#pragma once

#include "Slic3r/App/Yoga/RectangleButton.hpp"

#include <string>

namespace Slic3r::App::Yoga {
class Icon;
class Text;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

/**
 * @brief One option of an enum, shown as a picture above its name.
 *
 * For a choice made by looking rather than by reading: infill patterns are the
 * case that asked for it. Eighteen of them as a grid of bare labels is a
 * dropdown that takes more room; as a grid of pictures it is the comparison the
 * user is actually making.
 *
 * Falls back to the label alone when there is no picture -- an unreadable or
 * missing file leaves a card that still says what it is, rather than a hole.
 * Most enums will never have artwork, and they keep their plain radio buttons
 * rather than becoming empty frames.
 */
class EnumCard : public Yoga::RectangleButton
{
public:
    /**
     * @param image Absolute path to a PNG or SVG, already checked to be inside
     *              the declaring plugin's own directory. Empty for no picture.
     */
    EnumCard(const std::string& label, const std::string& tooltip, const std::string& image);

    /// False when no picture was given or the file could not be used.
    bool has_image() const { return m_icon != nullptr; }

protected:
    void checked_updated_internal() override;
    void enabled_updated_internal() override;
    void hovered_updated_internal() override;

private:
    void update_colors();

private:
    Yoga::Icon* m_icon{nullptr};
    Yoga::Text* m_label{nullptr};
};

} // namespace Slic3r::App
