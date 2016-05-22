#ifndef DDL_MASS_MIGRATOR_H
#define DDL_MASS_MIGRATOR_H

#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

#include <boost/filesystem/path.hpp>

#include <ddl_names.h>
#include <ddl_stnames_migrator.h>
namespace normal_ddl {
class ddl_names_filereader;
}
namespace normal_ddl {
class ddl_table_migrator_factory;
}
namespace middle {
struct ddl_source_root;
}

namespace middle {

class ddl_target_mass_migrator {
  const boost::filesystem::path target_ddl_path;
  const std::vector<std::string> filepaths;
  const bool have_usedcols;
  const normal_ddl::ddl_stname_usedcols stname_usedcols;
  const normal_ddl::ddl_assumed_identities assumed_idents;
  const normal_ddl::ddl_stnames_migration_msgs batch_stnames_migration_msgs;
  normal_ddl::ddl_stnames_migrator_factory& app_stnames_migrator_factory;
  std::ostream* verbose_os;

  unsigned int write_target_ddl_header(std::ostream& os, bool batch_source);

 public:
  ddl_target_mass_migrator(boost::filesystem::path&& target_ddl_path, std::vector<std::string>&& filepaths, bool have_usedcols, normal_ddl::ddl_stname_usedcols&& stname_usedcols, normal_ddl::ddl_assumed_identities&& assumed_idents, normal_ddl::ddl_stnames_migration_msgs&& batch_stnames_migration_msgs, normal_ddl::ddl_stnames_migrator_factory& app_stnames_migrator_factory, std::ostream* verbose_os) : target_ddl_path{std::move(target_ddl_path)}, filepaths{std::move(filepaths)}, have_usedcols{have_usedcols}, stname_usedcols{std::move(stname_usedcols)}, assumed_idents{std::move(assumed_idents)}, batch_stnames_migration_msgs{std::move(batch_stnames_migration_msgs)}, app_stnames_migrator_factory{app_stnames_migrator_factory}, verbose_os{verbose_os} {}

  void mass_migrate_target_ddl(normal_ddl::ddl_table_migrator_factory& app_table_migrator_factory, bool batch_sources);
};

class ddl_mass_migrator {
  normal_ddl::ddl_names_filereader& app_names_filereader;
  normal_ddl::ddl_table_migrator_factory& app_table_migrator_factory;
  normal_ddl::ddl_stnames_migrator_factory& app_stnames_migrator_factory;
  std::ostream* verbose_os;

 public:
  ddl_mass_migrator(normal_ddl::ddl_names_filereader& app_names_filereader, normal_ddl::ddl_table_migrator_factory& app_table_migrator_factory, normal_ddl::ddl_stnames_migrator_factory& app_stnames_migrator_factory, std::ostream* verbose_os = nullptr) : app_names_filereader{app_names_filereader}, app_table_migrator_factory{app_table_migrator_factory}, app_stnames_migrator_factory{app_stnames_migrator_factory}, verbose_os{verbose_os} {}

  void mass_migrate_ddl(const std::vector<std::string>& include_path_patts, const std::vector<std::string>& exclude_path_patts, const boost::filesystem::path& target_root, const std::vector<ddl_source_root>& src_roots, bool batch_sources);
};
}

#endif
