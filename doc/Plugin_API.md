# Prusa Slicer Plugin API

This is first version of Slice Plugin API, providing minimal operations to create new objects in Slicer project. 
This document should provide basic information on how plugin works in Prusa Slicer and how to create one.

## Getting started

To quickly create a hello world plugin, the Prusa Slicer CLI provides `plugin init` subcommand, so you can do 
something like this in your terminal:

```bash
cd <your_plugin_workspace>
/Applications/PrusaSlicer.app/Contents/MacOS/PrusaSlicer plugin init com.example.my-plugin 
```

Note: the snippet above is from macos and PrusaSlicer installed as ` /Applications/PrusaSlicer.app`, you may need
to provide path to you installation `PrusaSlicer` executable.

This command above will start interactive CLI wizard asking info about your new plugin and will create single lua file
with hello world like plugin in `com.example.my-plugin`. The info you entered can be changed in `manifest.json` file. 
Tip: for license field you can use Tab key to complete (or list known license identifiers).

To run the plugin in PrusaSlicer you will need to symlink (or copy) the `com.example.my-plugin` 
to `<config_dir>/lua/com.example.my-plugin` and run Plugins -> Rescan menu item command in Prusa Slicer. 


## Plugin anatomy

- Plugins are grouped into _plugin bundles_ (a directory, e.g. `com.prusa3d.slicer.calibratuin`)
- Plugin bundles contains `manifest.json` metadata file and one or more plugins
- Each plugin is single .lua file located under specific directory (e.g. `(datadir)/lua` or `(configdir)/lua`).
- Plugin file has to define `info` variable with description of the plugin.
- A `project.plugin` has to define an `execute` function, that runs the plugin logic.
- A `form.plugin` has to define a `forms` table instead; see [Form plugins](#form-plugins).

### Plugin Bundle Metadata `manifest.json`

The `manifest.json` file describes the plugin bundle.

Here is an example of bundled plugin manifest:

```
{
	"id": "com.prusa3d.slicer.calibration",
	"name": "Calibration patterns",
	"license": "AGPL-3.0-only",
	"min_slicer_version": "3.0.0",
	"version": "1.0.0",
	"author": "prusa3d",
	"description": "Calibration patterns",
	"required_apis": {
		"project.plugin": "1.0.0"
	}
}
```

This is list of recognized `manifest.json` fields. 

| Key                  | Required | Description                                                                         |
|:---------------------|:---------|:------------------------------------------------------------------------------------|
| `id`                 | Yes      | Unique identifer of plugin bundle in reverse DNS form  e.g. `com.example.my-plugin` |
| `name`               | Yes      | Human readable name of plugin bundle                                                |
| `license`            | Yes      | [SPDX identifier](https://spdx.org/licenses/) of license                            |
| `min_slicer_version` | Yes      | Minimal version of Prusa Slicer (e.g. `3.0.0`)                                      |
| `version`            | Yes      | Version of the plugin bundle                                                        |
| `author`             | Yes      | Unique author identifier (e.g. Prusa Account handle)                                |
| `description`        | No       | Description of the plugin bundle                                                    |
| `required_apis`      | Yes      | Map of Plugin APIs (key) and its required minimal version (value). Known APIs are `project.plugin` and `form.plugin` |
| `category`           | No       | Category identifier                                                                 |
| `web`                | No       | Plugin bundle hompage web URL                                                       |
| `repo`               | No       | Plugin bundle source code repository URL                                            |                


### Plugin Metadata `info` structure

The table `info` describes plugin with following keys:
- `id` (string) plugin unique identifier, recommended is reverse domain name like notation
- `type` (string) type of plugin, either `'project.plugin'` or `'form.plugin'`.
- `title` (string) displayed plugin name
- `menu` (string) menu item path to register the plugin under _Plugins_ menu item (e.g. `Calibration/My cool pattern`)
- `params` (array) list of parameter descriptions with following keys:
  - `name` (string) name of key in table as first argument passed to the `execute()` function.
  - `label` (string) displayed name in UI 
  - `type` (string) type of value / UI control, allowed values are:
    - `float` (UI: number input)
    - `int` (UI: number input)
    - `bool` (UI: checkbox)
  - `default` (number or string) default value

### Plugin `execute` function

The function `execute(params)` takes single argument,a table based upon description in the `info.params`. 
Values was filled prior calling the function, by user in UI constructed according the same description.


### Complete minimal example

This is the legendary hello world as a Slicer Plugin:

```lua
info = {
    id = "com.prusa3d.slicer.hello_world",
    type = "project.plugin",
    title = "Hello world",
    menu = "Minimal/Hello world",
    params = {
        {name = "num", label = "Your lucky number", type = "int", default = 42}
    }
}

function execute(params) 
    print("Hello no " .. params.num .. "!")
end
```

Once scanned, it should appear in the main menu under _Plugins_ → _Minimal_ → _Hello world_. 
After activation a simple UI will appear, with single input item of given label, so the user can pass an integer number.
The number can be than read by script as `params.num` in the `print` statement.

## Form plugins

A `form.plugin` changes how settings are presented. It does not add settings, remove
them, or change their values — it says that some of them are better rendered as
something other than one row each.

The settings form is otherwise generated straight from the config: one row per
setting, in the group its definition names. That is a faithful view of the data and
often a poor view of the decision. Two mutually exclusive checkboxes are one choice.
A density between 0 and 100 % is a slider. A group that is a feature plus its
parameters has a switch that belongs in its heading, not in its first row.

Nothing about storage changes. The controls read and write the same settings through
the same path an ordinary row uses, so profiles, the slicing backend and 3MFs never
learn a plugin was involved, and removing a declaration gives that setting its
ordinary row back.

### Declaring, not drawing

A form plugin declares controls; it does not draw them. The settings form is an
immediate-mode UI redrawn every frame alongside the 3D scene, so calling into Lua to
paint it would put a script interpreter in the frame loop. Instead the script runs
once, when plugins are scanned, and the controls it asked for are built and driven in
C++ from then on.

So a form plugin has no `execute()` and gets no _Plugins_ menu entry. Edit the file
and run _Plugins_ → _Rescan_ to see the result.

### The `forms` table

```lua
info = {
    id = "forms",
    type = "form.plugin",
    title = "Settings form controls",
}

forms = {
    { kind = "slider", key = "fill_density", step = 1 },
    { kind = "cards",  key = "fill_pattern", columns = 3 },
    { kind = "section_toggle", key = "ironing" },
    {
        kind = "choice",
        label = "Travel detour",
        options = {
            { label = "Straight",
              set = { avoid_crossing_perimeters = false,
                      avoid_crossing_curled_overhangs = false } },
            { label = "Around perimeters",
              description = "Detour around perimeters so the nozzle crosses them "
                            .. "as little as possible.",
              set = { avoid_crossing_perimeters = true },
              reveals = { "avoid_crossing_perimeters_max_detour" } },
            { label = "Around curled overhangs (experimental)",
              set = { avoid_crossing_curled_overhangs = true } },
        },
    },
}
```

| `kind`           | Renders                                             | Setting must be    | Keys                                            |
|:-----------------|:----------------------------------------------------|:-------------------|:------------------------------------------------|
| `cards`          | a list or grid of options instead of a dropdown      | a choice of values | `key`, optional `columns` (default 1)            |
| `slider`         | a slider with a readout                              | a percentage       | `key`, optional `step` (default 1)               |
| `section_toggle` | a switch in the group's heading, gating the group    | yes/no             | `key`                                            |
| `choice`         | several yes/no settings as the one choice they are   | yes/no per flag    | `label`, `options`                               |

Each `choice` option takes:

- `label` and optional `description`, shown in the control;
- `set`, the value each flag takes while that option is chosen. A flag an option does
  not name is cleared when it is chosen, and ignored when recognising it — so
  "Around perimeters" is still recognised in an older profile that has both
  exclusive flags set. An option with no `set` at all is the "none of the above"
  fallback;
- `reveals`, settings shown only while that option is chosen. They keep their own
  standard control, so they keep their units, bounds and formatting.

### Rules

A control never names its category or option group: the group is looked up from the
settings it renders, so it cannot drift when settings move. That has one consequence
worth knowing — **every setting one control names must be in the same group**, since
a control renders in one place and can only take rows away there.

Declarations are checked when plugins are scanned. A control is rejected, with a
reason in the log, if it names a setting that does not exist, names one whose type it
cannot render, spans two groups, or claims a setting another control already renders.
A rejected control costs that control and nothing else: the settings it named keep
their ordinary rows, and the rest of the file still loads.

In the print form a row is also where per-tool overrides are added and managed,
and a control edits only the print level. So on a printer with several tools, a
control naming a setting that can be overridden per tool is skipped, and that
setting keeps its full row — otherwise replacing the row would take the
overrides away with no way to get them back. On a single-tool printer no such
setting exists and every control applies.

Only register a `section_toggle` for a setting the whole group depends on. The group
collapses while the switch is off, which is right when the rest of the group is inert
and wrong the moment one of its settings still applies.

### Bundled example

`resources/lua/com.prusaslicer.forms/forms.lua` is a working form plugin and the
place to look first. It is a plain text file in the installation: edit it, run
_Plugins_ → _Rescan_, and the form changes.

### Security model

The plugin runtime is *intentionally limited and sandboxed*. 

There are two main restrictions to be aware of:
- no standard `os` and `io` modules are available,
- plugin can access (via `emboss_svg`, `load_stl` and `require`) only files that are in the same directory 
  as the plugin .lua file itself. 

## Plugin API

Plugin API reference is located [here](https://prusa.io/ps-plugins/)

## Plugin distribution

At the moment plugins can be distributed as signed zip files. The plugin author needs to generate her public and private 
RSA keys to create the plugin distribution zip. There is `PrusaSlicer plugin keygen` utility (or you can use `openssl` 
CLI tools). Generating keys is one-time action, you don't need to do again for another plugin bundle. You will need to 
distribute your *public* key named as `<author>.pem`, where `<author>` is value of `author` field in `manifest.json` file.
The *public key* file distribution is again a one-time action.

Following command generates for you private key (the one to **keep secret**) in file `the.author.private.pem`, 
and public key (the one to *distribute*) in file `the.author.public.pem`:

```
<path to you installation>/PrusaSlicer plugin keygen -P the.author.private.pem -p the.author.public.pem
```

Finally, to create sign zip file to distribute the plugin to users, you can run following command (assuming the files 
are named same as in the example commands above):

```
<path to you installation>/PrusaSlicer plugin sign -P the.author.private.pem com.example.my-plugin
```

The output of this command is `com.example.my-plugin.zip` file to distribute.
