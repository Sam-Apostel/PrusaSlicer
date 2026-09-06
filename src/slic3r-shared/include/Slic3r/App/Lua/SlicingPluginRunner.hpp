#pragma once

#include <functional>
#include <string>

#include "Slic3r/App/Lua/PluginRegistry.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Slicing/SlicingInteractor.hpp"

namespace Slic3r::App::Lua {

/**
 * @brief Tells slicing plugins about slices that finished.
 *
 * ### Why this is safe to have at all
 *
 * Slicing runs on a background thread and is cancelled on nearly every config
 * edit, and Lua has neither threads nor preemption. Rather than solve that,
 * this listens where the problem is already solved: `SlicingInteractor`
 * marshals every status update onto the main thread before invoking listeners,
 * and it only reports `Finished` for a slice that ran to completion. So a
 * handler runs on the main thread, after the work, on a result that is not
 * going to be thrown away underneath it -- and a cancelled slice, which ends
 * in `Modified`, never reaches one.
 *
 * ### Why it cannot break a print
 *
 * It reads the result and hands a copy of the numbers to the script. There is
 * no path from the script back to the result, the config, or the G-code, and
 * the delivery happens after the G-code exists. A handler that throws, loops
 * over its own data or does nothing at all costs its own plugin a turn and
 * nothing else -- each is run separately and a failure is logged against the
 * plugin that caused it.
 *
 * The one thing it cannot defend against is a handler that does not return:
 * Lua has no preemption, so an infinite loop in a plugin hangs the UI. That is
 * the same exposure `project.plugin` already has.
 */
class SlicingPluginRunner : public Biz::Slicing::IStatusListener
{
public:
    using ReportFn = std::function<
        void(const std::string& plugin_id, const std::string& title, const std::string& text)>;

    SlicingPluginRunner(
        const PluginRegistry& registry,
        Biz::ProjectInteractor& project_interactor,
        ReportFn report
    );

    void on_status_changed(Biz::Slicing::StatusUpdate status, Domain::SlicingId id) override;

private:
    /// Read the finished result and hand it to every plugin watching for it.
    void deliver_sliced(Domain::SlicingId id);

    /// The registry is asked at delivery time rather than cached: a rescan
    /// destroys and rebuilds every Plugin, so anything kept here would be a
    /// pointer to a plugin that no longer exists.
    const PluginRegistry& m_registry;
    Biz::ProjectInteractor& m_project_interactor;
    ReportFn m_report;
};

} // namespace Slic3r::App::Lua
