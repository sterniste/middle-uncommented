#ifndef DDL_SOURCES_H
#define DDL_SOURCES_H

#include <boost/filesystem/path.hpp>

namespace middle {

struct ddl_source_root : public boost::filesystem::path {
  const char* validate() const;

  bool grafted_path;

  ddl_source_root(bool grafted_path, const boost::filesystem::path& root) : boost::filesystem::path{root}, grafted_path{grafted_path} {}
};
}

#endif