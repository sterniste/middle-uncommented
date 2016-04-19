#include <boost/log/core/core.hpp>

#include <gtest/gtest.h>

namespace {
using namespace boost::log;
}

int
main(int argc, char* argv[]) {
  core::get()->set_logging_enabled(false);

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
