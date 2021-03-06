#ifndef DDL_TABLE_MIGRATOR_PREFS_READER_H
#define DDL_TABLE_MIGRATOR_PREFS_READER_H

#include <iosfwd>
#include <string>

#include <boost/property_tree/ptree_fwd.hpp>

#include <ddl_column_rules.h>
namespace normal_ddl {
struct ddl_table_migrator_prefs;
}

namespace middle {

class ddl_table_migrator_prefs_reader {
  std::ostream* verbose_os;

  void load_datatype_rules_prefs(const boost::property_tree::ptree& pref_root, normal_ddl::ddl_datatype_rules& datatype_rules);
  void load_default_value_rules_prefs(const boost::property_tree::ptree& pref_root, normal_ddl::ddl_default_value_rules& default_value_rules);

 public:
  static const std::string cant_parse_json_prefs_msg_prefix;
  static const std::string cant_parse_json_datatype_rule_prefs_msg_prefix;
  static const std::string cant_parse_json_default_value_rule_prefs_msg_prefix;
  static const std::string at_path_msg_infix;

  ddl_table_migrator_prefs_reader(std::ostream* verbose_os = nullptr) : verbose_os{verbose_os} {}

  normal_ddl::ddl_table_migrator_prefs read_table_migrator_prefs(std::istream& json_prefs_is);
};
}

#endif
