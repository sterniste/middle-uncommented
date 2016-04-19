#ifndef MOCK_SQLSVR_DDL_QUERIES_H
#define MOCK_SQLSVR_DDL_QUERIES_H

#include <string>
#include <vector>

#include <gmock/gmock.h>

#include <ddl_names.h>

#include <sqlsvr_ddl_ck_mapper.h>
#include <sqlsvr_ddl_fk_mapper.h>
#include <sqlsvr_ddl_pk_mapper.h>
#include <sqlsvr_ddl_queries.h>
#include <sqlsvr_ddl_tc_mapper.h>
#include <sqlsvr_ddl_uq_mapper.h>

struct mock_sqlsvr_ddl_queries : public sqlsvr_ddl::sqlsvr_ddl_queries {
  MOCK_METHOD1(find_ck, std::vector<sqlsvr_ddl::sqlsvr_ddl_ck_rec>*(const normal_ddl::ddl_stcname&));
  MOCK_METHOD2(find_fk, sqlsvr_ddl::sqlsvr_ddl_fk_rec*(const normal_ddl::ddl_stcname&, bool));
  MOCK_METHOD1(find_fk_reverse, std::vector<sqlsvr_ddl::sqlsvr_ddl_fk_reverse_rec>*(const normal_ddl::ddl_stcname&));
  MOCK_METHOD2(find_fk_constraint_name, sqlsvr_ddl::sqlsvr_ddl_fk_rec*(const std::string&, bool));
  MOCK_METHOD2(find_pk, sqlsvr_ddl::sqlsvr_ddl_pk_rec*(const normal_ddl::ddl_stcname&, bool));
  MOCK_METHOD1(find_tc, std::vector<sqlsvr_ddl::sqlsvr_ddl_tc_rec>*(const normal_ddl::ddl_stname&));
  MOCK_METHOD1(find_uq, std::vector<sqlsvr_ddl::sqlsvr_ddl_uq_rec>*(const normal_ddl::ddl_stcname&));
  MOCK_METHOD2(insert_uq, bool(const normal_ddl::ddl_stcname&, const std::vector<sqlsvr_ddl::sqlsvr_ddl_uq_rec>&));
};

#endif
