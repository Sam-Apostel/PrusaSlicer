#include "Slic3r/App/Config/OverridableSubcategoryItem.hpp"

#include "Slic3r/Domain/Config.hpp"

#include "Slic3r/Biz/OverridableConfigBoxInteractor.hpp"
#include "Slic3r/Biz/OverridableConfigBoxObservableList.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

#include "Slic3r/App/Config/ConfigRowVisibility.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

OverridableSubcategoryItem::OverridableSubcategoryItem(
    size_t index,
    const Biz::OverrideItem& data,
    Biz::IConfigBoxSetter& cbi_container,
    Biz::OverridableConfigBoxInteractor& cbi,
    size_t cbi_index
) :
    Biz::DataObserver<Biz::OverrideItem>(index, data),
    m_cbi(cbi),
    m_cbi_container(cbi_container),
    m_cbi_index(cbi_index),
    m_rows_filter_list(std::make_shared<Biz::ObservableListSortFilter<Biz::OverrideItem>>())
{
    set_object_name("OverridableSubcategoryItem");
    set_orientation(Orientation::Vertical);
    set_flex_shrink(0);
    set_flags(ImDrawFlags_None);
    set_rounding(0);
    set_gap(0);
    m_padding = Paddings(20.f);
    set_padding(m_padding);

    // The heading is a row rather than a bare label, because a group whose
    // settings all hang off one switch puts that switch here, beside the name
    // of the thing it switches.
    m_heading = emplace_back<Item>();
    m_heading->set_orientation(Orientation::Horizontal);
    m_heading->set_align_items(YGAlignCenter);
    m_heading->set_gap(10);

    m_label = m_heading->emplace_back<Text>(
        Biz::_u8(Domain::ConfigItemDef::translate_option_group(m_state->config_item->def().option_group)),
        Render::ImguiFontType::Bold
    );
    m_label->set_flex_grow(1);
    m_label->set_flex_shrink(1);
    m_label->set_wrap_mode(Text::WrapMode::WrapElide);

    // Custom controls render above the default rows. Built before the row list
    // so that they keep that position as the list changes.
    m_form_elements = emplace_back<Item>();
    m_form_elements->set_orientation(Orientation::Vertical);
    m_form_elements->set_gap(5);
    m_form_elements->set_padding(20);

    m_section.emplace(*m_heading, *m_form_elements);

    m_rows_filter_list->set_filter_fn(
        [this](const Biz::OverrideItem& item) -> bool
        {
            if (item.config_item->def().option_group != m_option_group)
                return false;
            const bool in_this_category = item.config_item->def().category == m_category
                || (item.is_override()
                    && m_category == Domain::ConfigItemDef::Category::Filament_Overrides);
            // A setting a custom control renders gets no row of its own.
            return in_this_category && !m_section->renders(item.name);
        }
    );
    // also group by row_group
    m_rows_filter_list->set_group_by_fn(
        [](const Biz::OverrideItem& item, std::unordered_set<std::string>& seen_keys) -> bool
        {
            const std::string& row_group = item.config_item->def().row_group;
            if (row_group.empty()) {
                return false;
            } else {
                if (seen_keys.contains(row_group)) {
                    return true;
                } else {
                    seen_keys.insert(row_group);
                    return false;
                }
            }
        }
    );
    m_rows_filter_list->set_sort_fn(
        [](const Biz::OverrideItem& lhs, const Biz::OverrideItem& rhs)
        { return lhs.config_item->def().order < rhs.config_item->def().order; }
    );

    m_rows_filter_list->set_source_model(m_cbi.config_box_overridable_list());

    m_rows_list_view = emplace_back<ConfigRowListView>(
        ConfigRowListViewFactory{m_cbi_container, m_cbi, m_cbi_index}
    );
    m_rows_list_view->set_source_list(m_rows_filter_list.get());
    m_rows_list_view->set_orientation(Orientation::Vertical);
    m_rows_list_view->set_gap(0);
    m_rows_list_view->set_padding(Paddings(20.f, 20.f, 20.f, 0.f));

    on_index_update();
    on_data_update();
}

void OverridableSubcategoryItem::navigate_to_item(const Domain::ConfigItem* config_item)
{
    const std::string& name = config_item->name();
    for (size_t row_index = 0; row_index < m_rows_filter_list->size(); ++row_index) {
        if (m_rows_filter_list->at(row_index).name == name) {
            // Being pointed at a row inside a collapsed or hidden group has to
            // open it, or the search highlights something nobody can see.
            m_section->set_force_expanded(true);
            m_navigating_to = name;
            apply_section_visibility();
            m_rows_list_view->item_at(row_index)->navigate_to_item(config_item);
            break;
        }
    }
}

void OverridableSubcategoryItem::clear_navigation()
{
    m_section->set_force_expanded(false);
    m_navigating_to.clear();
    apply_section_visibility();
    for (size_t row_index = 0; row_index < m_rows_list_view->object_count(); ++row_index) {
        m_rows_list_view->item_at(row_index)->clear_navigation();
    }
}

void OverridableSubcategoryItem::apply_section_visibility()
{
    const bool expanded = m_section->expanded();
    m_form_elements->set_visible(expanded);
    m_rows_list_view->set_visible(expanded);
}

void OverridableSubcategoryItem::apply_row_visibility()
{
    const bool any_visible = apply_visibility_to_rows(
        *m_rows_filter_list, *m_rows_list_view, m_cbi_container, m_navigating_to
    );

    const bool anything_to_show =
        any_visible || m_form_elements->object_count() > 0 || m_section->has_toggle();
    m_heading->set_visible(anything_to_show);
    set_padding(anything_to_show ? m_padding : Yoga::Paddings(0.f));
}

void OverridableSubcategoryItem::render(const Yoga::Vec2f& pos, const Yoga::Vec2f& size)
{
    apply_section_visibility();
    apply_row_visibility();
    Rectangle::render(pos, size);
}

void OverridableSubcategoryItem::rebuild_form_elements()
{
    // A row here is also where a filament override is switched on, and a
    // control edits the value only -- so a control may not stand in for a row
    // whose setting is overridable, the same rule the print form applies to
    // per-tool overrides.
    m_section->rebuild(
        m_category,
        m_option_group,
        ConfigFormContext{&m_cbi_container, &m_cbi, m_cbi_index, {}},
        // Asked of the interactor rather than of the filtered rows: this runs
        // before the filter is invalidated for the new group, so those rows are
        // still the previous group's and would answer about the wrong settings.
        [this](const std::string& key) { return !m_cbi.is_overridable(key); }
    );
}

void OverridableSubcategoryItem::on_data_update()
{
    const Domain::ConfigItem* config_item = m_state->config_item;

    m_label->set_text(
        Biz::_u8(Domain::ConfigItemDef::translate_option_group(config_item->def().option_group))
    );

    const Domain::ConfigItemDef::OptionGroup option_group = config_item->def().option_group;
    const Domain::ConfigItemDef::Category category        = config_item->def().category;

    if (option_group != m_option_group || m_category != category) {
        m_option_group = option_group;
        m_category     = category;
        rebuild_form_elements();
        m_rows_filter_list->invalidate();
    }
}

void OverridableSubcategoryItem::on_index_update()
{
    ImColor color = m_theme->color_imgui(Platform::Color::WindowBg);
    set_fill(m_index % 2 == 0 ? color : Imgui::adjust_brightness(color, 0.9f));
}

} // namespace Slic3r::App
