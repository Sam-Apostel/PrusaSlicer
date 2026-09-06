#include "Slic3r/App/Config/FormSpecInstaller.hpp"

#include "Slic3r/App/Config/ChoiceElement.hpp"
#include "Slic3r/App/Config/ConfigFormElementRegistry.hpp"
#include "Slic3r/App/Config/EnumCardsElement.hpp"
#include "Slic3r/App/Config/PercentSliderElement.hpp"

#include "Slic3r/Domain/ConfigDefsFDM.hpp"
#include "Slic3r/Domain/ConfigDefsSLA.hpp"
#include "Slic3r/Domain/Percentage.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <map>
#include <optional>

namespace Slic3r::App {

std::vector<std::string> FormElementSpec::keys() const
{
    if (kind != Kind::Choice)
        return key.empty() ? std::vector<std::string>{} : std::vector<std::string>{key};

    std::vector<std::string> all;
    const auto add = [&all](const std::string& k)
    {
        if (std::ranges::find(all, k) == all.end())
            all.push_back(k);
    };
    for (const ChoiceOptionSpec& option : options) {
        for (const auto& [flag, value] : option.flags)
            add(flag);
        for (const std::string& revealed : option.reveals)
            add(revealed);
    }
    return all;
}

namespace {

/// The definition of a setting, from whichever technology defines it.
const Domain::ConfigItemDef* find_def(const std::string& key)
{
    for (const Domain::ConfigDefinitions* defs :
         {&Domain::get_defs_fdm(), &Domain::get_defs_sla()})
    {
        const auto& all = defs->defs();
        const auto it =
            std::ranges::find_if(all, [&key](const Domain::ConfigItemDef& d) { return d.name == key; });
        if (it != all.end())
            return &*it;
    }
    return nullptr;
}

std::string kind_name(const FormElementSpec::Kind kind)
{
    switch (kind) {
    case FormElementSpec::Kind::Cards: return "cards";
    case FormElementSpec::Kind::Slider: return "slider";
    case FormElementSpec::Kind::Choice: return "choice";
    case FormElementSpec::Kind::SectionToggle: return "section_toggle";
    }
    return "?";
}

/// A spec that cannot be installed, and what a plugin author should do about it.
using Rejection = std::optional<std::string>;

Rejection check_type(
    const Domain::ConfigItemDef& def,
    const std::type_info& expected,
    const std::string_view expected_name
)
{
    if (def.type != nullptr && *def.type == expected)
        return std::nullopt;
    return fmt::format("'{}' is not {}", def.name, expected_name);
}

} // namespace

FormSpecInstallReport install_plugin_form_specs(
    const std::vector<FormElementSpec>& specs,
    ConfigFormElementRegistry& registry
)
{
    FormSpecInstallReport report;
    std::vector<ConfigFormElementRegistry::Entry> entries;
    std::vector<ConfigFormElementRegistry::SectionToggle> toggles;
    /// Which spec took each setting, so a second claim can name the first.
    std::map<std::string, std::string> claimed_by;
    std::map<Domain::ConfigItemDef::OptionGroup, std::string> gated_by;

    for (const FormElementSpec& spec : specs) {
        const std::vector<std::string> keys = spec.keys();
        if (keys.empty()) {
            report.rejected.push_back(fmt::format("{}: names no setting", kind_name(spec.kind)));
            continue;
        }

        // The group comes from the settings, never from the plugin. A control
        // renders inside one group and takes those rows away, so every setting
        // it claims has to be in that group or a row somewhere else would go
        // missing with nothing standing in for it.
        const Domain::ConfigItemDef* first = find_def(keys.front());
        if (first == nullptr) {
            report.rejected.push_back(
                fmt::format("{}: no setting named '{}'", kind_name(spec.kind), keys.front())
            );
            continue;
        }

        Rejection rejection;
        for (const std::string& key : keys) {
            const Domain::ConfigItemDef* def = find_def(key);
            if (def == nullptr) {
                rejection = fmt::format("no setting named '{}'", key);
                break;
            }
            if (def->category != first->category || def->option_group != first->option_group) {
                rejection = fmt::format(
                    "'{}' and '{}' are in different groups", keys.front(), key
                );
                break;
            }
        }
        if (!rejection.has_value()) {
            switch (spec.kind) {
            case FormElementSpec::Kind::Cards:
                rejection = check_type(*first, typeid(Domain::EnumWrapper), "a choice of values");
                break;
            case FormElementSpec::Kind::Slider:
                rejection = check_type(*first, typeid(Domain::Percentage), "a percentage");
                break;
            case FormElementSpec::Kind::SectionToggle:
                rejection = check_type(*first, typeid(bool), "a yes/no setting");
                break;
            case FormElementSpec::Kind::Choice:
                for (const ChoiceOptionSpec& option : spec.options) {
                    for (const auto& [flag, value] : option.flags) {
                        // Existence was settled by the group check above, which
                        // covers every key the spec names.
                        const Domain::ConfigItemDef* def = find_def(flag);
                        if (def == nullptr)
                            continue;
                        rejection = check_type(*def, typeid(bool), "a yes/no setting");
                        if (rejection.has_value())
                            break;
                    }
                    if (rejection.has_value())
                        break;
                }
                break;
            }
        }
        // A setting is rendered once. Two controls claiming it would both take
        // its row away and both draw it, so the second is refused -- and says
        // what already has it, since the two are usually in different files and
        // the author of one has no reason to suspect the other.
        if (!rejection.has_value()) {
            for (const std::string& key : keys) {
                const auto it = claimed_by.find(key);
                if (it != claimed_by.end()) {
                    rejection = fmt::format("'{}' is already rendered by {}", key, it->second);
                    break;
                }
            }
        }
        if (!rejection.has_value() && spec.kind == FormElementSpec::Kind::SectionToggle) {
            const auto it = gated_by.find(first->option_group);
            if (it != gated_by.end()) {
                rejection = fmt::format("that group is already switched by '{}'", it->second);
            }
        }

        if (rejection.has_value()) {
            report.rejected.push_back(fmt::format("{}: {}", kind_name(spec.kind), *rejection));
            continue;
        }

        for (const std::string& key : keys)
            claimed_by.emplace(key, fmt::format("the {} for '{}'", kind_name(spec.kind), keys.front()));

        if (spec.kind == FormElementSpec::Kind::SectionToggle) {
            gated_by.emplace(first->option_group, spec.key);
            toggles.push_back(ConfigFormElementRegistry::SectionToggle{
                .category = first->category, .option_group = first->option_group, .key = spec.key
            });
            ++report.installed;
            continue;
        }

        ConfigFormElementRegistry::Entry entry{
            .category     = first->category,
            .option_group = first->option_group,
            .claimed_keys = std::set<std::string>{keys.begin(), keys.end()},
            .factory      = {}
        };
        switch (spec.kind) {
        case FormElementSpec::Kind::Cards:
            entry.factory = [key = spec.key, columns = spec.columns, images = spec.images](
                                const ConfigFormContext& ctx
                            )
            { return std::make_unique<EnumCardsElement>(ctx, key, columns, images); };
            break;
        case FormElementSpec::Kind::Slider:
            entry.factory = [key = spec.key, step = spec.step](const ConfigFormContext& ctx)
            { return std::make_unique<PercentSliderElement>(ctx, key, step); };
            break;
        case FormElementSpec::Kind::Choice:
            entry.factory =
                [label = spec.label, options = spec.options](const ConfigFormContext& ctx)
            { return std::make_unique<ChoiceElement>(ctx, label, options); };
            break;
        case FormElementSpec::Kind::SectionToggle:
            break; // handled above
        }
        entries.push_back(std::move(entry));
        ++report.installed;
    }

    registry.set_form_controls(std::move(entries), std::move(toggles));
    return report;
}

} // namespace Slic3r::App
