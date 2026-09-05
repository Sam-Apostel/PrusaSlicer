#pragma once

#include <map>
#include <string>
#include <vector>

namespace Slic3r::App {

/**
 * @brief One option of a choice control, and what choosing it means.
 *
 * The settings underneath stay exactly what they were -- usually a handful of
 * booleans that only make sense one at a time. An option says which value each
 * of them takes, so the control can both write the choice and recognise it when
 * reading a profile back.
 */
struct ChoiceOptionSpec
{
    std::string label;
    std::string description;
    /// Boolean settings and the value each takes while this option is chosen.
    std::map<std::string, bool> flags;
    /// Settings that are only worth showing while this option is chosen.
    std::vector<std::string> reveals;
};

/**
 * @brief A control declared rather than written: what to render, for which keys.
 *
 * This is what a plugin hands over. It never mentions a category or an option
 * group -- a setting already knows which group it belongs to, and making a
 * plugin restate it would be one more thing to get out of step. The group is
 * looked up from the first setting the spec names.
 */
struct FormElementSpec
{
    enum class Kind
    {
        /// An enum as a list or grid of cards rather than a dropdown.
        Cards,
        /// A percentage as a slider with a readout.
        Slider,
        /// Several booleans as the one choice they actually are.
        Choice,
        /// A boolean promoted to its group's heading, gating the group.
        SectionToggle,
    };

    Kind kind{Kind::Cards};

    /// The setting rendered. Unused by Choice, which names its keys per option.
    std::string key;

    /// Cards: how many per row. One reads as a list, more as a grid.
    size_t columns{1};

    /// Slider: the step it snaps to.
    double step{1.0};

    /// Choice: the heading above the options.
    std::string label;
    std::vector<ChoiceOptionSpec> options;

    /// Every setting this spec renders, for claiming and for finding the group.
    std::vector<std::string> keys() const;
};

} // namespace Slic3r::App
