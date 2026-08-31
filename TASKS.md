# SolGen — Task List

Ordered, top-to-bottom. Each phase depends on the ones above it. Check items off
as they land (`- [x]`).

Spec references: `PLAN.md` (solution model, folder tree, manifest schema),
`CLI_SPEC.md` (command surface).

---

## Phase 0 — Project bootstrap

- [x] Decide `src/SolutionGenerator/` internal source layout (folders for `cli/`, `model/`, `gen/`, `io/`, …)
- [x] Vendor `nlohmann/json.hpp` single header into the tree
- [x] Confirm the interim build path (existing `bin/*.bat` + committed `.exe` + hand-kept `ide/` project) until SolGen can build itself
- [x] Remove dead `Core` references — `bin/make-core.bat`, `ide/Core/`, `Core` entry in `ide/SolutionGenerator.sln`
- [x] Drop the throwaway `New("../../", "TestSolution", ...)` call from `main.cpp`

## Phase 1 — Foundations

- [x] IO layer: atomic write (temp file + rename), recursive delete, directory create, path helpers — harden `FileHelper.hpp`
- [x] CLI parser: hybrid grammar (flat verbs + grouped nouns), global flags (`-n/--dry-run`, `-y/--yes`, `-v/--verbose`, `-h/--help`, `--version`, `--config`)
- [x] Command dispatch table (command path → handler function)
- [x] Error type + exit-code mapping (`0` ok, `1` validation, `2` I/O, `3` declined, `4` build)
- [ ] Logging with verbosity levels; `--verbose` switch

## Phase 2 — Manifest model

- [ ] Data types: `SolutionManifest`, `ProjectManifest`, vendor entry, per-config override block
- [ ] nlohmann (de)serialization for every type
- [ ] Defaults + inheritance merge: built-in profile < `solution.json` < project manifest (scalars replace, lists concat, config maps deep-merge)
- [ ] Solution discovery: upward search from cwd for `tools/config/solution.json`; `--config` override
- [ ] Load-all: parse `solution.json` + every `projects/*.json` into the model
- [ ] Validation pass (report every problem in one go): required fields, `kind` enum, unknown dependency names, duplicate project names, malformed JSON with path to the offending field

## Phase 3 — Dependency graph & identity

- [ ] Build the dependency graph from `dependencies`
- [ ] Topological sort + cycle detection; on failure print the exact cycle
- [ ] Resolve `exports` → additional include dirs propagated to each dependent
- [ ] Resolve `vendor` entries → include dirs, lib dirs (per config), `AdditionalDependencies`, DLLs to copy
- [ ] Deterministic GUID derivation: `uuidv5(SOLGEN_NAMESPACE, solutionName + "/" + projectName)`

## Phase 4 — Source discovery

- [ ] Glob `src/<name>/` for `*.c/cpp/cc/cxx/h/hpp/hxx/inl`
- [ ] Apply `exclude` globs
- [ ] Classify files: `ClCompile` / `ClInclude` / `ResourceCompile` (`.rc`) / `None`
- [ ] Collect `res/<name>/` contents as non-building `None` items

## Phase 5 — Generators

- [ ] Built-in `Debug` / `Release` config profiles (compiler + linker knob tables from `PLAN.md` §3)
- [ ] `.vcxproj` writer: configuration type + subsystem from `kind`, toolset, language standard, character set, conformance mode, warning level/`WX`, per-config profile, `defines`, include dirs, lib dirs, `AdditionalDependencies`, `ProjectReference` for internal deps, forced `OutDir`/`IntDir` into `.ide/bin` & `.ide/obj`, post-build DLL copy
- [ ] `.vcxproj.filters` writer: filter tree mirroring the on-disk folder structure
- [ ] `.sln` writer: project entries + GUIDs, solution configurations, nested solution folders from `group`, build order from the graph
- [ ] Write the full set atomically into `.ide/`

## Phase 6 — `make`

- [ ] Wire the pipeline: load → validate → graph → GUIDs → discover → generate
- [ ] `--only <a,b>`: regenerate listed `.vcxproj` only, still rewrite the full `.sln`
- [ ] Warn on missing declared vendor paths

## Phase 7 — MSBuild integration

- [ ] Locate MSBuild via `vswhere.exe`; `--msbuild <path>` override
- [ ] `build [Debug|Release|all]`: run `make` first, then invoke MSBuild; exit `4` on failure
- [ ] `build --only <a,b>`: restrict MSBuild to those projects

## Phase 8 — Remaining lifecycle commands

- [ ] `run [project] [config] [--detach]`: resolve target exe (default sole `console`/`window`), wait (stream output, forward exit code) or `--detach`
- [ ] `clean`: delete `.ide/bin`, `.ide/obj`, `.ide/.vs`
- [ ] `clear [--yes]`: delete `.ide/` (confirm unless `-y`)
- [ ] `rebuild [config]`: `clean` + `build`

## Phase 9 — Setup / manifest-mutation commands

- [ ] `.bat` wrapper generator: 6 general wrappers + per-project scoped ones, into `tools/bat/`
- [ ] `new <path> --name`: create `tools/`, self-copy the running `.exe`, write `solution.json`, `.gitignore` (`.ide`), general wrappers
- [ ] `project add <name> --kind`: folders `src/<name>` + `res/<name>`, starter `main.cpp` (`int main()` / `WinMain()`), `projects/<name>.json`, `solution.json` entry, per-project wrappers; `--no-scaffold`; `--depends-on`
- [ ] `project remove <name> [--cascade] [--yes]`: remove manifests + entry + wrappers, keep source
- [ ] `project list`
- [ ] `project set <name> <key> <value>` / `set <key> <value>`: scalar set, validate-then-write
- [ ] `dep add` / `dep remove`: with cycle check on add
- [ ] `vendor add` / `vendor remove`
- [ ] `sync-bats`: regenerate the whole `tools/bat/` set from the project list

## Phase 10 — Cross-cutting polish

- [ ] `--dry-run` on every writing/deleting command (print the file plan, touch nothing)
- [ ] Confirmation prompts on destructive ops; `-y` to skip
- [ ] Per-command `--help`; `--version`
- [ ] Non-destructive guarantees: never overwrite hand-modified source; `project remove` never deletes source

## Phase 11 — Validation

- [ ] Build a sandbox test solution with SolGen end to end: `new` → `project add` → `make`
- [ ] Open the generated `.sln` in Visual Studio; confirm it builds
- [ ] Multi-project case with internal `dependencies` (build order, include propagation)
- [ ] Vendor library case: include + lib link + DLL copy
- [ ] Full round trip: `new` → `project add` ×2 → `dep add` → `build` → `run`

## Phase 12 — Deferred (post-v1, see `PLAN.md` §6 / `CLI_SPEC.md` non-goals)

- [ ] Co-located `res/` mode (resources inside the project folder)
- [ ] Platforms beyond `x64`
- [ ] User-defined build configurations (base + delta in `solution.json`)
- [ ] Resource copy-to-output rules
- [ ] Precompiled headers, `.natvis`, custom build steps
- [ ] `--watch` — regenerate on file add/remove
- [ ] SolGen generates its own solution (dogfooding)
- [ ] `vendor` a self-built tool with one command
- [ ] `--only` accepting globs, not just exact names
- [ ] Redirect final release artifacts to a configurable output dir
