#ifndef OCHAIN_H
#define OCHAIN_H

#include <iterator>
#include <ostream>
#include <stdexcept>
#include <string>

#include <boost/iostreams/filter/counter.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/ref.hpp>

namespace universals {

template <typename Ch> class basic_counter_ochain : private boost::iostreams::filtering_stream<boost::iostreams::output, Ch> {
  unsigned int counter_pos;

 public:
  basic_counter_ochain(std::basic_ostream<Ch>& os) : boost::iostreams::filtering_stream<boost::iostreams::output, Ch>(boost::iostreams::template basic_counter<Ch>() | boost::ref(os)), counter_pos{} {}
  template <typename OFilter> basic_counter_ochain(const OFilter& ofilter, std::basic_ostream<Ch>& os) : boost::iostreams::filtering_stream<boost::iostreams::output, Ch>(ofilter | boost::iostreams::template basic_counter<Ch>() | boost::ref(os)), counter_pos{1} {}
  basic_counter_ochain(const boost::iostreams::template basic_counter<Ch>& counter, std::basic_ostream<Ch>& os) : boost::iostreams::filtering_stream<boost::iostreams::output, Ch>(counter | boost::ref(os)), counter_pos{} {}
  template <typename OFilter> basic_counter_ochain(const OFilter& ofilter, const boost::iostreams::template basic_counter<Ch>& counter, std::basic_ostream<Ch>& os) : boost::iostreams::filtering_stream<boost::iostreams::output, Ch>(ofilter | counter | os), counter_pos{1} {}

  std::basic_ostream<Ch>& os() { return *this; }

  const boost::iostreams::basic_counter<Ch>& counter() const { return *this->template component<boost::iostreams::basic_counter<Ch>>(counter_pos); }
  std::basic_ostream<Ch>& sink() {
    this->flush();
    return *this->template component<boost::reference_wrapper<std::basic_ostream<Ch>>>(counter_pos + 1);
  }
};

using counter_ochain = basic_counter_ochain<char>;
using wcounter_ochain = basic_counter_ochain<wchar_t>;

template <typename Ch> class basic_string_backed_ochain : private boost::iostreams::filtering_stream<boost::iostreams::output, Ch> {
  std::basic_string<Ch> sink;

 public:
  basic_string_backed_ochain() : boost::iostreams::filtering_stream<boost::iostreams::output, Ch>(std::back_inserter(sink)) {}
  template <typename OFilter> basic_string_backed_ochain(const OFilter& ofilter) : boost::iostreams::filtering_stream<boost::iostreams::output, Ch>(ofilter | std::back_inserter(sink)) {}

  std::basic_ostream<Ch>& os() { return *this; }

  const std::basic_string<Ch>& str() {
    this->flush();
    return sink;
  }
  void merge_into(std::basic_ostream<Ch>& os, bool insert_nl = false);
};

template <typename Ch>
void
basic_string_backed_ochain<Ch>::merge_into(std::basic_ostream<Ch>& os, bool insert_nl) {
  this->flush();
  if (insert_nl)
    os.put('\n');
  os.write(sink.data(), sink.length());
}

using string_backed_ochain = basic_string_backed_ochain<char>;
using wstring_backed_ochain = basic_string_backed_ochain<wchar_t>;

template <typename Ch> class basic_string_backed_counter_ochain : private boost::iostreams::filtering_stream<boost::iostreams::output, Ch> {
  std::basic_string<Ch> sink;
  unsigned int counter_pos;

 public:
  basic_string_backed_counter_ochain() : boost::iostreams::filtering_stream<boost::iostreams::output, Ch>(boost::iostreams::template basic_counter<Ch>() | std::back_inserter(sink)), counter_pos{} {}
  template <typename OFilter> basic_string_backed_counter_ochain(const OFilter& ofilter) : boost::iostreams::filtering_stream<boost::iostreams::output, Ch>(ofilter | boost::iostreams::template basic_counter<Ch>() | std::back_inserter(sink)), counter_pos{1} {}
  basic_string_backed_counter_ochain(const boost::iostreams::template basic_counter<Ch>& counter) : boost::iostreams::filtering_stream<boost::iostreams::output, Ch>(counter | std::back_inserter(sink)), counter_pos{} {}
  template <typename OFilter> basic_string_backed_counter_ochain(const OFilter& ofilter, const boost::iostreams::template basic_counter<Ch>& counter) : boost::iostreams::filtering_stream<boost::iostreams::output, Ch>(ofilter | counter | std::back_inserter(sink)), counter_pos{1} {}

  std::basic_ostream<Ch>& os() { return *this; }

  const boost::iostreams::basic_counter<Ch>& counter() const { return *this->template component<boost::iostreams::basic_counter<Ch>>(counter_pos); }
  const std::basic_string<Ch>& str() {
    this->flush();
    return sink;
  }
  void merge_into(std::basic_ostream<Ch>& os, bool insert_nl = false);
};

template <typename Ch>
void
basic_string_backed_counter_ochain<Ch>::merge_into(std::basic_ostream<Ch>& os, bool insert_nl) {
  this->flush();
  if (insert_nl)
    os.put('\n');
  os.write(sink.data(), sink.length());
}

using string_backed_counter_ochain = basic_string_backed_counter_ochain<char>;
using wstring_backed_counter_ochain = basic_string_backed_counter_ochain<wchar_t>;
}

#endif
