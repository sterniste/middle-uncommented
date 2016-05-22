#include <algorithm>
#include <cassert>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iterator>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
#ifdef LEVEL_LOGGING
#include <boost/log/trivial.hpp>
#endif

#include <app_version.h>
#include <ddl_mass_migrator.h>
#include <ddl_migration_msg.h>
#include <ddl_names.h>
#include <ddl_names_filereader.h>
#include <ddl_sources.h>
#include <ddl_stnames_migrator.h>
namespace normal_ddl {
class ddl_table_migrator_factory;
}

namespace middle {
using namespace std;
using namespace std::chrono;
using namespace std::regex_constants;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::iostreams;
using namespace normal_ddl;

vector<std::regex>
regex_compile_include_paths(const vector<string>& include_path_patts) {
  vector<std::regex> include_path_regexs;
  for (const auto& include_path_patt : include_path_patts)
    include_path_regexs.push_back(regex{include_path_patt});
  return include_path_regexs;
}

vector<std::regex>
regex_compile_exclude_paths(const vector<string>& exclude_path_patts) {
  vector<std::regex> exclude_path_regexs;
  for (const auto& exclude_path_patt : exclude_path_patts)
    exclude_path_regexs.push_back(regex{exclude_path_patt});
  return exclude_path_regexs;
}

string
mirror_filepath(const ddl_source_root& src_root, const string& filepath) {
  if (src_root.grafted_path)
    return filepath;
  assert(filepath.find(src_root.generic_string()) == 0);
  return filepath.substr(src_root.generic_string().size());
}

bool
search_any_regex(const string& against, const vector<std::regex>& search_regexs) {
  for (const auto& search_regex : search_regexs) {
    std::smatch what;
    if (std::regex_search(against, what, search_regex))
      return true;
  }
  return false;
}

path
mirror_target_ddl_filepath(const path& target_root, const string& filepath_mirror) {
  path mirror_path{filepath_mirror};
  mirror_path.replace_extension(path{".ddl"});
  return target_root / mirror_path;
}

const char*
now() {
  static char timebuf[64];
  const time_t time_now{system_clock::to_time_t(system_clock::now())};
  strftime(timebuf, sizeof(timebuf), "%c %Z", localtime(&time_now));
  return timebuf;
}

unsigned int
ddl_target_mass_migrator::write_target_ddl_header(ostream& os, bool batch_source) {
  unsigned int lineno_offset{};
  os << "-- DO NOT EDIT: this file was migrated by MIDDLE (version: " << app_version << ") at " << now() << " from " << filepaths.size() << (batch_source ? " batch " : " ") << "source(s): " << endl;
  ++lineno_offset;
  for (const auto& filepath : filepaths) {
    os << "-- " << filepath << endl;
    ++lineno_offset;
  }
  os << endl;
  return ++lineno_offset;
}

void
ddl_target_mass_migrator::mass_migrate_target_ddl(ddl_table_migrator_factory& app_table_migrator_factory, bool batch_source) {
  if (!stname_usedcols.empty()) {
    if (!exists(target_ddl_path.parent_path()))
      create_directories(target_ddl_path.parent_path());
    ofstream target_ddl_ofs{target_ddl_path.generic_string()};
    if (target_ddl_ofs.is_open()) {
#ifdef LEVEL_LOGGING
      BOOST_LOG_TRIVIAL(info) << "writing target DDL '" << target_ddl_path.generic_string() << '\'';
#endif
      if (verbose_os)
        *verbose_os << "writing target DDL '" << target_ddl_path.generic_string() << '\'' << endl;
      const unsigned int lineno_offset{write_target_ddl_header(target_ddl_ofs, batch_source)};
      const unique_ptr<ddl_stnames_migrator> app_stnames_migrator{app_stnames_migrator_factory.make_stnames_migrator(stname_usedcols, have_usedcols, assumed_idents, verbose_os)};
      assert(app_stnames_migrator);
      ddl_stnames_migration_msgs stnames_migration_msgs{app_stnames_migrator->migrate_ddl_stnames(&app_table_migrator_factory, target_ddl_ofs)};
      copy(batch_stnames_migration_msgs.cbegin(), batch_stnames_migration_msgs.cend(), back_inserter(stnames_migration_msgs));
      if (!stnames_migration_msgs.empty()) {
        sort(stnames_migration_msgs.begin(), stnames_migration_msgs.end());
        target_ddl_ofs << endl << "-- errors/warnings/remarks:" << endl;
        for (const auto& stnames_migration_msg : stnames_migration_msgs)
          target_ddl_ofs << "-- " << stnames_migration_msg.str(lineno_offset) << endl;
      }
    } else {
#ifdef LEVEL_LOGGING
      BOOST_LOG_TRIVIAL(info) << "can't write target '" << target_ddl_path.generic_string();
#endif
      throw runtime_error{string{"can't write target '"} + target_ddl_path.generic_string() + '\''};
    }
  } else
    // TODO
    ;
}

class batch_ddl_mass_migrator {
  vector<string> filepaths;
  unique_ptr<const bool> batch_have_usedcols;
  ddl_stname_usedcols batch_stname_usedcols;
  ddl_assumed_identities batch_assumed_idents;
  ddl_stnames_migration_msgs batch_stnames_migration_msgs;
  ddl_stnames_migrator_factory& app_stnames_migrator_factory;
  const ddl_source_root& src_root;
  const unsigned int root_pos;
  const path& target_root;
  ostream* verbose_os;

 public:
  batch_ddl_mass_migrator(ddl_stnames_migrator_factory& app_stnames_migrator_factory, const ddl_source_root& src_root, unsigned int root_pos, const path& target_root, ostream* verbose_os) : app_stnames_migrator_factory{app_stnames_migrator_factory}, src_root{src_root}, root_pos{root_pos}, target_root{target_root}, verbose_os{verbose_os} {}

  void validate_batch_ddl(const string& filepath, const ddl_stname_usedcols& stname_usedcols, bool have_usedcols, const ddl_assumed_identities& assumed_idents);
  ddl_target_mass_migrator get_target_mass_migrator();
};

void
batch_ddl_mass_migrator::validate_batch_ddl(const string& filepath, const ddl_stname_usedcols& stname_usedcols, bool have_usedcols, const ddl_assumed_identities& assumed_idents) {
  const unique_ptr<ddl_stnames_migrator> app_stnames_migrator{app_stnames_migrator_factory.make_stnames_migrator(stname_usedcols, have_usedcols, assumed_idents, verbose_os)};
  assert(app_stnames_migrator);
  filtering_ostream null_fos;
  ddl_stnames_migration_msgs stnames_migration_msgs{app_stnames_migrator->migrate_ddl_stnames(nullptr, null_fos)};
  // rewrite migration msgs here
  for (auto& stname_migration_msg : stnames_migration_msgs) {
    assert(!stname_migration_msg.lineno && stname_migration_msg.msg.msg.length());
    stname_migration_msg.msg.msg += ddl_stnames_migration_msg::batch_source_file_msg_suffix + filepath + "']";
  }
  copy(stnames_migration_msgs.cbegin(), stnames_migration_msgs.cend(), back_inserter(batch_stnames_migration_msgs));
  batch_stname_usedcols.merge_into(stname_usedcols);
  copy(assumed_idents.cbegin(), assumed_idents.cend(), inserter(batch_assumed_idents, batch_assumed_idents.end()));
  if (!batch_have_usedcols)
    batch_have_usedcols.reset(new bool{have_usedcols});
  else
    assert(have_usedcols == *batch_have_usedcols);
  filepaths.push_back(filepath);
}

ddl_target_mass_migrator
batch_ddl_mass_migrator::get_target_mass_migrator() {
  path filepath{src_root};
  filepath /= (string{"middle-batch-"} + lexical_cast<string>(root_pos + 1));
  const string filepath_mirror{mirror_filepath(src_root, filepath.generic_string())};
  const bool have_usedcols{batch_have_usedcols ? *batch_have_usedcols : false};
  return ddl_target_mass_migrator{path{mirror_target_ddl_filepath(target_root, filepath_mirror)}, std::move(filepaths), have_usedcols, std::move(batch_stname_usedcols), std::move(batch_assumed_idents), std::move(batch_stnames_migration_msgs), app_stnames_migrator_factory, verbose_os};
}

void
ddl_mass_migrator::mass_migrate_ddl(const vector<string>& include_path_patts, const vector<string>& exclude_path_patts, const path& target_root, const vector<ddl_source_root>& src_roots, bool batch_sources) {
  if (!exists(target_root))
    create_directories(target_root);

  const vector<std::regex> include_path_regexs{regex_compile_include_paths(include_path_patts)};
  const vector<std::regex> exclude_path_regexs{regex_compile_exclude_paths(exclude_path_patts)};
  unsigned int root_pos = 0;
  for (const auto& src_root : src_roots) {
    const unique_ptr<batch_ddl_mass_migrator> batch_mass_migrator{batch_sources ? new batch_ddl_mass_migrator{app_stnames_migrator_factory, src_root, root_pos, target_root, verbose_os} : nullptr};
    const recursive_directory_iterator dir_end;
    for (recursive_directory_iterator dir_it{src_root.generic_string()}; dir_it != dir_end; ++dir_it) {
      if (is_regular_file(dir_it->status())) {
        const string filepath{dir_it->path().generic_string()};
        const string filepath_mirror{mirror_filepath(src_root, filepath)};
        if (!search_any_regex(filepath_mirror, exclude_path_regexs) && (include_path_regexs.empty() || search_any_regex(filepath_mirror, include_path_regexs))) {
#ifdef LEVEL_LOGGING
          BOOST_LOG_TRIVIAL(info) << "reading source '" << filepath << '\'';
#endif
          if (verbose_os)
            *verbose_os << "reading source '" << filepath << '\'' << endl;
          try {
            ddl_stname_usedcols stname_usedcols;
            ddl_assumed_identities assumed_idents;
            const bool have_usedcols{app_names_filereader.read_ddl_names(filepath, stname_usedcols, assumed_idents)};
            if (!batch_mass_migrator)
              ddl_target_mass_migrator{path{mirror_target_ddl_filepath(target_root, filepath_mirror)}, vector<string>{filepath}, have_usedcols, std::move(stname_usedcols), std::move(assumed_idents), ddl_stnames_migration_msgs{}, app_stnames_migrator_factory, verbose_os}.mass_migrate_target_ddl(app_table_migrator_factory, false);
            else
              batch_mass_migrator->validate_batch_ddl(filepath, stname_usedcols, have_usedcols, assumed_idents);
          } catch (const ddl_names_filereader_parse_error& e) {
#ifdef LEVEL_LOGGING
            BOOST_LOG_TRIVIAL(debug) << "skipping source '" << filepath << "': DDL names filereader parse error: " << e.what();
#endif
            if (verbose_os)
              *verbose_os << "skipping source '" << filepath << "': DDL names filereader parse error: " << e.what() << endl;
          }
        }
      }
    }
    if (batch_mass_migrator)
      batch_mass_migrator->get_target_mass_migrator().mass_migrate_target_ddl(app_table_migrator_factory, true);
    ++root_pos;
  }
}
}
