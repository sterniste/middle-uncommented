#ifndef DBUNIT_XML_DDL_NAMES_FILEREADER_H
#define DBUNIT_XML_DDL_NAMES_FILEREADER_H

#include <iosfwd>
#include <string>

#include <xercesc/util/PlatformUtils.hpp>

#include <ddl_names.h>
#include <ddl_names_filereader.h>
namespace normal_ddl { class ddl_stnames_migrator_factory; }

namespace middle {

class dbunit_ddl_names_filereader_impl : public normal_ddl::ddl_names_filereader {
  const normal_ddl::ddl_stnames_migrator_factory& app_stnames_migrator_factory;

 public:
  virtual ~dbunit_ddl_names_filereader_impl() override { xercesc::XMLPlatformUtils::Terminate(); }
  dbunit_ddl_names_filereader_impl(const normal_ddl::ddl_stnames_migrator_factory& app_stnames_migrator_factory, std::ostream* verbose_os = nullptr) : normal_ddl::ddl_names_filereader{verbose_os}, app_stnames_migrator_factory{app_stnames_migrator_factory} { xercesc::XMLPlatformUtils::Initialize(); }

  virtual bool read_ddl_names(const std::string& filepath, normal_ddl::ddl_stname_usedcols& stname_usedcols, normal_ddl::ddl_assumed_identities& assumed_idents) override;
};
}

#endif
