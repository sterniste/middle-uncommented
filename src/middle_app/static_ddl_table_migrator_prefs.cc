#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include <ddl_column_rules.h>
#include <ddl_table_migrator_prefs.h>
#include <uc_string.h>

#include "static_ddl_table_migrator_prefs.h"

namespace middle {
using namespace std;
using namespace universals;
using namespace normal_ddl;

void
static_ddl_table_migrator_prefs::init_static_datatype_rules(ddl_datatype_rules& datatype_rules) {
  // can't do this statically, as initializer_list needs to copy construct and our ddl_datatype_rule has no copy ctor
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"BIT", ddl_datatype_rule{ddl_datatype_rule_type::unsized}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"CHAR", ddl_datatype_rule{ddl_datatype_rule_type::char_length}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"NCHAR", ddl_datatype_rule{ddl_datatype_rule_type::char_length, "char"}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"VARCHAR", ddl_datatype_rule{ddl_datatype_rule_type::char_length, "", vector<ddl_map_size>{ddl_map_size{ddl_map_size_type::assign, numeric_limits<int32_t>::min(), 0, 255}, ddl_map_size{ddl_map_size_type::assign, 256, numeric_limits<int32_t>::max(), 255}}}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"NVARCHAR", ddl_datatype_rule{ddl_datatype_rule_type::char_length, "varchar", vector<ddl_map_size>{ddl_map_size{ddl_map_size_type::assign, numeric_limits<int32_t>::min(), 0, 255}, ddl_map_size{ddl_map_size_type::assign, 256, numeric_limits<int32_t>::max(), 255}}}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"TINYINT", ddl_datatype_rule{ddl_datatype_rule_type::precision_scale, "", vector<ddl_map_size>{ddl_map_size{ddl_map_size_type::suppress}, ddl_map_size{ddl_map_size_type::scale_narrow, 0, 0}, ddl_map_size{ddl_map_size_type::scale_reject_defined}}}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"SMALLINT", ddl_datatype_rule{ddl_datatype_rule_type::precision_scale, "", vector<ddl_map_size>{ddl_map_size{ddl_map_size_type::suppress}, ddl_map_size{ddl_map_size_type::scale_narrow, 0, 0}, ddl_map_size{ddl_map_size_type::scale_reject_defined}}}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"INT", ddl_datatype_rule{ddl_datatype_rule_type::precision_scale, "", vector<ddl_map_size>{ddl_map_size{ddl_map_size_type::suppress}, ddl_map_size{ddl_map_size_type::scale_narrow, 0, 0}, ddl_map_size{ddl_map_size_type::scale_reject_defined}}}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"BIGINT", ddl_datatype_rule{ddl_datatype_rule_type::precision_scale, "", vector<ddl_map_size>{ddl_map_size{ddl_map_size_type::suppress}, ddl_map_size{ddl_map_size_type::scale_narrow, 0, 0}, ddl_map_size{ddl_map_size_type::scale_reject_defined}}}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"REAL", ddl_datatype_rule{ddl_datatype_rule_type::precision_scale, "", vector<ddl_map_size>{ddl_map_size{ddl_map_size_type::suppress}, ddl_map_size{ddl_map_size_type::scale_narrow, 0, 0}, ddl_map_size{ddl_map_size_type::scale_reject_defined}}}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"FLOAT", ddl_datatype_rule{ddl_datatype_rule_type::precision_scale, "", vector<ddl_map_size>{ddl_map_size{ddl_map_size_type::suppress}, ddl_map_size{ddl_map_size_type::scale_narrow, 0, 0}, ddl_map_size{ddl_map_size_type::scale_reject_defined}}}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"DOUBLE", ddl_datatype_rule{ddl_datatype_rule_type::precision_scale, "", vector<ddl_map_size>{ddl_map_size{ddl_map_size_type::suppress}, ddl_map_size{ddl_map_size_type::scale_narrow, 0, 0}, ddl_map_size{ddl_map_size_type::scale_reject_defined}}}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"DECIMAL", ddl_datatype_rule{ddl_datatype_rule_type::precision_scale}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"NUMERIC", ddl_datatype_rule{ddl_datatype_rule_type::precision_scale}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"SMALLDATETIME", ddl_datatype_rule{ddl_datatype_rule_type::dt_precision, "datetime", vector<ddl_map_size>{ddl_map_size{ddl_map_size_type::suppress}}}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"DATE", ddl_datatype_rule{ddl_datatype_rule_type::dt_precision, "", vector<ddl_map_size>{ddl_map_size{ddl_map_size_type::suppress}}}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"DATETIME", ddl_datatype_rule{ddl_datatype_rule_type::dt_precision, "", vector<ddl_map_size>{ddl_map_size{ddl_map_size_type::suppress}}}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"DATETIME2", ddl_datatype_rule{ddl_datatype_rule_type::dt_precision, "datetime", vector<ddl_map_size>{ddl_map_size{ddl_map_size_type::suppress}}}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"TIME", ddl_datatype_rule{ddl_datatype_rule_type::dt_precision}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"TIMESTAMP", ddl_datatype_rule{ddl_datatype_rule_type::dt_precision}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"BLOB", ddl_datatype_rule{ddl_datatype_rule_type::char_length, "", vector<ddl_map_size>{ddl_map_size{ddl_map_size_type::assign, numeric_limits<int32_t>::min(), 0, 255}, ddl_map_size{ddl_map_size_type::assign, 256, numeric_limits<int32_t>::max(), 255}}}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"CLOB", ddl_datatype_rule{ddl_datatype_rule_type::char_length, "blob", vector<ddl_map_size>{ddl_map_size{ddl_map_size_type::assign, numeric_limits<int32_t>::min(), 0, 255}, ddl_map_size{ddl_map_size_type::assign, 256, numeric_limits<int32_t>::max(), 255}}}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"TEXT", ddl_datatype_rule{ddl_datatype_rule_type::char_length, "blob", vector<ddl_map_size>{ddl_map_size{ddl_map_size_type::assign, numeric_limits<int32_t>::min(), 0, 255}, ddl_map_size{ddl_map_size_type::assign, 256, numeric_limits<int32_t>::max(), 255}}}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"NTEXT", ddl_datatype_rule{ddl_datatype_rule_type::char_length, "blob", vector<ddl_map_size>{ddl_map_size{ddl_map_size_type::assign, numeric_limits<int32_t>::min(), 0, 255}, ddl_map_size{ddl_map_size_type::assign, 256, numeric_limits<int32_t>::max(), 255}}}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"BINARY", ddl_datatype_rule{ddl_datatype_rule_type::char_length, "", vector<ddl_map_size>{ddl_map_size{ddl_map_size_type::assign, numeric_limits<int32_t>::min(), 0, 255}, ddl_map_size{ddl_map_size_type::assign, 256, numeric_limits<int32_t>::max(), 255}}}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"VARBINARY", ddl_datatype_rule{ddl_datatype_rule_type::char_length, "", vector<ddl_map_size>{ddl_map_size{ddl_map_size_type::assign, numeric_limits<int32_t>::min(), 0, 255}, ddl_map_size{ddl_map_size_type::assign, 256, numeric_limits<int32_t>::max(), 255}}}});
  datatype_rules.emplace(pair<uc_string, ddl_datatype_rule>{"IMAGE", ddl_datatype_rule{ddl_datatype_rule_type::char_length, "blob", vector<ddl_map_size>{ddl_map_size{ddl_map_size_type::assign, numeric_limits<int32_t>::min(), 0, 255}, ddl_map_size{ddl_map_size_type::assign, 256, numeric_limits<int32_t>::max(), 255}}}});
}

void
static_ddl_table_migrator_prefs::init_static_default_value_rules(ddl_default_value_rules& default_value_rules) {
  default_value_rules.emplace(pair<uc_string, ddl_default_value_rule>{"GETDATE()", ddl_default_value_rule{"now", unique_ptr<const ddl_default_value_rule_macro_args>{new ddl_default_value_rule_macro_args{0}}}});
}

ddl_table_migrator_prefs
static_ddl_table_migrator_prefs::get_table_migrator_prefs() {
  ddl_table_migrator_prefs table_migrator_prefs;
  init_static_datatype_rules(table_migrator_prefs.datatype_rules);
  init_static_default_value_rules(table_migrator_prefs.default_value_rules);
  return table_migrator_prefs;
}
}
