# Tests

Tests for the server code, written with [Boost.Test](https://www.boost.org/doc/libs/release/libs/test/) and run with
CTest, locally and in CI (`.github/workflows/tests.yml`). They build into two executables:

| Executable | Files | CTest label | Needs |
|---|---|---|---|
| `tfs_tests` | `tests/unit/test_<name>.cpp` | `unit` | nothing |
| `tfs_integration_tests` | `tests/integration/test_<name>.cpp` | `integration` | a MySQL/MariaDB database created from `schema.sql` |
| `tfs_tests` (suite `lua`) | `tests/lua/test_<name>.lua` | `lua` | nothing |

Each file is one CTest test, named after the file (`lua_<name>` for Lua files). Integration tests are reported as
skipped when no database is configured, so `ctest` always works.

## Running them

Linux (Ubuntu 24.04), with the dependencies from apt:

```bash
sudo apt-get install ninja-build libboost-system-dev libboost-iostreams-dev libboost-locale-dev \
  libboost-json-dev libboost-test-dev libfmt-dev libmariadb-dev libmariadb-dev-compat libpugixml-dev \
  liblua5.4-dev libssl-dev zlib1g-dev
```

```bash
cmake --preset test && cmake --build --preset test && ctest --preset test
```

With vcpkg instead (any platform, `VCPKG_ROOT` set), use the `test-vcpkg` presets. The first build compiles the server
code once (a few minutes); after that only what changed is rebuilt.

On Windows, `tests/run-windows.ps1` enters the Visual Studio developer environment (MSVC x64) and runs the three
`test-vcpkg` steps with your `VCPKG_ROOT`; the editor task `🧪 Tests (Visual Studio x64)` runs it:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tests\run-windows.ps1
```

### With a database

The integration tests add and delete rows, so give them a database of their own, never one with real data. With
Docker:

```bash
docker run -d --name tfs-test-db -p 3306:3306 -e MARIADB_ROOT_PASSWORD=root -e MARIADB_DATABASE=forgottenserver \
  -e MARIADB_USER=forgottenserver -e MARIADB_PASSWORD=forgottenserver mariadb:11.4
docker exec -i tfs-test-db mariadb -uroot -proot forgottenserver < schema.sql
```

Then point the tests at it; only `TFS_TEST_MYSQL_DATABASE` is required:

| Variable | Default |
|---|---|
| `TFS_TEST_MYSQL_DATABASE` | none: integration tests are skipped |
| `TFS_TEST_MYSQL_HOST` | `127.0.0.1` |
| `TFS_TEST_MYSQL_PORT` | `3306` |
| `TFS_TEST_MYSQL_USER` | `forgottenserver` |
| `TFS_TEST_MYSQL_PASSWORD` | empty |
| `TFS_TEST_MYSQL_SOCK` | empty |

```bash
export TFS_TEST_MYSQL_DATABASE=forgottenserver TFS_TEST_MYSQL_PASSWORD=forgottenserver
ctest --preset test
```

### Running part of them

```bash
ctest --preset test -L unit                            # only the unit tests (or -L integration, -L lua)
ctest --preset test -R networkmessage                  # suites whose name matches
build/test/tests/tfs_tests --run_test=test_tools       # one suite, straight from the executable
build/test/tests/tfs_tests --run_test=test_tools/ucfirst_and_ucwords
build/test/tests/tfs_tests --list_content              # every suite and test case
build/test/tests/tfs_tests --run_test=lua -- tests/lua/test_game.lua   # one Lua file
```

## Adding a test

Create `tests/unit/test_<area>.cpp`, or `tests/integration/test_<area>.cpp` when it needs the database. It is picked up
on the next build, and CTest runs it as `test_<area>`:

```cpp
#include "otpch.h"

#include "the_header_you_test.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(test_<area>)

BOOST_AUTO_TEST_CASE(what_it_does_in_plain_words)
{
	BOOST_TEST(someFunction(2) == 4);
}

BOOST_AUTO_TEST_SUITE_END()
```

- **The suite name is the file name.** Each CTest test runs only its own suite, so a mismatch fails with "no test
  cases matching filter".
- **Name test cases after the behavior** (`a_shared_prefix_is_ambiguous`, not `test3`): the name is what you read
  when it fails, and the code has no comments to explain it.
- **`BOOST_TEST(a == b)`** prints both values when it fails. Add `boost::test_tools::per_element()` for containers to
  see which elements differ, and a message as the second argument inside loops (`BOOST_TEST(ok, "index " << i)`).
  Compare enums through `static_cast<int>`: one-byte enums print as characters.
- **`BOOST_REQUIRE`** stops the test case when the rest makes no sense without that check.
- **Shared setup** goes in a fixture: `BOOST_FIXTURE_TEST_SUITE(test_<area>, MyFixture)` (see `test_wildcardtree.cpp`).
- **Helpers** used by one file go in an anonymous namespace in that file.
- **Integration tests clean up after themselves**: temporary tables, or rows with a `tfs_test` name deleted by the
  fixture (see `test_db_accounts.cpp`), so they can run any number of times on the same database.
- **Test the behavior the game relies on**, not the implementation. When the current behavior looks wrong, say so in
  the pull request instead of writing a test that locks it in.
- **A bug fix comes with a test** that fails without the fix, whenever the code can be reached from a test. Check
  it: put the old code back, see the test fail, then restore the fix.

Everything here builds with the project's warnings as errors (`-Wall -Wextra -pedantic -Werror`) and is checked by
clang-format, like `src/`.

## Lua tests

Lua tests exercise the Lua API the way scripts use it: each `tests/lua/test_<name>.lua` runs in a fresh copy of the
engine's Lua environment, with every class and constant registered and `data/global.lua` (and so `data/lib`) loaded,
from the repository root. No map, items, players or database are loaded.

```lua
test("getReverseDirection points back", function()
	assertEqual(Game.getReverseDirection(DIRECTION_NORTH), DIRECTION_SOUTH)
	assertTrue(Position(1, 2, 3) == Position(1, 2, 3))
end)
```

`tests/lua/harness.lua` provides `test(name, body)`, `assertEqual(actual, expected)`, `assertTrue(value)` and
`assertFalse(value)`; a failure reports the test name, the file and line, and both values. `tests/lua/runner.cpp`
runs the files from the `lua` Boost.Test suite. Lua files follow `.lua-format`, like `data/`, and are checked by
`.github/workflows/lua-check.yml`.

## What can be tested

Both executables link `tfslib`, the whole server except `main.cpp`, so any function or class can be called. What
needs more setup is code that depends on a running server: the loaded map and items, the Lua environment, the network,
the dispatcher threads. Prefer extracting the logic into a function that does not depend on them, and test that.
Lua code that does not need game objects can be tested from `tests/lua`.
