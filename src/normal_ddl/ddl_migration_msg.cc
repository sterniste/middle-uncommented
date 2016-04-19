#include <string>

#include "ddl_migration_msg.h"

namespace normal_ddl {
using namespace std;

string
ddl_migration_msg::level_str() const {
  switch (level) {
    case ddl_migration_msg_level::error:
      return "ERROR";
    case ddl_migration_msg_level::warning:
      return "WARNING";
    case ddl_migration_msg_level::remark:
      return "REMARK";
  }
}
}
