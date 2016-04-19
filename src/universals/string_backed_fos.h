#ifndef STRING_BACKED_FOS_H
#define STRING_BACKED_FOS_H

#include <iterator>
#include <ostream>
#include <string>

#include <boost/iostreams/filter/counter.hpp>
#include <boost/iostreams/filtering_stream.hpp>

namespace universals {

template <typename Ch> class basic_string_backed_fos : public boost::iostreams::filtering_stream<boost::iostreams::output, Ch> {
  std::basic_string<Ch> sink;

 public:
  basic_string_backed_fos() : boost::iostreams::filtering_stream<boost::iostreams::output, Ch>(std::back_inserter(sink)) {}

  template <typename OFilter> basic_string_backed_fos(const OFilter& ofilter) : boost::iostreams::filtering_stream<boost::iostreams::output, Ch>(ofilter | std::back_inserter(sink)) {}

  const std::basic_string<Ch>& str() const { return sink; }

  void merge_into(std::basic_ostream<Ch>& os, bool nl = false) {
    this->flush();
    if (nl)
      os << std::endl;
    os << sink;
  }
};

using string_backed_fos = basic_string_backed_fos<char>;
using wstring_backed_fos = basic_string_backed_fos<wchar_t>;

template <typename Ch> struct basic_string_backed_counter_fos : public basic_string_backed_fos<Ch> {
  basic_string_backed_counter_fos() : basic_string_backed_fos<Ch>(boost::iostreams::template basic_counter<Ch>()) {}

  template <typename OFilter> basic_string_backed_counter_fos(const OFilter& ofilter) : basic_string_backed_fos<Ch>(ofilter | boost::iostreams::template basic_counter<Ch>()) {}

  const boost::iostreams::basic_counter<Ch>& counter() const { return *this->template component<0, boost::iostreams::basic_counter<Ch>>(); }
};

using string_backed_counter_fos = basic_string_backed_counter_fos<char>;
using wstring_backed_counter_fos = basic_string_backed_counter_fos<wchar_t>;
}

#endif
