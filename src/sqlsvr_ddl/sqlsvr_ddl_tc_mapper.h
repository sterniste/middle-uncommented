#ifndef SQLSVR_DDL_TC_MAPPER_H
#define SQLSVR_DDL_TC_MAPPER_H

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <ddl_names.h>
namespace odbc_query { class connection; }

#include "sqlsvr_ddl_connection_error.h"
#include "sqlsvr_ddl_hidable.h"

namespace sqlsvr_ddl {

struct sqlsvr_ddl_tc_rec;
class sqlsvr_ddl_tc_mapper;

class sqlsvr_ddl_tc_map_loader {
  const std::vector<std::unique_ptr<odbc_query::connection>>& conns;
  std::ostream* verbose_os;
  sqlsvr_ddl_connection_errors connection_errors;

  void insert_tc_recs(sqlsvr_ddl_tc_mapper& tc_mapper, const normal_ddl::ddl_stname& stname, std::unique_ptr<std::vector<sqlsvr_ddl_tc_rec>>&& tc_rec, unsigned int conn_pos);

 public:
  sqlsvr_ddl_tc_map_loader(const std::vector<std::unique_ptr<odbc_query::connection>>& conns, std::ostream* verbose_os = nullptr) : conns{conns}, verbose_os{verbose_os} {}

  std::unique_ptr<sqlsvr_ddl_tc_mapper> load_tc_mapper();
  sqlsvr_ddl_connection_errors get_connection_errors() { return std::move(connection_errors); }
};

struct sqlsvr_ddl_tc_rec : public sqlsvr_ddl_hidable {
  normal_ddl::ddl_cname cname;
  std::unique_ptr<const std::string> default_val;
  bool nullable;
  std::string datatype;
  std::unique_ptr<const int32_t> char_max_len;
  std::unique_ptr<const int32_t> char_oct_len;
  std::unique_ptr<const uint8_t> num_prec;
  std::unique_ptr<const int16_t> num_prec_rad;
  std::unique_ptr<const int32_t> num_scale;
  std::unique_ptr<const int16_t> dt_prec;
  std::unique_ptr<const std::string> charset_name;
  bool identity;

  sqlsvr_ddl_tc_rec(normal_ddl::ddl_cname&& cname, std::unique_ptr<std::string>&& default_val, bool nullable, std::string&& datatype, std::unique_ptr<int32_t>&& char_max_len, std::unique_ptr<int32_t>&& char_oct_len, std::unique_ptr<uint8_t>&& num_prec, std::unique_ptr<int16_t>&& num_prec_rad, std::unique_ptr<int32_t>&& num_scale, std::unique_ptr<int16_t>&& dt_prec, std::unique_ptr<std::string>&& charset_name, bool identity) : cname{std::move(cname)}, default_val{std::move(default_val)}, nullable{nullable}, datatype{std::move(datatype)}, char_max_len{std::move(char_max_len)}, char_oct_len{std::move(char_oct_len)}, num_prec{std::move(num_prec)}, num_prec_rad{std::move(num_prec_rad)}, num_scale{std::move(num_scale)}, dt_prec{std::move(dt_prec)}, charset_name{std::move(charset_name)}, identity{identity} {}
  sqlsvr_ddl_tc_rec(const sqlsvr_ddl_tc_rec& that) = delete;
  sqlsvr_ddl_tc_rec(sqlsvr_ddl_tc_rec&& that) = default;

  void dump_tc_row(std::ostream& os, const normal_ddl::ddl_stname& stname, int32_t column_pos) const;
};

class sqlsvr_ddl_tc_mapper {
  friend class sqlsvr_ddl_tc_map_loader;

  std::unordered_map<normal_ddl::ddl_stname, std::unique_ptr<std::vector<sqlsvr_ddl_tc_rec>>, normal_ddl::ddl_stname::hasher> map;

  sqlsvr_ddl_tc_mapper(std::size_t reserve = 0) : map{reserve} {}
  sqlsvr_ddl_tc_mapper(const sqlsvr_ddl_tc_mapper& that) = delete;

  bool insert(const normal_ddl::ddl_stname& stname, std::unique_ptr<std::vector<sqlsvr_ddl_tc_rec>>&& tc_recs) { return map.insert(std::pair<normal_ddl::ddl_stname, std::unique_ptr<std::vector<sqlsvr_ddl_tc_rec>>>(stname, std::move(tc_recs))).second; }

 public:
  std::vector<sqlsvr_ddl_tc_rec>* find(const normal_ddl::ddl_stname& stname) const {
    const auto it = map.find(stname);
    return it == map.end() ? nullptr : it->second.get();
  }

  size_t size() const { return map.size(); }
};
}

#endif
