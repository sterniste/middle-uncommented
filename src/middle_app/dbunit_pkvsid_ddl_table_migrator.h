#ifndef DBUNIT_PKVSID_DDL_TABLE_MIGRATOR_H
#define DBUNIT_PKVSID_DDL_TABLE_MIGRATOR_H

// table migrator which enforces a single identity column per table, filters such from the list of table PKs, returns such to client code via uniquify_identity_columns()

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include <ddl_names.h>
#include <ddl_table_migrator.h>
namespace normal_ddl { struct ddl_table_migrator_prefs; }

namespace middle {

class dbunit_pkvsid_ddl_table_migrator_impl : public normal_ddl::ddl_table_migrator {
  normal_ddl::ddl_stname stname;
  std::vector<normal_ddl::ddl_cname> pk_cnames;
  std::vector<normal_ddl::ddl_cname> identity_cnames;
  unsigned int identity_cnt;
  bool filter_pk_columns_called, uniquify_identity_columns_called;

 public:
  dbunit_pkvsid_ddl_table_migrator_impl(const normal_ddl::ddl_table_migrator_prefs& table_migrator_prefs, const normal_ddl::ddl_stname& stname, const std::vector<normal_ddl::ddl_cname>& pk_cnames, std::ostream* verbose_os = nullptr) : ddl_table_migrator{table_migrator_prefs, verbose_os}, stname{stname}, pk_cnames{pk_cnames}, identity_cnt{}, filter_pk_columns_called{}, uniquify_identity_columns_called{} {}

  virtual normal_ddl::ddl_datatype_migration migrate_datatype_ddl(const normal_ddl::ddl_cname& cname, normal_ddl::ddl_datatype& datatype, bool nullable, const std::string& default_value, bool identity) override;
  virtual std::vector<normal_ddl::ddl_cname> filter_pk_columns() override;
  virtual std::vector<normal_ddl::ddl_cname> uniquify_identity_columns() override;
};

struct dbunit_pkvsid_ddl_table_migrator_factory_impl : public normal_ddl::ddl_table_migrator_factory {
  dbunit_pkvsid_ddl_table_migrator_factory_impl(const normal_ddl::ddl_table_migrator_prefs& table_migrator_prefs, std::ostream* verbose_os = nullptr) : ddl_table_migrator_factory{table_migrator_prefs, verbose_os} {}

  virtual std::unique_ptr<normal_ddl::ddl_table_migrator> make_table_migrator(const normal_ddl::ddl_stname& stname, const std::vector<normal_ddl::ddl_cname>& pk_cnames, std::ostream* verbose_os) override;
};
}

#endif
