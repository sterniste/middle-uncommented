#ifndef SQLSVR_DDL_HIDABLE_H
#define SQLSVR_DDL_HIDABLE_H

namespace sqlsvr_ddl {

struct sqlsvr_ddl_hidable {
  bool hidden;

  sqlsvr_ddl_hidable() : hidden{} {}
};
}
#endif
