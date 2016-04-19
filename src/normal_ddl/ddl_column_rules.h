#ifndef DDL_COLUMN_RULES_H
#define DDL_COLUMN_RULES_H

#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <macro_args_expander.h>
#include <text_processor.h>
#include <uc_string.h>

namespace normal_ddl {

enum class ddl_datatype_rule_type : uint8_t { rejected, unsized, char_length, precision_scale, dt_precision };

enum class ddl_map_size_type : uint8_t { reject_defined, suppress, narrow, assign, assign_undefined, reject_undefined, scale_reject_defined, scale_suppress, scale_narrow, scale_assign, scale_assign_undefined, scale_reject_undefined };

bool scale_map_size_type(ddl_map_size_type map_size_type);

struct ddl_map_size {
  ddl_map_size_type map_size_type;
  int32_t range_min;
  int32_t range_max;
  int32_t new_value;

  ddl_map_size(ddl_map_size_type map_size_type = ddl_map_size_type::reject_defined, int32_t range_min = std::numeric_limits<int32_t>::min(), int32_t range_max = std::numeric_limits<int32_t>::max(), int32_t new_value = 0) : map_size_type{map_size_type}, range_min{range_min}, range_max{range_max}, new_value{new_value} {}
};

struct ddl_datatype_rule {
  ddl_datatype_rule_type datatype_rule_type;
  std::string map_name;
  std::vector<ddl_map_size> map_sizes;

  ddl_datatype_rule(ddl_datatype_rule_type datatype_rule_type, std::string&& map_name = "", std::vector<ddl_map_size>&& map_sizes = std::vector<ddl_map_size>{}) : datatype_rule_type{datatype_rule_type}, map_name{std::move(map_name)}, map_sizes{std::move(map_sizes)} {}
  ddl_datatype_rule(const ddl_datatype_rule& that) = delete;
  ddl_datatype_rule(ddl_datatype_rule&& that) = default;
};

struct ddl_default_value_rule_macro_args {
  unsigned int argcnt;
  std::string rewrite;

  ddl_default_value_rule_macro_args(unsigned int argcnt, std::string&& rewrite = "") : argcnt{argcnt}, rewrite{std::move(rewrite)} {}
  ddl_default_value_rule_macro_args(const ddl_default_value_rule_macro_args& that) = delete;
  ddl_default_value_rule_macro_args(ddl_default_value_rule_macro_args&& that) = default;
};

struct ddl_default_value_rule {
  std::string macro_rhs;
  std::unique_ptr<const ddl_default_value_rule_macro_args> macro_args;

  ddl_default_value_rule(std::string&& macro_rhs, std::unique_ptr<const ddl_default_value_rule_macro_args>&& macro_args = std::unique_ptr<const ddl_default_value_rule_macro_args>{}) : macro_rhs{std::move(macro_rhs)}, macro_args{std::move(macro_args)} {}
  ddl_default_value_rule(const ddl_default_value_rule& that) = delete;
  ddl_default_value_rule(ddl_default_value_rule&& that) = default;
};

using ddl_datatype_rules = std::unordered_map<universals::uc_string, ddl_datatype_rule, universals::uc_string::hasher>;

using ddl_default_value_rules = std::unordered_map<universals::uc_string, ddl_default_value_rule, universals::uc_string::hasher>;

struct ddl_default_value_rule_macro_mask_matchers : public std::vector<universals::text_matcher*> {
  static universals::quote_matcher sql_single_quote_matcher;
  static universals::quote_matcher sql_double_quote_matcher;

  ddl_default_value_rule_macro_mask_matchers() : std::vector<universals::text_matcher*>{&sql_single_quote_matcher, &sql_double_quote_matcher} {}
};

struct ddl_default_value_rule_macro_reject_matchers : public std::vector<universals::text_matcher*> {
  static universals::block_comment_matcher sql_block_comment_matcher;
  static universals::line_comment_matcher sql_line_comment_matcher;

  ddl_default_value_rule_macro_reject_matchers() : std::vector<universals::text_matcher*>{&sql_block_comment_matcher, &sql_line_comment_matcher} {}
};

struct ddl_default_value_rule_macro_args_placeholder {
  unsigned int ph_number;
  unsigned int ph_range_end;
};

class ddl_default_value_rule_macro_args_rewriter : private universals::text_matcher, private universals::macro_args_expander {
  const ddl_default_value_rule_macro_args& macro_args;

  virtual std::string::const_iterator match(std::string::const_iterator text_cbegin, const std::string::const_iterator text_cend) override;
  virtual const char* what() const override { return "DDL default value rules macro args rewrite"; }

  virtual universals::process_match_result process_match(const std::string::const_iterator text_cbegin, const universals::text_match& match, const std::string::const_iterator text_cend) override;

 protected:
  static std::string::const_iterator parse_macro_args_placeholder(std::string::const_iterator text_cbegin, const std::string::const_iterator text_cend, ddl_default_value_rule_macro_args_placeholder& macro_args_placeholder);

  virtual void rewrite_arg_placeholder(std::string& macro_arg, std::string::iterator begin, std::string::iterator end){};

 public:
  ddl_default_value_rule_macro_args_rewriter(const ddl_default_value_rule_macro_args& macro_args) : universals::macro_args_expander{*this, ddl_default_value_rule_macro_mask_matchers{}, ddl_default_value_rule_macro_reject_matchers{}}, macro_args{macro_args} {}

  std::vector<std::string> rewrite_macro_args() { return expand_macro_args(macro_args.rewrite); }
};
}

#endif
