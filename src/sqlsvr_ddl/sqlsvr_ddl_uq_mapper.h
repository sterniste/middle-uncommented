#ifndef SQLSVR_DDL_UQ_MAPPER_H
#define SQLSVR_DDL_UQ_MAPPER_H

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

struct sqlsvr_ddl_uq_rec;
class sqlsvr_ddl_uq_mapper;

class sqlsvr_ddl_uq_map_loader {
  const std::vector<std::unique_ptr<odbc_query::connection>>& conns;
  std::ostream* verbose_os;
  sqlsvr_ddl_connection_errors connection_errors;

  void insert_uq_recs(sqlsvr_ddl_uq_mapper& uq_mapper, const normal_ddl::ddl_stcname& stcname, std::unique_ptr<std::vector<sqlsvr_ddl_uq_rec>>&& uq_rec, unsigned int conn_pos);

 public:
  sqlsvr_ddl_uq_map_loader(const std::vector<std::unique_ptr<odbc_query::connection>>& conns, std::ostream* verbose_os = nullptr) : conns{conns}, verbose_os{verbose_os} {}

  std::unique_ptr<sqlsvr_ddl_uq_mapper> load_uq_mapper();
  sqlsvr_ddl_connection_errors get_connection_errors() { return std::move(connection_errors); }
};

struct sqlsvr_ddl_uq_rec : public sqlsvr_ddl_hidable {
  struct hasher {
    std::size_t operator()(const sqlsvr_ddl_uq_rec& key) const { return std::hash<std::string>()(key.constraint_name); }
  };

  std::string constraint_name;

  sqlsvr_ddl_uq_rec(const std::string& constraint_name) : constraint_name{constraint_name} {}
  sqlsvr_ddl_uq_rec(const sqlsvr_ddl_uq_rec& that) : constraint_name{that.constraint_name} {}

  bool operator==(const sqlsvr_ddl_uq_rec& that) const { return constraint_name == that.constraint_name; }

  void dump_uq_row(std::ostream& os, const normal_ddl::ddl_stcname& stcname) const;
};

class sqlsvr_ddl_uq_mapper {
  friend class sqlsvr_ddl_uq_map_loader;

  std::unordered_map<normal_ddl::ddl_stcname, std::unique_ptr<std::vector<sqlsvr_ddl_uq_rec>>, normal_ddl::ddl_stcname::hasher> map;

  sqlsvr_ddl_uq_mapper(std::size_t reserve = 0) : map{reserve} {}
  sqlsvr_ddl_uq_mapper(const sqlsvr_ddl_uq_mapper& that) = delete;

 public:
  std::vector<sqlsvr_ddl_uq_rec>* find(const normal_ddl::ddl_stcname& stcname) const {
    const auto it = map.find(stcname);
    return it == map.end() ? nullptr : it->second.get();
  }
  bool insert(const normal_ddl::ddl_stcname& stcname, std::unique_ptr<std::vector<sqlsvr_ddl_uq_rec>>&& uq_recs) { return map.insert(std::pair<normal_ddl::ddl_stcname, std::unique_ptr<std::vector<sqlsvr_ddl_uq_rec>>>(stcname, std::move(uq_recs))).second; }

  size_t size() const { return map.size(); }
};
}

#endif
