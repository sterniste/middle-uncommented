#include <cassert>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <odbc_query.h>

#include <ddl_names.h>

#include "sqlsvr_ddl_connection_error.h"
#include "sqlsvr_ddl_tc_mapper.h"

namespace sqlsvr_ddl {
using namespace std;
using namespace odbc_query;
using namespace normal_ddl;

void
sqlsvr_ddl_tc_rec::dump_tc_row(ostream& os, const ddl_stname& stname, int32_t column_pos) const {
  os << '{';
  os << "schema_name,table_name,column_name,column_pos";
  if (default_val)
    os << ",default_val";
  os << ",is_nullable,datatype";
  if (char_max_len)
    os << ",char_max_len";
  if (char_oct_len)
    os << ",char_oct_len";
  if (num_prec)
    os << ",num_prec";
  if (num_prec_rad)
    os << ",num_prec_rad";
  if (num_scale)
    os << ",num_scale";
  if (dt_prec)
    os << ",dt_prec";
  if (charset_name)
    os << ",charset_name";
  os << ",is_identity";
  os << '}';
  os << '{';
  os << '"' << stname.schema_name() << "\",\"" << stname.table_name() << "\",\"" << cname.str() << "\"," << column_pos;
  if (default_val)
    os << ",\"" << make_quotable_ddl(*default_val, '"') << '"';
  os << ",\"" << (nullable ? "YES" : "NO") << "\",\"" << datatype << '"';
  if (char_max_len)
    os << ',' << *char_max_len;
  if (char_oct_len)
    os << ',' << *char_oct_len;
  if (num_prec)
    os << ',' << static_cast<int>(*num_prec);
  if (num_prec_rad)
    os << ',' << *num_prec_rad;
  if (num_scale)
    os << ',' << *num_scale;
  if (dt_prec)
    os << ',' << *dt_prec;
  if (charset_name)
    os << ",\"" << *charset_name << '"';
  os << ',' << identity;
  os << '}';
}

void
sqlsvr_ddl_tc_map_loader::insert_tc_recs(sqlsvr_ddl_tc_mapper& tc_mapper, const ddl_stname& stname, unique_ptr<vector<sqlsvr_ddl_tc_rec>>&& tc_recs, unsigned int conn_pos) {
  if (!tc_mapper.insert(stname, move(tc_recs))) {
    assert(conn_pos);
    connection_errors.push_back(sqlsvr_ddl_connection_error{conn_pos, string{"table "} + stname.str() + " redefined"});
  }
}

unique_ptr<sqlsvr_ddl_tc_mapper>
sqlsvr_ddl_tc_map_loader::load_tc_mapper() {
  unique_ptr<sqlsvr_ddl_tc_mapper> tc_mapper{new sqlsvr_ddl_tc_mapper{conns.size() * 512}};
  auto prev_mapper_size = tc_mapper->size();
  const auto cend = conns.cend();
  for (auto cit = conns.cbegin(); cit != cend; ++cit) {
    query tc_query = (*cit)->create_query();
    tc_query.bind_string_column(1, 128);
    tc_query.bind_string_column(2, 128);
    tc_query.bind_string_column(3, 128);
    tc_query.bind_int32_column(4);
    tc_query.bind_string_column(5, 4000);
    tc_query.bind_string_column(6, 3);
    tc_query.bind_string_column(7, 128);
    tc_query.bind_int32_column(8);
    tc_query.bind_int32_column(9);
    tc_query.bind_uint8_column(10);
    tc_query.bind_int16_column(11);
    tc_query.bind_int32_column(12);
    tc_query.bind_int16_column(13);
    tc_query.bind_string_column(14, 128);
    tc_query.bind_int32_column(15);
#ifdef FAKE_SQLSERVER
    const char* const sql = "SELECT"
                            " schema_name,"
                            " table_name,"
                            " column_name,"
                            " column_pos,"
                            " default_val,"
                            " is_nullable,"
                            " data_type,"
                            " char_max_len,"
                            " char_oct_len,"
                            " num_prec,"
                            " num_prec_rad,"
                            " num_scale,"
                            " dt_prec,"
                            " charset_name,"
                            " is_identity"
                            " FROM"
                            " tc_query"
                            " ORDER BY 1, 2, 4";
#else
    const char* const sql = "SELECT"
                            " table_schema,"
                            " table_name,"
                            " column_name,"
                            " ordinal_position,"
                            " column_default,"
                            " is_nullable,"
                            " data_type,"
                            " character_maximum_length,"
                            " character_octet_length,"
                            " numeric_precision,"
                            " numeric_precision_radix,"
                            " numeric_scale,"
                            " datetime_precision,"
                            " character_set_name,"
                            " CASE columnproperty(object_id(table_schema,table_name), column_name, 'IsIdentity') WHEN 1 THEN 1 ELSE 0 END"
                            " FROM"
                            " information_schema.columns"
                            " ORDER BY 1, 2, 4";
#endif
    const result_set rs{tc_query.exec(sql)};

    ddl_stname stname, last_stname;
    int32_t last_column_pos{};
    unique_ptr<vector<sqlsvr_ddl_tc_rec>> tc_recs{new vector<sqlsvr_ddl_tc_rec>{}};
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
      int32_t column_pos;
      assert(rs.get_int32_column(4, &column_pos));
      unique_ptr<string> default_val{new string{}};
      if (!rs.get_string_column(5, default_val.get()))
        default_val.reset();
      else
        assert(!default_val->empty());
      string nullable;
      ok = rs.get_string_column(6, &nullable);
      assert(ok && !nullable.empty());
      string datatype;
      ok = rs.get_string_column(7, &datatype);
      assert(ok && !datatype.empty());
      unique_ptr<int32_t> char_max_len{new int32_t{}};
      if (!rs.get_int32_column(8, char_max_len.get()))
        char_max_len.reset();
      unique_ptr<int32_t> char_oct_len{new int32_t{}};
      if (!rs.get_int32_column(9, char_oct_len.get()))
        char_oct_len.reset();
      unique_ptr<uint8_t> num_prec{new uint8_t{}};
      if (!rs.get_uint8_column(10, num_prec.get()))
        num_prec.reset();
      unique_ptr<int16_t> num_prec_rad{new int16_t{}};
      if (!rs.get_int16_column(11, num_prec_rad.get()))
        num_prec_rad.reset();
      unique_ptr<int32_t> num_scale{new int32_t{}};
      if (!rs.get_int32_column(12, num_scale.get()))
        num_scale.reset();
      unique_ptr<int16_t> dt_prec{new int16_t{}};
      if (!rs.get_int16_column(13, dt_prec.get()))
        dt_prec.reset();
      unique_ptr<string> charset_name{new string{}};
      if (!rs.get_string_column(14, charset_name.get()))
        charset_name.reset();
      else
        assert(!charset_name->empty());
      int32_t identity{};
      ok = rs.get_int32_column(15, &identity);
      assert(ok);

      sqlsvr_ddl_tc_rec tc_rec{ddl_cname{column_name}, move(default_val), nullable == "YES", move(datatype), move(char_max_len), move(char_oct_len), move(num_prec), move(num_prec_rad), move(num_scale), move(dt_prec), move(charset_name), static_cast<bool>(identity)};

      stname.reset(schema_name, table_name);
#ifdef DUMP_QUERY_ROWS_TO_VERBOSE
      if (verbose_os) {
        tc_rec.dump_tc_row(*verbose_os, stname, column_pos);
        *verbose_os << endl;
      }
#endif
      if (stname != last_stname) {
        assert(column_pos == 1);
        if (!tc_recs->empty()) {
          insert_tc_recs(*tc_mapper, last_stname, move(tc_recs), cit - conns.cbegin());
          tc_recs.reset(new vector<sqlsvr_ddl_tc_rec>{});
        }
      } else
        assert(column_pos == last_column_pos + 1);
      tc_recs->push_back(move(tc_rec));

      last_column_pos = column_pos;
      last_stname = stname;
      ++row_cnt;
    }
    if (!tc_recs->empty())
      insert_tc_recs(*tc_mapper, stname, move(tc_recs), cit - conns.cbegin());

    if (verbose_os)
      *verbose_os << "[ODBC datasource " << (cit - conns.cbegin() + 1) << "] " << (tc_mapper->size() - prev_mapper_size) << " tc keys loaded from " << row_cnt << " query rows" << endl;
    prev_mapper_size = tc_mapper->size();
  }
  return tc_mapper;
}
}
