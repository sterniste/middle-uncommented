#include <cctype>
#include <cwctype>

#include "macro_args_expander.h"

namespace universals {

template <>
bool
is_blank<char>(char c) {
  return std::isblank(c);
}

template <>
bool
is_blank<wchar_t>(wchar_t wc) {
  return std::iswblank(wc);
}
}
