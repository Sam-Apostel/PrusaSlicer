#pragma once

#include "Slic3r/Domain/Config.hpp"

#include "Slic3r/Biz/DataObserver.hpp"

#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/App/Config/ConfigRowDependency.hpp"
#include "Slic3r/App/IConfigNavigable.hpp"

namespace Slic3r::Biz {
class IConfigBoxSetter;
} // namespace Slic3r::Biz

namespace Slic3r::App::Yoga {
class Text;
class ToggleButton;
class LayoutButton;
} // namespace Slic3r::App::Yoga

namespace Slic3r::App {

class ConfigItemControl;
class ConfigItemSpinBox;

class ConfigRowItem :
    public Biz::DataObserver<Domain::ConfigItem>,
    public Yoga::Rectangle,
    public IConfigNavigable
{
public:
    using FnEnableRevert = std::function<bool()>;

    ConfigRowItem(
        size_t index,
        const Domain::ConfigItem& data,
        Biz::IConfigBoxSetter& cb_setter,
        FnEnableRevert enable_revert_fn,
        size_t cbi_index,
        std::optional<std::string> force_label = std::nullopt
    );

    void navigate_to_item(const Domain::ConfigItem* config_item) override;
    void clear_navigation() override;

    void set_label_text_color(const ImColor& color);

    /// Enable or disable the input for a caller's own reason, such as whether an
    /// override is active. Combined with the def's own dependency rule.
    void set_enabled_control(bool enabled);

    void render(const Yoga::Vec2f& pos, const Yoga::Vec2f& size) override;

private:
    void on_data_update() override;

    /// Re-evaluate the def's rules and apply the result. Called every frame.
    void refresh_dependency_state();

    void apply_enabled_state();
    void apply_label_color();

    /// Show or hide the text naming the unmet requirement.
    void apply_reason_text();

private:
    Biz::IConfigBoxSetter& m_cb_setter;
    std::optional<bool> m_last_full_width{std::nullopt};
    size_t m_cbi_index{0};
    std::optional<std::string> m_force_label;

    Domain::ConfigItemDef::GUIType m_created_gui_type{Domain::ConfigItemDef::GUIType::undefined};
    const std::type_info* m_created_value_type{nullptr};
    Yoga::Item* m_left_side{nullptr};
    Yoga::Text* m_label{nullptr};
    Yoga::LayoutButton* m_revert_button{nullptr};
    Yoga::Item* m_input{nullptr};
    Yoga::ToggleButton* m_toggle_enable{nullptr};

    ConfigItemControl* m_control{nullptr};
    ConfigItemSpinBox* m_config_item_spin_box{
        nullptr
    }; ///< valid only if ConfigItem gui type is spinbox

    FnEnableRevert m_enable_revert{nullptr};

    // The input is disabled when either reason says so: a caller's (an inactive
    // override) or the def's own dependency rule. Kept apart so that neither can
    // silently re-enable what the other disabled.
    bool m_enabled_by_caller{true};
    ConfigRowDependency m_dependency;

    bool m_can_revert{false};

    /// Reason currently displayed, empty when every requirement holds.
    /// Built only for settings that actually declare requirements.
    Yoga::Text* m_reason{nullptr};
};

} // namespace Slic3r::App
