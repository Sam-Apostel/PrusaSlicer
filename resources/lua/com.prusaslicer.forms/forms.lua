-- How some settings are presented.
--
-- Every entry below changes only what a setting looks like. The settings
-- themselves, their names, their values and the way they are stored are
-- untouched, so profiles, the slicing backend and 3MFs are unaffected and
-- deleting any entry gives that setting its ordinary row back.
--
-- Edit this file and pick Plugins -> Rescan to see the result. There is no
-- build step; a mistake costs the entry it is in and the rest still load.
--
-- Kinds:
--
--   cards           an enum as a list or grid instead of a dropdown.
--                   key, and optionally columns (default 1).
--   slider          a percentage as a slider with a readout.
--                   key, and optionally step (default 1).
--   section_toggle  a yes/no setting moved into its group's heading, where it
--                   switches the whole group. key.
--   choice          several yes/no settings as the one choice they are.
--                   label and options; each option has a label, a description,
--                   set = the value each flag takes while it is chosen, and
--                   reveals = settings only shown while it is chosen.
--
-- The group a control appears in comes from the settings it names, so it is
-- never written down here and cannot drift. Every setting one control names
-- must be in the same group.

info = {
    id = "forms",
    type = "form.plugin",
    title = "Settings form controls",
}

forms = {
    -- Infill density is a quantity tuned by feel between two ends, which is
    -- what a slider is for. A text field asks for a number when what the user
    -- has is a sense of "a bit more than last time".
    {
        kind = "slider",
        key = "fill_density",
        -- 1 % steps: the slider snaps, and a coarser step would put a stored
        -- value that is not a multiple of it out of reach of the handle.
        step = 1,
    },

    -- Enums chosen by comparing the options rather than by looking one up. A
    -- dropdown shows one and hides the rest, which is the wrong shape for a
    -- decision; laying them out shows the whole decision at once.
    --
    -- Not every enum belongs here. Without pictures, cards beat a dropdown
    -- only while the options are few and their names carry their meaning -- a
    -- long list of bare labels is a dropdown that takes more room. The eight
    -- top/bottom fill patterns and the degree-like ones (draft shield) keep
    -- their dropdowns for that reason.
    { kind = "cards", key = "seam_position" },
    { kind = "cards", key = "fuzzy_skin" },
    { kind = "cards", key = "brim_type" },
    { kind = "cards", key = "support_material_style" },
    { kind = "cards", key = "support_material_pattern" },
    { kind = "cards", key = "ironing_type" },

    -- Infill pattern has eighteen options and no artwork yet, so three columns
    -- of bare labels may well read worse than the dropdown it replaces. It is
    -- here rather than compiled in precisely so that judging it is one line in
    -- a text file: delete this entry and Plugins -> Rescan to compare.
    { kind = "cards", key = "fill_pattern", columns = 3 },

    -- Groups that are one feature and its parameters. The switch moves into
    -- the heading it belongs to, and the group collapses while it is off
    -- rather than showing rows that are all disabled.
    --
    -- Only register a setting the whole group depends on: collapsing hides the
    -- rest, which is right when they are inert and wrong the moment one still
    -- applies.
    { kind = "section_toggle", key = "ironing" },
    { kind = "section_toggle", key = "wipe_tower" },
    { kind = "section_toggle", key = "ooze_prevention" },

    -- Two mutually exclusive strategies and a parameter belonging to one of
    -- them, stored as two booleans and a number. As three checkbox-shaped
    -- rows, nothing stops both being ticked -- the slicer refuses the
    -- combination afterwards -- and the detour length sits there whichever is
    -- chosen. As one choice, the exclusivity is in the control and the
    -- parameter appears with the strategy it belongs to.
    --
    -- A flag an option does not name is cleared when that option is chosen and
    -- ignored when recognising it, so "around perimeters" is still recognised
    -- in an older profile that has both flags set.
    {
        kind = "choice",
        label = "Travel detour",
        options = {
            {
                label = "Straight",
                description = "Travel moves take the direct route.",
                set = {
                    avoid_crossing_perimeters = false,
                    avoid_crossing_curled_overhangs = false,
                },
            },
            {
                label = "Around perimeters",
                description = "Detour around perimeters so the nozzle crosses them as little " ..
                              "as possible. Mostly useful with Bowden extruders, which suffer " ..
                              "from oozing. Slows down both the print and the G-code generation.",
                set = { avoid_crossing_perimeters = true },
                reveals = { "avoid_crossing_perimeters_max_detour" },
            },
            {
                label = "Around curled overhangs (experimental)",
                description = "Detour around areas where the filament may have curled up, " ..
                              "which mostly happens on steeper rounded overhangs and can " ..
                              "otherwise crash the nozzle. Slows down both the print and the " ..
                              "G-code generation.",
                set = { avoid_crossing_curled_overhangs = true },
            },
        },
    },
}
