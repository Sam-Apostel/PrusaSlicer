#pragma once

#include "Slic3r/Biz/ConfigItemContext.hpp"
#include "Slic3r/Biz/IConfigBoxSetter.hpp"
#include "Slic3r/Biz/PrintToolItem.hpp"
#include "Slic3r/Domain/Config.hpp"

#include <algorithm>
#include <string>

namespace Slic3r::App {

/**
 * @brief Whether a setting should be shown at all, per its visible_if.
 *
 * Distinct from whether it applies. A setting that does not apply is greyed --
 * it exists, and something the user can see turns it on. A setting that is not
 * visible has no meaning in the configuration they have chosen, and a row for
 * it would be a question with no answer.
 *
 * True for the overwhelming majority of settings, which declare no visible_if
 * at all; the check for that costs nothing.
 */
bool config_item_visible(const Domain::ConfigItem& item, const Biz::IConfigBoxSetter& setter);

/// The setting behind a row of the ordinary settings list.
inline const Domain::ConfigItem* config_item_of(const Biz::ConfigItemContext& row)
{
    return row.config_item;
}

/// The setting behind a row of the print form, which also carries tool overrides.
inline const Domain::ConfigItem* config_item_of(const Biz::PrintToolItem& row)
{
    return row.print_item;
}

/**
 * @brief Show or hide each row of a group according to its setting's visible_if.
 *
 * Applied by the group rather than by the row, and not optional: a row that
 * hid itself could never show itself again, because an invisible item's render
 * does not run (Item::render_node skips it) and its render is the only place
 * it would learn that the rule now holds.
 *
 * @param forced_key A setting to show whatever its rule says -- the one search
 *                   is pointing at. Empty for none.
 * @return Whether any row is left visible, so an emptied group can collapse.
 */
template <class FilterList, class ListView>
bool apply_visibility_to_rows(
    const FilterList& rows,
    ListView& view,
    const Biz::IConfigBoxSetter& setter,
    const std::string& forced_key
)
{
    bool any_visible = false;
    // The two can disagree for a frame while the list view catches up with the
    // filtered list, so only walk as far as both go.
    const size_t count = std::min(rows.size(), view.object_count());
    for (size_t i = 0; i < count; ++i) {
        const Domain::ConfigItem* item = config_item_of(rows.at(i));
        const bool visible             = item == nullptr
            || (!forced_key.empty() && item->name() == forced_key)
            || config_item_visible(*item, setter);
        view.item_at(i)->set_visible(visible);
        any_visible = any_visible || visible;
    }
    return any_visible;
}

} // namespace Slic3r::App
