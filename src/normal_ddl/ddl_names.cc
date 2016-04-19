#include <algorithm>
#include <cassert>
#include <cctype>
#include <iterator>
#include <string>
#include <utility>

#include "ddl_names.h"

namespace normal_ddl {
using namespace std;

string
normalize_ddl_name(const string& ddl_name) {
  if (!isalpha(*ddl_name.cbegin()))
    return string{};
  string name;
  bool prev_dot = true;
  const auto cend = ddl_name.cend();
  for (auto cit = ddl_name.cbegin(); cit != cend; ++cit) {
    char c = *cit;
    if (isupper(c))
      c = tolower(c);
    else if (!isalnum(c) && c != '_') {
      if (c != '.' || prev_dot)
        return string{};
    }
    name += c;
    prev_dot = c == '.';
  }
  return name;
}

string
make_quotable_ddl(const string& ddl, char quote_c) {
  auto pos = ddl.find(quote_c);
  if (pos == string::npos)
    return ddl;
  const char escaped_quote[2]{'\\', quote_c};
  string quotable_ddl{ddl};
  do
    quotable_ddl.replace(pos, 1, escaped_quote, 2);
  while ((pos = quotable_ddl.find(quote_c, pos + 2)) != string::npos);
  return quotable_ddl;
}

ddl_stname
ddl_stname::parse(const string& name, const char* default_schema_name) {
  const auto dotpos = name.find('.');
  if (dotpos == string::npos)
    return ddl_stname{default_schema_name, name};
  return ddl_stname{name.substr(0, dotpos), name.substr(dotpos + 1)};
}

ddl_stcname
ddl_stcname::parse(const string& name, const char* default_schema_name) {
  const auto dot1pos = name.find('.');
  if (dot1pos == string::npos)
    return ddl_stcname{};
  const auto dot2pos = name.find('.', dot1pos + 1);
  if (dot2pos == string::npos)
    return ddl_stcname{default_schema_name, name.substr(0, dot1pos), name.substr(dot1pos + 1)};
  return ddl_stcname{name.substr(0, dot1pos), name.substr(dot1pos + 1, dot2pos - dot1pos - 1), name.substr(dot2pos + 1)};
}

void
ddl_stname_usedcols::merge_into(const ddl_stname_usedcols& that) {
  const auto that_end = that.end();
  for (auto that_it = that.begin(); that_it != that_end; ++that_it) {
    auto it = map.find(that_it->first);
    if (it == map.end()) {
      const auto insert = map.insert(pair<ddl_stname, ddl_usedcols>(that_it->first, ddl_usedcols{}));
      assert(insert.second);
      it = insert.first;
    }
    copy(that_it->second.cbegin(), that_it->second.cend(), inserter(it->second, it->second.end()));
  }
}

void
ddl_stname_usedcols::insert_name(const ddl_stname& stname) {
  if (map.find(stname) == map.end()) {
    const bool ok = map.insert(pair<ddl_stname, ddl_usedcols>(stname, ddl_usedcols{})).second;
    assert(ok);
  }
}

void
ddl_stname_usedcols::insert_name_usedcol(const ddl_stname& stname, const ddl_cname& usedcol_cname) {
  const auto it = map.find(stname);
  assert(it != map.end());
  it->second.insert(usedcol_cname);
}

const ddl_usedcols*
ddl_stname_usedcols::find_usedcols(const ddl_stname& stname) const {
  const auto it = map.find(stname);
  return it == map.end() ? nullptr : &it->second;
}

bool
ddl_stname_usedcols::used_column(const ddl_stcname& stcname) const {
  const auto it = map.find(stcname.stname());
  if (it == map.end())
    return false;
  return it->second.find(stcname.cname()) != it->second.end();
}
}
