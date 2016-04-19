#ifndef SQLSVR_DDL_QUERIES_H
#define SQLSVR_DDL_QUERIES_H

#include <memory>
#include <string>
#include <vector>

#include <ddl_names.h>

#include "sqlsvr_ddl_ck_mapper.h"
#include "sqlsvr_ddl_fk_mapper.h"
#include "sqlsvr_ddl_pk_mapper.h"
#include "sqlsvr_ddl_tc_mapper.h"
#include "sqlsvr_ddl_uq_mapper.h"

namespace sqlsvr_ddl {

struct sqlsvr_ddl_queries {
  virtual ~sqlsvr_ddl_queries() {}

  virtual std::vector<sqlsvr_ddl_ck_rec>* find_ck(const normal_ddl::ddl_stcname& stcname) = 0;
  virtual sqlsvr_ddl_fk_rec* find_fk(const normal_ddl::ddl_stcname& stcname, bool unhidden = true) = 0;
  virtual std::vector<sqlsvr_ddl_fk_reverse_rec>* find_fk_reverse(const normal_ddl::ddl_stcname& ref_stcname) = 0;
  virtual sqlsvr_ddl_fk_rec* find_fk_constraint_name(const std::string& constraint_name, bool unhidden = true) = 0;
  virtual sqlsvr_ddl_pk_rec* find_pk(const normal_ddl::ddl_stcname& stcname, bool unhidden = true) = 0;
  virtual std::vector<sqlsvr_ddl_tc_rec>* find_tc(const normal_ddl::ddl_stname& stname) = 0;
  virtual std::vector<sqlsvr_ddl_uq_rec>* find_uq(const normal_ddl::ddl_stcname& stcname) = 0;
  virtual bool insert_uq(const normal_ddl::ddl_stcname& stcname, const std::vector<sqlsvr_ddl_uq_rec>& uq_recs) = 0;
};
}

#endif
