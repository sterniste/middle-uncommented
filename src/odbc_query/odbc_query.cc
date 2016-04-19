extern "C" {
#ifdef _WIN32
#include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>
}

#include <cassert>
#include <cstddef>
#include <string>
#include <utility>

#include <boost/any.hpp>
#include <boost/lexical_cast.hpp>

#include "odbc_query.h"

namespace odbc_query {
using namespace std;
using namespace boost;

string
connection::make_connect_errors_msg() const {
  SQLCHAR sql_state[6], msgbuf[SQL_MAX_MESSAGE_LENGTH];
  SQLINTEGER native_err;
  SQLSMALLINT msglen;

  SQLSMALLINT i = 1;
  string msg;
  SQLRETURN rc;
  while ((rc = SQLGetDiagRec(SQL_HANDLE_DBC, hdbc, i, sql_state, &native_err, msgbuf, sizeof(msgbuf), &msglen)) != SQL_NO_DATA && rc >= 0) {
    if (i > 1)
      msg += "\n";
    msg += string("[native error: ") + lexical_cast<string>(native_err) + "] " + string(reinterpret_cast<char*>(msgbuf), static_cast<size_t>(msglen));
    i++;
  }
  return msg;
}

connection::~connection() {
  if (hdbc) {
    SQLDisconnect(hdbc);
    SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
  }
  if (henv)
    SQLFreeHandle(SQL_HANDLE_ENV, henv);
}

connection::connection(const char* ds_name, const char* username, const char* password) : henv{}, hdbc{} {
  // Allocate environment handle
  SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
    throw connection_error("can't allocate env handle");
  // Set the ODBC version environment attribute
  rc = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
    throw connection_error("can't set env attribute");
  // Allocate connection handle
  rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
    throw connection_error("can't allocate connection handle");

  // Set login timeout to 5 seconds
  SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
  // Connect to data source
  rc = SQLConnect(hdbc, (SQLCHAR*)ds_name, SQL_NTS, (SQLCHAR*)username, SQL_NTS, (SQLCHAR*)password, SQL_NTS);
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
    throw connection_error(make_connect_errors_msg());
}

query
connection::create_query() const {
  return query(hdbc);
}

query::~query() {
  if (hstmt) {
    SQLCancel(hstmt);
    SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
  }
  for (const pair<const unsigned int, any>& result_col : result_cols) {
    switch (col_types[result_col.first]) {
      case column_type::bit:
      case column_type::uint8:
        delete any_cast<pair<SQLCHAR, SQLLEN>*>(result_col.second);
        break;
      case column_type::int16:
        delete any_cast<pair<SQLSMALLINT, SQLLEN>*>(result_col.second);
        break;
      case column_type::int32:
        delete any_cast<pair<SQLINTEGER, SQLLEN>*>(result_col.second);
        break;
      case column_type::str:
        auto* const result = any_cast<pair<SQLCHAR*, SQLLEN>*>(result_col.second);
        delete[] result->first;
        delete result;
        break;
    }
  }
}

string
query::make_query_errors_msg() const {
  SQLCHAR sql_state[6], msgbuf[SQL_MAX_MESSAGE_LENGTH];
  SQLINTEGER native_err;
  SQLSMALLINT msglen;

  SQLSMALLINT i = 1;
  string msg;
  SQLRETURN rc;
  while ((rc = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, i, sql_state, &native_err, msgbuf, sizeof(msgbuf), &msglen)) != SQL_NO_DATA && rc >= 0) {
    if (i > 1)
      msg += "\n";
    msg += string("[native error: ") + lexical_cast<string>(native_err) + "] " + string(reinterpret_cast<char*>(msgbuf), static_cast<size_t>(msglen));
    i++;
  }
  return msg;
}

query::query(SQLHDBC hdbc) : hstmt{} {
  // Allocate statement handle
  const SQLRETURN rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
    throw query_error("can't allocate statement handle");
}

void
query::bind_bool_column(unsigned int col) {
  assert(col_types.find(col) == col_types.end());
  auto* const result = new pair<SQLCHAR, SQLLEN>();
  result_cols[col] = any(result);
  col_types[col] = column_type::bit;
  SQLBindCol(hstmt, col, SQL_C_CHAR, &result->first, 0, &result->second);
}

void
query::bind_uint8_column(unsigned int col) {
  assert(col_types.find(col) == col_types.end());
  auto* const result = new pair<SQLCHAR, SQLLEN>();
  result_cols[col] = any(result);
  col_types[col] = column_type::uint8;
  SQLBindCol(hstmt, col, SQL_C_UTINYINT, &result->first, 0, &result->second);
}

void
query::bind_int16_column(unsigned int col) {
  assert(col_types.find(col) == col_types.end());
  auto* const result = new pair<SQLSMALLINT, SQLLEN>();
  result_cols[col] = any(result);
  col_types[col] = column_type::int16;
  SQLBindCol(hstmt, col, SQL_C_SHORT, &result->first, 0, &result->second);
}

void
query::bind_int32_column(unsigned int col) {
  assert(col_types.find(col) == col_types.end());
  auto* const result = new pair<SQLINTEGER, SQLLEN>();
  result_cols[col] = any(result);
  col_types[col] = column_type::int32;
  SQLBindCol(hstmt, col, SQL_C_LONG, &result->first, 0, &result->second);
}

void
query::bind_string_column(unsigned int col, unsigned int maxlen) {
  assert(col_types.find(col) == col_types.end());
  auto* const result = new pair<SQLCHAR*, SQLLEN>(new SQLCHAR[maxlen + 1], 0);
  result_cols[col] = any(result);
  col_types[col] = column_type::str;
  SQLBindCol(hstmt, col, SQL_C_CHAR, result->first, maxlen + 1, &result->second);
}

result_set
query::exec(const char* sql) {
  const SQLRETURN rc = SQLExecDirect(hstmt, (SQLCHAR*)sql, SQL_NTS);
  if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
    throw query_error(make_query_errors_msg());
  return result_set(*this);
}

bool
result_set::next_row() const {
  const SQLRETURN rc = SQLFetch(parent.hstmt);
  if (rc == SQL_ERROR || rc == SQL_SUCCESS_WITH_INFO)
    throw query_error(parent.make_query_errors_msg());
  return rc == SQL_SUCCESS;
}

bool
result_set::get_bool_column(unsigned int col, bool* value) const {
  const pair<SQLCHAR, SQLLEN>* result{any_cast<pair<SQLCHAR, SQLLEN>*>(parent.result_cols[col])};
  if (result->second == SQL_NULL_DATA)
    return false;
  if (value)
    *value = result->first;
  return true;
}

bool
result_set::get_uint8_column(unsigned int col, uint8_t* value) const {
  const pair<SQLCHAR, SQLLEN>* result{any_cast<pair<SQLCHAR, SQLLEN>*>(parent.result_cols[col])};
  if (result->second == SQL_NULL_DATA)
    return false;
  if (value)
    *value = result->first;
  return true;
}

bool
result_set::get_int16_column(unsigned int col, int16_t* value) const {
  const pair<SQLSMALLINT, SQLLEN>* result{any_cast<pair<SQLSMALLINT, SQLLEN>*>(parent.result_cols[col])};
  if (result->second == SQL_NULL_DATA)
    return false;
  if (value)
    *value = result->first;
  return true;
}

bool
result_set::get_int32_column(unsigned int col, int32_t* value) const {
  const pair<SQLINTEGER, SQLLEN>* result{any_cast<pair<SQLINTEGER, SQLLEN>*>(parent.result_cols[col])};
  if (result->second == SQL_NULL_DATA)
    return false;
  if (value)
    *value = result->first;
  return true;
}

bool
result_set::get_string_column(unsigned int col, string* value) const {
  const pair<SQLCHAR*, SQLLEN>* result{any_cast<pair<SQLCHAR*, SQLLEN>*>(parent.result_cols[col])};
  if (result->second == SQL_NULL_DATA)
    return false;
  if (value)
    *value = string(reinterpret_cast<const char*>(result->first), static_cast<size_t>(result->second));
  return true;
}
}
