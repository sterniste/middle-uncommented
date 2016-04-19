#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include <ddl_table_migrator_prefs.h>
#include <ddl_table_migrator_prefs_reader.h>

using namespace std;
using namespace middle;
using namespace normal_ddl;

TEST(ddl_table_migrator_prefs_reader, test1) {
  const string json_prefs{"{\"datatype_rules\":[{\"type_name\":\"TEXT\",\"datatype\":{\"type\":\"char_length\",\"map_name\":\"blob\",\"map_sizes\":[{\"type\":\"assign\",\"range_max\":-1},{\"type\":\"assign\",\"range_min\":256}]}}],\"default_value_rules\":[{\"macro_lhs\":\"getdate\",\"macro_rhs\":\"NOW\",\"macro_args\":{\"argcnt\":0}}]}"};
  istringstream json_prefs_is{json_prefs};
  ddl_table_migrator_prefs_reader prefs_reader;

  const ddl_table_migrator_prefs table_migrator_prefs{prefs_reader.read_table_migrator_prefs(json_prefs_is)};
}
