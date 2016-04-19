#include <cassert>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <odbc_query.h>

#include <ddl_names.h>

#include "sqlsvr_ddl_connection_error.h"
#include "sqlsvr_ddl_fk_mapper.h"

namespace sqlsvr_ddl {
using namespace std;
using namespace odbc_query;
using namespace normal_ddl;

void
sqlsvr_ddl_fk_rec::dump_fk_row(ostream& os, const ddl_stcname& stcname) const {
  os << '{';
  os << "schema_name,table_name,column_name,constraint_name,ref_schema_name,ref_table_name,ref_column_name";
  os << '}';
  os << '{';
  os << '"' << stcname.schema_name() << "\",\"" << stcname.table_name() << "\",\"" << stcname.column_name() << "\",\"" << constraint_name << "\",\"" << ref_stcname.schema_name() << "\",\"" << ref_stcname.table_name() << "\",\"" << ref_stcname.column_name() << "\"";
  os << '}';
}

void
sqlsvr_ddl_fk_map_loader::insert_fk_rec(sqlsvr_ddl_fk_mapper& fk_mapper, const ddl_stcname& stcname, unique_ptr<sqlsvr_ddl_fk_rec>&& fk_rec, unsigned int conn_pos) {
  if (!fk_mapper.insert(stcname, move(fk_rec))) {
    assert(conn_pos);
    connection_errors.push_back(sqlsvr_ddl_connection_error{conn_pos, string{"fk column "} + stcname.str() + " redefined"});
  }
}

unique_ptr<sqlsvr_ddl_fk_mapper>
sqlsvr_ddl_fk_map_loader::load_fk_mapper() {
  unique_ptr<sqlsvr_ddl_fk_mapper> fk_mapper{new sqlsvr_ddl_fk_mapper{conns.size() * 256}};
  auto prev_mapper_size = fk_mapper->size();
  const auto cend = conns.cend();
  for (auto cit = conns.cbegin(); cit != cend; ++cit) {
    query fk_query = (*cit)->create_query();
    fk_query.bind_string_column(1, 128);
    fk_query.bind_string_column(2, 128);
    fk_query.bind_string_column(3, 128);
    fk_query.bind_string_column(4, 128);
    fk_query.bind_string_column(5, 128);
    fk_query.bind_string_column(6, 128);
    fk_query.bind_string_column(7, 128);
#ifdef FAKE_SQLSERVER
    const char* const sql = "SELECT"
                            " schema_name,"
                            " table_name,"
                            " column_name,"
                            " constraint_name,"
                            " ref_schema_name,"
                            " ref_table_name,"
                            " ref_column_name"
                            " FROM"
                            " fk_query"
                            " ORDER BY 1, 2, 3, 5, 6, 7";
#else
    const char* const sql = "SELECT"
                            " object_schema_name(fk.parent_object_id),"
                            " object_name(fk.parent_object_id),"
                            " c1.name,"
                            " fk.name,"
                            " object_schema_name(fk.referenced_object_id),"
                            " object_name(fk.referenced_object_id),"
                            " c2.name"
                            " FROM"
                            " sys.foreign_keys fk"
                            " INNER JOIN sys.foreign_key_columns fkc ON fkc.constraint_object_id = fk.object_id"
                            " INNER JOIN sys.columns c1 ON fkc.parent_column_id = c1.column_id AND fkc.parent_object_id = c1.object_id"
                            " INNER JOIN sys.columns c2 ON fkc.referenced_column_id = c2.column_id AND fkc.referenced_object_id = c2.object_id"
                            " ORDER BY 1, 2, 3, 5, 6, 7";
#endif
    const result_set rs{fk_query.exec(sql)};

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
      string ref_schema_name;
      ok = rs.get_string_column(5, &ref_schema_name);
      assert(ok && !ref_schema_name.empty());
      string ref_table_name;
      ok = rs.get_string_column(6, &ref_table_name);
      assert(ok && !ref_table_name.empty());
      string ref_column_name;
      ok = rs.get_string_column(7, &ref_column_name);
      assert(ok && !ref_table_name.empty());

      ddl_stcname stcname{schema_name, table_name, column_name};
      unique_ptr<sqlsvr_ddl_fk_rec> fk_rec{new sqlsvr_ddl_fk_rec{constraint_name, ddl_stcname{ref_schema_name, ref_table_name, ref_column_name}}};
      sqlsvr_ddl_fk_rec* const fk_rec_ptr = fk_rec.get();
#ifdef DUMP_QUERY_ROWS_TO_VERBOSE
      if (verbose_os) {
        fk_rec->dump_fk_row(*verbose_os, stcname);
        *verbose_os << endl;
      }
#endif
      insert_fk_rec(*fk_mapper, stcname, move(fk_rec), cit - conns.cbegin());

      fk_mapper->insert_constraint_name(constraint_name, fk_rec_ptr);
      fk_mapper->insert_reverse(ddl_stcname{ref_schema_name, ref_table_name, ref_column_name}, sqlsvr_ddl_fk_reverse_rec{constraint_name, move(stcname)});

      ++row_cnt;
    }

    if (verbose_os)
      *verbose_os << "[ODBC datasource " << (cit - conns.cbegin() + 1) << "] " << (fk_mapper->size() - prev_mapper_size) << " fk keys loaded from " << row_cnt << " query rows" << endl;
    prev_mapper_size = fk_mapper->size();
  }
  return fk_mapper;
}

bool
sqlsvr_ddl_fk_mapper::insert_reverse(ddl_stcname&& ref_stcname, sqlsvr_ddl_fk_reverse_rec&& fk_reverse_rec) {
  vector<sqlsvr_ddl_fk_reverse_rec>* const found_fk_reverse_recs{find_reverse(ref_stcname)};
  if (found_fk_reverse_recs) {
    found_fk_reverse_recs->push_back(move(fk_reverse_rec));
    return true;
  } else {
    unique_ptr<vector<sqlsvr_ddl_fk_reverse_rec>> fk_reverse_recs{new vector<sqlsvr_ddl_fk_reverse_rec>{}};
    fk_reverse_recs->push_back(move(fk_reverse_rec));
    return reverse_map.insert(pair<ddl_stcname, unique_ptr<vector<sqlsvr_ddl_fk_reverse_rec>>>(move(ref_stcname), move(fk_reverse_recs))).second;
  }
}
}
