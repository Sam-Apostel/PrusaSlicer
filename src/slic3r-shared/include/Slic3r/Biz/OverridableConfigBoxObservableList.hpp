#pragma once

#include "Slic3r/Biz/OverrideItem.hpp"
#include "Slic3r/Biz/IObservableList.hpp"

namespace Slic3r::Domain {
struct ConfigValue;
} // namespace Slic3r::Domain

namespace Slic3r::Biz {

class OverridableConfigBoxObservableList : public IObservableList<OverrideItem>
{
public:
    void
    set_config_box(Domain::ConfigBox* config_box, const Domain::ConfigBox* original_config_box);

    void set_value(const std::string_view key, const Domain::ConfigValue& value);

    void set_override(const std::string& key, bool enable);

    std::pair<const Domain::ConfigValue*, std::optional<bool>> find(const std::string& name) const;

    /**
     * @brief The setting itself, not just its value and override state.
     *
     * Writing a setting goes through IConfigBoxSetter, which takes the item, so
     * a control that writes a setting it was not built from needs this.
     *
     * @return nullptr when no such setting is in this box.
     */
    const Domain::ConfigItem* find_item(const std::string& name) const;

    /**
     * @brief Whether this setting is one the preset can override.
     *
     * Such a setting's row carries the override switch, so it is not a row a
     * control can stand in for -- replacing it would take the switch away.
     */
    bool is_overridable(const std::string& name) const;

    const OverrideItem& at(size_t index) const override;
    size_t size() const override;

    bool is_dirty(const std::string& key) const;
    bool is_dirty() const;
    void set_from_original_value(const std::string& key);

private:
    using Items = std::vector<Biz::OverrideItem>;

    Domain::ConfigBox* m_config_box{nullptr};
    Items m_items;
};

} // namespace Slic3r::Biz
