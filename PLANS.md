# Future plans

## Project manifest

Deferred out of the first version of `ProjectManifest` (2026-09-01):

- **`language` field** — for now every project is assumed C++. Add when a second
  language (C, C#, ...) is actually supported.
- **Source layout** — no `sourceDirs` / explicit `files` list yet. The generator
  currently discovers sources by convention. Add explicit control later.
- **Per-project `cppStandard`** — a project inherits the solution's C++ standard.
  Add a project-level override when a real case needs it.
- **Precompiled header** — no `precompiledHeader` (`{ header, source }`) yet.
- **`outputName` override** — output name is always the project `name` for now.
- **Manifest path references** — the solution and project manifests are assumed
  to live in fixed locations relative to each other; no manifest stores a path
  to another manifest.
