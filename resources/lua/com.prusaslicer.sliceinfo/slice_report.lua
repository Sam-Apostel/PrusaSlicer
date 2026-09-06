-- What a finished slice used, said out loud.
--
-- This is an example of a slicing plugin: it watches slicing rather than
-- taking part in it. It is handed a finished result, reads numbers out of it,
-- and says something. It cannot change the result, the config or the G-code,
-- and it runs after the G-code exists, so nothing it does -- including
-- failing outright -- can affect what gets printed.
--
-- Edit this file and pick Plugins -> Rescan to see the result. There is no
-- build step. Delete the report() call at the bottom if you would rather have
-- this in the log only; delete the whole bundle directory to be rid of it.

info = {
    id = "report",
    type = "slicing.plugin",
    title = "Slice report",

    -- Events are declared, not inferred from which handlers happen to exist.
    -- A misspelled handler is then a plugin that fails to load with a reason
    -- in the log, rather than one that silently never runs.
    events = { "sliced" },
}

local function format_duration(seconds)
    local total = math.floor(seconds + 0.5)
    local hours = math.floor(total / 3600)
    local minutes = math.floor((total % 3600) / 60)
    if hours > 0 then
        return string.format("%dh %02dm", hours, minutes)
    end
    if minutes > 0 then
        return string.format("%dm %02ds", minutes, total % 60)
    end
    return string.format("%ds", total)
end

-- The roles worth naming in a one-line summary, in the order they read best.
-- slice.filament.per_role holds every role the print actually used; this picks
-- the few a person compares between two settings.
local INTERESTING_ROLES = {
    { key = "perimeter",        label = "perimeters" },
    { key = "external_perimeter", label = "external perimeters" },
    { key = "internal_infill",  label = "infill" },
    { key = "support_material", label = "supports" },
    { key = "wipe_tower",       label = "wipe tower" },
}

-- on_sliced(slice) is called once per slice that ran to completion, on the
-- main thread, with the result already computed. It is not called for a slice
-- that was cancelled -- which is most of them, since slicing restarts on every
-- settings edit.
function on_sliced(slice)
    local lines = {}

    lines[#lines + 1] = string.format(
        "%s, %.2f m of filament, %.1f g",
        format_duration(slice.time.normal.total),
        slice.filament.total_mm / 1000.0,
        slice.filament.total_g
    )

    -- No currency: the cost is whatever unit the filament profile's price is
    -- in, and the slicer does not know which. Labelled rather than left as a
    -- bare number so it cannot be read as another weight.
    if slice.filament.total_cost > 0 then
        lines[#lines] = lines[#lines] ..
            string.format(", costing %.2f", slice.filament.total_cost)
    end

    -- Per extruder, but only when there is more than one: on a single-tool
    -- printer this line would repeat the total.
    if #slice.filament.per_extruder > 1 then
        local parts = {}
        for i, used in ipairs(slice.filament.per_extruder) do
            if used.g > 0 then
                parts[#parts + 1] = string.format("T%d %.1f g", i - 1, used.g)
            end
        end
        if #parts > 0 then
            lines[#lines + 1] = table.concat(parts, "  ")
        end
    end

    local parts = {}
    for _, role in ipairs(INTERESTING_ROLES) do
        local used = slice.filament.per_role[role.key]
        if used and used.g > 0 then
            parts[#parts + 1] = string.format("%s %.1f g", role.label, used.g)
        end
    end
    if #parts > 0 then
        lines[#lines + 1] = table.concat(parts, "  ")
    end

    if #slice.warnings > 0 then
        local codes = {}
        for _, warning in ipairs(slice.warnings) do
            codes[#codes + 1] = warning.code
        end
        lines[#lines + 1] = string.format("%d %s: %s",
            #slice.warnings,
            #slice.warnings == 1 and "warning" or "warnings",
            table.concat(codes, ", "))
    end

    local text = table.concat(lines, "\n")

    -- print() goes to the slicer's log, which is where to look while writing
    -- one of these. report() puts it on screen, replacing this plugin's
    -- previous report rather than stacking a new one on every slice.
    print(text)
    report("Slice report", text)
end
