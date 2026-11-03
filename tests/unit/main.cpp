// Entry point of tfs_tests. Boost.Test is used in its single-header variant: the framework is compiled once, here, so
// nothing has to be linked and the same code works with Boost from the system or from vcpkg. Test files include
// <boost/test/unit_test.hpp> and define one suite each.
#define BOOST_TEST_MODULE tfs

#include <boost/test/included/unit_test.hpp>
