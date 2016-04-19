#ifndef STATIC_DDL_TABLE_MIGRATOR_PREFS_H
#define STATIC_DDL_TABLE_MIGRATOR_PREFS_H

#include <iosfwd>

#include <ddl_column_rules.h>
#include <ddl_table_migrator_prefs.h>

namespace middle {

class static_ddl_table_migrator_prefs {
  std::ostream* verbose_os;

  void init_static_datatype_rules(normal_ddl::ddl_datatype_rules& datatype_rules);
  void init_static_default_value_rules(normal_ddl::ddl_default_value_rules& default_value_rules);

 public:
  static_ddl_table_migrator_prefs(std::ostream* verbose_os = nullptr) : verbose_os{verbose_os} {}

  normal_ddl::ddl_table_migrator_prefs get_table_migrator_prefs();
};
}

#endif
