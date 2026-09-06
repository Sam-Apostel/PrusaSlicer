#pragma once

#include "Slic3r/Biz/ObservableListSortFilter.hpp"
#include "Slic3r/Biz/DataObserver.hpp"
#include <Slic3r/Biz/ConfigItemContext.hpp>

#include "Slic3r/App/Config/ConfigFormSection.hpp"
#include "Slic3r/App/Config/ConfigRowItems.hpp"
#include "Slic3r/App/Yoga/ListView.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"
#include "Slic3r/App/IConfigNavigable.hpp"

#include <optional>

namespace Slic3r::App::Yoga {
class Text;
} // namespace Slic3r::App::Yoga

namespace Slic3r::Biz {
class ConfigBoxInteractor;
class IConfigBoxSetter;
} // namespace Slic3r::Biz

namespace Slic3r::App {

class ConfigSubcategoryItem :
    public Biz::DataObserver<Biz::ConfigItemContext>,
    public Yoga::Rectangle,
    public IConfigNavigable
{
    using ConfigRowListViewFactory = Yoga::ViewFactory<
        ConfigRowItems,
        Biz::ConfigItemContext,
        Biz::IConfigBoxSetter&,
        Biz::ConfigBoxInteractor&,
        size_t>;
    using ConfigRowListView =
        Yoga::ListView<ConfigRowItems, Biz::ConfigItemContext, ConfigRowListViewFactory>;

public:
    ConfigSubcategoryItem(
        size_t index,
        const Biz::ConfigItemContext& data,
        Biz::IConfigBoxSetter& cbi_container,
        Biz::ConfigBoxInteractor& cbi,
        size_t cbi_index
    );

    void navigate_to_item(const Domain::ConfigItem* config_item) override;
    void clear_navigation() override;

    void render(const Yoga::Vec2f& pos, const Yoga::Vec2f& size) override;

private:
    void on_data_update() override;

    void on_index_update() override;

    /**
     * @brief Build the custom controls registered for this group, if any.
     *
     * They render above the default rows, and the settings they claim are
     * filtered out of those rows so nothing is shown twice.
     */
    void rebuild_form_elements();

    /// Show or hide the group's body according to its heading switch.
    void apply_section_visibility();

    /**
     * @brief Hide the rows whose settings are not visible, and the group when
     *        that leaves nothing.
     *
     * Done here rather than in the rows because a row that hid itself could
     * never show itself again -- an invisible item's render does not run, and
     * its render is the only place it would learn the rule now holds.
     */
    void apply_row_visibility();

private:
    Biz::ConfigBoxInteractor& m_cbi;
    Biz::IConfigBoxSetter& m_cbi_container;
    size_t m_cbi_index{0};

    ConfigRowListView* m_rows_list_view{nullptr};
    Biz::UnsharedPointer<Biz::ObservableListSortFilter<Biz::ConfigItemContext>> m_rows_filter_list;
    Yoga::Item* m_heading{nullptr};
    Yoga::Text* m_label{nullptr};
    Yoga::Item* m_form_elements{nullptr};
    /// The setting search is pointing at, shown whatever its visibility rule says.
    std::string m_navigating_to;
    /// This group's own padding, restored when it has something to show again.
    Yoga::Paddings m_padding;
    std::optional<ConfigFormSection> m_section;
    Domain::ConfigItemDef::OptionGroup m_option_group{Domain::ConfigItemDef::OptionGroup::Unknown};
    Domain::ConfigItemDef::Category m_category{Domain::ConfigItemDef::Category::Unknown};
};

} // namespace Slic3r::App
