#include "Slic3r/App/Config/EnumCard.hpp"

#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/Text.hpp"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

namespace {

/// The picture is the point of a card, so it gets most of the height.
constexpr int IMAGE_SIZE = 64;

} // namespace

EnumCard::EnumCard(
    const std::string& label,
    const std::string& tooltip,
    const std::string& image
) :
    RectangleButton(tooltip)
{
    set_object_name("EnumCard");
    set_orientation(Orientation::Vertical);
    set_align_items(YGAlignCenter);
    set_gap(5.f);
    set_padding(8.f);
    set_checkable(true);
    set_background_border_width(1);

    // Checked before building the icon rather than after: Icon reports nothing
    // about whether the file loaded, so an unreadable path would leave a card
    // with an empty frame where the picture should be and no way to tell.
    boost::system::error_code ec;
    if (!image.empty() && boost::filesystem::is_regular_file(image, ec)) {
        m_icon = emplace_back<Icon>(Render::Icon::None, IMAGE_SIZE);
        m_icon->set_image(image);
        m_icon->set_fill_mode(Icon::FillMode::PreservedAspectCentered);
        m_icon->set_preserve_colors(true);
        m_icon->set_min_width(IMAGE_SIZE);
        m_icon->set_min_height(IMAGE_SIZE);
    }

    m_label = emplace_back<Text>(label);
    m_label->set_align({AlignH::Center, AlignV::Center});
    m_label->set_wrap_mode(Text::WrapMode::WrapElide);

    set_tooltip_position(Position::Bottom);

    update_colors();
}

void EnumCard::checked_updated_internal()
{
    RectangleButton::checked_updated_internal();
    update_colors();
}

void EnumCard::enabled_updated_internal()
{
    RectangleButton::enabled_updated_internal();
    update_colors();
}

void EnumCard::hovered_updated_internal()
{
    RectangleButton::hovered_updated_internal();
    update_colors();
}

void EnumCard::update_colors()
{
    // The border carries the selection, not the fill: a filled card would
    // compete with the picture it is there to show.
    set_background_color_border(m_theme->color_imgui(
        checked() ? Platform::Color::AccentTertiary : Platform::Color::Button,
        button_color_group()
    ));
    if (m_label != nullptr) {
        m_label->set_text_color(
            m_theme->color_imgui(Platform::Color::Text, button_color_group())
        );
    }
}

} // namespace Slic3r::App
