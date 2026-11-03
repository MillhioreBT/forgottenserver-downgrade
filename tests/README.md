# Tests

Unit tests for the server code, written with [Boost.Test](https://www.boost.org/doc/libs/release/libs/test/). They
build into a single executable, `tfs_tests`, and run with CTest, locally and in CI (`.github/workflows/tests.yml`).

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

With vcpkg instead (any platform, `VCPKG_ROOT` set), use the `test-vcpkg` presets:

```bash
cmake --preset test-vcpkg && cmake --build --preset test-vcpkg && ctest --preset test-vcpkg
```

The first build compiles the server code once (a few minutes); after that only what changed is rebuilt.

Useful while working on a test:

```bash
ctest --preset test -R networkmessage                  # suites whose name matches
build/test/tests/tfs_tests --run_test=test_tools       # one suite, straight from the executable
build/test/tests/tfs_tests --run_test=test_tools/ucfirst_and_ucwords
build/test/tests/tfs_tests --list_content              # every suite and test case
```

## Adding a test

Create `tests/unit/test_<area>.cpp`. It is picked up on the next build, and CTest runs it as `test_<area>`:

```cpp
#include "otpch.h"

#include "the_header_you_test.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(test_<area>) // must match the file name

BOOST_AUTO_TEST_CASE(what_it_does_in_plain_words)
{
	BOOST_TEST(someFunction(2) == 4);
}

BOOST_AUTO_TEST_SUITE_END()
```

- **The suite name is the file name.** Each CTest test runs only its own suite, so a mismatch fails with "no test
  cases matching filter".
- **Name test cases after the behavior** (`a_shared_prefix_is_ambiguous`, not `test3`): the name is what you read
  when it fails.
- **`BOOST_TEST(a == b)`** prints both values when it fails. Add `boost::test_tools::per_element()` for containers to
  see which elements differ, and a message as the second argument inside loops (`BOOST_TEST(ok, "index " << i)`).
- **`BOOST_REQUIRE`** stops the test case when the rest makes no sense without that check.
- **Shared setup** goes in a fixture: `BOOST_FIXTURE_TEST_SUITE(test_<area>, MyFixture)` (see `test_wildcardtree.cpp`).
- **Helpers** used by one file go in an anonymous namespace in that file; helpers shared by several go in
  `tests/support/`.
- **Test the behavior the game relies on**, not the implementation. When the current behavior looks wrong, say so in
  the pull request instead of writing a test that locks it in.
- **A bug fix comes with a test** that fails without the fix, whenever the code can be reached from a test.

Everything here builds with the project's warnings as errors (`-Wall -Wextra -pedantic -Werror`) and is checked by
clang-format, like `src/`.

## What can be tested

`tfs_tests` links `tfslib`, the whole server except `main.cpp`, so any function or class can be called. What does not
work without more setup is code that needs a running server: the database connection, the loaded map and items,
the Lua environment, the dispatcher threads. Prefer extracting the logic into a function that does not depend on
them, and test that.
