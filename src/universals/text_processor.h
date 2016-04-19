#ifndef TEXT_PROCESSOR_H
#define TEXT_PROCESSOR_H

#include <algorithm>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace universals {

template <typename Ch> struct basic_rejected_text_error : public std::invalid_argument {
  typename std::basic_string<Ch>::difference_type text_pos;

  basic_rejected_text_error(typename std::basic_string<Ch>::difference_type text_pos, const std::basic_string<Ch>& msg) : std::invalid_argument{msg}, text_pos{text_pos} {}
};

using rejected_text_error = basic_rejected_text_error<char>;
using rejected_wtext_error = basic_rejected_text_error<wchar_t>;

template <typename Ch> struct basic_malformed_text_error : public std::invalid_argument {
  typename std::basic_string<Ch>::difference_type text_pos;

  basic_malformed_text_error(typename std::basic_string<Ch>::difference_type text_pos, const std::basic_string<Ch>& msg) : std::invalid_argument{msg}, text_pos{text_pos} {}
};

using malformed_text_error = basic_malformed_text_error<char>;
using malformed_wtext_error = basic_malformed_text_error<wchar_t>;

template <typename Ch> struct basic_text_matcher {
  virtual ~basic_text_matcher() {}

  virtual typename std::basic_string<Ch>::const_iterator match(typename std::basic_string<Ch>::const_iterator text_cbegin, const typename std::basic_string<Ch>::const_iterator text_cend) = 0;
  virtual const char* what() const = 0;
};

using text_matcher = basic_text_matcher<char>;
using wtext_matcher = basic_text_matcher<wchar_t>;

template <typename Ch> class basic_quote_matcher : public basic_text_matcher<Ch> {
  Ch quote_c;
  Ch escape_c;
  const Ch* escapables;

 public:
  basic_quote_matcher(Ch quote_c = '\'', Ch escape_c = '\\', const Ch* escapables = nullptr) : quote_c{quote_c}, escape_c{escape_c}, escapables{escapables} {}

  virtual typename std::basic_string<Ch>::const_iterator match(typename std::basic_string<Ch>::const_iterator text_cbegin, const typename std::basic_string<Ch>::const_iterator text_cend) override;
  virtual const char* what() const override {
    static Ch quote_c_what[]{quote_c, '\0'};
    return quote_c_what;
  }
};

template <typename Ch>
typename std::basic_string<Ch>::const_iterator
basic_quote_matcher<Ch>::match(typename std::basic_string<Ch>::const_iterator text_cbegin, const typename std::basic_string<Ch>::const_iterator text_cend) {
  if (text_cbegin == text_cend || *text_cbegin != quote_c)
    return text_cbegin;
  typename std::basic_string<Ch>::const_iterator text_cit = text_cbegin + 1;
  for (; text_cit != text_cend; ++text_cit) {
    if (*text_cit == escape_c) {
      if (++text_cit == text_cend)
        throw basic_malformed_text_error<Ch>{text_cit - text_cbegin, "unterminated escape inside quote"};
      if (*text_cit != quote_c && *text_cit != escape_c && escapables && std::basic_string<Ch>{escapables}.find(*text_cit) == std::basic_string<Ch>::npos)
        throw basic_malformed_text_error<Ch>{text_cit - text_cbegin, "illegal escape inside quote"};
    } else if (*text_cit == quote_c)
      return ++text_cit;
  }
  throw basic_malformed_text_error<Ch>{text_cit - text_cbegin, "unterminated quote"};
}

using quote_matcher = basic_quote_matcher<char>;
using wquote_matcher = basic_quote_matcher<wchar_t>;

template <typename Ch> class basic_block_comment_matcher : public basic_text_matcher<Ch> {
  std::basic_string<Ch> open_comment;
  std::basic_string<Ch> close_comment;

 public:
  basic_block_comment_matcher(const std::basic_string<Ch>& open_comment = {'/', '*'}, const std::basic_string<Ch>& close_comment = {'*', '/'}) : open_comment{open_comment}, close_comment{close_comment} {}

  virtual typename std::basic_string<Ch>::const_iterator match(typename std::basic_string<Ch>::const_iterator text_cbegin, const typename std::basic_string<Ch>::const_iterator text_cend) override;
  virtual const char* what() const override { return open_comment.c_str(); }
};

template <typename Ch>
typename std::basic_string<Ch>::const_iterator
basic_block_comment_matcher<Ch>::match(typename std::basic_string<Ch>::const_iterator text_cbegin, const typename std::basic_string<Ch>::const_iterator text_cend) {
  if (text_cbegin == text_cend || open_comment.length() > text_cend - text_cbegin || !equal(open_comment.cbegin(), open_comment.cend(), text_cbegin))
    return text_cbegin;
  typename std::basic_string<Ch>::const_iterator text_cit = text_cbegin + open_comment.length();
  for (; close_comment.length() <= text_cend - text_cit; ++text_cit) {
    if (equal(close_comment.cbegin(), close_comment.cend(), text_cit))
      return text_cit + close_comment.length();
  }
  throw basic_malformed_text_error<Ch>{text_cit - text_cbegin, "unterminated block comment"};
}

using block_comment_matcher = basic_block_comment_matcher<char>;
using wblock_comment_matcher = basic_block_comment_matcher<wchar_t>;

template <typename Ch> class basic_line_comment_matcher : public basic_text_matcher<Ch> {
  std::basic_string<Ch> open_comment;

 public:
  basic_line_comment_matcher(const std::basic_string<Ch>& open_comment = {'/', '/'}) : open_comment{open_comment} {}

  virtual typename std::basic_string<Ch>::const_iterator match(typename std::basic_string<Ch>::const_iterator text_cbegin, const typename std::basic_string<Ch>::const_iterator text_cend) override;
  virtual const char* what() const override { return open_comment.c_str(); }
};

template <typename Ch>
typename std::basic_string<Ch>::const_iterator
basic_line_comment_matcher<Ch>::match(typename std::basic_string<Ch>::const_iterator text_cbegin, const typename std::basic_string<Ch>::const_iterator text_cend) {
  if (text_cbegin == text_cend || open_comment.length() > text_cend - text_cbegin || !equal(open_comment.cbegin(), open_comment.cend(), text_cbegin))
    return text_cbegin;
  for (typename std::basic_string<Ch>::const_iterator text_cit = text_cbegin + open_comment.length(); text_cit != text_cend; ++text_cit) {
    if (*text_cit == '\n')
      return ++text_cit;
  }
  return text_cend;
}

using line_comment_matcher = basic_line_comment_matcher<char>;
using wline_comment_matcher = basic_line_comment_matcher<wchar_t>;

template <typename Ch> struct basic_text_match {
  typename std::basic_string<Ch>::const_iterator text_cit;
  basic_text_matcher<Ch>* matcher;

  basic_text_match(typename std::basic_string<Ch>::const_iterator text_cit, basic_text_matcher<Ch>* matcher) : text_cit{text_cit}, matcher{matcher} {}
};

using text_match = basic_text_match<char>;
using wtext_match = basic_text_match<wchar_t>;

template <typename Ch> struct basic_process_match_result {
  typename std::basic_string<Ch>::const_iterator text_cit;
  bool at_matches_end;

  basic_process_match_result(typename std::basic_string<Ch>::const_iterator text_cit, bool at_matches_end) : text_cit{text_cit}, at_matches_end{at_matches_end} {}
};

using process_match_result = basic_process_match_result<char>;
using wprocess_match_result = basic_process_match_result<wchar_t>;

template <typename Ch> class basic_text_processor {
  basic_text_matcher<Ch>& app_matcher;
  std::vector<basic_text_matcher<Ch>*> mask_matchers;
  std::vector<basic_text_matcher<Ch>*> reject_matchers;

  basic_text_match<Ch> match_mask(const typename std::basic_string<Ch>::const_iterator text_cbegin, const typename std::basic_string<Ch>::const_iterator text_cend) const;

 protected:
  basic_text_processor(basic_text_matcher<Ch>& app_matcher, std::vector<basic_text_matcher<Ch>*> mask_matchers = std::vector<basic_text_matcher<Ch>*>{}, std::vector<basic_text_matcher<Ch>*> reject_matchers = std::vector<basic_text_matcher<Ch>*>{}) : app_matcher{app_matcher}, mask_matchers{mask_matchers}, reject_matchers{reject_matchers} {}

  typename std::basic_string<Ch>::const_iterator process_text(const std::basic_string<Ch>& text);

 public:
  virtual ~basic_text_processor() {}

  virtual basic_process_match_result<Ch> process_match(const typename std::basic_string<Ch>::const_iterator text_cbegin, const basic_text_match<Ch>& match, const typename std::basic_string<Ch>::const_iterator text_cend) = 0;
  virtual typename std::basic_string<Ch>::const_iterator at_text_end(const typename std::basic_string<Ch>::const_iterator text_cbegin, const typename std::basic_string<Ch>::const_iterator text_cend) { return text_cend; }
};

template <typename Ch>
basic_text_match<Ch>
basic_text_processor<Ch>::match_mask(const typename std::basic_string<Ch>::const_iterator text_cbegin, const typename std::basic_string<Ch>::const_iterator text_cend) const {
  for (auto* mask_matcher : mask_matchers) {
    const auto text_cit = mask_matcher->match(text_cbegin, text_cend);
    if (text_cit != text_cbegin)
      return basic_text_match<Ch>{text_cit, mask_matcher};
  }
  return basic_text_match<Ch>{text_cbegin, nullptr};
}

template <typename Ch>
typename std::basic_string<Ch>::const_iterator
basic_text_processor<Ch>::process_text(const std::basic_string<Ch>& text) {
  const auto text_cend = text.cend();
  auto text_cit = text.cbegin();
  while (text_cit != text_cend) {
    const auto it = find_if(reject_matchers.cbegin(), reject_matchers.cend(), [text_cit, text_cend](basic_text_matcher<Ch>* reject_segment) { return reject_segment->match(text_cit, text_cend) != text_cit; });
    if (it != reject_matchers.cend())
      throw basic_rejected_text_error<Ch>(text_cit - text.cbegin(), (*it)->what());
    basic_text_match<Ch> match{match_mask(text_cit, text_cend)};
    if (match.text_cit == text_cit) {
      match.text_cit = app_matcher.match(text_cit, text_cend);
      if (match.text_cit == text_cit) {
        match.text_cit = text_cit + 1;
        match.matcher = nullptr;
      } else
        match.matcher = &app_matcher;
    }
    const basic_process_match_result<Ch> match_result{process_match(text_cit, match, text_cend)};
    if (match_result.at_matches_end)
      return match_result.text_cit;
    text_cit = match_result.text_cit;
  }
  return at_text_end(text.cbegin(), text_cend);
}

using text_processor = basic_text_processor<char>;
using wtext_processor = basic_text_processor<wchar_t>;

template <typename Ch> class basic_text_appender : protected basic_text_processor<Ch> {
 protected:
  std::basic_string<Ch> appended_text;

  basic_text_appender(basic_text_matcher<Ch>& app_matcher, std::vector<basic_text_matcher<Ch>*> mask_matchers = std::vector<basic_text_matcher<Ch>*>{}, std::vector<basic_text_matcher<Ch>*> reject_matchers = std::vector<basic_text_matcher<Ch>*>{}) : basic_text_processor<Ch>{app_matcher, mask_matchers, reject_matchers} {}

 public:
  std::pair<typename std::basic_string<Ch>::const_iterator, std::basic_string<Ch>> append_text(const std::basic_string<Ch>& text) { return std::pair<typename std::basic_string<Ch>::const_iterator, std::basic_string<Ch>>{this->process_text(text), std::move(appended_text)}; }
};

using text_appender = basic_text_appender<char>;
using wtext_appender = basic_text_appender<wchar_t>;

template <typename Ch> class basic_text_tokenizer : protected basic_text_processor<Ch> {
 protected:
  std::vector<std::basic_string<Ch>> tokens;

  basic_text_tokenizer(basic_text_matcher<Ch>& app_matcher, std::vector<basic_text_matcher<Ch>*> mask_matchers = std::vector<basic_text_matcher<Ch>*>{}, std::vector<basic_text_matcher<Ch>*> reject_matchers = std::vector<basic_text_matcher<Ch>*>{}) : basic_text_processor<Ch>{app_matcher, mask_matchers, reject_matchers} {}

 public:
  std::pair<typename std::basic_string<Ch>::const_iterator, std::vector<std::basic_string<Ch>>> tokenize_text(const std::basic_string<Ch>& text) { return std::pair<typename std::basic_string<Ch>::const_iterator, std::vector<std::basic_string<Ch>>>{this->process_text(text), std::move(tokens)}; }
};

using text_tokenizer = basic_text_tokenizer<char>;
using wtext_tokenizer = basic_text_tokenizer<wchar_t>;
}

#endif
