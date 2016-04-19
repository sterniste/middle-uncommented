#ifndef SQLSVR_DDL_FK_MAPPER_H
#define SQLSVR_DDL_FK_MAPPER_H

#include <cstddef>
#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <ddl_names.h>
namespace odbc_query { class connection; }

#include "sqlsvr_ddl_connection_error.h"
#include "sqlsvr_ddl_hidable.h"

namespace sqlsvr_ddl {

struct sqlsvr_ddl_fk_rec;
class sqlsvr_ddl_fk_mapper;

class sqlsvr_ddl_fk_map_loader {
  const std::vector<std::unique_ptr<odbc_query::connection>>& conns;
  std::ostream* verbose_os;
  sqlsvr_ddl_connection_errors connection_errors;

  void insert_fk_rec(sqlsvr_ddl_fk_mapper& fk_mapper, const normal_ddl::ddl_stcname& stcname, std::unique_ptr<sqlsvr_ddl_fk_rec>&& fk_rec, unsigned int conn_pos);

 public:
  sqlsvr_ddl_fk_map_loader(const std::vector<std::unique_ptr<odbc_query::connection>>& conns, std::ostream* verbose_os = nullptr) : conns{conns}, verbose_os{verbose_os} {}

  std::unique_ptr<sqlsvr_ddl_fk_mapper> load_fk_mapper();
  sqlsvr_ddl_connection_errors get_connection_errors() { return std::move(connection_errors); }
};

struct sqlsvr_ddl_fk_rec : public sqlsvr_ddl_hidable {
  std::string constraint_name;
  normal_ddl::ddl_stcname ref_stcname;

  sqlsvr_ddl_fk_rec(const std::string& constraint_name, normal_ddl::ddl_stcname&& ref_stcname) : constraint_name{constraint_name}, ref_stcname{std::move(ref_stcname)} {}
  sqlsvr_ddl_fk_rec(const sqlsvr_ddl_fk_rec& that) = delete;

  void dump_fk_row(std::ostream& os, const normal_ddl::ddl_stcname& stcname) const;
};

// the reverse mapping (ref_stcname -> stcname): needn't be hidable as it doesn't drive DDL output
struct sqlsvr_ddl_fk_reverse_rec {
  std::string constraint_name;
  normal_ddl::ddl_stcname stcname;

  sqlsvr_ddl_fk_reverse_rec(const std::string& constraint_name, normal_ddl::ddl_stcname&& stcname) : constraint_name{constraint_name}, stcname{std::move(stcname)} {}
  sqlsvr_ddl_fk_reverse_rec(const sqlsvr_ddl_fk_rec& that) = delete;
};

class sqlsvr_ddl_fk_mapper {
  friend class sqlsvr_ddl_fk_map_loader;

  std::unordered_map<normal_ddl::ddl_stcname, std::unique_ptr<sqlsvr_ddl_fk_rec>, normal_ddl::ddl_stcname::hasher> map;
  std::unordered_map<std::string, sqlsvr_ddl_fk_rec*> constraint_name_map;
  std::unordered_map<normal_ddl::ddl_stcname, std::unique_ptr<std::vector<sqlsvr_ddl_fk_reverse_rec>>, normal_ddl::ddl_stcname::hasher> reverse_map;

  sqlsvr_ddl_fk_mapper(std::size_t reserve = 0) : map{reserve}, constraint_name_map{reserve}, reverse_map{reserve} {}
  sqlsvr_ddl_fk_mapper(const sqlsvr_ddl_fk_mapper& that) = delete;

  bool insert(const normal_ddl::ddl_stcname& stcname, std::unique_ptr<sqlsvr_ddl_fk_rec>&& fk_rec) { return map.insert(std::pair<normal_ddl::ddl_stcname, std::unique_ptr<sqlsvr_ddl_fk_rec>>(stcname, std::move(fk_rec))).second; }
  bool insert_constraint_name(const std::string& constraint_name, sqlsvr_ddl_fk_rec* fk_rec) { return constraint_name_map.insert(std::pair<std::string, sqlsvr_ddl_fk_rec*>(constraint_name, fk_rec)).second; }
  bool insert_reverse(normal_ddl::ddl_stcname&& ref_stcname, sqlsvr_ddl_fk_reverse_rec&& fk_reverse_rec);

 public:
  sqlsvr_ddl_fk_rec* find(const normal_ddl::ddl_stcname& stcname, bool unhidden = false) const {
    const auto it = map.find(stcname);
    return (it == map.end() || (unhidden && it->second->hidden)) ? nullptr : it->second.get();
  }
  sqlsvr_ddl_fk_rec* find_constraint_name(const std::string& constraint_name, bool unhidden = false) const {
    const auto it = constraint_name_map.find(constraint_name);
    return (it == constraint_name_map.end() || (unhidden && it->second->hidden)) ? nullptr : it->second;
  }
  std::vector<sqlsvr_ddl_fk_reverse_rec>* find_reverse(const normal_ddl::ddl_stcname& ref_stcname) const {
    const auto it = reverse_map.find(ref_stcname);
    return it == reverse_map.end() ? nullptr : it->second.get();
  }

  size_t size() const { return map.size(); }
};
}

#endif
