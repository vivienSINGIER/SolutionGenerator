# SolGen CLI Parser — Design

How raw `argv` becomes a validated, typed `Invocation` that the dispatch layer can
route. Covers the `cli/` module only.

Spec references: `CLI_SPEC.md` (command surface, flags, exit codes), `PLAN.md`
(solution model). Task: `TASKS.md` Phase 1 — "CLI parser" and "Command dispatch table".

Status: **design**. Object names and file split are proposals; adjust freely.

---

## 1. What the parser is responsible for

**In scope** — `argv` → `Invocation`:

1. Tokenize: `--flag=value` vs `--flag value`; positionals end at the first `--flag`.
2. Resolve the command path through the hybrid grammar (flat verbs + noun groups).
3. Separate global flags (may appear anywhere) from command-local args.
4. Bind positionals and options to the command's declared shape.
5. **Shape validation only**, every problem reported in one pass (mirrors
   `Error::details`): unknown flag, missing required positional/option, missing
   option value, extra positional, bad enum value, duplicate non-repeatable flag.
6. Early-exit pseudo-commands: `--version`, `--help` (general + per-command), no args.

**Out of scope** (explicitly not the parser's job):

| Concern | Owner |
|---|---|
| Routing a command id to a handler function | `cmd/Dispatch` (§6) |
| Resolving `--config` / upward search to a real file | manifest model (Phase 2) |
| Any filesystem access | handlers, via `FileHelper` |
| Semantic checks — project exists, dependency known, cycle | handlers (§7) |
| Confirmation prompts, logging setup | handlers; `GlobalFlags` only carries the switches |

---

## 2. Pipeline at a glance

```
argv
  │  Cli::Parse                     public entry, builds a Parser
  ▼
Parser                              owns all transient state for one run
  │
  ├─ [0] normalize        argv ..........................→ vector<string> tokens
  ├─ [1] extract globals  tokens + GlobalFlagTable ......→ GlobalFlags + wantHelp/Version, shorter tokens
  ├─ [2] version?         wantVersion ...................→ (print, done)
  ├─ [3] resolve path     tokens[0..1] + CommandTable ...→ CommandSpec const*, shorter tokens
  ├─ [4] help?            wantHelp + CommandSpec .........→ (Help renders, done)
  ├─ [5] bind args        tokens + CommandSpec ..........→ ArgBag  (+ Error.details on mistakes)
  └─ [6] assemble         id + GlobalFlags + ArgBag .....→ Invocation
  ▼
Invocation  →  Dispatch  →  Cmd::Xxx(GlobalFlags const&, ArgBag const&)
```

---

## 3. Object catalog

| Object | File | Kind | One-line responsibility |
|---|---|---|---|
| `ValueKind` | `cli/CommandSpec.h` | enum | classify what an arg carries: `Flag / String / Path / Enum / List` |
| `OptionSpec` | `cli/CommandSpec.h` | POD | declare one `--option`: names, kind, repeatable, required, enum values |
| `PositionalSpec` | `cli/CommandSpec.h` | POD | declare one positional slot: name, kind, required |
| `CommandSpec` | `cli/CommandSpec.h` | POD | the full accepted shape of one command |
| `CommandId` | `cli/CommandId.h` | enum | stable identity of each command, for the dispatch `switch` |
| `CommandTable` | `cli/CommandTable.{h,cpp}` | static data + lookup | hold every `CommandSpec`; resolve a name (or noun + subcommand) to one |
| `GlobalFlagTable` | `cli/Cli.cpp` (internal) | static data | the six shared global flags from `CLI_SPEC.md` |
| `GlobalFlags` | `cli/GlobalFlags.h` | POD result | the parsed values of those switches |
| `ArgReader` | `cli/Cli.cpp` (internal) | class | forward cursor over the token vector: `Peek / Next / Eof` |
| `ArgBag` | `cli/ArgBag.{h,cpp}` | result + getters | store parsed command-local values by key; typed accessors |
| `Invocation` | `cli/Invocation.h` | result | the finished parse: `id` + `global` + `args` + `done` |
| `Parser` | `cli/Cli.cpp` (internal) | class | run the stages in order, own transient state, accumulate errors |
| `Cli::Parse` | `cli/Cli.{h,cpp}` | free function | public entry point |
| `Help` | `cli/Help.{h,cpp}` | free functions | render general / group / per-command usage text from the table |
| `Error` | `core/Error.h` | existing | carry failure code + message + accumulated `details` |

### 3.1 `ValueKind` — `cli/CommandSpec.h`

```cpp
enum class ValueKind
{
    Flag,    // presence only, no value:            --force, --no-scaffold
    String,  // one opaque token:                   --name Engine
    Path,    // one token, handler treats as fs::path: --config x/y
    Enum,    // one token, must be in enumValues:    --kind staticlib
    List,    // one token, comma-split at parse:     --only Engine,Game
};
```

`Path` parses exactly like `String`; the distinction only tells the handler which
`ArgBag` getter to call and lets `Help` print `<path>` vs `<value>`.

### 3.2 `OptionSpec` — `cli/CommandSpec.h`

Declares one `--option`. Pure data, no logic.

| field | type | meaning |
|---|---|---|
| `longName` | `std::string` | without dashes: `"kind"` |
| `shortName` | `char` | `'n'`, or `'\0'` if none |
| `kind` | `ValueKind` | what it carries |
| `repeatable` | `bool` | may appear more than once (`--depends-on`); each occurrence appends |
| `required` | `bool` | absence is a shape error |
| `enumValues` | `std::vector<std::string>` | allowed values when `kind == Enum` |
| `help` | `std::string` | one line, for `Help` |

`repeatable` vs `List`: `--depends-on A --depends-on B` (repeatable String) and
`--only A,B` (single List) both land as `["A","B"]` under one key — handlers treat
them identically.

### 3.3 `PositionalSpec` — `cli/CommandSpec.h`

| field | type | meaning |
|---|---|---|
| `name` | `std::string` | key in the `ArgBag`: `"name"`, `"config"` |
| `type` | `CommandValueType` | `Flag` is meaningless here; usually `String` / `Path` / `Enum` |
| `required` | `bool` | `false` → optional trailing slot (`build [config]`) |
| `enumValues` | `std::vector<std::string>` | allowed values when `type == Enum` (mirrors `OptionSpec`) |
| `help` | `std::string` | one line |

The only enum positionals in v1 are the `config` slots of `build` / `rebuild`
(`Debug|Release|all`) and `run` (`Debug|Release`).

Positional order in the vector **is** the argument order on the command line.
No variadic positional in v1 (every command has a fixed count — see `CLI_SPEC.md`).

### 3.4 `CommandSpec` — `cli/CommandSpec.h`

The whole accepted shape of one command.

| field | type | meaning |
|---|---|---|
| `id` | `CommandId` | what dispatch switches on |
| `path` | `std::string` | canonical, space-joined: `"project add"`, `"make"` |
| `positionals` | `std::vector<PositionalSpec>` | in command-line order |
| `options` | `std::vector<OptionSpec>` | **command-local only** — never the globals |
| `summary` | `std::string` | one line for the command index in general help |

### 3.5 `CommandId` — `cli/CommandId.h`

```cpp
enum class CommandId
{
    None = 0,          // no command / parser already handled everything (help, version)
    New,
    ProjectAdd, ProjectRemove, ProjectList, ProjectSet,
    DepAdd, DepRemove, DepList,
    VendorAdd, VendorRemove, VendorList,
    Set, SyncBats,
    Make, Build, Run, Clean, Clear, Rebuild,
};
```

Its own tiny header so `cmd/Dispatch` can include it without pulling the table.

### 3.6 `CommandTable` — `cli/CommandTable.{h,cpp}`

Owns the static array of `CommandSpec` and all name resolution.

```cpp
namespace CommandTable
{
    std::span<CommandSpec const> All();                       // for Help

    // Resolve the leading tokens. Consumes 1 (flat verb) or 2 (noun + subcommand).
    // On success: *_out set, _consumed set, Error::Ok.
    // On failure: Error::Validation with a message (+ "did you mean" later).
    Error Resolve(std::span<std::string const> _tokens,
                  CommandSpec const*& _out, size_t& _consumed);

    bool IsNounGroup(std::string_view _token);                // "project" / "dep" / "vendor"
}
```

The `.cpp` holds the table literal:

```cpp
static CommandSpec const kTable[] =
{
    { CommandId::ProjectAdd, "project add",
      /* positionals */ { { "name", ValueKind::String, true, "new project name" } },
      /* options */ {
          { "kind", 0, ValueKind::Enum, false, true,
            { "console", "window", "staticlib", "dynamiclib" }, "project kind" },
          { "depends-on", 0, ValueKind::String, /*repeatable*/ true, false, {},
            "another project this one links (repeatable)" },
          { "no-scaffold", 0, ValueKind::Flag, false, false, {},
            "write manifests only, no folders or starter file" },
      },
      "create a project: folders, manifest, solution entry, wrappers" },

    { CommandId::Build, "build",
      { { "config", ValueKind::Enum, /*required*/ false, "Debug|Release|all" } },
      { { "only", 0, ValueKind::List, false, false, {}, "restrict to these projects" },
        { "msbuild", 0, ValueKind::Path, false, false, {}, "explicit MSBuild.exe" } },
      "make, then run MSBuild" },

    // ... one entry per row of CLI_SPEC.md's command index
};
```

### 3.7 `GlobalFlags` — `cli/GlobalFlags.h`

The parsed global switches. Every handler receives this by const ref.

```cpp
struct GlobalFlags
{
    bool                    dryRun  = false;   // -n / --dry-run
    bool                    yes     = false;   // -y / --yes
    bool                    verbose = false;   // -v / --verbose
    std::optional<fs::path> configPath;        // --config <path>, unresolved string
};
```

`--help` and `--version` are **not** here — the parser acts on them and never
reaches the handler, so there is nothing to carry.

### 3.8 `GlobalFlagTable` — internal to `cli/Cli.cpp`

```cpp
struct GlobalFlagDef { char const* longName; char shortName; bool takesValue; };

static GlobalFlagDef const kGlobals[] =
{
    { "dry-run", 'n', false },
    { "yes",     'y', false },
    { "verbose", 'v', false },
    { "help",    'h', false },   // special: sets wantHelp
    { "version", 0,   false },   // special: sets wantVersion
    { "config",  0,   true  },
};
```

### 3.9 `ArgReader` — internal to `cli/Cli.cpp`

Small cursor so stage [5] reads cleanly. No ownership, points at the `Parser`'s
token vector.

```cpp
class ArgReader
{
public:
    explicit ArgReader(std::vector<std::string> const& _tokens);

    bool               Eof()  const;
    std::string const& Peek() const;         // "" at Eof
    std::string        Next();               // advance, return current
    bool               NextLooksLikeOption() const;  // starts with "--"
private:
    std::vector<std::string> const* m_tokens;
    size_t                          m_pos = 0;
};
```

### 3.10 `ArgBag` — `cli/ArgBag.{h,cpp}`

Stores every parsed command-local value under its `longName` / positional `name`.
The only stringly-typed surface; the spec has already guaranteed shape by the time
a handler reads it.

```cpp
class ArgBag
{
public:
    void Add(std::string _key, std::string _value);   // parser only

    bool                       Has(std::string_view _key) const;
    std::string                GetString(std::string_view _key) const;   // "" if absent
    std::optional<std::string> GetOptional(std::string_view _key) const;
    fs::path                   GetPath(std::string_view _key) const;
    bool                       GetFlag(std::string_view _key) const;     // == Has
    std::vector<std::string>   GetList(std::string_view _key) const;     // {} if absent
private:
    std::map<std::string, std::vector<std::string>> m_values;
};
```

- Flag option → `Add(name, "true")`; handler calls `GetFlag`.
- Repeatable option → `Add` once per occurrence; handler calls `GetList`.
- `List` option → parser comma-splits, `Add` per piece; handler calls `GetList`.
- Scalar option / positional → single `Add`; handler calls `GetString` / `GetPath`
  (required) or `GetOptional` (optional positional like `build`'s `config`).

### 3.11 `Invocation` — `cli/Invocation.h`

The parser's entire output.

```cpp
struct Invocation
{
    CommandId   id     = CommandId::None;
    GlobalFlags global;
    ArgBag      args;
    bool        done   = false;   // parser already printed help/version; main just exits
};
```

### 3.12 `Parser` — internal to `cli/Cli.cpp`

Instantiated once per `Parse` call. Owns the state that flows between stages and
the accumulating `Error`.

```cpp
class Parser
{
public:
    explicit Parser(std::vector<std::string> _tokens);
    Error Run(Invocation& _out);

private:
    void               ExtractGlobals();                  // [1]
    CommandSpec const* ResolvePath(Error& _err);          // [3]
    void               BindArgs(CommandSpec const&);      // [5]

    std::vector<std::string> m_tokens;        // shrinks as stages consume from it
    GlobalFlags              m_global;
    bool                     m_wantHelp    = false;
    bool                     m_wantVersion = false;
    ArgBag                   m_bag;
    Error                    m_error;         // .details accumulates across stage [5]
};
```

### 3.13 `Cli::Parse` — `cli/Cli.{h,cpp}`

```cpp
namespace Cli
{
    // Non-throwing. On shape error: returns Error{Validation} with every problem
    // in .details, _out left with id == None. On help/version: prints, sets
    // _out.done, returns Error::Ok. On success: _out fully populated.
    Error Parse(int _argc, char** _argv, Invocation& _out);
}
```

### 3.14 `Help` — `cli/Help.{h,cpp}`

```cpp
namespace Help
{
    void General();                          // banner + command index (from CommandTable::All)
    void Group(std::string_view _noun);      // "project" -> its subcommands
    void Command(CommandSpec const& _spec);  // usage line + positionals + options + globals
}
```

Reads the table; writes to stdout; never fails.

---

## 4. Step by step — actor, input, output

### [0] Normalize — *actor: `Cli::Parse`*

- **In:** `argc`, `argv`.
- **Does:** copy `argv[1 .. argc-1]` into `std::vector<std::string>`; construct `Parser`.
- **Out:** `Parser::m_tokens`.
- **Edge:** empty → `Help::General()`, `_out.done = true`, return `Error::Ok`.

### [1] Extract globals — *actor: `Parser::ExtractGlobals`, using `GlobalFlagTable`*

- **In:** `m_tokens`, `kGlobals`.
- **Does:** walk `m_tokens`; for each token that matches a global by long or short
  name: set the field in `m_global` (or `m_wantHelp` / `m_wantVersion`), consume a
  following value for `--config`, and remove the token(s). Matches `--verbose`
  exactly — never a bare word like `verbose`. Non-matches stay.
- **Out:** `m_global`, `m_wantHelp`, `m_wantVersion`, `m_tokens` = command path +
  command-local args only.
- **Edge:** `--config` with no value → `Error::Validation("--config needs a path")`.

### [2] Version short-circuit — *actor: `Parser::Run`*

- **In:** `m_wantVersion`.
- **Does:** if set, print version string.
- **Out:** `_out.done = true`, return `Error::Ok`.

### [3] Resolve command path — *actor: `CommandTable::Resolve`, called by `Parser::Run`*

- **In:** `m_tokens` (front 1–2), the table.
- **Does:**
  - `m_tokens` empty + `m_wantHelp` → `Help::General()`, done.
  - `m_tokens` empty, no help → `Error::Validation("no command given")`.
  - `m_tokens[0]` is a flat verb → resolve, consume 1.
  - `m_tokens[0]` is a noun group → need `m_tokens[1]` as a known subcommand;
    resolve `"<noun> <sub>"`, consume 2. Missing/unknown sub →
    `Error::Validation("`project` needs a subcommand: add, remove, list, set")`.
  - unknown `m_tokens[0]` → `Error::Validation("unknown command 'projct'")`.
- **Out:** `CommandSpec const*`; `m_tokens` = command-local args only.

### [4] Per-command help short-circuit — *actor: `Parser::Run`*

- **In:** `m_wantHelp`, the resolved `CommandSpec`.
- **Does:** if set, `Help::Command(spec)`.
- **Out:** `_out.done = true`, return `Error::Ok`.

### [5] Bind args — *actor: `Parser::BindArgs`, using `ArgReader`, filling `ArgBag`*

- **In:** `m_tokens`, the `CommandSpec`.
- **Does:** iterate tokens with `ArgReader`. Positionals come first; the first
  token starting with `--` flips the reader into "options seen" mode.
  - token **not** starting with `--`:
    - options not yet seen → collect into a local `positionalArgs` vector.
    - options already seen → `details += "unexpected argument 'x'"` (a stray
      positional after options started).
  - `--name` / `--name=val` → look up in `spec.options`.
    - not found → `m_error.details += "unknown option --name"`, keep going.
    - `Flag` → `m_bag.Add(longName, "true")`; inline value → error.
    - value kind → take inline or `Next()`; missing / next looks like an option →
      `details += "--kind needs a value"`.
    - not `repeatable` and key already present → `details += "--kind given more than once"`.
    - `Enum` and value not in `enumValues` → `details += "--kind must be one of ..."`.
    - `List` → comma-split, `Add` each; else `Add` the one value.
  - Then bind `positionalArgs` in `spec.positionals` order: required slot with
    nothing left → `details += "missing <name>"`; leftover args →
    `details += "unexpected argument 'x'"`.
  - Then required-option check: each `required` option absent →
    `details += "missing required option --kind"`.
- **Out:** `m_bag` populated. If `m_error.details` non-empty:
  `m_error.code = Validation`, `m_error.message = "invalid \`project add\` invocation"`.

### [6] Assemble — *actor: `Parser::Run`*

- **In:** resolved `id`, `m_global`, `m_bag`, `m_error`.
- **Does:** if `m_error` has details → return it (`_out.id` stays `None`).
  Else `_out = { spec->id, m_global, std::move(m_bag), false }`.
- **Out:** `Invocation`, `Error::Ok`.

### Compact table

| Step | Actor | Consumes | Produces |
|---|---|---|---|
| 0 normalize | `Cli::Parse` | `argc/argv` | `vector<string>` tokens, a `Parser` |
| 1 globals | `Parser::ExtractGlobals` + `GlobalFlagTable` | tokens | `GlobalFlags`, `wantHelp/Version`, shorter tokens |
| 2 version | `Parser::Run` | `wantVersion` | printout, `done` |
| 3 path | `CommandTable::Resolve` | tokens[0..1] + table | `CommandSpec const*`, shorter tokens |
| 4 help | `Parser::Run` + `Help` | `wantHelp` + spec | printout, `done` |
| 5 bind | `Parser::BindArgs` + `ArgReader` | tokens + spec | `ArgBag` (+ `Error.details`) |
| 6 assemble | `Parser::Run` | id + flags + bag | `Invocation` |

---

## 5. `main` — wiring

```cpp
int main(int _argc, char** _argv)
{
    Invocation inv;
    Error e = Cli::Parse(_argc, _argv, inv);

    if (inv.done)  return e.Exit();                       // help / version already printed
    if (e)       { std::cerr << e.Format() << '\n'; return e.Exit(); }

    e = Dispatch(inv);
    if (e)       { std::cerr << e.Format() << '\n'; return e.Exit(); }
    return 0;
}
```

---

## 6. The seam to dispatch — `cmd/Dispatch.{h,cpp}` (separate task)

Pure routing. Reads only `inv.id`. No parsing knowledge.

```cpp
Error Dispatch(Invocation const& _inv)
{
    switch (_inv.id)
    {
    case CommandId::ProjectAdd: return Cmd::ProjectAdd(_inv.global, _inv.args);
    case CommandId::Build:      return Cmd::Build(_inv.global, _inv.args);
    // ... one line per CommandId
    case CommandId::None:       return Error::Ok();       // unreachable: done was set
    }
    return Error::Validation("unhandled command");
}
```

Every handler has the same signature:

```cpp
Error Cmd::ProjectAdd(GlobalFlags const& _g, ArgBag const& _args);
```

---

## 7. Handler contract — how `ArgBag` is consumed

```cpp
Error Cmd::ProjectAdd(GlobalFlags const& _g, ArgBag const& _args)
{
    // (1) unpack — the one stringly-typed step, top of the function
    std::string const              name       = _args.GetString("name");
    std::string const              kind       = _args.GetString("kind");
    std::vector<std::string> const dependsOn  = _args.GetList("depends-on");
    bool const                     noScaffold = _args.GetFlag("no-scaffold");

    // (2) semantic validation — needs the model, so the parser could not do it.
    //     Load solution.json + projects/*.json, then check, collecting every
    //     problem into one Error{Validation} (same "report all at once" rule):
    //       - is `name` already a project?
    //       - do all `dependsOn` targets exist?
    //       - would the new edges introduce a cycle?  (print the cycle)

    // (3) compute the file plan (create / modify / delete list).

    // (4) if _g.dryRun -> print the plan, return Error::Ok(), touch nothing.

    // (5) execute the plan through FileHelper (atomic writes).
}
```

### Two validation layers

| | Parser | Handler |
|---|---|---|
| Question | "well-formed sentence?" | "makes sense in this solution?" |
| Examples | unknown flag, missing `--kind`, `--kind blah`, extra positional | project exists, unknown dependency, cycle, `solution.json` not found |
| Touches disk | never | yes |
| Test fixture | a `vector<string>` | a fake solution tree |

The parser hands over a **guaranteed well-shaped** `Invocation`; the handler never
re-checks arity or spelling.

---

## 8. File layout

```
src/SolutionGenerator/cli/
  CommandSpec.h      ValueKind, OptionSpec, PositionalSpec, CommandSpec
  CommandId.h        CommandId enum
  CommandTable.h     CommandTable::All / Resolve / IsNounGroup
  CommandTable.cpp   the static kTable literal
  GlobalFlags.h      GlobalFlags
  ArgBag.h / .cpp    ArgBag
  Invocation.h       Invocation
  Cli.h              Cli::Parse
  Cli.cpp            Parser, ArgReader, GlobalFlagTable, Cli::Parse
  Help.h / .cpp      Help::General / Group / Command
```

---

## 9. Worked examples

### 9.1 `solgen -v project add Engine --kind staticlib --depends-on Core --depends-on Math -n`

| step | result |
|---|---|
| 0 | `tokens = [-v, project, add, Engine, --kind, staticlib, --depends-on, Core, --depends-on, Math, -n]` |
| 1 | `global = { verbose:true, dryRun:true }`; `tokens = [project, add, Engine, --kind, staticlib, --depends-on, Core, --depends-on, Math]` |
| 2 | not version |
| 3 | noun `project` + sub `add` → `spec = "project add"`, consume 2; `tokens = [Engine, --kind, staticlib, --depends-on, Core, --depends-on, Math]` |
| 4 | not help |
| 5 | `Engine` → positional. `--kind` seen → options mode. `--kind staticlib` → enum ok → `bag[kind]=[staticlib]`. `--depends-on Core`, `--depends-on Math` (repeatable) → `bag[depends-on]=[Core,Math]`. bind positional `name` ← `Engine` → `bag[name]=[Engine]`. required `--kind` present. no errors. |
| 6 | `Invocation{ id: ProjectAdd, global:{verbose,dryRun}, args:{ name:[Engine], kind:[staticlib], depends-on:[Core,Math] } }` |

Handler unpacks `name="Engine"`, `kind="staticlib"`, `dependsOn=["Core","Math"]`,
`noScaffold=false`; `_g.dryRun` true → prints the file plan, writes nothing.

### 9.2 `solgen build Release --only Engine,Game`

| step | result |
|---|---|
| 0 | `tokens = [build, Release, --only, Engine,Game]` |
| 1–2 | no globals; not version |
| 3 | `build` is a flat verb → `spec = "build"`, consume 1; `tokens = [Release, --only, Engine,Game]` |
| 4 | not help |
| 5 | `Release` → positional. `--only` is `List` → comma-split → `bag[only]=[Engine,Game]`. bind positional `config` ← `Release`, enum ok → `bag[config]=[Release]`. |
| 6 | `Invocation{ id: Build, global:{}, args:{ config:[Release], only:[Engine,Game] } }` |

Handler: `config = _args.GetOptional("config").value_or("Debug")` → `"Release"`;
`only = _args.GetList("only")` → `["Engine","Game"]`.

### 9.3 Error case — `solgen project add --kind statlib`

- step 5 collects: `missing <name>` (positional), `--kind must be one of console,
  window, staticlib, dynamiclib (got 'statlib')`.
- step 6 returns `Error{ Validation, "invalid \`project add\` invocation",
  details: [above two] }`; `main` prints `e.Format()` then a
  `try: solgen project add --help` hint; exit `1`.

---

## 10. Decisions

- **Unknown command/subcommand** → the `Error` message lists every valid command.
  `CommandTable::Resolve` returns the `Error`, so Levenshtein "did you mean …?" can
  be added to that message later with no interface change. _(deferred)_
- **`ArgReader`** → keep, as a ~15-line local struct in `Cli.cpp` (stage [5] needs
  `NextLooksLikeOption()` and inline-`=` handling).
- **Global flags** → accepted **anywhere** in the token stream. Safe: every
  command-local option in `CLI_SPEC.md` is long-form only and none reuse a global
  name.
- **Argument order** → each command is `<path> <positionals...> <options...>`.
  The first `--`-prefixed token ends the positional region; a plain token after
  that is `unexpected argument 'x'`. No `--` end-of-options delimiter in v1 — no
  command has a positional value that can begin with `--` (see `CLI_SPEC.md`
  "Argument order"). Add it later, non-breaking, when a command needs it.
- **Exit codes** → `solgen`, `solgen --help`, `solgen <cmd> --help` print to
  **stdout**, exit **0**. Shape errors print to **stderr**, exit **1** (the
  existing `ErrorCode::Validation` — a missing required arg is a validation error).

---

## 11. Build order

Bottom-up. Every step compiles and is checkable on its own before the next. Steps
1–4 are pure headers (minutes each); the weight is in step 10.

### Phase A — result types (leaf, no dependencies)

- [x] **1. `cli/CommandId.h`** — the `enum class CommandId` (all 17 + `None`).
      Nothing depends on anything. Compiles.
- [x] **2. `cli/GlobalFlags.h`** — the struct (`dryRun`, `yes`, `verbose`,
      `configPath`). Compiles.
- [x] **3. `cli/ArgBag.{h,cpp}`** — the `map` + `Add` + all six getters.
      _Check:_ a scratch `assert(bag.GetString("x") == "1")` after `bag.Add("x","1")`;
      `GetList` returns both after two `Add`s; `GetFlag` false when absent.
- [x] **4. `cli/Invocation.h`** — bundles `CommandId` + `GlobalFlags` + `ArgBag` +
      `done`. Compiles.

### Phase B — the command table

- [x] **5. `cli/CommandSpec.h`** — `ValueKind`, `OptionSpec`, `PositionalSpec`,
      `CommandSpec`. Pure data. Compiles.
- [x] **6. `cli/CommandTable.{h,cpp}` — skeleton with two entries only**: `make`
      (flat verb) and `project add` (noun group). Implement `All()`,
      `IsNounGroup()`, `Resolve()`.
      _Check:_ `Resolve(["make", ...])` → make spec, `consumed == 1`;
      `Resolve(["project","add", ...])` → `consumed == 2`;
      `Resolve(["project"])` → `Error` "needs a subcommand: …";
      `Resolve(["bogus"])` → `Error` listing all commands.

### Phase C — the parser, one stage at a time

Each step adds one private method to `Parser` (in `cli/Cli.cpp`) and wires it into
`Parser::Run`. `Cli::Parse` + `Parser` shell + `main` wiring land in step 7 and
don't change afterward.

- [x] **7. Shell + stage [0] + no-args.** `Cli.h` (`Cli::Parse`), `Parser` class
      with just member state and `Run`. Stage 0: copy `argv` → `m_tokens`.
      Empty → print a one-line placeholder usage, `done = true`, `Error::Ok`.
      Wire `main.cpp` (§5). _Check:_ `solgen` prints the placeholder, exit 0.
- [x] **8. Stage [1] — `ExtractGlobals` + `GlobalFlagTable`.** Recognize and remove
      `-n -y -v --config <v>`; set `m_wantHelp` / `m_wantVersion`.
      _Check:_ `solgen -v --config x make` → `m_global.verbose`, `configPath == "x"`,
      `m_tokens == [make]`. `solgen --version` → prints version, exit 0.
- [x] **9. Stage [3] — resolve path.** Call `CommandTable::Resolve` from `Run`,
      store the `CommandSpec const*`, drop the consumed tokens.
      _Check:_ `solgen make` and `solgen project add` resolve; `solgen bogus`
      errors, exit 1.
- [x] **10. Stage [5] — `BindArgs` + `ArgReader`.** Build it up in sub-steps, each
      verified against the `project add` / `build` specs:
  - [x] 10a. positionals only → `bag["name"]` from `solgen project add Engine`
  - [x] 10b. `Flag` options → `--no-scaffold`
  - [x] 10c. value options, both `--kind x` and `--kind=x`
  - [x] 10d. `Enum` check → `--kind statlib` adds a `details` line
  - [x] 10e. `repeatable` → `--depends-on A --depends-on B` → `["A","B"]`
  - [x] 10f. `List` → `--only A,B` comma-split → `["A","B"]`
  - [x] 10g. order rule → a plain token after the first `--flag` →
        `unexpected argument 'x'`
  - [x] 10h. error accumulation → missing required positional/option, unknown
        option, extra positional, duplicate non-repeatable — all reported at once
- [x] **11. Stage [6] — assemble.** `Run` returns `m_error` if it has details,
      else fills `Invocation`. _Check:_ example 9.1 and 9.2 from this doc produce
      the stated `Invocation`; 9.3 produces the stated error + exit 1.

### Phase D — fill out

- [x] **12. Complete `CommandTable.cpp`** — all 17 commands from `CLI_SPEC.md`'s
      command index, every positional and option. No new logic.
- [x] **13. `cli/Help.{h,cpp}`** — `General()` (banner + command index from
      `CommandTable::All`), `Group(noun)`, `Command(spec)` (usage line built from
      the spec + the global-flags list). Replace the step-7 placeholder; wire the
      `m_wantHelp` short-circuits inside stage [3] (general / group) and at
      stage [4] (per-command).
      _Check:_ `solgen --help`, `solgen project --help`, `solgen build --help`.

### Phase E — the dispatch seam (`TASKS.md` "Command dispatch table")

- [ ] **14. `cmd/Dispatch.{h,cpp}`** — `switch (inv.id)`, every arm
      `return Error::Validation("not implemented: <path>")` for now. Wire into
      `main` after `Cli::Parse`.
      _Milestone:_ every command in `CLI_SPEC.md` now parses, validates its shape,
      and routes. `solgen build Release --only A,B` → "not implemented: build",
      exit 1. The CLI shell is complete; handlers (Phase 9) fill the arms in.

### What "done" looks like

```
solgen                        -> general help, exit 0
solgen --version              -> version, exit 0
solgen project --help         -> project subcommands, exit 0
solgen bogus                  -> "unknown command 'bogus'. commands: …", exit 1
solgen project add            -> "missing <name>", "missing required option --kind", exit 1
solgen -v project add Engine --kind staticlib --depends-on Core -n
                              -> "not implemented: project add", exit 1
                                 (but Invocation is fully built and correct)
```
