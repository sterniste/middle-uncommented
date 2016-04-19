#ifndef SQLSVR_DDL_CONNECTION_ERROR_H
#define SQLSVR_DDL_CONNECTION_ERROR_H

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sqlsvr_ddl {

struct sqlsvr_ddl_connection_error : public std::runtime_error {
  unsigned int conn_pos;

  sqlsvr_ddl_connection_error(unsigned int conn_pos, const std::string& msg) : std::runtime_error{msg}, conn_pos{conn_pos} {}
  sqlsvr_ddl_connection_error(const sqlsvr_ddl_connection_error& that) = delete;
  sqlsvr_ddl_connection_error(sqlsvr_ddl_connection_error&& that) : std::runtime_error{std::move(that)}, conn_pos{that.conn_pos} {}
};

using sqlsvr_ddl_connection_errors = std::vector<sqlsvr_ddl_connection_error>;
}

#endif
