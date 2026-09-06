#include "Slic3r/App/Config/PrintToolSubcategoryItem.hpp"

#include <Slic3r/Domain/Config.hpp>
#include <Slic3r/Domain/ConfigDefsFDM.hpp>

#include <algorithm>

#include "Slic3r/App/Config/ConfigRowVisibility.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"

#include "Slic3r/Biz/PrintToolConfigBoxInteractor.hpp"
#include "Slic3r/Biz/PrintToolConfigObservableList.hpp"
#include <Slic3r/Biz/I18N/I18N.hpp>

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

namespace {

/// The definition of a print setting, or nullptr when there is no such setting.
const Domain::ConfigItemDef* find_print_def(const std::string& key)
{
    const std::vector<Domain::ConfigItemDef>& defs = Domain::get_defs_fdm().defs();
    const auto it = std::ranges::find_if(
        defs, [&key](const Domain::ConfigItemDef& def) { return def.name == key; }
    );
    return it == defs.end() ? nullptr : &*it;
}

} // namespace

PrintToolSubcategoryItem::PrintToolSubcategoryItem(
    size_t index,
    const Biz::PrintToolItem& data,
    Biz::PrintToolConfigBoxInteractor& cbi,
    Biz::IConfigBoxSetter& cbi_setter,
    Biz::ProjectInteractor& project_interactor
) :
    Biz::DataObserver<Biz::PrintToolItem>(index, data),
    m_cbi(cbi),
    m_cbi_setter(cbi_setter),
    m_rows_filter_list(std::make_shared<Biz::ObservableListSortFilter<Biz::PrintToolItem>>())
{
    set_object_name("PrintToolSubcategoryItem");
    set_orientation(Orientation::Vertical);
    set_flex_shrink(0);
    set_flags(ImDrawFlags_None);
    set_rounding(0);
    set_gap(0);
    m_padding = Paddings(20.f, 20.f, 20.f, 0.f);
    set_padding(m_padding);

    // The heading is a row rather than a bare label, because a group whose
    // settings all hang off one switch puts that switch here, beside the name
    // of the thing it switches.
    m_heading = emplace_back<Item>();
    m_heading->set_orientation(Orientation::Horizontal);
    m_heading->set_align_items(YGAlignCenter);
    m_heading->set_gap(10);

    m_label = m_heading->emplace_back<Text>(
        Biz::_u8(
            Domain::ConfigItemDef::translate_option_group(m_state->print_item->def().option_group)
        ),
        Render::ImguiFontType::Bold
    );
    // Takes the space the switch does not, and gives it back on a narrow panel
    // rather than pushing the switch off the end.
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
        [this](const Biz::PrintToolItem& item) -> bool
        {
            const Domain::ConfigItemDef& def = item.print_item->def();
            if (def.option_group != m_option_group || def.category != m_category)
                return false;
            // A setting a custom control renders gets no row of its own.
            return !m_section->renders(item.name);
        }
    );
    // also group by row_group
    m_rows_filter_list->set_group_by_fn(
        [](const Biz::PrintToolItem& item, std::unordered_set<std::string>& seen_keys) -> bool
        {
            const std::string& row_group = item.print_item->def().row_group;
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
        [](const Biz::PrintToolItem& lhs, const Biz::PrintToolItem& rhs)
        { return lhs.print_item->def().order < rhs.print_item->def().order; }
    );

    m_rows_filter_list->set_source_model(m_cbi.observable_list());

    m_rows_list_view = emplace_back<PrintToolRowListView>(
        PrintToolRowListViewFactory{m_cbi, m_cbi_setter, project_interactor}
    );
    m_rows_list_view->set_object_name("PrintToolRowListView");
    m_rows_list_view->set_orientation(Orientation::Vertical);
    m_rows_list_view->set_gap(5);
    m_rows_list_view->set_padding(20);

    on_index_update();
    on_data_update();

    m_rows_list_view->set_source_list(m_rows_filter_list.get());
}

void PrintToolSubcategoryItem::navigate_to_item(const Domain::ConfigItem* config_item)
{
    const std::string& name = config_item->name();
    for (size_t row_index = 0; row_index < m_rows_filter_list->size(); ++row_index) {
        if (m_rows_filter_list->at(row_index).name == name) {
            // Being pointed at a row inside a collapsed group has to open it.
            // Otherwise a search for a setting whose feature is switched off
            // highlights a row nobody can see, and looks like a search that
            // found nothing.
            m_section->set_force_expanded(true);
            m_navigating_to = name;
            apply_section_visibility();
            m_rows_list_view->item_at(row_index)->navigate_to_item(config_item);
            break;
        }
    }
}

void PrintToolSubcategoryItem::clear_navigation()
{
    m_section->set_force_expanded(false);
    m_navigating_to.clear();
    apply_section_visibility();
    for (size_t row_index = 0; row_index < m_rows_list_view->object_count(); ++row_index) {
        m_rows_list_view->item_at(row_index)->clear_navigation();
    }
}

void PrintToolSubcategoryItem::apply_section_visibility()
{
    const bool expanded = m_section->expanded();
    m_form_elements->set_visible(expanded);
    m_rows_list_view->set_visible(expanded);
}

void PrintToolSubcategoryItem::apply_row_visibility()
{
    const bool any_visible =
        apply_visibility_to_rows(*m_rows_filter_list, *m_rows_list_view, m_cbi_setter, m_navigating_to);

    // A group with no visible row, no control of its own and no switch in its
    // heading is not a group. Its heading goes, and its padding with it -- but
    // the group item itself stays, because hiding it would stop its render and
    // it could never find out that the rule had changed back.
    const bool anything_to_show =
        any_visible || m_form_elements->object_count() > 0 || m_section->has_toggle();
    m_heading->set_visible(anything_to_show);
    set_padding(anything_to_show ? m_padding : Yoga::Paddings(0.f));
}

void PrintToolSubcategoryItem::render(const Yoga::Vec2f& pos, const Yoga::Vec2f& size)
{
    // Every frame, like the rest of this UI. Nothing notifies us that the
    // switch was flipped, or that a preset switch or an undo flipped it; the
    // switch itself only learns in its own render, which runs after this one,
    // so the group follows a frame behind.
    apply_section_visibility();
    apply_row_visibility();
    Rectangle::render(pos, size);
}

void PrintToolSubcategoryItem::rebuild_form_elements()
{
    // A row in this form is not only where the print-level value is edited: on
    // a printer with several tools it is also where per-tool overrides are
    // added and managed. A control edits the print level and nothing else, so
    // replacing such a row would take the overrides away with no way back --
    // the user could not even add one, since the button to do so lived there.
    //
    // So a control may only stand in for a setting that has nowhere else to
    // go. On a single-tool printer that is every setting; on a multi-tool one
    // it is those the definitions do not allow overriding per tool. The test
    // is the printer, not whether an override happens to be set right now, so
    // the form does not change shape underneath someone adding one.
    const bool multiple_tools = m_state->shared_context.has_multiple_extruders;
    ConfigFormSection::ClaimAllowedFn claim_allowed;
    if (multiple_tools) {
        claim_allowed = [](const std::string& key) -> bool
        {
            const Domain::ConfigItemDef* def = find_print_def(key);
            return def == nullptr
                || !def->overrides_in.contains(Domain::ConfigLocation{
                    Domain::FDMConfigLocation::Tool
                });
        };
    }

    // The print level is where a control writes, the same level an ordinary
    // row writes at: index 0, the print config box.
    m_section->rebuild(
        m_category,
        m_option_group,
        ConfigFormContext{&m_cbi_setter, &m_cbi, 0, {}},
        claim_allowed
    );
}

void PrintToolSubcategoryItem::on_data_update()
{
    m_label->set_text(
        Biz::_u8(
            Domain::ConfigItemDef::translate_option_group(m_state->print_item->def().option_group)
        )
    );

    const Domain::ConfigItemDef::OptionGroup option_group = m_state->print_item->def().option_group;
    const Domain::ConfigItemDef::Category category        = m_state->print_item->def().category;

    if (option_group != m_option_group || m_category != category) {
        m_option_group = option_group;
        m_category     = category;
        rebuild_form_elements();
        m_rows_filter_list->invalidate();
    }
}

void PrintToolSubcategoryItem::on_index_update()
{
    ImColor color = m_theme->color_imgui(Platform::Color::WindowBg);
    set_fill(m_index % 2 == 0 ? color : Imgui::adjust_brightness(color, 0.9f));
}

} // namespace Slic3r::App
