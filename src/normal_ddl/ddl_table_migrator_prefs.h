#ifndef DDL_TABLE_MIGRATOR_PREFS_H
#define DDL_TABLE_MIGRATOR_PREFS_H

#include <utility>

#include "ddl_column_rules.h"

namespace normal_ddl {

struct ddl_table_migrator_prefs {
  ddl_datatype_rules datatype_rules;
  ddl_default_value_rules default_value_rules;

  ddl_table_migrator_prefs() = default;
  ddl_table_migrator_prefs(ddl_datatype_rules&& datatype_rules, ddl_default_value_rules&& default_value_rules) : datatype_rules{std::move(datatype_rules)}, default_value_rules{std::move(default_value_rules)} {}
  ddl_table_migrator_prefs(const ddl_table_migrator_prefs& that) = delete;
  ddl_table_migrator_prefs(ddl_table_migrator_prefs&& that) = default;
  ddl_table_migrator_prefs& operator=(const ddl_table_migrator_prefs& that) = delete;
  ddl_table_migrator_prefs& operator=(ddl_table_migrator_prefs&& that) = default;
};
}
#endif