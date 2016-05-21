#ifndef SQLSVR_DDL_CK_MAPPER_H
#define SQLSVR_DDL_CK_MAPPER_H

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <ddl_names.h>
namespace odbc_query {
class connection;
}

#include "sqlsvr_ddl_connection_error.h"
#include "sqlsvr_ddl_hidable.h"

namespace sqlsvr_ddl {

struct sqlsvr_ddl_ck_rec;
class sqlsvr_ddl_ck_mapper;

class sqlsvr_ddl_ck_map_loader {
  const std::vector<std::unique_ptr<odbc_query::connection>>& conns;
  std::ostream* verbose_os;
  sqlsvr_ddl_connection_errors connection_errors;

  void insert_ck_recs(sqlsvr_ddl_ck_mapper& ck_mapper, const normal_ddl::ddl_stcname& stcname, std::unique_ptr<std::vector<sqlsvr_ddl_ck_rec>>&& ck_recs, unsigned int conn_pos);

 public:
  sqlsvr_ddl_ck_map_loader(const std::vector<std::unique_ptr<odbc_query::connection>>& conns, std::ostream* verbose_os = nullptr) : conns{conns}, verbose_os{verbose_os} {}

  std::unique_ptr<sqlsvr_ddl_ck_mapper> load_ck_mapper();
  sqlsvr_ddl_connection_errors get_connection_errors() { return std::move(connection_errors); }
};

struct sqlsvr_ddl_ck_rec : public sqlsvr_ddl_hidable {
  std::string constraint_name;
  std::string check_clause;

  sqlsvr_ddl_ck_rec(const std::string& constraint_name, const std::string& check_clause) : constraint_name{constraint_name}, check_clause{check_clause} {}
  sqlsvr_ddl_ck_rec(const sqlsvr_ddl_ck_rec& that) : constraint_name{that.constraint_name}, check_clause{that.check_clause} {}

  void dump_ck_row(std::ostream& os, const normal_ddl::ddl_stcname& stcname) const;
};

class sqlsvr_ddl_ck_mapper {
  friend class sqlsvr_ddl_ck_map_loader;

  std::unordered_map<normal_ddl::ddl_stcname, std::unique_ptr<std::vector<sqlsvr_ddl_ck_rec>>, normal_ddl::ddl_stcname::hasher> map;

  sqlsvr_ddl_ck_mapper(std::size_t reserve = 0) : map{reserve} {}
  sqlsvr_ddl_ck_mapper(const sqlsvr_ddl_ck_mapper& that) = delete;

  bool insert(const normal_ddl::ddl_stcname& stcname, std::unique_ptr<std::vector<sqlsvr_ddl_ck_rec>>&& ck_recs) { return map.insert(std::pair<normal_ddl::ddl_stcname, std::unique_ptr<std::vector<sqlsvr_ddl_ck_rec>>>(stcname, std::move(ck_recs))).second; }

 public:
  std::vector<sqlsvr_ddl_ck_rec>* find(const normal_ddl::ddl_stcname& stcname) const {
    const auto it = map.find(stcname);
    return it == map.end() ? nullptr : it->second.get();
  }

  size_t size() const { return map.size(); }
};
}

#endif
