#include <cstdint>
#include <exception>
#include <istream>
#include <limits>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <boost/lexical_cast.hpp>
#ifdef LEVEL_LOGGING
#include <boost/log/trivial.hpp>
#endif
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <ddl_column_rules.h>
#include <ddl_table_migrator_prefs.h>
#include <text_processor.h>
#include <uc_string.h>

#include "ddl_table_migrator_prefs_error.h"
#include "ddl_table_migrator_prefs_reader.h"

namespace middle {
using namespace std;
using namespace boost;
using namespace boost::property_tree;
using namespace universals;
using namespace normal_ddl;

enum class int32_range : int8_t { full, positive, non_negative };

class ptree_cursor {
  ptree::const_iterator cit;
  const ptree::const_iterator cend;
  const string& tree_pref_path;
  const bool is_array;
  unsigned int pos;

 public:
  ptree_cursor(const ptree& pref_tree, const string& pref_path, bool is_array = false) : cit{pref_tree.begin()}, cend{pref_tree.end()}, tree_pref_path{pref_path}, is_array{is_array}, pos{0} {}

  string pref_path() const { return is_array ? (tree_pref_path + '[' + lexical_cast<string>(pos + 1) + ']') : (tree_pref_path + '#' + lexical_cast<string>(pos + 1)); }

  unsigned int at() const { return pos; }
  operator bool() const { return cit != cend; }
  const string& operator*() const { return cit->first; }
  ptree_cursor& operator++() {
    ++cit;
    ++pos;
    return *this;
  }

  ptree get_child(const char* name = nullptr) const;
  string get_string(const char* name = nullptr) const;
  string get_nonempty_string(const char* name = nullptr) const;
  int32_t get_int32_t(const char* name = nullptr, int32_range range = int32_range::full) const;
};

ptree
ptree_cursor::get_child(const char* name) const {
  if (name && cit->first != name)
    throw ddl_table_migrator_prefs_error{string{"expected node name '"} + name + "' at path " + pref_path() + "; found name '" + cit->first + '\''};
  if (cit->second.empty())
    throw ddl_table_migrator_prefs_error{string{"unexpected non-node at path "} + pref_path()};
  return cit->second;
}

string
ptree_cursor::get_string(const char* name) const {
  if (name && cit->first != name)
    throw ddl_table_migrator_prefs_error{string{"expected string value name '"} + name + "' at path " + pref_path() + "; found name '" + cit->first + '\''};
  if (!cit->second.empty())
    throw ddl_table_migrator_prefs_error{string{"unexpected node at path "} + pref_path()};
  return cit->second.data();
}

string
ptree_cursor::get_nonempty_string(const char* name) const {
  const string value{get_string(name)};
  if (value.empty())
    throw ddl_table_migrator_prefs_error{string{"invalid empty string value at path "} + pref_path()};
  return value;
}

int32_t
ptree_cursor::get_int32_t(const char* name, int32_range range) const {
  if (name && cit->first != name)
    throw ddl_table_migrator_prefs_error{string{"expected int32_t value name '"} + name + "' at path " + pref_path() + "; found name '" + cit->first + '\''};
  try {
    const int32_t value{cit->second.get_value<int32_t>()};
    switch (range) {
      case int32_range::non_negative:
        if (value < 0)
          throw ddl_table_migrator_prefs_error{string{"negative int32_t value at path '"} + pref_path()};
        break;
      case int32_range::positive:
        if (value <= 0)
          throw ddl_table_migrator_prefs_error{string{"non-positive int32_t value at path '"} + pref_path()};
        break;
    }
    return value;
  } catch (const boost::exception& e) {
    throw ddl_table_migrator_prefs_error{string{"invalid int32_t value at path "} + pref_path()};
  }
}

ddl_datatype_rule_type
parse_datatype_rule_type(const string& value, const string& pref_path) {
  if (value == "rejected")
    return ddl_datatype_rule_type::rejected;
  if (value == "unsized")
    return ddl_datatype_rule_type::unsized;
  if (value == "char_length")
    return ddl_datatype_rule_type::char_length;
  if (value == "precision_scale")
    return ddl_datatype_rule_type::precision_scale;
  if (value == "dt_precision")
    return ddl_datatype_rule_type::dt_precision;
  throw ddl_table_migrator_prefs_error{string{"invalid type '"} + value + "' at path " + pref_path};
}

ddl_map_size_type
parse_map_size_type(const string& value, const string& pref_path) {
  if (value == "reject_defined")
    return ddl_map_size_type::reject_defined;
  if (value == "suppress")
    return ddl_map_size_type::suppress;
  if (value == "narrow")
    return ddl_map_size_type::narrow;
  if (value == "assign")
    return ddl_map_size_type::assign;
  if (value == "assign_undefined")
    return ddl_map_size_type::assign_undefined;
  if (value == "reject_undefined")
    return ddl_map_size_type::reject_undefined;
  if (value == "scale_reject_defined")
    return ddl_map_size_type::scale_reject_defined;
  if (value == "scale_suppress")
    return ddl_map_size_type::scale_suppress;
  if (value == "scale_narrow")
    return ddl_map_size_type::scale_narrow;
  if (value == "scale_assign")
    return ddl_map_size_type::scale_assign;
  if (value == "scale_assign_undefined")
    return ddl_map_size_type::scale_assign_undefined;
  if (value == "scale_reject_undefined")
    return ddl_map_size_type::scale_reject_undefined;
  throw ddl_table_migrator_prefs_error{string{"invalid type '"} + value + "' at path " + pref_path};
}

ddl_map_size
parse_map_size(const ptree& pref_tree, const string& pref_path) {
  ptree_cursor cursor{pref_tree, pref_path};
  if (!cursor)
    throw ddl_table_migrator_prefs_error{string{"empty structure at path "} + pref_path};
  const ddl_map_size_type type{parse_map_size_type(cursor.get_string("type"), pref_path + ".type")};
  if (!++cursor)
    return ddl_map_size{type};
  int32_t range_min{numeric_limits<int32_t>::min()};
  if (*cursor == "range_min") {
    range_min = cursor.get_int32_t();
    if (!++cursor)
      return ddl_map_size{type, range_min};
  }
  int32_t range_max{numeric_limits<int32_t>::max()};
  if (*cursor == "range_max") {
    range_max = cursor.get_int32_t();
    if (!++cursor)
      return ddl_map_size{type, range_min, range_max};
  }
  int32_t new_value{0};
  if (*cursor == "new_value") {
    new_value = cursor.get_int32_t();
    if (!++cursor)
      return ddl_map_size{type, range_min, range_max, new_value};
  }
  if (cursor)
    throw ddl_table_migrator_prefs_error{string{"unexpected node at path "} + cursor.pref_path()};
  return ddl_map_size{type, range_min, range_max, new_value};
}

vector<ddl_map_size>
parse_map_sizes(const ptree& pref_tree, const string& pref_path) {
  ptree_cursor cursor{pref_tree, pref_path, true};
  if (!cursor)
    throw ddl_table_migrator_prefs_error{string{"empty array at path "} + pref_path};
  vector<ddl_map_size> map_sizes;
  for (; cursor; ++cursor)
    map_sizes.push_back(parse_map_size(cursor.get_child(), cursor.pref_path()));
  bool has_scale_map_size_type = false;
  for (const auto& map_size : map_sizes) {
    if (scale_map_size_type(map_size.map_size_type))
      has_scale_map_size_type = true;
    else if (has_scale_map_size_type)
      throw ddl_table_migrator_prefs_error{string{"scale map size types must follow all non-scale map size types in array at path "} + pref_path};
  }
  return map_sizes;
};

ddl_datatype_rule
parse_datatype_rule(const ptree& pref_tree, const string& pref_path) {
  ptree_cursor cursor{pref_tree, pref_path};
  const ddl_datatype_rule_type type{parse_datatype_rule_type(cursor.get_string("type"), pref_path + ".type")};
  if (!++cursor)
    return ddl_datatype_rule{type};
  string map_name;
  if (*cursor == "map_name") {
    map_name = cursor.get_nonempty_string();
    if (!++cursor)
      return ddl_datatype_rule{type, std::move(map_name)};
  }
  const ptree map_sizes_node{cursor.get_child("map_sizes")};
  if (++cursor)
    throw ddl_table_migrator_prefs_error{string{"unexpected node at path "} + cursor.pref_path()};
  return ddl_datatype_rule{type, std::move(map_name), parse_map_sizes(map_sizes_node, pref_path + ".map_sizes")};
}

pair<uc_string, ddl_datatype_rule>
parse_type_name_datatype_rule(const ptree& pref_tree, const string& pref_path) {
  ptree_cursor cursor{pref_tree, pref_path};
  if (!cursor)
    throw ddl_table_migrator_prefs_error{string{"empty structure at path '"} + pref_path + '\''};
  const uc_string type_name{cursor.get_nonempty_string("type_name")};
  ++cursor;
  const ptree datatype_node{cursor.get_child("datatype")};
  if (++cursor)
    throw ddl_table_migrator_prefs_error{string{"unexpected node at path "} + cursor.pref_path()};
  return pair<uc_string, ddl_datatype_rule>{type_name, parse_datatype_rule(datatype_node, pref_path + ".datatype")};
}

const string ddl_table_migrator_prefs_reader::cant_parse_json_prefs_msg_prefix{"can't parse JSON prefs: "};

void
ddl_table_migrator_prefs_reader::load_datatype_rules_prefs(const ptree& pref_root, ddl_datatype_rules& datatype_rules) {
  try {
    const string pref_path{"datatype_rules"};
    const ptree not_a_node;
    const auto& pref_tree = pref_root.get_child(pref_path, not_a_node);
    if (&pref_tree == &not_a_node || pref_tree.empty())
      throw ddl_table_migrator_prefs_error{string{"missing/empty array at path "} + pref_path};
    ptree_cursor cursor{pref_tree, pref_path, true};
    for (; cursor; ++cursor) {
      pair<uc_string, ddl_datatype_rule> type_name_datatype_rule{parse_type_name_datatype_rule(cursor.get_child(), cursor.pref_path())};
      if (!datatype_rules.insert(pair<uc_string, ddl_datatype_rule>(type_name_datatype_rule.first, std::move(type_name_datatype_rule.second))).second)
        throw ddl_table_migrator_prefs_error{string{"duplicate map datatype type_name '"} + type_name_datatype_rule.first.str() + "' at path " + cursor.pref_path()};
    }
    if (!cursor.at() || datatype_rules.empty())
      throw ddl_table_migrator_prefs_error{"no JSON datatype rule prefs loaded"};
#ifdef LEVEL_LOGGING
    BOOST_LOG_TRIVIAL(info) << "loaded " << datatype_rules.size() << " JSON datatype rule prefs";
#endif
    if (verbose_os)
      *verbose_os << "loaded " << datatype_rules.size() << " JSON datatype rule prefs" << endl;
  } catch (const std::exception& e) {
#ifdef LEVEL_LOGGING
    BOOST_LOG_TRIVIAL(debug) << "caught std::exception: can't parse JSON datatype rule prefs: " << e.what();
#endif
    throw ddl_table_migrator_prefs_error{string{"can't parse JSON datatype rule prefs: "} + e.what()};
  }
}

unique_ptr<const ddl_default_value_rule_macro_args>
parse_macro_args(const ptree& pref_tree, const string& pref_path) {
  ptree_cursor cursor{pref_tree, pref_path};
  const int32_t argcnt{cursor.get_int32_t("argcnt", int32_range::non_negative)};
  if (!++cursor)
    return unique_ptr<const ddl_default_value_rule_macro_args>{new ddl_default_value_rule_macro_args{static_cast<unsigned int>(argcnt)}};
  unique_ptr<const ddl_default_value_rule_macro_args> macro_args{new ddl_default_value_rule_macro_args{static_cast<unsigned int>(argcnt), cursor.get_nonempty_string("rewrite")}};
  // validate rewrite
  try {
    ddl_default_value_rule_macro_args_rewriter macro_args_rewriter{*macro_args};
    macro_args_rewriter.rewrite_macro_args();
  } catch (const rejected_text_error& e) {
    throw ddl_table_migrator_prefs_error{string{"rejected text found in default value rule macro rewrite '"} + macro_args->rewrite + "' at path " + pref_path + ": " + e.what()};
  } catch (const malformed_text_error& e) {
    throw ddl_table_migrator_prefs_error{string{"malformed text found in default value rule macro rewrite '"} + macro_args->rewrite + "' at path " + pref_path + ": " + e.what()};
  }
  if (!++cursor)
    return macro_args;
  throw ddl_table_migrator_prefs_error{string{"unexpected node at path "} + cursor.pref_path()};
}

pair<uc_string, ddl_default_value_rule>
parse_macro_lhs_default_value_rule(const ptree& pref_tree, const string& pref_path) {
  ptree_cursor cursor{pref_tree, pref_path};
  if (!cursor)
    throw ddl_table_migrator_prefs_error{string{"empty structure at path '"} + pref_path + '\''};
  const uc_string macro_lhs{cursor.get_nonempty_string("macro_lhs")};
  ++cursor;
  string macro_rhs{cursor.get_string("macro_rhs")};
  ++cursor;
  if (*cursor == "macro_args") {
    const ptree macro_args_node{cursor.get_child()};
    if (!++cursor)
      return pair<uc_string, ddl_default_value_rule>{macro_lhs, ddl_default_value_rule{std::move(macro_rhs), parse_macro_args(macro_args_node, pref_path + ".macro_args")}};
  }
  if (!cursor)
    return pair<uc_string, ddl_default_value_rule>{macro_lhs, ddl_default_value_rule{std::move(macro_rhs)}};
  throw ddl_table_migrator_prefs_error{string{"unexpected node at path "} + cursor.pref_path()};
}

void
ddl_table_migrator_prefs_reader::load_default_value_rules_prefs(const ptree& pref_root, ddl_default_value_rules& default_value_rules) {
  try {
    const string pref_path{"default_value_rules"};
    const ptree not_a_node;
    const auto& pref_tree = pref_root.get_child(pref_path, not_a_node);
    if (&pref_tree == &not_a_node || pref_tree.empty())
      throw ddl_table_migrator_prefs_error{string{"missing/empty array at path "} + pref_path};
    ptree_cursor cursor{pref_tree, pref_path, true};
    for (; cursor; ++cursor) {
      pair<uc_string, ddl_default_value_rule> macro_lhs_default_value_rules{parse_macro_lhs_default_value_rule(cursor.get_child(), cursor.pref_path())};
      if (!default_value_rules.insert(pair<uc_string, ddl_default_value_rule>(macro_lhs_default_value_rules.first, std::move(macro_lhs_default_value_rules.second))).second)
        throw ddl_table_migrator_prefs_error{string{"duplicate map default value macro LHS '"} + macro_lhs_default_value_rules.first.str() + "' at path " + cursor.pref_path()};
    }
    if (!cursor.at() || default_value_rules.empty())
      throw ddl_table_migrator_prefs_error{"no JSON default value rule prefs loaded"};
#ifdef LEVEL_LOGGING
    BOOST_LOG_TRIVIAL(info) << "loaded " << default_value_rules.size() << " JSON default value rule prefs";
#endif
    if (verbose_os)
      *verbose_os << "loaded " << default_value_rules.size() << " JSON default value rule prefs" << endl;
  } catch (const std::exception& e) {
#ifdef LEVEL_LOGGING
    BOOST_LOG_TRIVIAL(debug) << "caught std::exception: can't parse JSON default value rule prefs: " << e.what();
#endif
    throw ddl_table_migrator_prefs_error{string{"can't parse JSON default value rule prefs: "} + e.what()};
  }
}

ddl_table_migrator_prefs
ddl_table_migrator_prefs_reader::read_table_migrator_prefs(istream& json_prefs_is) {
  ptree pref_root;
  try {
    read_json(json_prefs_is, pref_root);
  } catch (const ddl_table_migrator_prefs_error& e) {
    throw;
  } catch (const std::exception& e) {
#ifdef LEVEL_LOGGING
    BOOST_LOG_TRIVIAL(info) << "caught std::exception: " << cant_parse_json_prefs_msg_prefix << e.what();
#endif
    throw ddl_table_migrator_prefs_error{cant_parse_json_prefs_msg_prefix + e.what()};
  }
  ddl_table_migrator_prefs table_migrator_prefs;
  load_datatype_rules_prefs(pref_root, table_migrator_prefs.datatype_rules);
  load_default_value_rules_prefs(pref_root, table_migrator_prefs.default_value_rules);
  return table_migrator_prefs;
}
}
