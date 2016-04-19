#include <cassert>
#include <ostream>
#include <string>
#include <utility>

#include <odbc_query.h>

#include <ddl_names.h>

#include "sqlsvr_ddl_connection_error.h"
#include "sqlsvr_ddl_pk_mapper.h"

namespace sqlsvr_ddl {
using namespace std;
using namespace odbc_query;
using namespace normal_ddl;

void
sqlsvr_ddl_pk_rec::dump_pk_row(std::ostream& os, const normal_ddl::ddl_stcname& stcname) const {
  os << '{';
  os << "schema_name,table_name,column_name";
  os << '}';
  os << '{';
  os << '"' << stcname.schema_name() << "\",\"" << stcname.table_name() << "\",\"" << stcname.column_name() << "\"";
  os << '}';
}

void
sqlsvr_ddl_pk_map_loader::insert_pk_rec(sqlsvr_ddl_pk_mapper& pk_mapper, const ddl_stcname& stcname, sqlsvr_ddl_pk_rec&& pk_rec, unsigned int conn_pos) {
  if (!pk_mapper.insert(stcname, move(pk_rec))) {
    assert(conn_pos);
    connection_errors.push_back(sqlsvr_ddl_connection_error{conn_pos, string{"pk column "} + stcname.str() + " redefined"});
  }
}

unique_ptr<sqlsvr_ddl_pk_mapper>
sqlsvr_ddl_pk_map_loader::load_pk_mapper() {
  unique_ptr<sqlsvr_ddl_pk_mapper> pk_mapper{new sqlsvr_ddl_pk_mapper{conns.size() * 512}};
  auto prev_mapper_size = pk_mapper->size();
  const auto cend = conns.cend();
  for (auto cit = conns.cbegin(); cit != cend; ++cit) {
    query pk_query = (*cit)->create_query();
    pk_query.bind_string_column(1, 128);
    pk_query.bind_string_column(2, 128);
    pk_query.bind_string_column(3, 128);
#ifdef FAKE_SQLSERVER
    const char* const sql = "SELECT"
                            " schema_name,"
                            " table_name,"
                            " column_name"
                            " FROM"
                            " pk_query"
                            " ORDER BY 1, 2, 3";
#else
    const char* const sql = "SELECT"
                            " t.table_schema,"
                            " t.table_name,"
                            " c.column_name"
                            " FROM"
                            " information_schema.table_constraints t,"
                            " information_schema.constraint_column_usage c"
                            " WHERE"
                            " c.constraint_name = t.constraint_name"
                            " AND c.table_name = t.table_name"
                            " AND t.constraint_type = 'PRIMARY KEY'"
                            " ORDER BY 1, 2, 3";
#endif
    const result_set rs{pk_query.exec(sql)};

    auto row_cnt = 0U;
    while (rs.next_row()) {
      bool ok{};
      string schema_name;
      ok = rs.get_string_column(1, &schema_name);
      assert(ok && !schema_name.empty());
      string table_name;
      ok = rs.get_string_column(2, &table_name);
      assert(ok);
      string column_name;
      ok = rs.get_string_column(3, &column_name);
      assert(ok);

      const ddl_stcname stcname{schema_name, table_name, column_name};
      sqlsvr_ddl_pk_rec pk_rec{};
#ifdef DUMP_QUERY_ROWS_TO_VERBOSE
      if (verbose_os) {
        pk_rec.dump_pk_row(*verbose_os, stcname);
        *verbose_os << endl;
      }
#endif
      insert_pk_rec(*pk_mapper, stcname, move(pk_rec), cit - conns.cbegin());

      ++row_cnt;
    }

    if (verbose_os)
      *verbose_os << "[ODBC datasource " << (cit - conns.cbegin() + 1) << "] " << (pk_mapper->size() - prev_mapper_size) << " pk keys loaded from " << row_cnt << " query rows" << endl;
    prev_mapper_size = pk_mapper->size();
  }
  return pk_mapper;
}
}
