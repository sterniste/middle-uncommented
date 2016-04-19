#include <iostream>
#include <string>

#include <gtest/gtest.h>

#include <ochain.h>

using namespace std;
using namespace universals;

TEST(string_backed_counter_ochain, test1) {
  string_backed_counter_ochain oc;
  oc.os() << "Hello there Sugar Bear!";
  EXPECT_EQ(0, oc.counter().lines());
  EXPECT_EQ(23, oc.counter().characters());
  string_backed_counter_ochain sbcoc{oc.counter()};
  sbcoc.os() << "growl howl meowl prowl scowl" << endl;
  EXPECT_EQ(1, sbcoc.counter().lines());
  EXPECT_EQ(23 + 29, sbcoc.counter().characters());
  sbcoc.merge_into(oc.os(), true);
  EXPECT_EQ(2, oc.counter().lines());
  EXPECT_EQ(23 + 1 + 29, oc.counter().characters());
  EXPECT_EQ(string{"Hello there Sugar Bear!\ngrowl howl meowl prowl scowl\n"}, oc.str());
}
