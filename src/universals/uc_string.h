#ifndef UC_STRING_H
#define UC_STRING_H

#include <cstddef>
#include <functional>
#include <string>

namespace universals {

template <typename Ch> std::basic_string<Ch> to_upper(const std::basic_string<Ch>& s);

template <typename Ch>
std::basic_string<Ch>
to_upper(const Ch* cs) {
  return universals::to_upper(std::basic_string<Ch>{cs});
}

template <typename Ch> class basic_uc_string {
  std::basic_string<Ch> ucs;

 public:
  struct hasher {
    std::size_t operator()(const basic_uc_string& key) const { return std::hash<std::basic_string<Ch>>()(key.str()); }
  };

  basic_uc_string(const std::basic_string<Ch>& s) : ucs{universals::to_upper(s)} {}
  basic_uc_string(const char* cs) : ucs{universals::to_upper(cs)} {}

  bool operator==(const basic_uc_string& that) const { return ucs == that.ucs; }
  bool operator<(const basic_uc_string& that) const { return ucs < that.ucs; }

  std::basic_string<Ch> str() const { return ucs; }
  bool empty() const { return ucs.empty(); }
};

using uc_string = basic_uc_string<char>;
using uc_wstring = basic_uc_string<wchar_t>;
}

#endif
