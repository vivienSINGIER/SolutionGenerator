# SolGen CLI Specification

Command reference for `solgen`. CLI only — no GUI in v1.

See `PLAN.md` for the solution model, folder tree, and manifest schema. This file
is the source of truth for CLI scope; keep it in sync when the manifest schema
changes.

---

## Grammar

Hybrid:

- **Flat verbs** for the build lifecycle — the commands run constantly:
  `make`, `build`, `run`, `clean`, `clear`, `rebuild`.
- **Grouped nouns** for setup — run once at project start, occasionally later:
  `project`, `dep`, `vendor`, plus the flat `new`, `set`, and `sync-bats`.

## Solution discovery

Every command except `new` locates the solution by **searching upward from the
current directory** for `tools/config/solution.json` — the first match wins, like
`git` finding `.git/`. So commands work from anywhere inside the tree
(`src/Engine/`, the root, …). `--config <path>` points at a `solution.json`
explicitly and skips the search.

## Global flags

| flag | meaning |
|---|---|
| `-n`, `--dry-run` | print the plan (files to create / modify / delete), touch nothing |
| `-y`, `--yes` | assume yes on confirmation prompts |
| `-v`, `--verbose` | verbose logging |
| `-h`, `--help` | help for the current command |
| `--version` | print version and exit |
| `--config <path>` | explicit path to `solution.json` (skips upward search) |

## Conventions

- **Atomic writes** — every file write is temp-file + rename; a crash never leaves
  a half-written `.sln` / `.vcxproj` / manifest.
- **Confirmations** — `clear`, `project remove`, and any overwrite prompt
  interactively, or take `-y` non-interactively. Declined prompt → exit `3`.
- **Dry runs** — every command that writes or deletes supports `--dry-run`.
- **GUIDs** are derived, never cached (see `PLAN.md` §3). A clean `make` always
  reproduces identical project identities.

## Exit codes

| code | meaning |
|---|---|
| `0` | success |
| `1` | manifest / validation error — bad JSON, missing required field, unknown dependency name, dependency cycle, duplicate project |
| `2` | I/O or generation error — cannot write a file, permission denied, path collision |
| `3` | user declined a confirmation prompt |
| `4` | build failure — MSBuild returned non-zero |

---

## Command index

| command | category | status |
|---|---|---|
| `solgen new` | setup | [ ] |
| `solgen project add` | setup | [ ] |
| `solgen project remove` | setup | [ ] |
| `solgen project list` | setup | [ ] |
| `solgen project set` | setup | [ ] |
| `solgen dep add` | setup | [ ] |
| `solgen dep remove` | setup | [ ] |
| `solgen vendor add` | setup | [ ] |
| `solgen vendor remove` | setup | [ ] |
| `solgen set` | setup | [ ] |
| `solgen sync-bats` | setup | [ ] |
| `solgen make` | lifecycle | [ ] |
| `solgen build` | lifecycle | [ ] |
| `solgen run` | lifecycle | [ ] |
| `solgen clean` | lifecycle | [ ] |
| `solgen clear` | lifecycle | [ ] |
| `solgen rebuild` | lifecycle | [ ] |

---

## 1. Setup

### `solgen new <path> --name <name>`

```
solgen new <path> --name <name> [--toolset v143] [--cpp-standard 20] [--force]
```

Creates a new solution at `<path>` (created if missing):

- `tools/solgen.exe` — the running executable copies **itself** here; the in-tree
  copy is what every later command uses.
- `tools/bat/` — the six general wrappers `make.bat` `build.bat` `rebuild.bat`
  `clean.bat` `clear.bat` `run.bat`, each a one-liner forwarding to
  `..\solgen.exe <verb> %*`.
- `tools/config/solution.json` — solution block, empty `projects`.
- `.gitignore` — a single line, `.ide`.

`tools/solgen.exe` and `tools/bat/*.bat` are committed (only `.ide` is ignored),
so a fresh clone builds with no extra step — `build` runs `make` first.

Does **not** create `vendor/` (conditional) or `.ide/` (generated). No interactive
prompts — `--name` is required and the command fails without it. Refuses to run if
`solution.json` already exists unless `--force`.

`--toolset` / `--cpp-standard` override the built-in defaults (`v143`, `20`) in
the written manifest.

### `solgen project add <name> --kind <kind>`

```
solgen project add <name> --kind <console|window|staticlib|dynamiclib>
                   [--depends-on <p>]... [--no-scaffold]
```

In one step:

- creates `src/<name>/` and `res/<name>/`
- for `console` / `window`: writes a starter `src/<name>/main.cpp` — an empty
  `int main()` for `console`, an empty `WinMain()` for `window`
- writes `tools/config/projects/<name>.json` with `kind`
- appends `<name>` to `solution.json` `projects`
- writes `tools/bat/<name>.make.bat` and `<name>.build.bat` (scoped `--only
  <name>`), plus `<name>.run.bat` for `console` / `window`

`--depends-on` is repeatable; each target must already exist (validated, cycle-checked).
`--no-scaffold` writes only the manifests, no folders or starter file.
Fails (exit `1`) if `<name>` is already a project.

### `solgen project remove <name>`

```
solgen project remove <name> [--cascade] [--yes]
```

Removes `projects/<name>.json`, the `solution.json` entry, and the
`tools/bat/<name>.*.bat` wrappers. **Never deletes `src/<name>/`, `res/<name>/`,
or any source.** Refuses (exit `1`) if another project depends on `<name>`, unless
`--cascade` also strips those edges. Prompts unless `--yes`.

### `solgen project list`

```
solgen project list
```

Lists projects: name, kind, dependencies. Read-only.

### `solgen project set <name> <key> <value>`

```
solgen project set <name> <key> <value>
```

Sets a scalar field in `projects/<name>.json` by manifest field name
(`kind`, `subsystem` flags come from `kind`, …). List values are comma-separated
(`defines a,b,c`). The result is validated before writing; an invalid set is
rejected and nothing is written. Structured blocks (`vendor`, `config`) are not
reachable here — use `vendor add` / `vendor remove`, or hand-edit `config`.

### `solgen dep add <project> <dependency>` / `solgen dep remove <project> <dependency>`

```
solgen dep add    <project> <dependency>
solgen dep remove <project> <dependency>
```

Add / remove an entry in `<project>`'s `dependencies`. `add` validates both
projects exist and that the new edge introduces no cycle — on failure it prints
the exact cycle. `remove` is a no-op (exit `0`) if the edge was not present.

### `solgen vendor add <project> <lib>` / `solgen vendor remove <project> <lib>`

```
solgen vendor add <project> <lib> --include <path>... --lib-dir <path>...
                  --libs <a,b,...> [--dlls <x,y,...>]
solgen vendor remove <project> <lib>
```

Add / remove a key in `<project>`'s `vendor` block. Paths are relative to
`vendor/` and are **not** required to exist at edit time — `make` warns if a
declared path is still missing when it runs. `--lib-dir` may be given once (used
for all configs) or as `Debug=<path>` / `Release=<path>` pairs.

### `solgen set <key> <value>`

```
solgen set <key> <value>
```

Sets a scalar field in `solution.json` (`toolset`, `cppStandard`, `warningLevel`,
`warningsAsErrors`, `characterSet`, `conformanceMode`, …). Same validate-then-write
rule as `project set`.

### `solgen sync-bats`

```
solgen sync-bats
```

Regenerates the full `tools/bat/` set from the current project list — the six
general wrappers plus per-project ones. Use it if the wrappers drift or after a
manifest was hand-edited to add a project. Overwrites existing wrappers; removes
wrappers for projects no longer in the manifest.

---

## 2. Lifecycle

### `solgen make`

```
solgen make [--only <a,b,...>]
```

Reads the manifests and writes the IDE files into `.ide/`:

- `.ide/<solution>.sln`
- `.ide/<project>.vcxproj` + `.vcxproj.filters` per project

No compilation. Pipeline: load manifests → validate → resolve dependency graph
(topological sort, cycle check) → derive GUIDs → write files (atomic).

`--only <a,b>` rewrites the `.sln` from the full project list (references stay
valid) but regenerates only the listed `.vcxproj` files.

### `solgen build [config]`

```
solgen build [Debug|Release|all] [--only <a,b,...>]
```

Runs `make` first (always — no change detection), then invokes MSBuild on the
generated `.sln`.

- no arg → `Debug`
- `Release` → Release
- `all` → both configs

`--only` restricts the MSBuild target to the listed projects. Exit `4` if MSBuild
fails. MSBuild is located by running Microsoft's `vswhere.exe` to find the VS
install; `--msbuild <path>` points at `MSBuild.exe` directly (Build-Tools-only
installs, portable installs, CI).

### `solgen run [project] [config]`

```
solgen run [project] [Debug|Release] [--detach]
```

`build` for the given config (default `Debug`), then launches the executable.
`project` defaults to the only `console` / `window` project; with more than one
and no argument, exits `1` listing the candidates.

By default `solgen` **waits** — the app's output goes to the terminal, Ctrl+C
kills it, and `solgen` returns the app's exit code. `--detach` starts the app as
an independent process and returns to the shell immediately (useful for a GUI app
you want to keep open).

### `solgen clean`

```
solgen clean
```

Deletes compiler output only — `.ide/bin/`, `.ide/obj/`, and `.ide/.vs/`. Leaves
the generated `.sln` / `.vcxproj` in place.

### `solgen clear`

```
solgen clear [--yes]
```

Deletes the entire `.ide/` folder — generated projects and build output both.
Prompts unless `--yes`. Never touches `src/`, `res/`, `vendor/`, or `tools/`.

### `solgen rebuild [config]`

```
solgen rebuild [Debug|Release|all]
```

`clean` followed by `build [config]`.

---

## Non-goals for v1

- No GUI — CLI only; revisit once the command set is stable.
- No package-manager integration (vcpkg / Conan) — `vendor` assumes the folder is
  populated by hand.
- No non-MSBuild output (Makefiles, Ninja, Xcode).
- No `--json` machine-readable output mode.
- No user-defined build configurations — `Debug` / `Release` are hardcoded
  (see `PLAN.md` §6).

## Open decisions

_None outstanding._
