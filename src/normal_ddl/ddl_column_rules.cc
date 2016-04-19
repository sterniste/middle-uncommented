#include <cassert>
#include <cctype>
#include <string>

#include <boost/lexical_cast.hpp>

#include <macro_args_expander.h>
#include <text_processor.h>

#include "ddl_column_rules.h"

namespace normal_ddl {
using namespace std;
using namespace boost;
using namespace universals;

bool
scale_map_size_type(ddl_map_size_type map_size_type) {
  switch (map_size_type) {
    case ddl_map_size_type::scale_reject_defined:
    case ddl_map_size_type::scale_suppress:
    case ddl_map_size_type::scale_narrow:
    case ddl_map_size_type::scale_assign:
    case ddl_map_size_type::scale_assign_undefined:
    case ddl_map_size_type::scale_reject_undefined:
      return true;
    default:
      return false;
  }
}

block_comment_matcher ddl_default_value_rule_macro_reject_matchers::sql_block_comment_matcher;

line_comment_matcher ddl_default_value_rule_macro_reject_matchers::sql_line_comment_matcher{"--"};

quote_matcher ddl_default_value_rule_macro_mask_matchers::sql_single_quote_matcher;

quote_matcher ddl_default_value_rule_macro_mask_matchers::sql_double_quote_matcher{'"'};

string::const_iterator
ddl_default_value_rule_macro_args_rewriter::parse_macro_args_placeholder(string::const_iterator text_cbegin, const string::const_iterator text_cend, ddl_default_value_rule_macro_args_placeholder& macro_arg_placeholder) {
  if (*text_cbegin == '\\' && text_cbegin + 1 != text_cend && text_cbegin[1] == '$')
    return text_cbegin + 1;
  if (*text_cbegin != '$')
    return text_cbegin;
  string arg_ph_number;
  auto text_cit = text_cbegin + 1;
  for (; text_cit != text_cend; ++text_cit) {
    if (arg_ph_number.empty() && *text_cit == '0')
      throw malformed_text_error{text_cit - text_cbegin, string{"invalid macro arg placeholder number leading 0"}};
    if (!isdigit(*text_cit))
      break;
    arg_ph_number.push_back(*text_cit);
  }
  if (arg_ph_number.empty())
    throw malformed_text_error{text_cit - text_cbegin, string{"empty macro arg placeholder number"}};
  macro_arg_placeholder.ph_number = macro_arg_placeholder.ph_range_end = lexical_cast<unsigned int>(arg_ph_number);
  if (text_cit != text_cend && *text_cit == '\\' && text_cit + 1 != text_cend && (isdigit(text_cit[1]) || text_cit[1] == '-'))
    return text_cit + 1;
  if (text_cit != text_cend && *text_cit == '-') {
    string arg_ph_range_end;
    while (++text_cit != text_cend) {
      if (arg_ph_range_end.empty() && *text_cit == '0')
        throw malformed_text_error{text_cit - text_cbegin, string{"invalid macro arg placeholder range end leading 0"}};
      if (!isdigit(*text_cit))
        break;
      arg_ph_range_end.push_back(*text_cit);
    }
    if (!arg_ph_range_end.empty()) {
      macro_arg_placeholder.ph_range_end = lexical_cast<unsigned int>(arg_ph_range_end);
      if (macro_arg_placeholder.ph_range_end <= macro_arg_placeholder.ph_number)
        throw malformed_text_error{text_cit - text_cbegin, string{"invalid macro arg placeholder range"}};
    } else
      macro_arg_placeholder.ph_range_end = 0;
    if (text_cit != text_cend && *text_cit == '\\' && text_cit + 1 != text_cend && isdigit(text_cit[1]))
      return text_cit + 1;
  }
  return text_cit;
}

string::const_iterator
ddl_default_value_rule_macro_args_rewriter::match(string::const_iterator text_cbegin, const string::const_iterator text_cend) {
  // match a macro arg placeholder
  ddl_default_value_rule_macro_args_placeholder macro_arg_placeholder;
  auto text_cit = parse_macro_args_placeholder(text_cbegin, text_cend, macro_arg_placeholder);
  if (text_cit != text_cbegin && *text_cbegin == '$' && (macro_arg_placeholder.ph_number > macro_args.argcnt || macro_arg_placeholder.ph_range_end > macro_args.argcnt))
    throw malformed_text_error{text_cit - text_cbegin, string{"macro arg placeholder number "} + lexical_cast<string>(macro_arg_placeholder.ph_number) + " exceeds arg count " + lexical_cast<string>(macro_args.argcnt)};
  return text_cit;
}

process_match_result
ddl_default_value_rule_macro_args_rewriter::process_match(const string::const_iterator text_cbegin, const text_match& match, const string::const_iterator text_cend) {
  const process_match_result match_result{macro_args_expander::process_match(text_cbegin, match, text_cend)};
  if (match_result.at_matches_end)
    return match_result;
  assert(!tokens.empty() && match.text_cit > text_cbegin);
  if (match.matcher == this && *text_cbegin != '(') {
    assert(tokens.crbegin()->size() >= match.text_cit - text_cbegin);
    rewrite_arg_placeholder(*tokens.rbegin(), tokens.rbegin()->end() - (match.text_cit - text_cbegin), tokens.rbegin()->end());
  }
  return match_result;
}
}
