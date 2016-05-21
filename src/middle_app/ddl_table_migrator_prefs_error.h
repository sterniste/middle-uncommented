#ifndef DDL_TABLE_MIGRATOR_PREFS_ERROR_H
#define DDL_TABLE_MIGRATOR_PREFS_ERROR_H

#include <stdexcept>
#include <string>

namespace middle {

struct ddl_table_migrator_prefs_error : public std::runtime_error {
  ddl_table_migrator_prefs_error(const std::string& msg) : std::runtime_error{msg} {}
};
}

#endif
