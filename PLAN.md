Oh ok, # Solution Generator — Project Plan

Working notes for the custom Solution Generator (`solgen`). `solgen` reads a set of
JSON config files and emits the `.sln` / `.vcxproj` files for a solution, plus the
helper scripts needed to generate and clean it.

Status: **planning**. Nothing here is final; sections marked _(later)_ are deferred.

---

## 1. Solution folder tree

This is the layout `solgen` expects (and can scaffold). Everything except `.ide/`
is committed to source control.

```
MySolution/
  src/                 -> one folder per project; user organizes the inside freely
    Proj1/
    Proj2/
    Sandbox/           -> tests live here as an ordinary project (no special casing)
  res/                 -> parallel to src/, one folder per project
    Proj1/
    Proj2/
  tools/
    solgen.exe         -> the generator (committed; self-copied here by `solgen new`)
    bat/               -> committed wrappers around `solgen <command>`
      make.bat  clean.bat  clear.bat
      Proj1.make.bat   -> per-project, scoped
    config/
      solution.json    -> solution manifest (see section 3)
      projects/
        Proj1.json     -> per-project manifest
        Proj2.json
  vendor/              -> created only when the solution has external dependencies
    <lib>/             -> headers + prebuilt .lib / .dll, paths declared in config
  .ide/                -> GENERATED, git-ignored (see section 2)
  .gitignore           -> a single line: .ide
```

### Notes

- **`src/` and `res/` are parallel trees.** A project `Proj1` has its sources in
  `src/Proj1/` and its resources in `res/Proj1/`. The internal layout of each
  folder is **not enforced** — the user arranges headers, sources, and assets
  however they like. `solgen` discovers files by globbing the project folder.
  _(later: optional co-located mode where `res/` sits inside the project folder.)_
- **Tests are not a special concept.** A test suite is just another project (e.g.
  `src/Sandbox/`) that depends on the projects it exercises.
- **`tools/` replaces the conventional `build/` name** on purpose — `build/` too
  strongly implies compiler output in C++ projects. The generator, its scripts,
  and its config all live under `tools/`.
- **`vendor/` is conditional.** It only exists when the solution links external
  libraries. Libraries are wired in through include paths + lib references; the
  exact paths are declared per-dependency in config because vendor drops have
  inconsistent folder structures. _(later: `solgen` command to vendor an
  internally-built tool with one step.)_

---

## 2. The `.ide/` folder

Everything `solgen` generates, and everything the compiler produces, goes here so
a single `.gitignore` line (`.ide`) covers all of it.

```
.ide/
  MySolution.sln
  Proj1.vcxproj
  Proj1.vcxproj.filters
  Proj1.vcxproj.user
  .vs/                 -> Visual Studio's own cache, lands here next to the .sln
  obj/                 -> intermediate build output (forced IntDir)
  bin/                 -> compiled output (forced OutDir)
```

- `solgen` forces MSBuild's `OutDir` / `IntDir` into `.ide/bin` and `.ide/obj` so
  nothing leaks into `src/`.
- `.ide/` is disposable. After cloning, a developer runs `solgen make` and gets a
  working `.sln`.
- _(later: a config option to redirect final artifacts — the `.exe` and its
  runtime deps — to a chosen output directory, to make producing a usable
  release build easy.)_

---

## 3. Manifests

Two manifest types, both **strict JSON** (no comments, no trailing commas). They
are meant to be managed by `solgen` commands; hand-editing works but is not the
intended workflow.

### Inheritance

```
built-in config profile  <  solution.json  <  projects/<Name>.json
```

- Scalars (`toolset`, `cppStandard`, …): last wins.
- Lists (`defines`, `includeDirs`): concatenated, not replaced.
- Config-keyed maps (`Debug` / `Release` blocks): deep-merged per key.

### GUIDs

Derived, never stored: `uuidv5(SOLGEN_NAMESPACE, solutionName + "/" + projectName)`.
A clean regeneration always yields identical GUIDs, so there is no state file to
commit or resolve in a merge.

### `tools/config/solution.json`

```json
{
  "schemaVersion": 1,

  "name": "MySolution",

  "configurations": ["Debug", "Release"],
  "platform": "x64",
  "toolset": "v143",
  "cppStandard": "20",

  "characterSet": "Unicode",
  "conformanceMode": true,
  "warningLevel": "Level4",
  "warningsAsErrors": false,

  "defines": [],
  "includeDirs": [],

  "config": {
    "Release": { "warningsAsErrors": true }
  },

  "projects": [
    { "name": "Engine", "group": "Libs" },
    { "name": "Game",   "group": "Apps" }
  ]
}
```

| field | default | meaning |
|---|---|---|
| `schemaVersion` | _(required)_ | manifest format version; bump on breaking schema changes |
| `name` | _(required)_ | solution name; `.sln` filename |
| `configurations` | `["Debug","Release"]` | build configurations to emit |
| `platform` | `"x64"` | target platform |
| `toolset` | `"v143"` | `PlatformToolset` |
| `cppStandard` | `"20"` | `LanguageStandard` → `stdcpp20`; also `14` `17` `23` `latest` |
| `characterSet` | `"Unicode"` | `Unicode` or `MultiByte` |
| `conformanceMode` | `true` | `/permissive-` — enforce standard C++ |
| `warningLevel` | `"Level4"` | `TurnOffAllWarnings`, `Level1`..`Level4` |
| `warningsAsErrors` | `false` | `/WX` |
| `defines` | `[]` | preprocessor defines added to every project |
| `includeDirs` | `[]` | include dirs added to every project |
| `config` | `{}` | per-configuration overrides, deep-merged over the scalar fields above; each project's own `config` block deep-merges on top of this |
| `projects` | _(required)_ | ordered list; `group` = IDE solution folder (optional) |

### `tools/config/projects/<Name>.json`

Filename = project name = folder name in `src/` and `res/`.

```json
{
  "schemaVersion": 1,

  "kind": "console",

  "dependencies": ["Engine"],
  "exports": ["."],

  "defines": ["GAME_INTERNAL"],
  "includeDirs": [],
  "exclude": ["experimental/**"],

  "vendor": {
    "fmt": {
      "include": ["fmt/include"],
      "libs": ["fmt.lib"],
      "libDirs": { "Debug": "fmt/lib/x64/Debug", "Release": "fmt/lib/x64/Release" },
      "dlls": []
    }
  },

  "config": {
    "Release": { "defines": ["GAME_SHIP"] }
  }
}
```

| field | default | meaning |
|---|---|---|
| `schemaVersion` | _(required)_ | manifest format version; bump on breaking schema changes |
| `kind` | _(required)_ | `console` \| `window` \| `staticlib` \| `dynamiclib` |
| `dependencies` | `[]` | other solution projects; emits `ProjectReference` and pulls their `exports` onto this project's include path |
| `exports` | `["."]` | lib only: folders (rel. `src/<Name>/`) added to dependents' include path |
| `defines` | `[]` | preprocessor defines, on top of solution `defines` |
| `includeDirs` | `[]` | extra include dirs, rel. `src/<Name>/` |
| `exclude` | `[]` | globs (rel. `src/<Name>/`) removed from source discovery |
| `vendor` | `{}` | external libs; paths rel. `vendor/`; `dlls` copied next to the exe post-build |
| `config` | `{}` | per-configuration overrides, deep-merged over the built-in profile |

Flags applied automatically from `kind`:

| `kind` | `ConfigurationType` | subsystem |
|---|---|---|
| `console` | `Application` | `/SUBSYSTEM:CONSOLE`, entry `main()` |
| `window` | `Application` | `/SUBSYSTEM:WINDOWS`, entry `WinMain()` |
| `staticlib` | `StaticLibrary` | — |
| `dynamiclib` | `DynamicLibrary` | — |

### Built-in config profiles

`Debug` and `Release` are **hardcoded** meanings for now — no user-defined
configurations yet (see section 6). What `solgen` emits for each, before any
project `config` override:

**Compiler (`ClCompile`)**

| knob | Debug | Release |
|---|---|---|
| `Optimization` | `/Od` | `/O2` |
| `InlineFunctionExpansion` | disabled | `/Ob2` |
| `IntrinsicFunctions` | off | `/Oi` |
| `WholeProgramOptimization` | off | `/GL` |
| `RuntimeLibrary` | `/MDd` | `/MD` |
| `BasicRuntimeChecks` | `/RTC1` | off |
| `FunctionLevelLinking` | off | `/Gy` |
| `SupportJustMyCode` | `/JMC` | off |
| `DebugInformationFormat` | `/Zi` | `/Zi` |
| `MultiProcessorCompilation` | `/MP` | `/MP` |
| defines | `_DEBUG` | `NDEBUG` |

**Linker**

| knob | Debug | Release |
|---|---|---|
| `GenerateDebugInformation` | `/DEBUG` | `/DEBUG` |
| `LinkIncremental` | on | off |
| `OptimizeReferences` | off | `/OPT:REF` |
| `EnableCOMDATFolding` | off | `/OPT:ICF` |
| `LinkTimeCodeGeneration` | off | `/LTCG` |

**Always on**, both configs: forced `OutDir` / `IntDir` (section 2),
`WIN32` / `_WIN64`, and `_CONSOLE` or `_WINDOWS` from `kind`.

---

## 4. Command surface _(outline — see CLI_SPEC.md)_

Primary interaction is `solgen <command>`, with `tools/*.bat` as convenience
wrappers. Full spec in `CLI_SPEC.md`. Hybrid grammar:

- **flat lifecycle verbs** — `make`, `clean`, `clear`
- **grouped setup nouns** (manifests are command-managed, not hand-edited) —
  `project add/remove/list/set`, `dep add/remove`, `vendor add/remove`, plus the
  flat `new` and `set`

---

## 5. Decided

- Config lives centrally in `tools/config/`, one file per project.
- Project list is explicit in `solution.json` (no auto-discovery).
- Vendor dependencies are declared per-project, not in a shared registry.
- Manifests are strict JSON, managed through `solgen` commands.
- Project GUIDs are derived, not stored.
- Cross-project include propagation uses the `exports` field only; project
  internals stay freeform.
- `solgen new` copies the running `solgen.exe` into `tools/`; the in-tree copy is
  authoritative from then on.
- Generated folder is `.ide/`; `.gitignore` is the single line `.ide`.
- `tools/solgen.exe` and `tools/bat/*.bat` are committed. A fresh clone needs no
  extra step — `make` regenerates `.ide/`.
- `.bat` wrappers live in `tools/bat/`: three general ones (`make`,
  `clean`, `clear`) plus per-project scoped ones added/removed
  with the project.

## 6. Open questions / deferred

- `build [config] [--only]` — run `make` first, then invoke MSBuild (located via
  `vswhere.exe`, `--msbuild <path>` override); exit `4` on failure.
- `rebuild [config]` — `clean` followed by `build`.
- `run [project] [config] [--detach]` — build, then launch the executable; needs a
  `startupProject` solution field for the multi-runnable default. Also brings back
  `build.bat`, `rebuild.bat`, `run.bat` and per-project `-build.bat` / `-run.bat`
  wrappers.
- Co-located `res/` (inside the project folder) as an alternative to the parallel
  tree.
- Additional platforms beyond `x64`.
- User-defined configurations beyond the hardcoded `Debug` / `Release` (would need
  a base-config + delta mechanism in `solution.json`).
- Resource copy-to-output rules for `res/<Name>/` contents.
- Precompiled headers, `.natvis`, custom build steps.
- `--watch` mode to regenerate on file add/remove.
- Whether `solgen` generates its own solution (dogfooding).
- `make --only` accepting globs rather than exact project names.
```