#pragma once

#include "Slic3r/App/Config/ConfigFormElement.hpp"
#include "Slic3r/Domain/ConfigDef.hpp"

#include <functional>
#include <set>
#include <string>

namespace Slic3r::App {

class SectionToggleElement;

/**
 * @brief The custom controls for one option group, in whichever dialog shows it.
 *
 * There are three settings dialogs -- print, filament, printer and preferences
 * -- and each builds its own rows over its own interactor, sharing no base
 * class. What they do share is this: which controls stand in for rows in a
 * group, the switch that belongs in the group's heading, and whether the group
 * is currently expanded.
 *
 * That lives here rather than in any one of them because the first version of
 * it lived in one of them. The other two went on rendering plain rows, and
 * nothing said so -- a registered control simply never appeared. Anything a
 * dialog has to remember to opt into is something two dialogs will forget.
 */
class ConfigFormSection
{
public:
    /**
     * @brief Whether a control may stand in for this setting's row here.
     *
     * A view where the row does more than edit the value -- the print form's
     * rows are also where per-tool overrides are added and managed -- has to
     * be able to refuse. Replacing such a row with a control that edits only
     * the print level would take the overrides away with no way back.
     */
    using ClaimAllowedFn = std::function<bool(const std::string& key)>;

    /**
     * @param heading  The group's heading row; the switch is added at its end.
     * @param elements The container the controls are built into.
     */
    ConfigFormSection(Yoga::Item& heading, Yoga::Item& elements);

    /**
     * @brief Build the controls registered for this group, replacing the last set.
     *
     * A control is built only if every setting it claims is allowed; one that
     * is not is skipped whole, so no setting it renders is left without either
     * a control or a row.
     *
     * @param context       Setter, item source and config box index. Its
     *                      claimed_keys are ignored -- each control is told its own.
     * @param claim_allowed Empty means every setting may be claimed.
     */
    void rebuild(
        Domain::ConfigItemDef::Category category,
        Domain::ConfigItemDef::OptionGroup option_group,
        const ConfigFormContext& context,
        const ClaimAllowedFn& claim_allowed = {}
    );

    /**
     * @brief True when a control built here renders this setting.
     *
     * The row list asks this rather than the registry, so what is hidden is
     * exactly what was built. Asking the registry instead would be a second
     * answer to the same question, free to disagree with the first.
     */
    bool renders(const std::string& key) const { return m_claimed.contains(key); }

    /**
     * @brief Whether the group's body should be shown.
     *
     * False only for a group whose heading switch is off: everything below it
     * is disabled, and a list of greyed rows says less than their absence does.
     */
    bool expanded() const;

    /**
     * @brief Whether this group's heading carries a switch.
     *
     * A group whose rows are all hidden still has something to show if the
     * switch is there -- the switch is a setting too, and it is what turns the
     * rest back on.
     */
    bool has_toggle() const
    {
        return m_toggle != nullptr;
    }

    /**
     * @brief Show the body whatever the switch says.
     *
     * For search: a setting the user went looking for has to be somewhere they
     * can see it, however its group is set.
     */
    void set_force_expanded(bool force_expanded) { m_force_expanded = force_expanded; }

private:
    Yoga::Item& m_heading;
    Yoga::Item& m_elements;
    SectionToggleElement* m_toggle{nullptr};
    /// Settings a control built here renders, so they get no row of their own.
    std::set<std::string> m_claimed;
    bool m_force_expanded{false};
};

} // namespace Slic3r::App
