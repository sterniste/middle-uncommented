#ifndef DDL_NAMES_H
#define DDL_NAMES_H

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace normal_ddl {

std::string normalize_ddl_name(const std::string& ddl_name);

std::string make_quotable_ddl(const std::string& ddl, char quote_c = '\'');

class ddl_cname {
  std::string name;

 public:
  struct hasher {
    std::size_t operator()(const ddl_cname& key) const { return std::hash<std::string>()(key.str()); }
  };

  ddl_cname() {}
  ddl_cname(const std::string& column_name) : name{normalize_ddl_name(column_name)} {}

  const std::string& str() const { return name; }
  bool empty() const { return name.empty(); }
  bool operator==(const ddl_cname& that) const { return name == that.name; }
  bool operator!=(const ddl_cname& that) const { return name != that.name; }
  bool operator<(const ddl_cname& that) const { return name < that.name; }

  void reset(const std::string& column_name) { name = normalize_ddl_name(name); }
};

class ddl_stname {
  static std::string make_name(const std::string& schema_name, const std::string& table_name) { return normalize_ddl_name(schema_name + '.' + table_name); }

  std::string name;

 public:
  struct hasher {
    std::size_t operator()(const ddl_stname& key) const { return std::hash<std::string>()(key.str()); }
  };

  static ddl_stname parse(const std::string& name, const char* default_schema_name);

  ddl_stname() {}
  ddl_stname(const std::string& schema_name, const std::string& table_name) : name{make_name(schema_name, table_name)} {}

  const std::string& str() const { return name; }
  bool empty() const { return name.empty(); }
  const std::string schema_name() const {
    const auto dotpos = name.find('.');
    return name.substr(0, dotpos);
  }
  const std::string table_name() const {
    const auto dotpos = name.find('.');
    return name.substr(dotpos + 1);
  }

  bool operator==(const ddl_stname& that) const { return name == that.name; }
  bool operator!=(const ddl_stname& that) const { return name != that.name; }
  bool operator<(const ddl_stname& that) const { return name < that.name; }

  void reset(const std::string& schema_name, const std::string& table_name) { name = make_name(schema_name, table_name); }
};

class ddl_stcname {
  static std::string make_name(const std::string& schema_name, const std::string& table_name, const std::string& column_name) { return normalize_ddl_name(schema_name + '.' + table_name + '.' + column_name); }

  std::string name;

 public:
  struct hasher {
    std::size_t operator()(const ddl_stcname& key) const { return std::hash<std::string>()(key.str()); }
  };

  static ddl_stcname parse(const std::string& name, const char* default_schema_name);

  ddl_stcname() {}
  ddl_stcname(const std::string& schema_name, const std::string& table_name, const std::string& column_name) : name{make_name(schema_name, table_name, column_name)} {}
  ddl_stcname(const ddl_stname stname, const std::string& column_name) : name{make_name(stname.schema_name(), stname.table_name(), column_name)} {}

  const std::string& str() const { return name; }
  bool empty() const { return name.empty(); }
  const std::string schema_name() const {
    const auto dot1pos = name.find('.');
    return name.substr(0, dot1pos);
  }
  const std::string table_name() const {
    const auto dot1pos = name.find('.');
    return name.substr(dot1pos + 1, name.rfind('.') - dot1pos - 1);
  }
  const std::string column_name() const {
    const auto dot2pos = name.rfind('.');
    return name.substr(dot2pos + 1);
  }
  ddl_stname stname() const { return ddl_stname{schema_name(), table_name()}; }
  ddl_cname cname() const { return ddl_cname{column_name()}; }

  bool operator==(const ddl_stcname& that) const { return name == that.name; }
  bool operator!=(const ddl_stcname& that) const { return name != that.name; }
  bool operator<(const ddl_stcname& that) const { return name < that.name; }

  void reset(const std::string& schema_name, const std::string& table_name, const std::string& column_name) { name = make_name(schema_name, table_name, column_name); }
};

using ddl_usedcols = std::unordered_set<ddl_cname, ddl_cname::hasher>;

class ddl_stname_usedcols {
  std::unordered_map<ddl_stname, ddl_usedcols, ddl_stname::hasher> map;

 public:
  void clear() { map.clear(); }
  bool empty() const { return map.empty(); }
  auto begin() const -> decltype(map.cbegin()) { return map.cbegin(); }
  auto end() const -> decltype(map.cend()) { return map.cend(); }

  void merge_into(const ddl_stname_usedcols& that);
  void insert_name(const ddl_stname& stname);
  void insert_name_usedcol(const ddl_stname& stname, const ddl_cname& usedcol_cname);

  const ddl_usedcols* find_usedcols(const ddl_stname& stname) const;
  bool used_column(const ddl_stcname& stcname) const;
};

using ddl_assumed_identities = std::unordered_set<ddl_stcname, ddl_stcname::hasher>;
}

#endif
