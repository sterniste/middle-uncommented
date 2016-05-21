#ifndef SQLSVR_DDL_PK_MAPPER_H
#define SQLSVR_DDL_PK_MAPPER_H

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <ddl_names.h>
namespace odbc_query {
class connection;
}

#include "sqlsvr_ddl_connection_error.h"
#include "sqlsvr_ddl_hidable.h"

namespace sqlsvr_ddl {

struct sqlsvr_ddl_pk_rec;
class sqlsvr_ddl_pk_mapper;

class sqlsvr_ddl_pk_map_loader {
  const std::vector<std::unique_ptr<odbc_query::connection>>& conns;
  std::ostream* verbose_os;
  sqlsvr_ddl_connection_errors connection_errors;

  void insert_pk_rec(sqlsvr_ddl_pk_mapper& pk_mapper, const normal_ddl::ddl_stcname& stcname, sqlsvr_ddl_pk_rec&& pk_rec, unsigned int conn_pos);

 public:
  sqlsvr_ddl_pk_map_loader(const std::vector<std::unique_ptr<odbc_query::connection>>& conns, std::ostream* verbose_os = nullptr) : conns{conns}, verbose_os{verbose_os} {}

  std::unique_ptr<sqlsvr_ddl_pk_mapper> load_pk_mapper();
  sqlsvr_ddl_connection_errors get_connection_errors() { return std::move(connection_errors); }
};

struct sqlsvr_ddl_pk_rec : public sqlsvr_ddl_hidable {
  void dump_pk_row(std::ostream& os, const normal_ddl::ddl_stcname& stcname) const;
};

class sqlsvr_ddl_pk_mapper {
  friend class sqlsvr_ddl_pk_map_loader;

  std::unordered_map<normal_ddl::ddl_stcname, sqlsvr_ddl_pk_rec, normal_ddl::ddl_stcname::hasher> map;

  sqlsvr_ddl_pk_mapper(std::size_t reserve = 0) : map{reserve} {}
  sqlsvr_ddl_pk_mapper(const sqlsvr_ddl_pk_mapper& that) = delete;

  bool insert(const normal_ddl::ddl_stcname& stcname, sqlsvr_ddl_pk_rec&& pk_rec) { return map.insert(std::pair<normal_ddl::ddl_stcname, sqlsvr_ddl_pk_rec>(stcname, std::move(pk_rec))).second; }

 public:
  sqlsvr_ddl_pk_rec* find(const normal_ddl::ddl_stcname& stcname, bool unhidden = false) {
    const auto it = map.find(stcname);
    return (it == map.end() || (unhidden && it->second.hidden)) ? nullptr : &it->second;
  }

  size_t size() const { return map.size(); }
};
}

#endif
