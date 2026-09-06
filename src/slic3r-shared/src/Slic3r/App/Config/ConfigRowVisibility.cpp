#include "Slic3r/App/Config/ConfigRowVisibility.hpp"

#include "Slic3r/Domain/ConfigItemPredicate.hpp"

namespace Slic3r::App {

bool config_item_visible(const Domain::ConfigItem& item, const Biz::IConfigBoxSetter& setter)
{
    // Checked before anything else: nearly every setting declares no visible_if,
    // and this runs for every row of every group on every frame.
    if (!item.def().visible_if)
        return true;

    const Domain::ConfigItemLookup* lookup = setter.item_lookup();
    if (lookup == nullptr) {
        // A view whose setter cannot read siblings cannot evaluate the rule.
        // Showing the setting is the safe way to be wrong: the user can see it
        // and decide, where hiding it would leave no trace that it exists.
        return true;
    }
    return Domain::evaluate(item.def().visible_if, *lookup);
}

} // namespace Slic3r::App
