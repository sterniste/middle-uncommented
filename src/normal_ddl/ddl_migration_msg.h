#ifndef DDL_MIGRATION_MSG_H
#define DDL_MIGRATION_MSG_H

#include <string>

namespace normal_ddl {

enum class ddl_migration_msg_level : uint8_t { error, warning, remark };

struct ddl_migration_msg {
  static constexpr const char* const table_used_but_undefined_msg = "table used but undefined";
  static constexpr const char* const column_used_but_rejected_msg = "column used but rejected";
  static std::string make_fk_target_table_used_but_undefined_msg(const char* fkref_stname) { return std::string{"FK target table "} + fkref_stname + " used but undefined"; }

  ddl_migration_msg_level level;
  std::string msg;

  ddl_migration_msg(ddl_migration_msg_level level, const std::string& msg) : level{level}, msg{msg} {}

  bool operator==(const ddl_migration_msg& that) const { return level == that.level; }  // compare only level
  bool operator<(const ddl_migration_msg& that) const { return level < that.level; }    // sort only on level

  std::string level_str() const;
};
}

#endif
