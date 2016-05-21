#include <cctype>
#include <cwctype>
#include <string>

#include "uc_string.h"

namespace universals {

template <>
std::basic_string<char>
to_upper(const std::basic_string<char>& s) {
  std::basic_string<char> ucs;
  for (const auto& c : s)
    ucs += std::islower(c) ? std::toupper(c) : c;
  return ucs;
}

template <>
std::basic_string<wchar_t>
to_upper(const std::basic_string<wchar_t>& ws) {
  std::basic_string<wchar_t> ucws;
  for (const auto& wc : ws)
    ucws += std::iswlower(wc) ? std::towupper(wc) : wc;
  return ucws;
}
}
