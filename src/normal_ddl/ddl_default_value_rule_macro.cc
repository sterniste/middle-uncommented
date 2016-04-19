#include <algorithm>
#include <cassert>
#include <cctype>
#include <iterator>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/lexical_cast.hpp>

#include <text_processor.h>
#include <uc_string.h>

#include "ddl_column_rules.h"
#include "ddl_default_value_rule_macro.h"

namespace normal_ddl {
using namespace std;
using namespace boost;
using namespace universals;

class macro_args_rewriter : public ddl_default_value_rule_macro_args_rewriter {
  const vector<string>& macro_args_tokens;

 public:
  macro_args_rewriter(const ddl_default_value_rule_macro_args& macro_args, const vector<string>& macro_args_tokens) : ddl_default_value_rule_macro_args_rewriter{macro_args}, macro_args_tokens{macro_args_tokens} {}

  virtual void rewrite_arg_placeholder(string& macro_arg, string::iterator begin, string::iterator end) override;
};

void
macro_args_rewriter::rewrite_arg_placeholder(string& macro_arg, string::iterator begin, string::iterator end) {
  ddl_default_value_rule_macro_args_placeholder macro_arg_placeholder;
  parse_macro_args_placeholder(begin, end, macro_arg_placeholder);
  assert(macro_arg_placeholder.ph_number > 0 && macro_arg_placeholder.ph_number - 1 < macro_args_tokens.size());
  string rewritten_arg_placeholder{macro_args_tokens[macro_arg_placeholder.ph_number - 1]};
  assert(macro_arg_placeholder.ph_range_end == 0 || (macro_arg_placeholder.ph_range_end >= macro_arg_placeholder.ph_number && macro_arg_placeholder.ph_range_end - 1 < macro_args_tokens.size()));
  if (macro_arg_placeholder.ph_range_end > macro_arg_placeholder.ph_number) {
    const auto range_end = (macro_arg_placeholder.ph_range_end == 0) ? macro_args_tokens.size() : macro_arg_placeholder.ph_range_end;
    for (auto i = macro_arg_placeholder.ph_number + 1; i <= range_end; ++i)
      rewritten_arg_placeholder += ", " + macro_args_tokens[i - 1];
  }
  macro_arg.replace(begin, end, rewritten_arg_placeholder);
}

string
ddl_default_value_rule_macro_args_expander::rewrite_macro_args(const vector<string>& macro_args_tokens) {
  if (macro_args.rewrite.empty())
    return "";
  macro_args_rewriter rewriter{macro_args, macro_args_tokens};
  const vector<string> rewritten_macro_args{rewriter.rewrite_macro_args()};
  ostringstream oss;
  oss << '(';
  const auto cend = rewritten_macro_args.cend();
  for (auto cit = rewritten_macro_args.cbegin(); cit != cend; ++cit) {
    if (cit != rewritten_macro_args.cbegin())
      oss << ", ";
    oss << *cit;
  }
  oss << ')';
  return oss.str();
}

ddl_default_value_rule_macro_args_result
ddl_default_value_rule_macro_args_expander::expand_macro_args(const string& macro_args_text) {
  const pair<string::const_iterator, vector<string>> tokenize_text_result{tokenize_text(macro_args_text)};
  return ddl_default_value_rule_macro_args_result{tokenize_text_result.first, static_cast<unsigned int>(tokenize_text_result.second.size()), rewrite_macro_args(tokenize_text_result.second)};
}

string::const_iterator
ddl_default_value_rule_macro_expander::match(string::const_iterator text_cbegin, const string::const_iterator text_cend) {
  // match an SQL id
  if (text_cbegin == text_cend || !isalpha(*text_cbegin))
    return text_cbegin;
  auto text_cit = text_cbegin + 1;
  for (; text_cit != text_cend && (isalnum(*text_cit) || *text_cit == '_'); ++text_cit)
    ;
  return text_cit;
}

process_match_result
ddl_default_value_rule_macro_expander::process_match(const string::const_iterator text_cbegin, const text_match& match, const string::const_iterator text_cend) {
  assert(match.text_cit != text_cbegin);
  if (!isalpha(*text_cbegin))
    copy(text_cbegin, match.text_cit, back_inserter(appended_text));
  else {
    const string macro_lhs{text_cbegin, match.text_cit};
    auto it = default_value_rules.find(uc_string{macro_lhs});
    if (it == default_value_rules.end())
      copy(text_cbegin, match.text_cit, back_inserter(appended_text));
    else {
      if (*match.text_cit == '(' && it->second.macro_args) {
        const string macro_args_text{match.text_cit, text_cend};
        try {
          ddl_default_value_rule_macro_args_expander macro_args_expander{*it->second.macro_args};
          const ddl_default_value_rule_macro_args_result macro_args_result{
            macro_args_expander.expand_macro_args(macro_args_text)};
          if (macro_args_result.macro_lhs_argcnt != it->second.macro_args->argcnt)
            throw invalid_argument{make_argcnt_mismatch_msg(macro_lhs, macro_args_text, it->second.macro_args->argcnt, macro_args_result.macro_lhs_argcnt)};
          copy(it->second.macro_rhs.cbegin(), it->second.macro_rhs.cend(), back_inserter(appended_text));
          copy(macro_args_result.macro_rhs_args.cbegin(), macro_args_result.macro_rhs_args.cend(),
               back_inserter(appended_text));
          return process_match_result{match.text_cit + (macro_args_result.text_cit - macro_args_text.cbegin()), false};
        } catch (const rejected_text_error& e) {
          throw invalid_argument{make_rejected_text_msg(macro_lhs, macro_args_text, e.text_pos) + ": " + e.what()};
        } catch (const malformed_text_error& e) {
          throw invalid_argument{make_malformed_text_msg(macro_lhs, macro_args_text, e.text_pos) + ": " + e.what()};
        }
      } else
        copy(it->second.macro_rhs.cbegin(), it->second.macro_rhs.cend(), back_inserter(appended_text));
    }
  }
  return process_match_result{match.text_cit, false};
}

unique_ptr<const string>
ddl_default_value_rule_macro_expander::expand_macro(const string& macro_text) {
  const string text{append_text(macro_text).second};
  return text == macro_text ? unique_ptr<const string>{} : unique_ptr<const string>{new string{text}};
}

unique_ptr<const string>
ddl_default_value_rule_macro_expander::expand_all_macros(const string& macro_text) {
  unique_ptr<const string> text{expand_macro(macro_text)};
  if (!text || text->empty())
    return text;
  unique_ptr<const string> new_text;
  do {
    ddl_default_value_rule_macro_expander macro_expander{default_value_rules};
    new_text = macro_expander.expand_macro(*text);
    if (!new_text)
      return text;
  } while (!(text = move(new_text))->empty());
  return text;
}

string
ddl_default_value_rule_macro_expander::make_argcnt_mismatch_msg(const string& macro_lhs, const string& macro_args_text, unsigned int expected_argcnt, unsigned int found_argcnt) {
  return string{"arg count mismatch on expanding args for default value rule macro '"} + macro_lhs + "' at '" + macro_args_text + "': expected " + lexical_cast<string>(expected_argcnt) + ", found " + lexical_cast<string>(found_argcnt);
}

string
ddl_default_value_rule_macro_expander::make_rejected_text_msg(const string& macro_lhs, const string& macro_args_text, string::difference_type text_pos) {
  return string{"rejected text on expanding args for default value rule macro '"} + macro_lhs + "' at '" + macro_args_text + "' (pos " + lexical_cast<string>(text_pos) + ')';
}

string
ddl_default_value_rule_macro_expander::make_malformed_text_msg(const string& macro_lhs, const string& macro_args_text, string::difference_type text_pos) {
  return string{"malformed text on expanding args for default value rule macro '"} + macro_lhs + "' at '" + macro_args_text + "' (pos " + lexical_cast<string>(text_pos) + ')';
}
}
