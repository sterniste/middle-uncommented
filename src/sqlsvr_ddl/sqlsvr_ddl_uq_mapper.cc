#include <cassert>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include <odbc_query.h>

#include <ddl_names.h>

#include "sqlsvr_ddl_connection_error.h"
#include "sqlsvr_ddl_uq_mapper.h"

namespace sqlsvr_ddl {
using namespace std;
using namespace odbc_query;
using namespace normal_ddl;

void
sqlsvr_ddl_uq_rec::dump_uq_row(ostream& os, const ddl_stcname& stcname) const {
  os << '{';
  os << "schema_name,table_name,column_name,constraint_name";
  os << '}';
  os << '{';
  os << '"' << stcname.schema_name() << "\",\"" << stcname.table_name() << "\",\"" << stcname.column_name() << "\",\"" << constraint_name << "\"";
  os << '}';
}

void
sqlsvr_ddl_uq_map_loader::insert_uq_recs(sqlsvr_ddl_uq_mapper& uq_mapper, const ddl_stcname& stcname, unique_ptr<vector<sqlsvr_ddl_uq_rec>>&& uq_recs, unsigned int conn_pos) {
  if (!uq_mapper.insert(stcname, move(uq_recs))) {
    assert(conn_pos);
    connection_errors.push_back(sqlsvr_ddl_connection_error{conn_pos, string{"uq column "} + stcname.str() + " redefined"});
  }
}

unique_ptr<sqlsvr_ddl_uq_mapper>
sqlsvr_ddl_uq_map_loader::load_uq_mapper() {
  unique_ptr<sqlsvr_ddl_uq_mapper> uq_mapper{new sqlsvr_ddl_uq_mapper{conns.size() * 128}};
  auto prev_mapper_size = uq_mapper->size();
  const auto cend = conns.cend();
  for (auto cit = conns.cbegin(); cit != cend; ++cit) {
    query uq_query = (*cit)->create_query();
    uq_query.bind_string_column(1, 128);
    uq_query.bind_string_column(2, 128);
    uq_query.bind_string_column(3, 128);
    uq_query.bind_string_column(4, 128);
#ifdef FAKE_SQLSERVER
    const char* const sql = "SELECT"
                            " schema_name,"
                            " table_name,"
                            " column_name,"
                            " constraint_name"
                            " FROM"
                            " uq_query"
                            " ORDER BY 1, 2, 3, 4";
#else
    const char* const sql = "SELECT"
                            " t.table_schema,"
                            " t.table_name,"
                            " c.column_name,"
                            " t.constraint_name"
                            " FROM"
                            " information_schema.table_constraints t,"
                            " information_schema.constraint_column_usage c"
                            " WHERE"
                            " c.constraint_name = t.constraint_name"
                            " AND c.table_name = t.table_name"
                            " AND t.constraint_type = 'UNIQUE'"
                            " ORDER BY 1, 2, 3, 4";
#endif
    const result_set rs{uq_query.exec(sql)};

    ddl_stcname stcname, last_stcname;
    unique_ptr<vector<sqlsvr_ddl_uq_rec>> uq_recs{new vector<sqlsvr_ddl_uq_rec>{}};
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

      sqlsvr_ddl_uq_rec uq_rec{constraint_name};

      stcname.reset(schema_name, table_name, column_name);
#ifdef DUMP_QUERY_ROWS_TO_VERBOSE
      if (verbose_os) {
        uq_rec.dump_uq_row(*verbose_os, stcname);
        *verbose_os << endl;
      }
#endif
      if (stcname != last_stcname && !uq_recs->empty()) {
        insert_uq_recs(*uq_mapper, last_stcname, move(uq_recs), cit - conns.cbegin());
        uq_recs.reset(new vector<sqlsvr_ddl_uq_rec>{});
      }
      uq_recs->push_back(move(uq_rec));

      last_stcname = stcname;
      ++row_cnt;
    }
    if (!uq_recs->empty())
      insert_uq_recs(*uq_mapper, stcname, move(uq_recs), cit - conns.cbegin());

    if (verbose_os)
      *verbose_os << "[ODBC datasource " << (cit - conns.cbegin() + 1) << "] " << (uq_mapper->size() - prev_mapper_size) << " uq keys loaded from " << row_cnt << " query rows" << endl;
    prev_mapper_size = uq_mapper->size();
  }
  return uq_mapper;
}
}
