#ifndef ODBC_QUERY_H
#define ODBC_QUERY_H

extern "C" {
#if _WIN32
#include <windows.h>
#endif
#include <sqltypes.h>
}

#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include <boost/any.hpp>

namespace odbc_query {

class connection_error : public std::runtime_error {
 public:
  connection_error(const std::string& msg) : std::runtime_error{msg} {}
};

class query_error : public std::runtime_error {
 public:
  query_error(const std::string& msg) : std::runtime_error{msg} {}
};

class query;

class connection {
  SQLHENV henv;
  SQLHDBC hdbc;

  connection(const connection& that) = delete;
  std::string make_connect_errors_msg() const;

 public:
  ~connection();
  connection(const char* ds_name, const char* username, const char* password);

  query create_query() const;
};

class result_set;

class query {
  friend class connection;
  friend class result_set;

  enum class column_type : std::int8_t { bit, uint8, int16, int32, str };
  std::unordered_map<unsigned int, column_type> col_types;
  std::unordered_map<unsigned int, boost::any> result_cols;
  SQLHSTMT hstmt;

  std::string make_query_errors_msg() const;
  query(const query& that) = delete;
  query(SQLHDBC hdbc);

 public:
  ~query();
  query(query&& that) : result_cols{std::move(that.result_cols)}, hstmt{that.hstmt} {}

  void bind_bool_column(unsigned int col);
  void bind_uint8_column(unsigned int col);
  void bind_int16_column(unsigned int col);
  void bind_int32_column(unsigned int col);
  void bind_string_column(unsigned int col, unsigned int maxlen);
  result_set exec(const char* sql);
};

class result_set {
  friend class query;

  query& parent;

  result_set(query& parent) : parent{parent} {}
  result_set(const result_set& that) = delete;

 public:
  result_set(result_set&& that) = default;

  bool next_row() const;

  bool get_bool_column(unsigned int col, bool* value) const;
  bool get_uint8_column(unsigned int col, uint8_t* value) const;
  bool get_int16_column(unsigned int col, int16_t* value) const;
  bool get_int32_column(unsigned int col, int32_t* value) const;
  bool get_string_column(unsigned int col, std::string* value) const;
};
}

#endif
