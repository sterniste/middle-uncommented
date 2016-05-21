#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>

#include <app_version.h>
#include <ddl_sources.h>
#include <sqlsvr_ddl_data_source.h>

#include "logging.h"
#include "options.h"

namespace middle {
using namespace std;
using namespace boost::filesystem;
using namespace boost::program_options;
using namespace sqlsvr_ddl;

const char*
middle_runner::validate_target_root(const path& target_root) {
  if (exists(target_root)) {
    if (!is_directory(target_root))
      return "not a directory";
  } else {
    path target_path{target_root}, target_parent_path;
    while ((target_parent_path = target_path.parent_path()) != target_path) {
      if (exists(target_parent_path)) {
        if (is_directory(target_parent_path))
          break;
        return "not a directory";
      }
      target_path = target_parent_path;
    }
  }
  return nullptr;
}

vector<sqlsvr_ddl_odbc_data_source>
middle_runner::parse_odbc_data_sources(const vector<string>& dsn_app_pwds) {
  vector<sqlsvr_ddl_odbc_data_source> data_sources;
  for (const auto& dsn_app_pwd : dsn_app_pwds) {
    const auto pos1 = dsn_app_pwd.find(':');
    if (pos1 == 0 || pos1 == string::npos)
      throw invalid_argument{string{"missing/empty DSN in ODBC datasource '"} + dsn_app_pwd + '\''};
    const auto pos2 = dsn_app_pwd.find(':', pos1 + 1);
    if (pos2 == pos1 + 1 || pos2 == string::npos)
      throw invalid_argument{string{"missing/empty username in ODBC datasource '"} + dsn_app_pwd + '\''};
    data_sources.push_back(sqlsvr_ddl_odbc_data_source{dsn_app_pwd.substr(0, pos1), dsn_app_pwd.substr(pos1 + 1, pos2 - (pos1 + 1)), dsn_app_pwd.substr(pos2 + 1)});
  }
  return data_sources;
}

middle_runner
middle_runner::parse_args(int argc, const char* argv[]) {
  middle_runner runner{argc, argv};

  // gather options
  ostringstream opt_headers_oss;
  const char* const usage = "usage: middle [options] target-root source-root [source-root ...]";
  opt_headers_oss << "middle (MIgrate DDL Everywhere); version: " << app_version << endl << usage << endl << "Command-line options";
  options_description cmd_line_opts_desc(opt_headers_oss.str());
  cmd_line_opts_desc.add_options()("help,h", "this help message")("config-file,c", value<string>(), "configuration file")("time,t", "output migration time (successful runs only)")("verbose,v", "verbose")("version", "version");

  options_description config_file_opts_desc("Configuration options");
  config_file_opts_desc.add_options()("odbc-datasource,d", value<vector<string>>()->composing(), "ODBC DSN:username:password")("graft-source-paths,g", "graft source paths to target root")("include-source-paths,i", value<vector<string>>()->composing(), "include source filepaths (partial regex)")("exclude-source-paths,x", value<vector<string>>()->composing(), "exclude source filepaths (partial regex)")("json-prefs-file,j", value<string>(), "json preferences file")("batch-sources,b", "batch sources under each source root")
#ifdef LEVEL_LOGGING
      ("log-file", value<string>(), "log to file")("log-level", value<string>()->default_value("info"), "log level: [fatal, error, warn, info, debug, trace]")
#endif
      ;
  cmd_line_opts_desc.add(config_file_opts_desc);

  variables_map var_map;
  vector<string> unrecognized_opts;
  try {
    const parsed_options parsed{command_line_parser(argc, argv).options(cmd_line_opts_desc).run()};
    store(parsed, var_map);
    notify(var_map);
    unrecognized_opts = collect_unrecognized(parsed.options, include_positional);
  } catch (const exception& e) {
    throw middle_opts_exception{string{"can't parse command line: "} + e.what()};
  }

  // help/version
  if (var_map.count("help")) {
    ostringstream oss;
    oss << cmd_line_opts_desc;
    throw middle_opts_exception{oss.str(), true};
  }
  runner.time_migration = var_map.count("time");
  if (var_map.count("version"))
    throw middle_opts_exception{app_version, true};

  if (var_map.count("config-file")) {
    const char* const config_file = var_map["config-file"].as<string>().c_str();
    if (!exists(config_file) || !is_regular_file(config_file))
      throw middle_opts_exception{string{"can't find configuration file '"} + config_file + '\''};
    store(parse_config_file<char>(var_map["config-file"].as<string>().c_str(), config_file_opts_desc), var_map);
  } else {
    const char* const config_file = "middle.cfg";
    if (exists(config_file) && is_regular_file(config_file))
      store(parse_config_file<char>(config_file, config_file_opts_desc), var_map);
  }
  notify(var_map);

  runner.verbose = var_map.count("verbose");
  // validate target root
  if (unrecognized_opts.empty())
    throw middle_opts_exception{string{"no target root set"}};
  const string target_filepath{*unrecognized_opts.cbegin()};
  runner.target_root = target_filepath;
  const char* const msg{validate_target_root(runner.target_root)};
  if (msg)
    throw middle_opts_exception{string{"target root '"} + target_filepath + "': " + msg};
  unrecognized_opts.erase(unrecognized_opts.cbegin(), unrecognized_opts.cbegin() + 1);

  // validate source root(s) (1)
  if (unrecognized_opts.empty())
    throw middle_opts_exception{string{"no source roots set"}};
  const vector<string> source_filepaths{move(unrecognized_opts)};

  // option validation

  // ODBC datasources
  if (!var_map.count("odbc-datasource"))
    throw middle_opts_exception{string{"no ODBC datasources set"}};
  try {
    runner.data_sources = parse_odbc_data_sources(var_map["odbc-datasource"].as<vector<string>>());
  } catch (const invalid_argument& e) {
    throw middle_opts_exception{e.what()};
  }
  const bool graft_source_paths{static_cast<bool>(var_map.count("graft-source-paths"))};
  // validate source root(s) (2)
  for (const auto& source_filepath : source_filepaths) {
    const ddl_source_root src_root{graft_source_paths, path{source_filepath}};
    const char* const msg{src_root.validate()};
    if (msg)
      throw middle_opts_exception{string{graft_source_paths ? "grafted path" : ""} + " source root '" + source_filepath + "': " + msg};
    runner.src_roots.push_back(move(src_root));
  }

  // validate include/exclude source filepath patterns opts
  if (var_map.count("include-source-paths"))
    runner.include_path_patts = move(var_map["include-source-paths"].as<vector<string>>());
  if (var_map.count("exclude-source-paths"))
    runner.exclude_path_patts = move(var_map["exclude-source-paths"].as<vector<string>>());

  // validate JSON preference file opt
  if (var_map.count("json-prefs-file")) {
    runner.json_prefs_is.reset(new ifstream{var_map["json-prefs-file"].as<string>()});
    if (!static_cast<ifstream*>(runner.json_prefs_is.get())->is_open())
      throw middle_opts_exception{string{"can't open JSON preference file '"} + var_map["json-prefs-file"].as<string>() + '\''};
  }

#ifdef LEVEL_LOGGING
  // validate logging opts
  const int log_level = parse_log_level(var_map["log-level"].as<string>());
  if (log_level < 0)
    throw middle_opts_exception{string{"log level: '"} + var_map["log-level"].as<string>() + "' not in [fatal, error, warn, info, debug, trace]"};
  // logging setup
  setup_log_file(var_map["log-file"].as<string>(), log_level);
#endif

  return runner;
}
}
