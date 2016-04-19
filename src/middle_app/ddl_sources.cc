#include <boost/filesystem/operations.hpp>

#include "ddl_sources.h"

namespace middle {
using namespace boost::filesystem;

const char*
ddl_source_root::validate() const {
  if (!exists(*this))
    return "doesn't exist";
  if (!is_directory(*this))
    return "not a directory";
  if (grafted_path && !is_relative())
    return "directory path not relative";
  return nullptr;
}
}