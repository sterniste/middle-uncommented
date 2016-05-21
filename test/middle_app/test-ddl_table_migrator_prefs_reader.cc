#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include <ddl_table_migrator_prefs.h>
#include <ddl_table_migrator_prefs_error.h>
#include <ddl_table_migrator_prefs_reader.h>
#include <uc_string.h>

using namespace std;
using namespace middle;
using namespace normal_ddl;
using namespace universals;

TEST(ddl_table_migrator_prefs_reader, test1) {
  istringstream json_prefs_is;
  ddl_table_migrator_prefs_reader prefs_reader;

  try {
    const ddl_table_migrator_prefs table_migrator_prefs{prefs_reader.read_table_migrator_prefs(json_prefs_is)};
    FAIL() << "expected thrown invalid_argument; nothing caught" << endl;
  } catch (const ddl_table_migrator_prefs_error& e) {
    EXPECT_EQ(0, string{e.what()}.find(ddl_table_migrator_prefs_reader::cant_parse_json_prefs_msg_prefix));
  } catch (...) {
    FAIL() << "expected thrown ddl_table_migrator_prefs_error; another caught" << endl;
  }
}

TEST(ddl_table_migrator_prefs_reader, test2) {
  istringstream json_prefs_is{"{\"datatype_rules\":false}"};
  ddl_table_migrator_prefs_reader prefs_reader;

  try {
    const ddl_table_migrator_prefs table_migrator_prefs{prefs_reader.read_table_migrator_prefs(json_prefs_is)};
    FAIL() << "expected thrown invalid_argument; nothing caught" << endl;
  } catch (const ddl_table_migrator_prefs_error& e) {
    EXPECT_EQ(0, string{e.what()}.find(ddl_table_migrator_prefs_reader::cant_parse_json_datatype_rule_prefs_msg_prefix));
    const string at_path{ddl_table_migrator_prefs_reader::at_path_msg_infix + "datatype_rules"};
    EXPECT_EQ(string{e.what()}.length() - at_path.length(), string{e.what()}.rfind(at_path));
  } catch (...) {
    FAIL() << "expected thrown ddl_table_migrator_prefs_error; another caught" << endl;
  }
}

TEST(ddl_table_migrator_prefs_reader, test3) {
  istringstream json_prefs_is{"{\"datatype_rules\":[{\"type_name\":\"TEXT\",\"datatype\":{\"type\":\"char_length\"}}],\"default_value_rules\":{\"howl\":false}}"};
  ddl_table_migrator_prefs_reader prefs_reader;

  try {
    const ddl_table_migrator_prefs table_migrator_prefs{prefs_reader.read_table_migrator_prefs(json_prefs_is)};
    FAIL() << "expected thrown invalid_argument; nothing caught" << endl;
  } catch (const ddl_table_migrator_prefs_error& e) {
    EXPECT_EQ(0, string{e.what()}.find(ddl_table_migrator_prefs_reader::cant_parse_json_default_value_rule_prefs_msg_prefix));
    const string at_path{ddl_table_migrator_prefs_reader::at_path_msg_infix + "default_value_rules[1]"};
    EXPECT_EQ(string{e.what()}.length() - at_path.length(), string{e.what()}.rfind(at_path));
  } catch (...) {
    FAIL() << "expected thrown ddl_table_migrator_prefs_error; another caught" << endl;
  }
}

TEST(ddl_table_migrator_prefs_reader, test4) {
  const string json_prefs{"{\"datatype_rules\":[{\"type_name\":\"TEXT\",\"datatype\":{\"type\":\"char_length\",\"map_name\":\"blob\",\"map_sizes\":[{\"type\":\"assign\",\"range_max\":-1},{\"type\":\"assign\",\"range_min\":256}]}}],\"default_value_rules\":[{\"macro_lhs\":\"getdate\",\"macro_rhs\":\"NOW\",\"macro_args\":{\"argcnt\":0}}]}"};
  istringstream json_prefs_is{json_prefs};
  ddl_table_migrator_prefs_reader prefs_reader;

  const ddl_table_migrator_prefs table_migrator_prefs{prefs_reader.read_table_migrator_prefs(json_prefs_is)};

  EXPECT_EQ(1, table_migrator_prefs.datatype_rules.size());
  const auto datatype_rules_it = table_migrator_prefs.datatype_rules.find(uc_string{"TEXT"});
  EXPECT_NE(table_migrator_prefs.datatype_rules.cend(), datatype_rules_it);
  EXPECT_EQ("blob", datatype_rules_it->second.map_name);
  EXPECT_EQ(ddl_datatype_rule_type::char_length, datatype_rules_it->second.datatype_rule_type);
  EXPECT_EQ(2, datatype_rules_it->second.map_sizes.size());
  auto map_sizes_it = datatype_rules_it->second.map_sizes.cbegin();
  EXPECT_EQ(ddl_map_size_type::assign, map_sizes_it->map_size_type);
  EXPECT_EQ(-1, map_sizes_it->range_max);
  ++map_sizes_it;
  EXPECT_EQ(ddl_map_size_type::assign, map_sizes_it->map_size_type);
  EXPECT_EQ(256, map_sizes_it->range_min);

  EXPECT_EQ(1, table_migrator_prefs.default_value_rules.size());
  const auto default_value_rules_it = table_migrator_prefs.default_value_rules.find(uc_string{"getdate"});
  EXPECT_NE(table_migrator_prefs.default_value_rules.cend(), default_value_rules_it);
  EXPECT_EQ("NOW", default_value_rules_it->second.macro_rhs);
  EXPECT_NE(nullptr, default_value_rules_it->second.macro_args.get());
  EXPECT_EQ(0, default_value_rules_it->second.macro_args->argcnt);
}
