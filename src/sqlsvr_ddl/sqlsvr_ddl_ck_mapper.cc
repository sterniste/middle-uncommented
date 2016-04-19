#include <cassert>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <odbc_query.h>

#include <ddl_names.h>

#include "sqlsvr_ddl_ck_mapper.h"
#include "sqlsvr_ddl_connection_error.h"

namespace sqlsvr_ddl {
using namespace std;
using namespace odbc_query;
using namespace normal_ddl;

void
sqlsvr_ddl_ck_rec::dump_ck_row(ostream& os, const ddl_stcname& stcname) const {
  os << '{';
  os << "table_name,column_name,constraint_name,check_clause";
  os << '}';
  os << '{';
  os << '"' << stcname.table_name() << "\",\"" << stcname.column_name() << "\",\"" << constraint_name << "\",\"" << make_quotable_ddl(check_clause, '"') << "\"";
  os << '}';
}

void
sqlsvr_ddl_ck_map_loader::insert_ck_recs(sqlsvr_ddl_ck_mapper& ck_mapper, const ddl_stcname& stcname, unique_ptr<vector<sqlsvr_ddl_ck_rec>>&& ck_recs, unsigned int conn_pos) {
  if (!ck_mapper.insert(stcname, move(ck_recs))) {
    assert(conn_pos);
    connection_errors.push_back(sqlsvr_ddl_connection_error{conn_pos, string{"ck column "} + stcname.str() + " redefined"});
  }
}

unique_ptr<sqlsvr_ddl_ck_mapper>
sqlsvr_ddl_ck_map_loader::load_ck_mapper() {
  unique_ptr<sqlsvr_ddl_ck_mapper> ck_mapper{new sqlsvr_ddl_ck_mapper{conns.size() * 128}};
  auto prev_mapper_size = ck_mapper->size();
  const auto cend = conns.cend();
  for (auto cit = conns.cbegin(); cit != cend; ++cit) {
    query ck_query = (*cit)->create_query();
    ck_query.bind_string_column(1, 128);
    ck_query.bind_string_column(2, 128);
    ck_query.bind_string_column(3, 128);
    ck_query.bind_string_column(4, 128);
    ck_query.bind_string_column(5, 4000);
#ifdef FAKE_SQLSERVER
    const char* const sql = "SELECT"
                            " schema_name,"
                            " table_name,"
                            " column_name,"
                            " constraint_name,"
                            " check_clause"
                            " FROM"
                            " ck_query"
                            " ORDER BY 1, 2, 3, 4";
#else
    const char* const sql = "SELECT"
                            " t.table_schema,"
                            " t.table_name,"
                            " c.column_name,"
                            " t.constraint_name,"
                            " ck.check_clause"
                            " FROM"
                            " information_schema.table_constraints t,"
                            " information_schema.constraint_column_usage c,"
                            " information_schema.check_constraints ck"
                            " WHERE"
                            " c.constraint_name = t.constraint_name"
                            " AND ck.constraint_name = t.constraint_name"
                            " AND c.table_name = t.table_name"
                            " AND t.constraint_type = 'CHECK'"
                            " ORDER BY 1, 2, 3, 4";
#endif
    const result_set rs{ck_query.exec(sql)};

    ddl_stcname stcname, last_stcname;
    unique_ptr<vector<sqlsvr_ddl_ck_rec>> ck_recs{new vector<sqlsvr_ddl_ck_rec>{}};
    auto row_cnt = 0U;
    while (rs.next_row()) {
      bool ok{};
      string schema_name;
      ok = rs.get_string_column(1, &schema_name);
      assert(ok && !schema_name.empty());
      string table_name;
      ok = rs.get_string_column(2, &table_name);
      assert(ok && !table_name.empty());
      string column_name;
      ok = rs.get_string_column(3, &column_name);
      assert(ok && !column_name.empty());
      string constraint_name;
      ok = rs.get_string_column(4, &constraint_name);
      assert(ok && !constraint_name.empty());
      string check_clause;
      ok = rs.get_string_column(5, &check_clause);
      assert(ok && !check_clause.empty());

      sqlsvr_ddl_ck_rec ck_rec{constraint_name, check_clause};

      stcname.reset(schema_name, table_name, column_name);
#ifdef DUMP_QUERY_ROWS_TO_VERBOSE
      if (verbose_os) {
        ck_rec.dump_ck_row(*verbose_os, stcname);
        *verbose_os << endl;
      }
#endif
      if (stcname != last_stcname && !ck_recs->empty()) {
        insert_ck_recs(*ck_mapper, last_stcname, move(ck_recs), cit - conns.cbegin());
        ck_recs.reset(new vector<sqlsvr_ddl_ck_rec>{});
      }
      ck_recs->push_back(move(ck_rec));

      last_stcname = stcname;
      ++row_cnt;
    }
    if (!ck_recs->empty())
      insert_ck_recs(*ck_mapper, stcname, move(ck_recs), cit - conns.cbegin());

    if (verbose_os)
      *verbose_os << "[ODBC datasource " << (cit - conns.cbegin() + 1) << "] " << (ck_mapper->size() - prev_mapper_size) << " ck keys loaded from " << row_cnt << " query rows" << endl;
    prev_mapper_size = ck_mapper->size();
  }
  return ck_mapper;
}
}
