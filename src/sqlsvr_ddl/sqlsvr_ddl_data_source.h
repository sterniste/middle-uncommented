#ifndef SQLSVR_DDL_DATA_SOURCE_H
#define SQLSVR_DDL_DATA_SOURCE_H

#include <string>

namespace sqlsvr_ddl {

struct sqlsvr_ddl_odbc_data_source {
  std::string ds_name;
  std::string username;
  std::string password;

  sqlsvr_ddl_odbc_data_source(const std::string& ds_name, const std::string& username, const std::string& password) : ds_name{ds_name}, username{username}, password{password} {}
};
}
#endif
