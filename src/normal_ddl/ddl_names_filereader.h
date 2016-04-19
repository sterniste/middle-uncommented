#ifndef DDL_NAMES_FILEREADER_H
#define DDL_NAMES_FILEREADER_H

#include <iosfwd>
#include <stdexcept>
#include <string>

#include "ddl_names.h"

namespace normal_ddl {

struct ddl_names_filereader_parse_error : public std::runtime_error {
  ddl_names_filereader_parse_error(const std::string& msg) : std::runtime_error{msg} {}
};

class ddl_names_filereader {
 protected:
  std::ostream* verbose_os;

  ddl_names_filereader(std::ostream* verbose_os = nullptr) : verbose_os{verbose_os} {}

 public:
  virtual ~ddl_names_filereader() {}

  virtual bool read_ddl_names(const std::string& filepath, ddl_stname_usedcols& stname_usedcols, ddl_assumed_identities& assumed_idents) = 0;
};
}

#endif
