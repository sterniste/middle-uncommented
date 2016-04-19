#ifndef MACRO_ARGS_EXPANDER_H
#define MACRO_ARGS_EXPANDER_H

#include <cassert>
#include <string>
#include <vector>

#include <boost/algorithm/string/trim.hpp>

#include "text_processor.h"

namespace universals {

template <typename Ch> class basic_macro_args_expander : protected basic_text_tokenizer<Ch> {
 protected:
  unsigned int group_cnt;

  virtual basic_process_match_result<Ch> process_match(const typename std::basic_string<Ch>::const_iterator text_cbegin, const basic_text_match<Ch>& match, const typename std::basic_string<Ch>::const_iterator text_cend) override;
  virtual typename std::basic_string<Ch>::const_iterator at_text_end(const typename std::basic_string<Ch>::const_iterator text_cbegin, const typename std::basic_string<Ch>::const_iterator text_cend) override;

  basic_macro_args_expander(basic_text_matcher<Ch>& app_matcher, const std::vector<basic_text_matcher<Ch>*>& mask_matchers, const std::vector<basic_text_matcher<Ch>*>& reject_matchers) : basic_text_tokenizer<Ch>{app_matcher, mask_matchers, reject_matchers}, group_cnt{} {}

  std::vector<std::basic_string<Ch>> expand_macro_args(const std::basic_string<Ch>& text) { return this->tokenize_text(text).second; }
};

template <typename Ch> bool is_blank(Ch c);

template <typename Ch>
basic_process_match_result<Ch>
basic_macro_args_expander<Ch>::process_match(const typename std::basic_string<Ch>::const_iterator text_cbegin, const basic_text_match<Ch>& match, const typename std::basic_string<Ch>::const_iterator text_cend) {
  assert(match.text_cit != text_cbegin);
  if (group_cnt == 0) {
    assert(match.matcher == nullptr);
    if (*text_cbegin != '(')
      throw basic_malformed_text_error<Ch>{0, "missing '(' at beginning of macro args"};
    ++group_cnt;
    this->tokens.resize(1);
  } else {
    if (match.matcher == nullptr) {
      assert(match.text_cit == text_cbegin + 1);
      if (*text_cbegin == '(')
        ++group_cnt;
      else if (*text_cbegin == ')') {
        if (--group_cnt == 0) {
          boost::trim_if(*this->tokens.rbegin(), universals::is_blank<Ch>);
          if (this->tokens.rbegin()->empty()) {
            if (this->tokens.size() > 1)
              throw basic_malformed_text_error<Ch>{0, "empty macro arg"};
            this->tokens.resize(0);
          }
          return basic_process_match_result<Ch>{match.text_cit, true};
        }
      } else if (*text_cbegin == ',') {
        if (group_cnt == 1) {
          boost::trim_if(*this->tokens.rbegin(), universals::is_blank<Ch>);
          if (this->tokens.rbegin()->empty())
            throw basic_malformed_text_error<Ch>{0, "empty macro arg"};
          this->tokens.resize(this->tokens.size() + 1);
          return basic_process_match_result<Ch>{match.text_cit, false};
        }
      }
    }
    this->tokens.rbegin()->append(text_cbegin, match.text_cit);
  }
  return basic_process_match_result<Ch>{match.text_cit, false};
}

template <typename Ch>
typename std::basic_string<Ch>::const_iterator
basic_macro_args_expander<Ch>::at_text_end(const typename std::basic_string<Ch>::const_iterator text_cbegin, const typename std::basic_string<Ch>::const_iterator text_cend) {
  throw basic_malformed_text_error<Ch>{text_cend - text_cbegin, "missing ')' at end of macro args"};
}

using macro_args_expander = basic_macro_args_expander<char>;
using wmacro_args_expander = basic_macro_args_expander<wchar_t>;
}

#endif
