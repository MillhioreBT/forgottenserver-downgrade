# forgottenserver-downgrade

An Open Tibia server for the 8.60 client, based on The Forgotten Server 1.5 downgrades. C++20 engine in `src/`, game
content and Lua scripts in `data/`, database schema in `schema.sql` (MySQL/MariaDB), server settings in
`config.lua.dist`.

## Layout

- `src/`: the engine. Everything but `main.cpp` builds into the `tfslib` static library; `tfs` links it.
- `src/lua*.cpp`: the Lua API, one file per class (`luaplayer.cpp`, `luaitem.cpp`, ...).
- `data/`: Lua scripts (`actions`, `creaturescripts`, `scripts`, `spells`, ...), shared Lua in `data/lib`, XML in
  `data/XML`, `data/items`, `data/monster`, `data/npc`, map in `data/world`.
- `tests/`: unit tests (`tests/unit/test_<area>.cpp`, Boost.Test). Read `tests/README.md` before adding one.
- `vc17/`: the Visual Studio solution used for Windows builds.

## Build and test

Linux with the dependencies from apt (Ubuntu 24.04; the package list is in `tests/README.md`):

```bash
cmake --preset test && cmake --build --preset test && ctest --preset test
```

This builds `tfslib` and the `tfs_tests` executable in Debug with the tests, then runs them. Use the `test-vcpkg`
presets to take the dependencies from vcpkg instead. To build the server itself, configure without `BUILD_TESTING`
(`cmake --preset default`, or `vcpkg` with the vcpkg toolchain).

## Conventions

- Warnings are errors on every platform but Windows (`-Wall -Wextra -pedantic -Werror`): code must build clean with
  GCC and Clang.
- C++ is formatted with clang-format **22.1.8** (pinned in `.github/workflows/clang-format.yml`; other versions
  format differently). `src/` and `tests/` are checked in CI.
- Lua is checked with `luac -p`, luacheck and LuaFormatter (`.github/workflows/lua-check.yml`); XML with xmllint.
- Commit messages: a short imperative subject in sentence case (`Fix the shop item count overflowing its byte`),
  then a body that says what was wrong and why the change fixes it.
- Pull requests follow `.github/PULL_REQUEST_TEMPLATE.md`.

## Tests

- A bug fix comes with a test that fails without the fix whenever the code can be reached from a unit test. When it
  cannot (it needs the database, the map, the Lua environment or the network), extract the logic into a function
  that does not, and test that.
- Test the behavior the game relies on, not the implementation. If the current behavior looks wrong, report it in
  the pull request instead of writing a test that locks it in.
- Run `ctest --preset test` before pushing C++ changes; CI runs the same command (`.github/workflows/tests.yml`).

## CI

| Workflow | What it checks |
|---|---|
| `tests.yml` | Unit tests on Linux, dependencies from apt |
| `docker-image.yml` | Linux build in Docker, dependencies from vcpkg |
| `windows-build.yml` | `vc17` solution with MSVC, dependencies from vcpkg |
| `macos-build.yml` | Apple Silicon build with AppleClang, dependencies from vcpkg |
| `clang-format.yml` | C++ formatting |
| `lua-check.yml`, `xml-syntax.yml` | Lua and XML in `data/` |
| `claude-code-review.yml` | `@claude` on issues and pull requests (repository owner only) |
