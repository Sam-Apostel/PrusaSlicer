#pragma once

#include "Slic3r/Biz/IConfigItemSource.hpp"

#include "Slic3r/Domain/ConfigDef.hpp"
#include "Slic3r/Biz/IListObserver.hpp"

#include <memory>
#include <set>

namespace Slic3r::Domain {
struct ConfigBox;
struct ConfigValue;
} // namespace Slic3r::Domain

namespace Slic3r::Biz {

class OverridableConfigBoxObservableList;

class OverridableConfigBoxInteractor : public IConfigItemSource
{
public:
    struct ConfigBoxes {
        Domain::ConfigBox* editable;
        const Domain::ConfigBox* original;
    };

    class SetAccessor
    {
    public:
        void set_source(std::weak_ptr<OverridableConfigBoxObservableList> config_box_list);

        void set_value(const std::string& key, const Domain::ConfigValue& value);

        void set_override(const std::string& key, bool enable);

        void set_config_box(
            Domain::ConfigBox* config_box,
            const Domain::ConfigBox* original_config_box
        );

        void set_from_original_value(const std::string& key);

    private:
        std::weak_ptr<OverridableConfigBoxObservableList> m_config_box_list;
    };

    explicit OverridableConfigBoxInteractor(
        SetAccessor& set_accessor,
        const ConfigBoxes& config_boxes
    );
    OverridableConfigBoxInteractor();

    const Domain::ConfigValue* find(const std::string& name) const;

    /// The setting itself, for a control that writes a setting it was not built from.
    const Domain::ConfigItem* find_item(const std::string& name) const override;

    /// Whether the preset can override this setting, so its row carries a switch.
    bool is_overridable(const std::string& name) const;

    std::weak_ptr<const OverridableConfigBoxObservableList> config_box_overridable_list() const;

    std::weak_ptr<OverridableConfigBoxObservableList> config_box_overridable_list();

private:
    UnsharedPointer<OverridableConfigBoxObservableList> m_config_box_list;
};

} // namespace Slic3r::Biz
