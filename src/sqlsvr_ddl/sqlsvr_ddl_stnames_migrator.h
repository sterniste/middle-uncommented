#ifndef SQLSVR_DDL_STNAMES_MIGRATOR_H
#define SQLSVR_DDL_STNAMES_MIGRATOR_H

#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <ddl_names.h>
#include <ddl_stnames_migrator.h>
namespace normal_ddl { class ddl_table_migrator_factory; }
#include <ochain.h>

#include "sqlsvr_ddl_ck_mapper.h"
#include "sqlsvr_ddl_data_source.h"
#include "sqlsvr_ddl_fk_mapper.h"
#include "sqlsvr_ddl_pk_mapper.h"
#include "sqlsvr_ddl_queries.h"
#include "sqlsvr_ddl_tc_mapper.h"
#include "sqlsvr_ddl_uq_mapper.h"

namespace sqlsvr_ddl {

class sqlsvr_ddl_queries_impl : public sqlsvr_ddl_queries {
  std::unique_ptr<sqlsvr_ddl_ck_mapper> ck_mapper;
  std::unique_ptr<sqlsvr_ddl_fk_mapper> fk_mapper;
  std::unique_ptr<sqlsvr_ddl_pk_mapper> pk_mapper;
  std::unique_ptr<sqlsvr_ddl_uq_mapper> uq_mapper;
  std::unique_ptr<sqlsvr_ddl_tc_mapper> tc_mapper;

 public:
  sqlsvr_ddl_queries_impl(const std::vector<sqlsvr_ddl_odbc_data_source>& data_sources, std::ostream* verbose_os = nullptr);

  virtual std::vector<sqlsvr_ddl_ck_rec>* find_ck(const normal_ddl::ddl_stcname& stcname) override { return ck_mapper->find(stcname); }
  virtual sqlsvr_ddl_fk_rec* find_fk(const normal_ddl::ddl_stcname& stcname, bool unhidden = false) override { return fk_mapper->find(stcname, unhidden); }
  virtual std::vector<sqlsvr_ddl_fk_reverse_rec>* find_fk_reverse(const normal_ddl::ddl_stcname& ref_stcname) override { return fk_mapper->find_reverse(ref_stcname); }
  virtual sqlsvr_ddl_fk_rec* find_fk_constraint_name(const std::string& constraint_name, bool unhidden = false) override { return fk_mapper->find_constraint_name(constraint_name, unhidden); }
  virtual sqlsvr_ddl_pk_rec* find_pk(const normal_ddl::ddl_stcname& stcname, bool unhidden = false) override { return pk_mapper->find(stcname, unhidden); }
  virtual std::vector<sqlsvr_ddl_tc_rec>* find_tc(const normal_ddl::ddl_stname& stname) override { return tc_mapper->find(stname); }
  virtual std::vector<sqlsvr_ddl_uq_rec>* find_uq(const normal_ddl::ddl_stcname& stcname) override { return uq_mapper->find(stcname); }
  virtual bool insert_uq(const normal_ddl::ddl_stcname& stcname, const std::vector<sqlsvr_ddl_uq_rec>& uq_recs) override { return uq_mapper->insert(stcname, std::unique_ptr<std::vector<sqlsvr_ddl_uq_rec>>{new std::vector<sqlsvr_ddl_uq_rec>{uq_recs}}); }
};

class sqlsvr_ddl_stnames_migrator_impl : public normal_ddl::ddl_stnames_migrator, private normal_ddl::ddl_stname_migrator_base {
  sqlsvr_ddl_queries& queries;
  const normal_ddl::ddl_stname_usedcols& stname_usedcols;
  bool have_usedcols;
  const normal_ddl::ddl_assumed_identities& assumed_idents;
  std::ostream* verbose_os;

  using ddl_uqrec_colnames = std::unordered_map<sqlsvr_ddl_uq_rec, std::vector<normal_ddl::ddl_cname>, sqlsvr_ddl_uq_rec::hasher>;
  using ddl_fkref_stnames = std::unordered_set<normal_ddl::ddl_stname, normal_ddl::ddl_stname::hasher>;

  static std::vector<std::string> sort_uq_constraint_names(const ddl_uqrec_colnames& uqrec_colnames);

  void uniquify_identity_column(const normal_ddl::ddl_stcname& identity_stcname);
  void hide_rejected_column(const normal_ddl::ddl_stcname& rejected_stcname);
  void migrate_ck_constraints_ddl(const normal_ddl::ddl_stname& stname, universals::counter_ochain& coc);
  ddl_uqrec_colnames map_uqrec_colnames(const normal_ddl::ddl_stname& stname) const;
  void migrate_uq_constraints_ddl(const normal_ddl::ddl_stname& stname, universals::counter_ochain& coc);
  std::unique_ptr<const normal_ddl::ddl_table_info> find_table_info(const normal_ddl::ddl_stname& stname) const;
  ddl_fkref_stnames migrate_validate_fk_constraints_ddl(const normal_ddl::ddl_stname& stname, const normal_ddl::ddl_usedcols* usedcols, bool validate_only, universals::counter_ochain& coc);
  void migrate_validate_fk_constraints_ddl(const normal_ddl::ddl_stname_usedcols& stname_usedcols, bool have_usedcols, const normal_ddl::ddl_assumed_identities& assumed_idents, normal_ddl::ddl_stnames_migration_msgs& stnames_migration_msgs, bool validate_only, universals::counter_ochain& coc);
  void migrate_ddl_stname(normal_ddl::ddl_table_migrator_factory& app_table_migrator_factory, const normal_ddl::ddl_stname& stname, std::string& last_schema_name, const normal_ddl::ddl_table_info& table_info, normal_ddl::ddl_stnames_migration_msgs& stnames_migration_msgs, universals::counter_ochain& coc);

 public:
  sqlsvr_ddl_stnames_migrator_impl(sqlsvr_ddl_queries& queries, const normal_ddl::ddl_stname_usedcols& stname_usedcols, bool have_usedcols, const normal_ddl::ddl_assumed_identities& assumed_idents, std::ostream* verbose_os) : normal_ddl::ddl_stnames_migrator{verbose_os}, normal_ddl::ddl_stname_migrator_base{verbose_os}, queries{queries}, stname_usedcols{stname_usedcols}, have_usedcols{have_usedcols}, assumed_idents{assumed_idents}, verbose_os{verbose_os} {}

  virtual normal_ddl::ddl_stnames_migration_msgs migrate_ddl_stnames(normal_ddl::ddl_table_migrator_factory* app_table_migrator_factory, std::ostream& os) override;
};

class sqlsvr_ddl_stnames_migrator_factory_impl : public normal_ddl::ddl_stnames_migrator_factory, private sqlsvr_ddl_queries_impl {
  std::ostream* verbose_os;

 public:
  sqlsvr_ddl_stnames_migrator_factory_impl(const std::vector<sqlsvr_ddl_odbc_data_source>& data_sources, std::ostream* verbose_os = nullptr) : normal_ddl::ddl_stnames_migrator_factory{verbose_os}, sqlsvr_ddl_queries_impl{data_sources, verbose_os}, verbose_os{verbose_os} {}

  virtual std::unique_ptr<normal_ddl::ddl_stnames_migrator> make_stnames_migrator(const normal_ddl::ddl_stname_usedcols& stname_usedcols, bool have_usedcols, const normal_ddl::ddl_assumed_identities& assumed_idents, std::ostream* verbose_os) override;
  virtual const char* get_default_schema_name() const override { return "dbo"; }
};
}

#endif
