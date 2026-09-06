#pragma once

#include <set>

#include "Slic3r/App/Yoga/ComboBox.hpp"
#include "Slic3r/App/Config/ConfigItemControl.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"

namespace Slic3r::Biz {
class IConfigBoxSetter;
} // namespace Slic3r::Biz

namespace Slic3r::App {

class ConfigItemComboBox : public ConfigItemControl, public Yoga::ComboBox
{
public:
    ConfigItemComboBox(
        size_t index,
        const Domain::ConfigItem& config_item,
        Biz::IConfigBoxSetter& cb_setter,
        std::vector<size_t> cbi_index
    );

    void render(const Yoga::Vec2f& pos, const Yoga::Vec2f& size) override;

protected:
    void on_data_update() override;

    /**
     * @brief Re-evaluate which of this enum's values the config rules out.
     *
     * Every frame, like the other rule evaluation: the answer depends on other
     * settings, so nothing about this one changes when it does.
     */
    void refresh_disabled_values();
    void update_value(const Domain::ConfigValue& value);
    void initialize();

private:
    Yoga::Passthrough<Yoga::IntValidator> m_int_validator;
    Yoga::Passthrough<Yoga::DoubleValidator> m_double_validator;

    const Domain::ConfigItem* m_last_item{nullptr};
    bool m_init = false;
    std::set<int> m_shown_unavailable;
};

} // namespace Slic3r::App
