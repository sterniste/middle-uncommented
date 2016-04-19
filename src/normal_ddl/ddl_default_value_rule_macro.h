#ifndef DDL_DEFAULT_VALUE_RULE_MACRO_H
#define DDL_DEFAULT_VALUE_RULE_MACRO_H

#include <memory>
#include <string>
#include <vector>

#include <macro_args_expander.h>
#include <text_processor.h>

#include "ddl_column_rules.h"

namespace normal_ddl {

struct ddl_default_value_rule_macro_args_result {
  std::string::const_iterator text_cit;
  const unsigned int macro_lhs_argcnt;
  const std::string macro_rhs_args;

  ddl_default_value_rule_macro_args_result(std::string::const_iterator text_cit, unsigned int macro_lhs_argcnt, const std::string macro_rhs_args) : text_cit{text_cit}, macro_lhs_argcnt{macro_lhs_argcnt}, macro_rhs_args{macro_rhs_args} {}
};

class ddl_default_value_rule_macro_args_expander : private universals::text_matcher, private universals::macro_args_expander {
  friend class ddl_default_value_rule_macro_expander;

  const ddl_default_value_rule_macro_args& macro_args;

  std::string rewrite_macro_args(const std::vector<std::string>& macro_args_tokens);

  virtual std::string::const_iterator match(std::string::const_iterator text_cbegin, const std::string::const_iterator text_cend) override { return text_cbegin; }
  virtual const char* what() const override { return "DDL default value rule macro args"; }

  ddl_default_value_rule_macro_args_expander(const ddl_default_value_rule_macro_args& macro_args) : universals::macro_args_expander{*this, ddl_default_value_rule_macro_mask_matchers{}, ddl_default_value_rule_macro_reject_matchers{}}, macro_args{macro_args} {}

 public:
  ddl_default_value_rule_macro_args_result expand_macro_args(const std::string& macro_args_text);
};

class ddl_default_value_rule_macro_expander : private universals::text_matcher, private universals::text_appender {
  const ddl_default_value_rules& default_value_rules;

  virtual std::string::const_iterator match(std::string::const_iterator text_cbegin, const std::string::const_iterator text_cend) override;
  virtual const char* what() const override { return "DDL default value rule macro"; }

  virtual universals::process_match_result process_match(const std::string::const_iterator text_cbegin, const universals::text_match& match, const std::string::const_iterator text_cend) override;

 public:
  static std::string make_argcnt_mismatch_msg(const std::string& macro_lhs, const std::string& macro_args_text, unsigned int expected_argcnt, unsigned int found_argcnt);
  static std::string make_rejected_text_msg(const std::string& macro_lhs, const std::string& macro_args_text, std::string::difference_type text_pos);
  static std::string make_malformed_text_msg(const std::string& macro_lhs, const std::string& macro_args_text, std::string::difference_type text_pos);

  ddl_default_value_rule_macro_expander(const ddl_default_value_rules& default_value_rules) : universals::text_appender{*this, ddl_default_value_rule_macro_mask_matchers{}}, default_value_rules{default_value_rules} {}

  std::unique_ptr<const std::string> expand_macro(const std::string& macro_text);
  std::unique_ptr<const std::string> expand_all_macros(const std::string& macro_text);
};
}

#endif
