#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <ddl_names.h>
#include <ddl_table_migrator.h>
#include <ddl_table_migrator_prefs.h>

#include "mock-sqlsvr_ddl_queries.h"

struct test_ddl_datatype_rules : public normal_ddl::ddl_datatype_rules {
  test_ddl_datatype_rules();
};

struct test_ddl_table_migrator_impl : public normal_ddl::ddl_table_migrator {
  const std::vector<normal_ddl::ddl_cname> pk_cnames;

  test_ddl_table_migrator_impl(const normal_ddl::ddl_table_migrator_prefs& table_migrator_prefs, const std::vector<normal_ddl::ddl_cname>& pk_cnames) : normal_ddl::ddl_table_migrator{table_migrator_prefs, nullptr}, pk_cnames{pk_cnames} {}

  virtual normal_ddl::ddl_datatype_migration migrate_datatype_ddl(const normal_ddl::ddl_cname& cname, normal_ddl::ddl_datatype& datatype, bool nullable, const std::string& default_value, bool identity) override;
  virtual std::vector<normal_ddl::ddl_cname> filter_pk_columns() override;
  virtual std::vector<normal_ddl::ddl_cname> uniquify_identity_columns() override;
};

struct test_ddl_table_migrator_factory_impl : public normal_ddl::ddl_table_migrator_factory {
  const normal_ddl::ddl_table_migrator_prefs table_migrator_prefs;

  test_ddl_table_migrator_factory_impl() : normal_ddl::ddl_table_migrator_factory{table_migrator_prefs, nullptr}, table_migrator_prefs{test_ddl_datatype_rules{}, normal_ddl::ddl_default_value_rules{}} {}

  virtual std::unique_ptr<normal_ddl::ddl_table_migrator> make_table_migrator(const normal_ddl::ddl_stname& stname, const std::vector<normal_ddl::ddl_cname>& pk_cnames, std::ostream* verbose_os) override { return std::unique_ptr<normal_ddl::ddl_table_migrator>{new test_ddl_table_migrator_impl{table_migrator_prefs, pk_cnames}}; }
};

// The fixture for testing class sqlsvr_ddl_stnames_migrator_impl
class sqlsvr_ddl_stnames_migrator_impl_testfix : public ::testing::Test {
 protected:
  mock_sqlsvr_ddl_queries mock_queries;
  test_ddl_table_migrator_factory_impl table_migrator_factory;

  // You can do set-up work for each test here.
  sqlsvr_ddl_stnames_migrator_impl_testfix();

  // You can do clean-up work that doesn't throw exceptions here.
  virtual ~sqlsvr_ddl_stnames_migrator_impl_testfix() {}

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  // Code here will be called immediately after the constructor (right
  // before each test).
  virtual void SetUp() {}

  // Code here will be called immediately after each test (right
  // before the destructor).
  virtual void TearDown() {}
};
