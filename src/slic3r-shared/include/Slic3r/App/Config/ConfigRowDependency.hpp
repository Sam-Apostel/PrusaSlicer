#pragma once

#include <string>

namespace Slic3r::Domain {
class ConfigItem;
} // namespace Slic3r::Domain

namespace Slic3r::Biz {
class IConfigBoxSetter;
} // namespace Slic3r::Biz

namespace Slic3r::App {

/**
 * @brief Whether a setting currently applies, and why not when it does not.
 *
 * A setting's rules are about *other* settings, so nothing about the setting
 * itself changes when the answer does and there is no notification to hang the
 * evaluation off. Rows re-evaluate every frame instead, which is a handful of
 * map lookups; this remembers the last answer so that only a real change costs
 * anything downstream.
 *
 * Held rather than inherited because the settings form has three kinds of row
 * -- print, filament and everything else -- that share no base class. The first
 * version of this lived inside one of them and the other two silently had no
 * rules at all.
 */
class ConfigRowDependency
{
public:
    /**
     * @brief Re-evaluate against the current config.
     *
     * @return true when the answer changed, so a caller can skip its own work
     *         on the frames where it did not -- which is nearly all of them.
     */
    bool refresh(const Domain::ConfigItem& item, const Biz::IConfigBoxSetter& setter);

    /**
     * @brief Force the next refresh() to report a change.
     *
     * Rows are recycled: a list view hands the same row a different setting,
     * and a row rebuilds its input when the new setting wants a different kind
     * of control. Fresh widgets start enabled, so a cached "does not apply"
     * that happens to match the new setting would be reported as no change and
     * never applied to them. Call this whenever the widgets are rebuilt.
     */
    void invalidate() { m_valid = false; }

    /// False when a rule or an unmet requirement says the setting has no effect.
    bool applies() const { return m_applies; }

    /**
     * @brief Why the setting cannot apply, translated and ready to show.
     *
     * Empty when it applies, and also when it is ruled out by an enable_if,
     * which states a dependency rather than explaining one. Only requirements
     * carry a reason, because only they were written to be read by the user.
     */
    const std::string& reason() const { return m_reason; }

private:
    /// False until an answer has been applied to widgets that still exist.
    bool m_valid{false};
    bool m_applies{true};
    std::string m_reason;
};

} // namespace Slic3r::App
