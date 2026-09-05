#pragma once

#include "Slic3r/App/Config/FormSpec.hpp"

#include <string>
#include <vector>

namespace Slic3r::App {

class ConfigFormElementRegistry;

struct FormSpecInstallReport
{
    size_t installed{0};
    /// One message per rejected spec, saying which and why. Ready for the log.
    std::vector<std::string> rejected;
};

/**
 * @brief Turn declared controls into registrations, replacing the previous set.
 *
 * Every spec is checked against the config definitions before it is installed:
 * the settings it names have to exist, be of the type its control can render,
 * and -- for a control standing in for several settings -- live in the same
 * group, since a control renders in one place and can only claim rows there.
 *
 * A spec that fails is dropped and reported, and the rest are installed. One
 * bad line in a plugin costs that line, not the plugin, and never the form:
 * whatever is not rendered by a control still has its ordinary row.
 */
FormSpecInstallReport install_plugin_form_specs(
    const std::vector<FormElementSpec>& specs,
    ConfigFormElementRegistry& registry
);

} // namespace Slic3r::App
