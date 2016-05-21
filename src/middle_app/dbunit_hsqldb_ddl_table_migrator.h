#ifndef DBUNIT_HSQLDB_DDL_TABLE_MIGRATOR_H
#define DBUNIT_HSQLDB_DDL_TABLE_MIGRATOR_H

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include <ddl_names.h>
#include <ddl_table_migrator.h>
namespace normal_ddl {
struct ddl_table_migrator_prefs;
}

namespace middle {

class dbunit_hsqldb_ddl_table_migrator_impl : public normal_ddl::ddl_table_migrator {
  normal_ddl::ddl_stname stname;
  std::vector<normal_ddl::ddl_cname> pk_cnames;
  bool filter_pk_columns_called;

 public:
  dbunit_hsqldb_ddl_table_migrator_impl(const normal_ddl::ddl_table_migrator_prefs& table_migrator_prefs, const normal_ddl::ddl_stname& stname, const std::vector<normal_ddl::ddl_cname>& pk_cnames, std::ostream* verbose_os = nullptr) : ddl_table_migrator{table_migrator_prefs, verbose_os}, stname{stname}, pk_cnames{pk_cnames}, filter_pk_columns_called{} {}

  virtual normal_ddl::ddl_datatype_migration migrate_datatype_ddl(const normal_ddl::ddl_cname& cname, normal_ddl::ddl_datatype& datatype, bool nullable, const std::string& default_value, bool identity) override;
  virtual std::vector<normal_ddl::ddl_cname> filter_pk_columns() override;
  virtual std::vector<normal_ddl::ddl_cname> uniquify_identity_columns() override;
};

struct dbunit_hsqldb_ddl_table_migrator_factory_impl : public normal_ddl::ddl_table_migrator_factory {
  dbunit_hsqldb_ddl_table_migrator_factory_impl(const normal_ddl::ddl_table_migrator_prefs& table_migrator_prefs, std::ostream* verbose_os = nullptr) : ddl_table_migrator_factory{table_migrator_prefs, verbose_os} {}

  virtual std::unique_ptr<normal_ddl::ddl_table_migrator> make_table_migrator(const normal_ddl::ddl_stname& stname, const std::vector<normal_ddl::ddl_cname>& pk_cnames, std::ostream* verbose_os) override;
};
}

#endif
