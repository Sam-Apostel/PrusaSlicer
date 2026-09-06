#pragma once

#include <variant>
#include <string>
#include <string_view>
#include <map>

#include <boost/filesystem/path.hpp>
#include <tl/expected.hpp>

#include "Slic3r/App/Config/FormSpec.hpp"
#include "Slic3r/Biz/Lua/LuaEngine.hpp"

namespace Slic3r::App::Lua {

enum class PluginType
{
    ProjectPlugin,
    /**
     * @brief A plugin that changes how settings are rendered, not what they are.
     *
     * It declares controls rather than drawing them. The settings form is an
     * immediate-mode UI redrawn every frame alongside the 3D scene, so calling
     * into Lua to paint it would put a script interpreter in the frame loop.
     * Declaring instead means the script runs once, at scan time, and the
     * controls it asked for are built and driven in C++ from then on.
     *
     * Nothing about the config changes: the controls read and write the same
     * settings through the same path an ordinary row uses, so profiles, the
     * slicing backend and 3MFs never learn that a plugin was involved.
     */
    FormPlugin,
    /**
     * @brief A plugin that watches slicing without taking part in it.
     *
     * It is handed a finished result and can say something about it. It
     * cannot change the result, the config or the G-code, and it runs after
     * the slice rather than inside it, so nothing it does -- including
     * failing -- can affect what is printed.
     *
     * That is the whole of the API for now, and deliberately so: observation
     * and mutation are different features with different risk, and this one
     * exists to find out whether the shape of the data is right before
     * anything is allowed to change it.
     */
    SlicingPlugin
};

/**
 * @brief A point in slicing a plugin can watch.
 *
 * Every event is delivered after the fact, on the main thread, with the
 * result already computed. There is no event that runs *during* a slice:
 * slicing happens on a background thread and is cancelled on every config
 * edit, and neither of those is something a Lua script can be exposed to
 * safely. Dodging both is what makes this version small enough to ship.
 */
enum class SlicingEvent
{
    /**
     * @brief A slice finished and produced a complete result.
     *
     * Not delivered for a slice that was cancelled, failed, or stopped at a
     * step: those never reach the finished state. Not delivered for a result
     * that carries only the basic statistics either -- there are no filament
     * totals in one, so there is nothing to report.
     */
    Sliced
};

tl::expected<PluginType, std::string> parse_plugin_type(std::string_view s);
std::string to_string(PluginType type);

tl::expected<SlicingEvent, std::string> parse_slicing_event(std::string_view s);
std::string to_string(SlicingEvent event);
/// The name of the Lua function an event is delivered to.
std::string handler_name(SlicingEvent event);

using PluginParamValue = std::variant<bool, int, double, std::string>;
using PluginParamValueMap = std::map<std::string, PluginParamValue>;

struct PluginParamDef
{
    std::string name;
    std::string label;
    std::string type;
    std::optional<PluginParamValue> default_value;
};

using PluginParamDefs = std::vector<PluginParamDef>;

struct PluginMeta
{
    std::string id;
    PluginType type;
    std::optional<std::string> title;
    std::vector<std::string> menu;
    PluginParamDefs params;
    /// FormPlugin only: the controls this plugin declares.
    std::vector<FormElementSpec> form_elements;
    /// SlicingPlugin only: the events this plugin asked to be told about.
    std::vector<SlicingEvent> slicing_events;
};


class Plugin
{
public:
    const PluginMeta& meta() const { return m_meta; }
    PluginMeta& meta() { return m_meta; }
    const std::string& path() const { return m_path; }

    void execute(Biz::Lua::LuaEngine& lua, const PluginParamValueMap& params) const;

    /**
     * @brief Hand one slicing event to this plugin's handler.
     *
     * The engine is the caller's, so the caller decides what the script can
     * reach. Failure is returned rather than thrown or logged here: the
     * caller is delivering to several plugins in a row and one of them
     * misbehaving must not cost the others their turn.
     */
    tl::expected<void, std::string>
    deliver(Biz::Lua::LuaEngine& lua, SlicingEvent event, const sol::table& payload) const;

    using ParseResult = tl::expected<Plugin, std::string>;
    static ParseResult
    parse(Biz::Lua::LuaEngine& lua, const std::string& id_prefix, const std::string& path);

private:
    Plugin(std::string  path, PluginMeta  meta);

private:
    std::string m_path;
    PluginMeta m_meta;
};

bool is_path_in_sandbox(
    const boost::filesystem::path& sandbox_path,
    const boost::filesystem::path& tested_path
);

}