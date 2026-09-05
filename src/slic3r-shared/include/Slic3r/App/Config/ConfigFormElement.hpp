#pragma once

#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/Domain/Config.hpp"

#include <set>
#include <string>

namespace Slic3r::Biz {
class ConfigBoxInteractor;
class IConfigBoxSetter;
} // namespace Slic3r::Biz

namespace Slic3r::App {

/// Everything a form element needs in order to read and write the settings it claims.
struct ConfigFormContext
{
    Biz::IConfigBoxSetter* setter{nullptr};
    Biz::ConfigBoxInteractor* cbi{nullptr};
    size_t cbi_index{0};
    /// Settings this element renders, so it can honour their own rules.
    std::set<std::string> claimed_keys;
};

/**
 * @brief A control that presents several settings as one thing.
 *
 * The settings form is otherwise generated straight from the config: one row per
 * key, in the group the key's definition names. That is a faithful view of the
 * data and often a poor view of the decision — two mutually exclusive booleans
 * are one choice, not two switches, and a parameter of one of them only means
 * anything once that one is chosen.
 *
 * An element claims a set of keys and renders them however suits the decision.
 * It changes nothing about how those settings are stored: it reads and writes
 * the same keys through the same setter a default row uses, so profiles, the
 * slicing backend and 3MFs are untouched.
 *
 * Keys an element claims are excluded from the default rows, so the two never
 * render the same setting twice.
 */
class ConfigFormElement : public Yoga::Item
{
public:
    explicit ConfigFormElement(const ConfigFormContext& context);

    /**
     * @brief Re-read the claimed settings and update the control.
     *
     * Called when the config changes underneath, so that an element reflects an
     * undo, a preset switch or a write from somewhere else.
     */
    virtual void refresh_from_config() = 0;

    /**
     * @brief Re-read the config and re-apply the claimed settings' own rules.
     *
     * Elements call this from their render rather than refresh_from_config()
     * directly, so that honouring a rule is not something each one has to
     * remember. A claimed setting gets no default row, so its enable_if would
     * otherwise go unenforced — an element replacing a row inherits the row's
     * obligations along with its job.
     */
    void refresh_element();

protected:
    /**
     * @brief Whether any claimed setting currently applies.
     *
     * Any rather than all: a composite may claim settings whose rules are
     * mutually exclusive — the travel-avoidance strategies disable each other —
     * and requiring all of them to hold would disable the control permanently.
     * The element is relevant while at least one setting it renders is.
     */
    bool any_claimed_setting_applies() const;

    /**
     * @brief The first requirement of a setting that does not hold, or nullptr.
     *
     * A requirement carries the reason it is not met, which is worth showing:
     * a control that is greyed out for a reason the user cannot see is only
     * marginally better than one that accepts the value and fails at slice time.
     */
    const Domain::ConfigItemRequirement* unmet_requirement(const std::string& key) const;

    /// The claimed setting, or nullptr when this config box does not carry it.
    const Domain::ConfigItem* config_item(const std::string& key) const;

    /// Value of a claimed boolean, falling back when absent or of another type.
    bool flag_of(const std::string& key, bool fallback = false) const;

    /**
     * @brief Write a claimed setting, through the same path a default row uses.
     *
     * A no-op when the setting is absent, so an element referring to a key that
     * a given printer technology does not define stays harmless.
     */
    void set_flag(const std::string& key, bool value);

    /// Underlying integer of a claimed enum, or @p fallback when absent.
    int enum_of(const std::string& key, int fallback = -1) const;

    /**
     * @brief Write a claimed enum by its underlying integer.
     *
     * The value definitions come from the setting's current value, so the
     * caller does not need to know the enum's C++ type.
     */
    void set_enum(const std::string& key, int value);

    /// Value of a claimed percentage, or @p fallback when absent.
    double percent_of(const std::string& key, double fallback = 0.0) const;

    void set_percent(const std::string& key, double value);

    const ConfigFormContext& context() const { return m_context; }

private:
    ConfigFormContext m_context;
};

} // namespace Slic3r::App
