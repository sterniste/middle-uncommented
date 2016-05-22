#include <algorithm>
#include <cassert>
#include <iterator>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/lexical_cast.hpp>
#ifdef LEVEL_LOGGING
#include <boost/log/trivial.hpp>
#endif

#include <ddl_migration_msg.h>
#include <ddl_names.h>
#include <ddl_stnames_migrator.h>
#include <ddl_table_migrator.h>
#include <ochain.h>
#include <odbc_query.h>

#include "sqlsvr_ddl_connection_error.h"
#include "sqlsvr_ddl_data_source.h"
#include "sqlsvr_ddl_fk_mapper.h"
#include "sqlsvr_ddl_pk_mapper.h"
#include "sqlsvr_ddl_stnames_migrator.h"

namespace sqlsvr_ddl {
using namespace std;
using namespace boost;
using namespace boost::iostreams;
using namespace odbc_query;
using namespace normal_ddl;
using namespace universals;

sqlsvr_ddl_queries_impl::sqlsvr_ddl_queries_impl(const vector<sqlsvr_ddl_odbc_data_source>& data_sources, ostream* verbose_os) {
  assert(!data_sources.empty());
  vector<unique_ptr<connection>> conns;
  sqlsvr_ddl_connection_errors connection_errors;
  auto cend = data_sources.cend();
  for (auto cit = data_sources.cbegin(); cit != cend; ++cit) {
    try {
      conns.push_back(unique_ptr<connection>{new connection{cit->ds_name.c_str(), cit->username.c_str(), cit->password.c_str()}});
    } catch (const connection_error& e) {
      connection_errors.push_back(sqlsvr_ddl_connection_error{static_cast<unsigned int>(cit - data_sources.cbegin()), e.what()});
    }
  }
  if (!connection_errors.empty())
    throw connection_errors;

  sqlsvr_ddl_ck_map_loader ck_loader{conns, verbose_os};
  ck_mapper = ck_loader.load_ck_mapper();
  sqlsvr_ddl_connection_errors ck_connection_errors{ck_loader.get_connection_errors()};
  std::move(ck_connection_errors.begin(), ck_connection_errors.end(), back_inserter(connection_errors));
  sqlsvr_ddl_fk_map_loader fk_loader{conns, verbose_os};
  fk_mapper = fk_loader.load_fk_mapper();
  sqlsvr_ddl_connection_errors fk_connection_errors{fk_loader.get_connection_errors()};
  std::move(fk_connection_errors.begin(), fk_connection_errors.end(), back_inserter(connection_errors));
  sqlsvr_ddl_pk_map_loader pk_loader{conns, verbose_os};
  pk_mapper = pk_loader.load_pk_mapper();
  sqlsvr_ddl_connection_errors pk_connection_errors{pk_loader.get_connection_errors()};
  std::move(pk_connection_errors.begin(), pk_connection_errors.end(), back_inserter(connection_errors));
  sqlsvr_ddl_uq_map_loader uq_loader{conns, verbose_os};
  uq_mapper = uq_loader.load_uq_mapper();
  sqlsvr_ddl_connection_errors uq_connection_errors{uq_loader.get_connection_errors()};
  std::move(uq_connection_errors.begin(), uq_connection_errors.end(), back_inserter(connection_errors));
  sqlsvr_ddl_tc_map_loader tc_loader{conns, verbose_os};
  tc_mapper = tc_loader.load_tc_mapper();
  sqlsvr_ddl_connection_errors tc_connection_errors{tc_loader.get_connection_errors()};
  std::move(tc_connection_errors.begin(), tc_connection_errors.end(), back_inserter(connection_errors));

  if (!connection_errors.empty())
    throw connection_errors;
}

class uq_constraint_sharing {
  unordered_map<string, bool> uq_constraints;

 public:
  uq_constraint_sharing(sqlsvr_ddl_queries& queries, const vector<sqlsvr_ddl_uq_rec>& uq_recs, const ddl_stcname& stcname);

  bool has_unshared_constraint() const;
  bool find_constraint_name(const string& constraint_name) const { return uq_constraints.find(constraint_name) != uq_constraints.end(); }
};

uq_constraint_sharing::uq_constraint_sharing(sqlsvr_ddl_queries& queries, const vector<sqlsvr_ddl_uq_rec>& uq_recs, const ddl_stcname& stcname) {
  // all constraints initially unshared
  for (const auto& uq_rec : uq_recs)
    uq_constraints.insert(pair<string, bool>{uq_rec.constraint_name, false});
  const ddl_stname stname{stcname.stname()};
  // visit all sibling columns (who might share an uniqueness constraint)
  const vector<sqlsvr_ddl_tc_rec>* const tc_recs{queries.find_tc(stname)};
  assert(tc_recs);
  for (const auto& tc_rec : *tc_recs) {
    if (!tc_rec.hidden && tc_rec.cname != ddl_cname{stcname.column_name()}) {
      const vector<sqlsvr_ddl_uq_rec>* const tc_uq_recs{queries.find_uq(ddl_stcname{stname, tc_rec.cname.str()})};
      for (const auto& tc_uq_rec : *tc_uq_recs) {
        auto it = uq_constraints.find(tc_uq_rec.constraint_name);
        if (it != uq_constraints.end())
          it->second = true;  // constraint shared
      }
    }
  }
}

bool
uq_constraint_sharing::has_unshared_constraint() const {
  for (auto const& uq_constraint : uq_constraints) {
    if (!uq_constraint.second)
      return true;
  }
  return false;
}

void
sqlsvr_ddl_stnames_migrator_impl::uniquify_identity_column(const ddl_stcname& identity_stcname) {
  string new_uq_constraint_name{string{"uq_"} + identity_stcname.column_name()};
  const vector<sqlsvr_ddl_uq_rec>* const uq_recs{queries.find_uq(identity_stcname)};
  if (uq_recs) {
    const uq_constraint_sharing constraint_sharing{queries, *uq_recs, identity_stcname};
    if (!constraint_sharing.has_unshared_constraint())
      return;
    if (constraint_sharing.find_constraint_name(new_uq_constraint_name)) {
      auto i = 0U;
      do
        new_uq_constraint_name = string{"uq"} + lexical_cast<string>(i) + identity_stcname.column_name();
      while (constraint_sharing.find_constraint_name(new_uq_constraint_name) && ++i < 10);
      throw runtime_error{string{"can't find unused unique constraint name for identity column "} + identity_stcname.str()};
    }
  }
  const bool ok = queries.insert_uq(identity_stcname, vector<sqlsvr_ddl_uq_rec>{vector<sqlsvr_ddl_uq_rec>{sqlsvr_ddl_uq_rec{new_uq_constraint_name}}});
  assert(ok);
}

void
sqlsvr_ddl_stnames_migrator_impl::hide_rejected_column(const ddl_stcname& rejected_stcname) {
  vector<sqlsvr_ddl_tc_rec>* const tc_recs{queries.find_tc(rejected_stcname.stname())};
  assert(tc_recs);
  bool found = false;
  for (auto& tc_rec : *tc_recs) {
    if (tc_rec.cname == rejected_stcname.cname()) {
      tc_rec.hidden = true;
      found = true;
      break;
    }
  }
  assert(found);
  vector<sqlsvr_ddl_ck_rec>* const ck_recs{queries.find_ck(rejected_stcname)};
  if (ck_recs) {
    for (auto& ck_rec : *ck_recs)
      ck_rec.hidden = true;
  }
  sqlsvr_ddl_fk_rec* const fk_rec{queries.find_fk(rejected_stcname, true)};
  if (fk_rec)
    fk_rec->hidden = true;
  // a rejected column might also be the target of (multiple) FK constraints
  vector<sqlsvr_ddl_fk_reverse_rec>* const fk_reverse_recs{queries.find_fk_reverse(rejected_stcname)};
  if (fk_reverse_recs) {
    for (const auto& fk_reverse_rec : *fk_reverse_recs) {
      sqlsvr_ddl_fk_rec* const fk_constraint_name_rec{queries.find_fk_constraint_name(fk_reverse_rec.constraint_name, true)};
      if (fk_constraint_name_rec)
        fk_constraint_name_rec->hidden = true;
    }
  }
  vector<sqlsvr_ddl_uq_rec>* const uq_recs{queries.find_uq(rejected_stcname)};
  if (uq_recs) {
    for (auto& uq_rec : *uq_recs)
      uq_rec.hidden = true;
  }
}

void
sqlsvr_ddl_stnames_migrator_impl::migrate_ck_constraints_ddl(const ddl_stname& stname, counter_ochain& coc) {
  const vector<sqlsvr_ddl_tc_rec>* const tc_recs{queries.find_tc(stname)};
  if (tc_recs) {
    bool first = true;
    for (const auto& tc_rec : *tc_recs) {
      if (!tc_rec.hidden) {
        const ddl_stcname stcname{stname.schema_name(), stname.table_name(), tc_rec.cname.str()};
        const vector<sqlsvr_ddl_ck_rec>* const ck_recs{queries.find_ck(stcname)};
        if (ck_recs) {
          for (const auto& ck_rec : *ck_recs) {
            if (!ck_rec.hidden) {
              string_backed_counter_ochain sbcoc;
              migrate_ck_constraint_ddl(stcname, ck_rec.constraint_name, ck_rec.check_clause, first ? sbcoc.os() : coc.os());
              if (first)
                sbcoc.merge_into(coc.os(), true);
              first = false;
            }
          }
        }
      }
    }
  } else
    // TODO
    ;
}

sqlsvr_ddl_stnames_migrator_impl::ddl_uqrec_colnames
sqlsvr_ddl_stnames_migrator_impl::map_uqrec_colnames(const ddl_stname& stname) const {
  ddl_uqrec_colnames uqrec_colnames;
  const vector<sqlsvr_ddl_tc_rec>* const tc_recs{queries.find_tc(stname)};
  if (tc_recs) {
    for (const auto& tc_rec : *tc_recs) {
      if (!tc_rec.hidden) {
        const vector<sqlsvr_ddl_uq_rec>* const uq_recs{queries.find_uq(ddl_stcname{stname.schema_name(), stname.table_name(), tc_rec.cname.str()})};
        if (uq_recs) {
          for (const auto& uq_rec : *uq_recs) {
            if (!uq_rec.hidden) {
              const auto it = uqrec_colnames.find(uq_rec);
              if (it != uqrec_colnames.end())
                it->second.push_back(tc_rec.cname);
              else {
                const bool ok = uqrec_colnames.insert(pair<sqlsvr_ddl_uq_rec, vector<ddl_cname>>{uq_rec, vector<ddl_cname>{tc_rec.cname}}).second;
                assert(ok);
              }
            }
          }
        }
      }
    }
  }
  return uqrec_colnames;
}

vector<string>
sqlsvr_ddl_stnames_migrator_impl::sort_uq_constraint_names(const ddl_uqrec_colnames& uqrec_colnames) {
  vector<string> uq_constraint_names;
  for (const auto uqrec_colname : uqrec_colnames)
    uq_constraint_names.push_back(uqrec_colname.first.constraint_name);
  sort(uq_constraint_names.begin(), uq_constraint_names.end());
  return uq_constraint_names;
}

void
sqlsvr_ddl_stnames_migrator_impl::migrate_uq_constraints_ddl(const ddl_stname& stname, counter_ochain& coc) {
  const vector<sqlsvr_ddl_tc_rec>* const tc_recs{queries.find_tc(stname)};
  if (tc_recs) {
    const ddl_uqrec_colnames uqrec_colnames{map_uqrec_colnames(stname)};
    const vector<string> uq_constraint_names{sort_uq_constraint_names(uqrec_colnames)};
    bool first = true;
    for (const auto& uq_constraint_name : uq_constraint_names) {
      const auto it = uqrec_colnames.find(sqlsvr_ddl_uq_rec{uq_constraint_name});
      assert(it != uqrec_colnames.end() && !it->second.empty());
      const vector<ddl_cname>& cnames{it->second};
      string_backed_counter_ochain sbcoc;
      migrate_uq_constraint_ddl(stname, cnames, uq_constraint_name, first ? sbcoc.os() : coc.os());
      if (first)
        sbcoc.merge_into(coc.os(), true);
      first = false;
    }
  } else
    // TODO
    ;
}

sqlsvr_ddl_stnames_migrator_impl::ddl_fkref_stnames
sqlsvr_ddl_stnames_migrator_impl::migrate_validate_fk_constraints_ddl(const ddl_stname& stname, const ddl_usedcols* usedcols, bool validate_only, counter_ochain& coc) {
  ddl_fkref_stnames fk_ref_stnames;
  const vector<sqlsvr_ddl_tc_rec>* const tc_recs{queries.find_tc(stname)};
  if (tc_recs) {
    bool first = true;
    for (const auto& tc_rec : *tc_recs) {
      const ddl_stcname stcname{stname, tc_rec.cname.str()};
      const sqlsvr_ddl_fk_rec* const fk_rec{queries.find_fk(stcname, true)};
      if (fk_rec) {
        string_backed_counter_ochain sbcoc;
        if (!validate_only)
          migrate_fk_constraint_ddl(stcname, fk_rec->ref_stcname, first ? sbcoc.os() : coc.os());
        if (usedcols) {
          const auto it = usedcols->find(ddl_cname{stcname.column_name()});
          if (it != usedcols->end())
            fk_ref_stnames.insert(fk_rec->ref_stcname.stname());
        }
        if (!validate_only) {
          if (first)
            sbcoc.merge_into(coc.os(), true);
          first = false;
        }
      }
    }
  } else
    // TODO
    ;
  return fk_ref_stnames;
}

void
sqlsvr_ddl_stnames_migrator_impl::migrate_validate_fk_constraints_ddl(const ddl_stname_usedcols& stname_usedcols, bool have_usedcols, const ddl_assumed_identities& assumed_idents, ddl_stnames_migration_msgs& stnames_migration_msgs, bool validate_only, counter_ochain& coc) {
  ddl_fkref_stnames fkref_stnames;
  for (const auto& stname_usedcol : stname_usedcols) {
    const ddl_usedcols* usedcols = nullptr;
    if (have_usedcols) {
      usedcols = stname_usedcols.find_usedcols(stname_usedcol.first);
      assert(usedcols);
    }
    const ddl_fkref_stnames table_fk_ref_stnames{migrate_validate_fk_constraints_ddl(stname_usedcol.first, usedcols, validate_only, coc)};
    copy(table_fk_ref_stnames.cbegin(), table_fk_ref_stnames.cend(), inserter(fkref_stnames, fkref_stnames.end()));
  }
  unsigned int missing_fk_target_cnt = 0;
  for (const auto& fkref_stname : fkref_stnames) {
    if (!stname_usedcols.find_usedcols(fkref_stname)) {
      stnames_migration_msgs.push_back(ddl_stnames_migration_msg{0, ddl_stname{}, ddl_cname{}, ddl_migration_msg{ddl_migration_msg_level::warning, ddl_migration_msg::make_fk_target_table_used_but_undefined_msg(fkref_stname.str().c_str())}});
      ++missing_fk_target_cnt;
    }
  }
  if (missing_fk_target_cnt) {
#ifdef LEVEL_LOGGING
    BOOST_LOG_TRIVIAL(info) << "migrating fk constraints: " << missing_fk_target_cnt << " FK target table(s) used but undefined";
#endif
    if (verbose_os)
      *verbose_os << missing_fk_target_cnt << " FK target table(s) used but undefined" << endl;
  }
}

vector<ddl_stname>
sort_stname_usedcols_stnames(const ddl_stname_usedcols& stname_usedcols) {
  vector<ddl_stname> stnames;
  auto end = stname_usedcols.end();
  for (auto it = stname_usedcols.begin(); it != end; ++it)
    stnames.push_back(it->first);
  sort(stnames.begin(), stnames.end());
  return stnames;
}

unique_ptr<const ddl_table_info>
sqlsvr_ddl_stnames_migrator_impl::find_table_info(const ddl_stname& stname) const {
  const vector<sqlsvr_ddl_tc_rec>* const tc_recs{queries.find_tc(stname)};
  if (!tc_recs)
    return nullptr;
  vector<ddl_column> columns;
  vector<ddl_cname> pk_cnames;
  for (const auto& tc_rec : *tc_recs) {
    ddl_datatype datatype{tc_rec.datatype, tc_rec.char_max_len.get(), tc_rec.char_oct_len.get(), tc_rec.num_prec.get(), tc_rec.num_prec_rad.get(), tc_rec.num_scale.get(), tc_rec.dt_prec.get(), tc_rec.charset_name.get()};
    unique_ptr<const string> default_val{tc_rec.default_val ? new string(*tc_rec.default_val) : nullptr};
    columns.push_back(ddl_column{ddl_cname{tc_rec.cname}, std::move(datatype), std::move(default_val), tc_rec.nullable, tc_rec.identity});
    if (queries.find_pk(ddl_stcname{stname.schema_name(), stname.table_name(), tc_rec.cname.str()}, true))
      pk_cnames.push_back(tc_rec.cname);
  }
  return unique_ptr<const ddl_table_info>{new ddl_table_info{std::move(columns), std::move(pk_cnames)}};
}

void
sqlsvr_ddl_stnames_migrator_impl::migrate_ddl_stname(ddl_table_migrator_factory& app_table_migrator_factory, const ddl_stname& stname, string& last_schema_name, const ddl_table_info& table_info, ddl_stnames_migration_msgs& stnames_migration_msgs, counter_ochain& coc) {
  const ddl_usedcols* const usedcols{stname_usedcols.find_usedcols(stname)};
  assert(usedcols);
  if (last_schema_name.empty() || stname.schema_name() != last_schema_name) {
    if (!last_schema_name.empty())
      coc.os() << endl;
    migrate_create_schema_ddl(stname.schema_name().c_str(), coc.os());
  }
  const ddl_create_table_migration create_table_migration{migrate_create_table_ddl(app_table_migrator_factory, stname, table_info, assumed_idents, stnames_migration_msgs, coc)};
  for (const auto& uniquify_identity_cname : create_table_migration.uniquify_identity_cnames)
    uniquify_identity_column(ddl_stcname{stname, uniquify_identity_cname.str()});
  for (const auto& rejected_cname : create_table_migration.rejected_cnames) {
    const ddl_stcname rejected_stcname{stname, rejected_cname.str()};
    if (!stname_usedcols.used_column(rejected_stcname)) {
      stnames_migration_msgs.push_back(ddl_stnames_migration_msg{0, stname, rejected_cname, ddl_migration_msg{ddl_migration_msg_level::error, ddl_migration_msg::column_used_but_rejected_msg}});
#ifdef LEVEL_LOGGING
      BOOST_LOG_TRIVIAL(info) << "migrating stnames: column " << rejected_stcname.str() << " used but rejected";
#endif
      if (verbose_os)
        *verbose_os << "column " << rejected_stcname.str() << " used but rejected" << endl;
    }
    hide_rejected_column(rejected_stcname);
  }
  migrate_ck_constraints_ddl(stname, coc);
  migrate_uq_constraints_ddl(stname, coc);

  last_schema_name = stname.schema_name();
}

ddl_stnames_migration_msgs
sqlsvr_ddl_stnames_migrator_impl::migrate_ddl_stnames(ddl_table_migrator_factory* app_table_migrator_factory, ostream& target_ddl_os) {
  counter_ochain target_ddl_coc{target_ddl_os};
  ddl_stnames_migration_msgs stnames_migration_msgs;
  const vector<ddl_stname> stnames{sort_stname_usedcols_stnames(stname_usedcols)};
  unsigned int table_info_cnt{};
  string last_schema_name;
  for (const auto& stname : stnames) {
    const unique_ptr<const ddl_table_info> table_info{find_table_info(stname)};
    if (table_info) {
      if (app_table_migrator_factory)
        migrate_ddl_stname(*app_table_migrator_factory, stname, last_schema_name, *table_info, stnames_migration_msgs, target_ddl_coc);
      ++table_info_cnt;
    } else {
      stnames_migration_msgs.push_back(ddl_stnames_migration_msg{0, stname, ddl_cname{}, ddl_migration_msg{ddl_migration_msg_level::error, ddl_migration_msg::table_used_but_undefined_msg}});
#ifdef LEVEL_LOGGING
      BOOST_LOG_TRIVIAL(info) << "migrating stnames: table " << stname.str() << " used but undefined";
#endif
      if (verbose_os)
        *verbose_os << "table " << stname.str() << " used but undefined" << endl;
    }
  }
  if (table_info_cnt)
    migrate_validate_fk_constraints_ddl(stname_usedcols, have_usedcols, assumed_idents, stnames_migration_msgs, !app_table_migrator_factory, target_ddl_coc);
  return stnames_migration_msgs;
}

unique_ptr<ddl_stnames_migrator>
sqlsvr_ddl_stnames_migrator_factory_impl::make_stnames_migrator(const ddl_stname_usedcols& stname_usedcols, bool have_usedcols, const ddl_assumed_identities& assumed_idents, ostream* verbose_os) {
  return unique_ptr<ddl_stnames_migrator>{new sqlsvr_ddl_stnames_migrator_impl{*this, stname_usedcols, have_usedcols, assumed_idents, verbose_os}};
}
}
