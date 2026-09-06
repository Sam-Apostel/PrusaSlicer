#pragma once

#include <string>

namespace Slic3r::Domain {
class ConfigItem;
} // namespace Slic3r::Domain

namespace Slic3r::Biz {

/**
 * @brief Reads a setting by name out of whichever config box a view is showing.
 *
 * The settings dialogs each have their own interactor over their own kind of
 * config box -- print with per-tool overrides, printer, filament. A control
 * standing in for several settings needs to read siblings of the one it was
 * built from, and that is the only thing it needs from an interactor, so it
 * takes this rather than any one of them and works in all of them.
 */
class IConfigItemSource
{
public:
    virtual ~IConfigItemSource() = default;

    /// @return nullptr when no such setting is in this box.
    virtual const Domain::ConfigItem* find_item(const std::string& name) const = 0;
};

} // namespace Slic3r::Biz
