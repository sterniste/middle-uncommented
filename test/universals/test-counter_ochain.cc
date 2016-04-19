#include <ostream>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include <ochain.h>

using namespace std;
using namespace universals;

TEST(counter_ochain, test1) {
  ostringstream oss;
  counter_ochain oc{oss};
  EXPECT_EQ(&oss, &oc.sink());
  oc.os() << "Hello there Sugar Bear!" << endl;
  EXPECT_EQ(1, oc.counter().lines());
  EXPECT_EQ(24, oc.counter().characters());
  counter_ochain coc{oc.counter(), oc.sink()};
  coc.os() << "growl howl meowl prowl scowl" << endl;
  EXPECT_EQ(2, coc.counter().lines());
  EXPECT_EQ(24 + 29, coc.counter().characters());
  EXPECT_EQ(string{"Hello there Sugar Bear!\ngrowl howl meowl prowl scowl\n"}, static_cast<ostringstream&>(coc.sink()).str());
}
