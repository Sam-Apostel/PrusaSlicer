#pragma once

#include <sol/sol.hpp>

#include "Slic3r/Biz/Lua/LuaEngine.hpp"

namespace Slic3r::App::Lua {

/**
 * @brief A `require` that can only reach the plugin's own directory.
 *
 * Lua's own `require` searches a path and can load anything on it, which is
 * the whole machine. This one resolves a module name through the engine's
 * path resolver -- set per plugin, and confined to that plugin's directory --
 * loads the file in C++, and caches the result in the Lua registry, which a
 * script cannot reach and therefore cannot poison.
 */
class PackageRegistry
{
public:
    void register_api(Biz::Lua::LuaEngine& lua);

private:
    sol::object safe_require(sol::this_state ts, const std::string& module_name);

    Biz::Lua::LuaEngine::FilePathResolveFn m_path_resolver;
};

} // namespace Slic3r::App::Lua
