#ifndef DDL_STNAMES_MIGRATOR_H
#define DDL_STNAMES_MIGRATOR_H

#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <ochain.h>

#include "ddl_migration_msg.h"
#include "ddl_names.h"
#include "ddl_table_migrator.h"

namespace normal_ddl {

struct ddl_column {
  ddl_cname cname;
  ddl_datatype datatype;
  std::unique_ptr<const std::string> default_value;
  bool nullable;
  bool identity;

  ddl_column(ddl_cname&& cname, ddl_datatype&& datatype, std::unique_ptr<const std::string>&& default_value, bool nullable, bool identity) : cname{std::move(cname)}, datatype{std::move(datatype)}, default_value{std::move(default_value)}, nullable{nullable}, identity{identity} {}
  ddl_column(const ddl_column& that) = delete;
  ddl_column(ddl_column&& that) = default;
};

struct ddl_table_info {
  std::vector<ddl_column> columns;
  std::vector<ddl_cname> pk_cnames;

  ddl_table_info(std::vector<ddl_column>&& columns, std::vector<ddl_cname>&& pk_cnames) : columns{std::move(columns)}, pk_cnames{std::move(pk_cnames)} {}
  ddl_table_info(const ddl_table_info& that) = delete;
  ddl_table_info(ddl_table_info&& that) = default;
};

struct ddl_create_table_migration {
  const std::vector<ddl_cname> rejected_cnames;
  const std::vector<ddl_cname> uniquify_identity_cnames;

  ddl_create_table_migration(std::vector<ddl_cname>&& rejected_cnames, std::vector<ddl_cname>&& uniquify_identity_cnames) : rejected_cnames{std::move(rejected_cnames)}, uniquify_identity_cnames{std::move(uniquify_identity_cnames)} {}
};

struct ddl_stnames_migration_msg {
  static constexpr const char* const batch_source_file_msg_suffix = " [batch source file: '";

  unsigned int lineno;
  ddl_stname stname;
  ddl_cname cname;
  ddl_migration_msg msg;

  ddl_stnames_migration_msg(unsigned int lineno, const ddl_stname& stname, const ddl_cname& cname, ddl_migration_msg&& msg) : lineno{lineno}, stname{std::move(stname)}, cname{std::move(cname)}, msg{std::move(msg)} {}

  bool operator==(const ddl_stnames_migration_msg& that) const;
  bool operator<(const ddl_stnames_migration_msg& that) const;

  std::string str(unsigned int lineno_offset) const;
};

using ddl_stnames_migration_msgs = std::vector<ddl_stnames_migration_msg>;

class ddl_stname_migrator_base {
 protected:
  std::ostream* verbose_os;

  bool migrate_char_data_len_ddl(const ddl_datatype& datatype, std::ostream& os);
  bool migrate_num_data_prec_ddl(const ddl_datatype& datatype, std::ostream& os);
  void migrate_datatype_ddl(const ddl_datatype& datatype, std::ostream& os);
  ddl_datatype_migration migrate_table_column_ddl(const ddl_column& column, ddl_table_migrator& app_table_migrator, bool assumed_identity, std::ostream& os);

 public:
  ddl_stname_migrator_base(std::ostream* verbose_os = nullptr) : verbose_os{verbose_os} {}

  void migrate_create_schema_ddl(const char* schema_name, std::ostream& os);
  ddl_create_table_migration migrate_create_table_ddl(normal_ddl::ddl_table_migrator_factory& app_table_migrator_factory, const ddl_stname& stname, const ddl_table_info& table_info, const ddl_assumed_identities& assumed_idents, ddl_stnames_migration_msgs& stnames_migration_msgs, universals::counter_ochain& coc);
  void migrate_ck_constraint_ddl(const ddl_stcname& stcname, const std::string& constraint_name, const std::string& check_clause, std::ostream& os);
  void migrate_uq_constraint_ddl(const ddl_stname& stname, const std::vector<ddl_cname>& cnames, const std::string& constraint_name, std::ostream& os);
  void migrate_fk_constraint_ddl(const ddl_stcname& stcname, const ddl_stcname& ref_stcname, std::ostream& os);
};

class ddl_stnames_migrator {
 protected:
  std::ostream* verbose_os;

  ddl_stnames_migrator(std::ostream* verbose_os) : verbose_os{verbose_os} {}

 public:
  virtual ~ddl_stnames_migrator() {}

  virtual ddl_stnames_migration_msgs migrate_ddl_stnames(normal_ddl::ddl_table_migrator_factory* app_table_migrator_factory, std::ostream& target_ddl_os) = 0;
};

class ddl_stnames_migrator_factory {
 protected:
  std::ostream* verbose_os;

  ddl_stnames_migrator_factory(std::ostream* verbose_os = nullptr) : verbose_os{verbose_os} {}

 public:
  virtual ~ddl_stnames_migrator_factory() {}

  virtual std::unique_ptr<ddl_stnames_migrator> make_stnames_migrator(const ddl_stname_usedcols& stname_usedcols, bool have_usedcols, const ddl_assumed_identities& assumed_idents, std::ostream* verbose_os) = 0;
  virtual const char* get_default_schema_name() const = 0;
};
}

#endif
