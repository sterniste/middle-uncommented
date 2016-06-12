#ifndef OPTIONS_H
#define OPTIONS_H

#include <iosfwd>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>

#include <ddl_sources.h>
#include <ddl_table_migrator_prefs.h>
#include <sqlsvr_ddl_data_source.h>

namespace middle {

struct middle_opts_exception : public std::runtime_error {
  bool not_an_error;

  middle_opts_exception(const std::string& msg, bool not_an_error = false) : std::runtime_error{msg}, not_an_error{not_an_error} {}
};

class middle_runner {
  static const char* validate_target_root(const boost::filesystem::path& target_root);
  static std::vector<sqlsvr_ddl::sqlsvr_ddl_odbc_data_source> parse_odbc_data_sources(const std::vector<std::string>& odbc_data_source_specs);

  const int argc;
  const char* const* const argv;
  bool verbose, time_migration;
  std::string json_prefs_file;
  std::unique_ptr<std::istream> json_prefs_is;
  std::vector<sqlsvr_ddl::sqlsvr_ddl_odbc_data_source> data_sources;
  std::vector<std::string> include_path_patts;
  std::vector<std::string> exclude_path_patts;
  boost::filesystem::path target_root;
  std::vector<ddl_source_root> src_roots;
  bool batch_sources;

  middle_runner(int argc, const char* argv[]) : argc{argc}, argv{argv}, verbose{}, time_migration{}, batch_sources{} {}

  normal_ddl::ddl_table_migrator_prefs get_table_migrator_prefs(std::ostream* verbose_os);

 public:
  static middle_runner parse_args(int argc, const char* argv[]);

  int run();
};
}
#endif
