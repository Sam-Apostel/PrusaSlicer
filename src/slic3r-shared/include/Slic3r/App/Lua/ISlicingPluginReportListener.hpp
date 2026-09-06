#pragma once

#include <string>

namespace Slic3r::App::Lua {

/**
 * @brief Somewhere for a slicing plugin's report() to end up.
 *
 * A slicing plugin observes a finished slice and the only thing it can do with
 * what it saw is say it. Saying it is a UI decision -- where it appears, how
 * long it stays, whether it replaces the last one -- so the plugin system
 * hands it over rather than deciding.
 */
class ISlicingPluginReportListener
{
public:
    virtual ~ISlicingPluginReportListener() = default;

    /**
     * @param plugin_id which plugin spoke. A report replaces that plugin's
     *        previous one: slicing happens over and over as settings are
     *        edited, and a plugin that spoke on each slice would otherwise
     *        stack up a report per keystroke.
     * @param title may be empty.
     */
    virtual void on_slicing_plugin_report(
        const std::string& plugin_id,
        const std::string& title,
        const std::string& text
    ) = 0;
};

} // namespace Slic3r::App::Lua
