#include "Slic3r/App/Config/ConfigRowDependency.hpp"

#include "Slic3r/Biz/IConfigBoxSetter.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"
#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Domain/ConfigItemPredicate.hpp"

namespace Slic3r::App {

bool
ConfigRowDependency::refresh(const Domain::ConfigItem& item, const Biz::IConfigBoxSetter& setter)
{
    const Domain::ConfigItemLookup* lookup = setter.item_lookup();
    if (lookup == nullptr) {
        // A view whose setter cannot read siblings cannot evaluate a rule, so
        // every setting stays editable rather than every setting going grey.
        return false;
    }

    const Domain::ConfigItemDef& def           = item.def();
    const Domain::ConfigItemRequirement* unmet = Domain::first_unmet(def.requirements, *lookup);
    const bool applies = unmet == nullptr && Domain::evaluate(def.enable_if, *lookup);
    std::string reason = unmet == nullptr ? std::string{} : Biz::_u8(unmet->reason);

    if (m_valid && applies == m_applies && reason == m_reason)
        return false;

    m_valid   = true;
    m_applies = applies;
    m_reason  = std::move(reason);
    return true;
}

} // namespace Slic3r::App
