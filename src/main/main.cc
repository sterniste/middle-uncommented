#include <cassert>
#include <chrono>
#include <exception>
#include <iostream>
#include <string>

#include <boost/algorithm/string/replace.hpp>
#ifdef LEVEL_LOGGING
#include <boost/log/trivial.hpp>
#endif

#include <dbunit_hsqldb_ddl_table_migrator.h>
#include <dbunit_xml_ddl_names_filereader.h>
#include <ddl_mass_migrator.h>
#include <ddl_table_migrator_prefs.h>
#include <ddl_table_migrator_prefs_error.h>
#include <ddl_table_migrator_prefs_reader.h>
#include <sqlsvr_ddl_connection_error.h>
#include <sqlsvr_ddl_stnames_migrator.h>

#include "options.h"

namespace {
using namespace std;
using namespace std::chrono;
using namespace boost::algorithm;
using namespace normal_ddl;
using namespace sqlsvr_ddl;
using namespace middle;

string
log_cmd_args(int argc, const char* const argv[]) {
  string line;
  for (int i = 0; i < argc; ++i) {
    if (i)
      line += ' ';
    line += argv[i];
  }
  return line;
}
}

ddl_table_migrator_prefs
middle_runner::get_table_migrator_prefs(ostream* verbose_os) {
  return json_prefs_is ? ddl_table_migrator_prefs_reader{verbose_os}.read_table_migrator_prefs(*json_prefs_is) : ddl_table_migrator_prefs{};
}

int
middle_runner::run() {
#ifdef LEVEL_LOGGING
  BOOST_LOG_TRIVIAL(info) << "started as: '" << log_cmd_args(argc, argv) << '\'';
// TODO: options, cwd
#endif
  ostream* const verbose_os = verbose ? &cout : nullptr;
  try {
    const ddl_table_migrator_prefs table_migrator_prefs{get_table_migrator_prefs(verbose_os)};
    dbunit_hsqldb_ddl_table_migrator_factory_impl app_table_migrator_factory{table_migrator_prefs, verbose_os};
    sqlsvr_ddl_stnames_migrator_factory_impl app_stnames_migrator_factory{data_sources, verbose_os};
    dbunit_ddl_names_filereader_impl app_names_filereader{app_stnames_migrator_factory, verbose_os};
    ddl_mass_migrator mass_migrator{app_names_filereader, app_table_migrator_factory, app_stnames_migrator_factory, verbose_os};

    const auto before = high_resolution_clock::now();
    mass_migrator.mass_migrate_ddl(include_path_patts, exclude_path_patts, target_root, src_roots, batch_sources);
#ifdef LEVEL_LOGGING
    BOOST_LOG_TRIVIAL(info) << "finished normally";
#endif
    if (time_migration) {
      const auto after = high_resolution_clock::now();
      cout << "migration time taken: " << duration_cast<milliseconds>(after - before).count() << " ms" << endl;
    }
    return 0;
  } catch (const ddl_table_migrator_prefs_error& e) {
    assert(!json_prefs_file.empty());
    string what{e.what()};
    if (what.find("<unspecified file>") != string::npos)
      replace_all(what, "<unspecified file>", string{"file '"} + json_prefs_file + '\'');
    else
      what.insert(0, string{"file '"} + json_prefs_file + "': ");
#ifdef LEVEL_LOGGING
    BOOST_LOG_TRIVIAL(fatal) << "caught ddl_table_migrator_prefs_error: " << what;
#endif
    cerr << "caught ddl_table_migrator_prefs_error: " << what << endl;
    return 1;
  } catch (const sqlsvr_ddl_connection_errors& e) {
    assert(!e.empty());
#ifdef LEVEL_LOGGING
    BOOST_LOG_TRIVIAL(fatal) << "caught " << e.size() << " sqlsvr_ddl_connection_errors: ";
#endif
    cerr << "caught " << e.size() << " sqlsvr_ddl_connection_errors: " << endl;
    for (const auto& connection_error : e) {
      assert(connection_error.conn_pos < data_sources.size());
#ifdef LEVEL_LOGGING
      BOOST_LOG_TRIVIAL(fatal) << "ODBC data-source " << data_sources[connection_error.conn_pos].ds_name << ": " << connection_error.what();
#endif
      cerr << "ODBC data-source " << data_sources[connection_error.conn_pos].ds_name << ": " << connection_error.what() << endl;
    }
    return 1;
  } catch (const exception& e) {
#ifdef LEVEL_LOGGING
    BOOST_LOG_TRIVIAL(fatal) << "caught std::exception: " << e.what();
#endif
    cerr << "caught std::exception: " << e.what() << endl;
    return 1;
  }
}

int
main(int argc, const char* argv[]) {
  try {
    return middle_runner::parse_args(argc, argv).run();
  } catch (const middle_opts_exception& e) {
    if (e.not_an_error)
      cout << e.what() << endl;
    else {
      cerr << e.what() << endl;
      return 1;
    }
  }
}
