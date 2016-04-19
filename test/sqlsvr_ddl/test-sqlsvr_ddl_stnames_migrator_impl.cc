#include <string>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ddl_column_rules.h>
#include <ddl_migration_msg.h>
#include <ddl_names.h>
#include <ddl_stnames_migrator.h>
#include <ddl_table_migrator.h>
#include <ddl_table_migrator_prefs.h>
#include <ochain.h>
#include <sqlsvr_ddl_fk_mapper.h>
#include <sqlsvr_ddl_pk_mapper.h>
#include <sqlsvr_ddl_stnames_migrator.h>
#include <sqlsvr_ddl_tc_mapper.h>
#include <uc_string.h>

#include "mock-sqlsvr_ddl_queries.h"
#include "test-sqlsvr_ddl_stnames_migrator_impl.h"

using namespace std;
using ::testing::An;
using ::testing::Return;
using namespace normal_ddl;
using namespace sqlsvr_ddl;
using namespace universals;

test_ddl_datatype_rules::test_ddl_datatype_rules() {
  emplace(pair<uc_string, ddl_datatype_rule>{"DATATYPE", ddl_datatype_rule{ddl_datatype_rule_type::precision_scale, "", vector<ddl_map_size>{ddl_map_size{ddl_map_size_type::suppress}, ddl_map_size{ddl_map_size_type::scale_narrow, 0, 0}, ddl_map_size{ddl_map_size_type::scale_reject_defined}}}});
}

ddl_datatype_migration
test_ddl_table_migrator_impl::migrate_datatype_ddl(const ddl_cname& cname, ddl_datatype& datatype, bool nullable, const string& default_value, bool identity) {
  return ddl_datatype_migration{ddl_table_migrator::migrate_datatype_ddl(cname, datatype, nullable, default_value, identity)};
}

vector<ddl_cname>
test_ddl_table_migrator_impl::filter_pk_columns() {
  return pk_cnames;
}

vector<ddl_cname>
test_ddl_table_migrator_impl::uniquify_identity_columns() {
  return vector<ddl_cname>{};
}

sqlsvr_ddl_stnames_migrator_impl_testfix::sqlsvr_ddl_stnames_migrator_impl_testfix() {
  ON_CALL(mock_queries, find_ck(An<const ddl_stcname&>())).WillByDefault(Return(nullptr));
  ON_CALL(mock_queries, find_fk(An<const ddl_stcname&>(), An<bool>())).WillByDefault(Return(nullptr));
  ON_CALL(mock_queries, find_fk_reverse(An<const ddl_stcname&>())).WillByDefault(Return(nullptr));
  ON_CALL(mock_queries, find_fk_constraint_name(An<const string&>(), An<bool>())).WillByDefault(Return(nullptr));
  ON_CALL(mock_queries, find_pk(An<const ddl_stcname&>(), An<bool>())).WillByDefault(Return(nullptr));
  ON_CALL(mock_queries, find_tc(An<const ddl_stname&>())).WillByDefault(Return(nullptr));
  ON_CALL(mock_queries, find_uq(An<const ddl_stcname&>())).WillByDefault(Return(nullptr));
  ON_CALL(mock_queries, insert_uq(An<const ddl_stcname&>(), An<const vector<sqlsvr_ddl_uq_rec>&>())).WillByDefault(Return(false));
}

TEST_F(sqlsvr_ddl_stnames_migrator_impl_testfix, test1) {
  ddl_stname_usedcols stname_usedcols{};
  const ddl_stname stname{"schema", "table"};
  stname_usedcols.insert_name(stname);
  const ddl_assumed_identities assumed_idents;
  sqlsvr_ddl_stnames_migrator_impl stnames_migrator{mock_queries, stname_usedcols, true, assumed_idents, nullptr};

  EXPECT_CALL(mock_queries, find_tc(stname)).Times(1);

  string_backed_counter_ochain ochain;
  const ddl_stnames_migration_msgs stnames_migration_msgs{stnames_migrator.migrate_ddl_stnames(nullptr, ochain.os())};
  EXPECT_EQ(1, stnames_migration_msgs.size());
  const ddl_stnames_migration_msg expected_stnames_migration_msg{0, stname, ddl_cname{}, ddl_migration_msg{ddl_migration_msg_level::error, ddl_migration_msg::table_used_but_undefined_msg}};
  EXPECT_EQ(expected_stnames_migration_msg, *stnames_migration_msgs.cbegin());
  EXPECT_EQ(true, ochain.str().empty());
}

TEST_F(sqlsvr_ddl_stnames_migrator_impl_testfix, test2) {
  ddl_stname_usedcols stname_usedcols{};
  const ddl_stname stname{"schema", "table"};
  stname_usedcols.insert_name(stname);
  const ddl_assumed_identities assumed_idents;
  sqlsvr_ddl_stnames_migrator_impl stnames_migrator{mock_queries, stname_usedcols, true, assumed_idents, nullptr};

  vector<sqlsvr_ddl_tc_rec> tc_recs{};
  tc_recs.emplace_back(sqlsvr_ddl_tc_rec{ddl_cname{"column"}, unique_ptr<string>{new string{"default_value"}}, false, string{"datatype"}, unique_ptr<int32_t>{}, unique_ptr<int32_t>{}, unique_ptr<uint8_t>{}, unique_ptr<int16_t>{}, unique_ptr<int32_t>{}, unique_ptr<int16_t>{}, unique_ptr<string>{}, false});
  EXPECT_CALL(mock_queries, find_tc(stname)).Times(2).WillRepeatedly(Return(&tc_recs));
  const ddl_stcname stcname{stname, "column"};
  EXPECT_CALL(mock_queries, find_fk(stcname, true)).Times(1);
  EXPECT_CALL(mock_queries, find_pk(stcname, true)).Times(1);

  string_backed_counter_ochain ochain;
  const ddl_stnames_migration_msgs stnames_migration_msgs{stnames_migrator.migrate_ddl_stnames(nullptr, ochain.os())};
  EXPECT_EQ(0, stnames_migration_msgs.size());
  EXPECT_EQ(true, ochain.str().empty());
}

TEST_F(sqlsvr_ddl_stnames_migrator_impl_testfix, test3) {
  ddl_stname_usedcols stname_usedcols{};
  const ddl_stname stname{"schema", "table"};
  stname_usedcols.insert_name(stname);
  const ddl_assumed_identities assumed_idents;
  sqlsvr_ddl_stnames_migrator_impl stnames_migrator{mock_queries, stname_usedcols, true, assumed_idents, nullptr};

  vector<sqlsvr_ddl_tc_rec> tc_recs{};
  tc_recs.emplace_back(sqlsvr_ddl_tc_rec{ddl_cname{"column"}, unique_ptr<string>{new string{"default_value"}}, false, string{"datatype"}, unique_ptr<int32_t>{}, unique_ptr<int32_t>{}, unique_ptr<uint8_t>{}, unique_ptr<int16_t>{}, unique_ptr<int32_t>{}, unique_ptr<int16_t>{}, unique_ptr<string>{}, false});
  EXPECT_CALL(mock_queries, find_tc(stname)).Times(5).WillRepeatedly(Return(&tc_recs));
  const ddl_stcname stcname{stname, "column"};
  EXPECT_CALL(mock_queries, find_ck(stcname)).Times(1);
  EXPECT_CALL(mock_queries, find_fk(stcname, true)).Times(1);
  EXPECT_CALL(mock_queries, find_pk(stcname, true)).Times(1);
  EXPECT_CALL(mock_queries, find_uq(stcname)).Times(1);

  string_backed_counter_ochain ochain;
  const ddl_stnames_migration_msgs stnames_migration_msgs{stnames_migrator.migrate_ddl_stnames(&table_migrator_factory, ochain.os())};
  EXPECT_EQ(0, stnames_migration_msgs.size());
  EXPECT_EQ("CREATE SCHEMA schema;\n\nCREATE TABLE schema.table(column DATATYPE DEFAULT default_value NOT NULL);\n", ochain.str());
}

TEST_F(sqlsvr_ddl_stnames_migrator_impl_testfix, test4) {
  ddl_stname_usedcols stname_usedcols{};
  const ddl_stname stname{"schema", "table"};
  stname_usedcols.insert_name(stname);
  const ddl_assumed_identities assumed_idents;
  sqlsvr_ddl_stnames_migrator_impl stnames_migrator{mock_queries, stname_usedcols, true, assumed_idents, nullptr};

  vector<sqlsvr_ddl_tc_rec> tc_recs{};
  tc_recs.emplace_back(sqlsvr_ddl_tc_rec{ddl_cname{"column"}, unique_ptr<string>{new string{"default_value"}}, false, string{"datatype"}, unique_ptr<int32_t>{}, unique_ptr<int32_t>{}, unique_ptr<uint8_t>{}, unique_ptr<int16_t>{}, unique_ptr<int32_t>{}, unique_ptr<int16_t>{}, unique_ptr<string>{}, false});
  EXPECT_CALL(mock_queries, find_tc(stname)).Times(5).WillRepeatedly(Return(&tc_recs));
  const ddl_stcname stcname{stname, "column"};
  EXPECT_CALL(mock_queries, find_ck(stcname)).Times(1);
  EXPECT_CALL(mock_queries, find_fk(stcname, true)).Times(1);
  sqlsvr_ddl_pk_rec pk_rec{};
  EXPECT_CALL(mock_queries, find_pk(stcname, true)).WillOnce(Return(&pk_rec));
  EXPECT_CALL(mock_queries, find_uq(stcname)).Times(1);

  string_backed_counter_ochain ochain;
  const ddl_stnames_migration_msgs stnames_migration_msgs{stnames_migrator.migrate_ddl_stnames(&table_migrator_factory, ochain.os())};
  EXPECT_EQ(0, stnames_migration_msgs.size());
  EXPECT_EQ("CREATE SCHEMA schema;\n\nCREATE TABLE schema.table(column DATATYPE DEFAULT default_value NOT NULL,\nPRIMARY KEY(column)\n);\n", ochain.str());
}

TEST_F(sqlsvr_ddl_stnames_migrator_impl_testfix, test5) {
  ddl_stname_usedcols stname_usedcols{};
  const ddl_stname stname{"schema", "table"};
  stname_usedcols.insert_name(stname);
  const ddl_assumed_identities assumed_idents;
  sqlsvr_ddl_stnames_migrator_impl stnames_migrator{mock_queries, stname_usedcols, true, assumed_idents, nullptr};

  vector<sqlsvr_ddl_tc_rec> tc_recs{};
  tc_recs.emplace_back(sqlsvr_ddl_tc_rec{ddl_cname{"column"}, unique_ptr<string>{new string{"default_value"}}, false, string{"datatype"}, unique_ptr<int32_t>{}, unique_ptr<int32_t>{}, unique_ptr<uint8_t>{}, unique_ptr<int16_t>{}, unique_ptr<int32_t>{}, unique_ptr<int16_t>{}, unique_ptr<string>{}, false});
  EXPECT_CALL(mock_queries, find_tc(stname)).Times(5).WillRepeatedly(Return(&tc_recs));
  const ddl_stcname stcname{stname, "column"};
  EXPECT_CALL(mock_queries, find_ck(stcname)).Times(1);
  sqlsvr_ddl_fk_rec fk_rec{"constraint", ddl_stcname{"schema", "table", "ref_column"}};
  EXPECT_CALL(mock_queries, find_fk(stcname, true)).WillOnce(Return(&fk_rec));
  sqlsvr_ddl_pk_rec pk_rec{};
  EXPECT_CALL(mock_queries, find_pk(stcname, true)).WillOnce(Return(&pk_rec));
  EXPECT_CALL(mock_queries, find_uq(stcname)).Times(1);

  string_backed_counter_ochain ochain;
  const ddl_stnames_migration_msgs stnames_migration_msgs{stnames_migrator.migrate_ddl_stnames(&table_migrator_factory, ochain.os())};
  EXPECT_EQ(0, stnames_migration_msgs.size());
  EXPECT_EQ("CREATE SCHEMA schema;\n\nCREATE TABLE schema.table(column DATATYPE DEFAULT default_value NOT NULL,\nPRIMARY KEY(column)\n);\n\nALTER TABLE schema.table ADD FOREIGN KEY(column) REFERENCES schema.table(ref_column);\n", ochain.str());
}