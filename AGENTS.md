# AGENTS.md

Instructions for AI coding agents working on this repository.

## Rules

1. **Do not add comments to the code.** Write self-explanatory, readable code instead: names that say what things are
   and do, small functions, and tests whose names describe the behavior. This applies to C++, Lua, CMake, shell and
   workflow files alike. Tool directives are not comments and stay: `// clang-format off` / `// clang-format on`, the
   `} // namespace` markers that clang-format requires, and the `# vX.Y.Z` next to an action pinned to a commit SHA.

## Project

An Open Tibia server for the 8.60 client, based on The Forgotten Server 1.5 downgrades.

- `src/`: the C++20 engine. Everything but `main.cpp` builds into the `tfslib` static library; `tfs` links it. The
  Lua API is in `src/lua*.cpp`, one file per class.
- `data/`: Lua scripts and XML content. `data/global.lua` loads the shared Lua libraries in `data/lib`.
- `tests/`: the tests; `tests/README.md` explains how to run and write them.
- `schema.sql`: the MySQL/MariaDB schema. `config.lua.dist`: the server settings.
- `vc17/`: the Visual Studio solution used for Windows builds.

## Build and test

On Ubuntu 24.04 with the dependencies from apt (the package list is in `tests/README.md`):

```bash
cmake --preset test && cmake --build --preset test && ctest --preset test
```

This builds `tfslib` and the test executables in Debug and runs every test. Use the `test-vcpkg` presets to take the
dependencies from vcpkg. Run it before pushing C++ or Lua changes; CI runs the same command
(`.github/workflows/tests.yml`).

## Tests

| Kind | Files | CTest label | Needs |
|---|---|---|---|
| Unit | `tests/unit/test_<name>.cpp` | `unit` | nothing |
| Integration | `tests/integration/test_<name>.cpp` | `integration` | a database from `schema.sql`, named in `TFS_TEST_MYSQL_DATABASE` |
| Lua | `tests/lua/test_<name>.lua` | `lua` | nothing |

- A bug fix comes with a test that fails without the fix whenever the code can be reached from a test. Check it: put
  the old code back, watch the test fail, restore the fix.
- When the code cannot be reached (it needs the map, the network or a running game), extract the logic into a
  function that does not depend on them and test that function.
- Test the behavior the game relies on. If the current behavior looks wrong, report it in the pull request instead of
  writing a test that locks it in.
- One file is one Boost.Test suite named exactly like the file; CTest runs it on its own. New files are picked up on
  the next build.
- Without `TFS_TEST_MYSQL_DATABASE` the integration tests are reported as skipped, not failed. They add and delete
  rows: never point them at a database with real data.
- Lua tests run in the engine's real Lua environment with `data/global.lua` loaded, through `test(name, body)` and
  the `assert*` functions of `tests/lua/harness.lua`.
- Boost.Test evaluates the message argument of `BOOST_TEST`, `BOOST_REQUIRE_MESSAGE` and friends even when the check
  passes: never give it side effects, such as popping the Lua stack.
- Compare enums through `static_cast<int>` in `BOOST_TEST`: one-byte enums print as characters.

## Conventions

- Warnings are errors on every platform but Windows (`-Wall -Wextra -pedantic -Werror`): code must build clean with
  GCC and Clang.
- C++ in `src/` and `tests/` is formatted with clang-format **22.1.8**, the version pinned in
  `.github/workflows/clang-format.yml`; other versions format differently.
- Lua follows `.lua-format` and is checked by `.github/workflows/lua-check.yml`. XML in `data/` is checked with
  xmllint.
- Commit messages: a short imperative subject in sentence case (`Fix the shop item count overflowing its byte`), then
  a body that says what was wrong and why the change fixes it.
- Pull requests follow `.github/PULL_REQUEST_TEMPLATE.md`, including its Tests section.

## CI

| Workflow | What it checks |
|---|---|
| `tests.yml` | Unit, integration (MariaDB service) and Lua tests on Linux, dependencies from apt |
| `docker-image.yml` | Linux build in Docker, dependencies from vcpkg |
| `windows-build.yml` | `vc17` solution with MSVC, dependencies from vcpkg |
| `macos-build.yml` | Apple Silicon build with AppleClang, dependencies from vcpkg |
| `clang-format.yml` | C++ formatting |
| `lua-check.yml`, `xml-syntax.yml` | Lua and XML |
| `claude-code-review.yml` | `@claude` on issues and pull requests, for the repository owner only |
