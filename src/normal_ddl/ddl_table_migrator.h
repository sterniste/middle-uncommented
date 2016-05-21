#ifndef DDL_TABLE_MIGRATOR_H
#define DDL_TABLE_MIGRATOR_H

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <uc_string.h>

#include "ddl_column_rules.h"
#include "ddl_migration_msg.h"
#include "ddl_names.h"
namespace normal_ddl {
struct ddl_table_migrator_prefs;
}

namespace normal_ddl {

struct ddl_datatype {
  universals::uc_string name;
  std::unique_ptr<const int32_t> char_max_len;
  std::unique_ptr<const int32_t> char_oct_len;
  std::unique_ptr<const uint8_t> num_prec;
  std::unique_ptr<const int16_t> num_prec_rad;
  std::unique_ptr<const int32_t> num_scale;
  std::unique_ptr<const int16_t> dt_prec;
  std::unique_ptr<const std::string> charset_name;

  ddl_datatype(const universals::uc_string& name, const int32_t* char_max_len, const int32_t* char_oct_len, const uint8_t* num_prec, const int16_t* num_prec_rad, const int32_t* num_scale, const int16_t* dt_prec, const std::string* charset_name) : name{name}, char_max_len{char_max_len ? new int32_t{*char_max_len} : nullptr}, char_oct_len{char_oct_len ? new int32_t{*char_oct_len} : nullptr}, num_prec{num_prec ? new uint8_t{*num_prec} : nullptr}, num_prec_rad{num_prec_rad ? new int16_t{*num_prec_rad} : nullptr}, num_scale{num_scale ? new int32_t{*num_scale} : nullptr}, dt_prec{dt_prec ? new int16_t{*dt_prec} : nullptr}, charset_name{charset_name ? new std::string{*charset_name} : nullptr} {}
  ddl_datatype(const ddl_datatype& that) : name{that.name}, char_max_len{that.char_max_len ? new int32_t{*that.char_max_len} : nullptr}, char_oct_len{that.char_oct_len ? new int32_t{*that.char_oct_len} : nullptr}, num_prec{that.num_prec ? new uint8_t{*that.num_prec} : nullptr}, num_prec_rad{that.num_prec_rad ? new int16_t{*that.num_prec_rad} : nullptr}, num_scale{that.num_scale ? new int32_t{*that.num_scale} : nullptr}, dt_prec{that.dt_prec ? new int16_t{*that.dt_prec} : nullptr}, charset_name{that.charset_name ? new std::string{*that.charset_name} : nullptr} {}
  ddl_datatype(ddl_datatype&& that) = default;
};

struct ddl_datatype_migration {
  bool column_rejected;
  std::string default_value;
  std::string identity_clause;
  std::string datatype_comment;
  std::vector<ddl_migration_msg> msgs;

  ddl_datatype_migration(bool column_rejected, std::string&& default_value, std::string&& identity_clause, std::string&& datatype_comment, std::vector<ddl_migration_msg>&& msgs) : column_rejected{column_rejected}, default_value{std::move(default_value)}, identity_clause{std::move(identity_clause)}, datatype_comment{std::move(datatype_comment)}, msgs{std::move(msgs)} {}
  ddl_datatype_migration(const ddl_datatype_migration& that) = delete;
  ddl_datatype_migration(ddl_datatype_migration&& that) = default;
};

class ddl_table_migrator {
 protected:
  const ddl_table_migrator_prefs& table_migrator_prefs;
  std::ostream* verbose_os;

  std::string expand_default_value_macro(const std::string& old_default_value);

  ddl_table_migrator(const ddl_table_migrator_prefs& table_migrator_prefs, std::ostream* verbose_os = nullptr) : table_migrator_prefs{table_migrator_prefs}, verbose_os{verbose_os} {}

 public:
  virtual ~ddl_table_migrator() {}

  virtual ddl_datatype_migration migrate_datatype_ddl(const ddl_cname& cname, ddl_datatype& datatype, bool nullable, const std::string& default_value, bool identity) = 0;
  virtual std::vector<ddl_cname> filter_pk_columns() = 0;
  virtual std::vector<ddl_cname> uniquify_identity_columns() = 0;
};

class ddl_table_migrator_factory {
 protected:
  const normal_ddl::ddl_table_migrator_prefs& table_migrator_prefs;
  std::ostream* verbose_os;

 public:
  virtual ~ddl_table_migrator_factory() {}
  ddl_table_migrator_factory(const normal_ddl::ddl_table_migrator_prefs& table_migrator_prefs, std::ostream* verbose_os = nullptr) : table_migrator_prefs{table_migrator_prefs}, verbose_os{verbose_os} {}

  virtual std::unique_ptr<ddl_table_migrator> make_table_migrator(const ddl_stname& stname, const std::vector<ddl_cname>& pk_cnames, std::ostream* verbose_os) = 0;
};
}

#endif
