#include <stdexcept>
#include <string>

#include "sqlsvr_ddl_data_source.h"

namespace sqlsvr_ddl {
using namespace std;

sqlsvr_ddl_odbc_data_source
sqlsvr_ddl_odbc_data_source::parse(const string& odbc_data_source_spec) {
  const auto pos1 = odbc_data_source_spec.find(':');
  if (pos1 == 0 || pos1 == string::npos)
    throw invalid_argument{string{"missing/empty DSN in ODBC datasource '"} + odbc_data_source_spec + '\''};
  const auto pos2 = odbc_data_source_spec.find(':', pos1 + 1);
  if (pos2 == pos1 + 1 || pos2 == string::npos)
    throw invalid_argument{string{"missing/empty username in ODBC datasource '"} + odbc_data_source_spec + '\''};
  return sqlsvr_ddl_odbc_data_source{odbc_data_source_spec.substr(0, pos1), odbc_data_source_spec.substr(pos1 + 1, pos2 - (pos1 + 1)), odbc_data_source_spec.substr(pos2 + 1)};
}
}
