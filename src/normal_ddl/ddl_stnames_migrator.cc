#include <cassert>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <boost/lexical_cast.hpp>
#ifdef LEVEL_LOGGING
#include <boost/log/trivial.hpp>
#endif

#include <ochain.h>

#include "ddl_migration_msg.h"
#include "ddl_names.h"
#include "ddl_stnames_migrator.h"

namespace normal_ddl {
using namespace std;
using namespace boost;
using namespace boost::iostreams;
using namespace universals;

bool ddl_stnames_migration_msg::operator==(const ddl_stnames_migration_msg& that) const {
  if (lineno != that.lineno)
    return false;
  if (stname != that.stname)
    return false;
  if (cname != that.cname)
    return false;
  if (msg.level != that.msg.level)
    return false;
  if (msg.msg != that.msg.msg)
    return false;
  return true;
}

bool ddl_stnames_migration_msg::operator<(const ddl_stnames_migration_msg& that) const {
  if (msg < that.msg)
    return true;
  if (msg == that.msg) {
    if (lineno < that.lineno)
      return true;
    if (lineno == that.lineno) {
      if (stname < that.stname)
        return true;
      if (stname == that.stname) {
        if (cname < that.cname)
          return true;
        if (cname == that.cname) {
          if (msg.msg.find(that.msg.msg) == 0 && msg.msg.find(batch_source_file_msg_suffix, that.msg.msg.length()) == that.msg.msg.length())
            return true;
          if (that.msg.msg.find(msg.msg) == 0 && that.msg.msg.find(batch_source_file_msg_suffix, msg.msg.length()) == msg.msg.length())
            return false;
          if (that.msg.msg.find(batch_source_file_msg_suffix) == string::npos && msg.msg.find(batch_source_file_msg_suffix) != string::npos)
            return true;
          return msg.msg < that.msg.msg;
        }
      }
    }
  }
  return false;
}

string
ddl_stnames_migration_msg::str(unsigned int lineno_offset) const {
  string str{msg.level_str()};
  for (auto i = str.length(); i < 8; ++i)
    str += ' ';
  if (lineno)
    str += string{"[line "} + lexical_cast<string>(lineno_offset + lineno) + "]: ";
  if (!stname.empty()) {
    str += stname.str();
    if (!cname.empty())
      str += '.' + cname.str();
    str += ": ";
  }
  str += msg.msg;
  return str;
}

void
ddl_stname_migrator_base::migrate_create_schema_ddl(const char* schema_name, ostream& os) {
  os << "CREATE SCHEMA " << schema_name << ';' << endl;
}

bool
ddl_stname_migrator_base::migrate_char_data_len_ddl(const ddl_datatype& datatype, ostream& os) {
  // TODO: charset_name ignored
  if (datatype.char_max_len) {
    os << '(';
    os << *datatype.char_max_len;
    // TODO: char_oct_len ignored
    os << ')';
    return true;
  } else
    assert(!datatype.char_oct_len);
  return false;
}

bool
ddl_stname_migrator_base::migrate_num_data_prec_ddl(const ddl_datatype& datatype, ostream& os) {
  // TODO: num_prec_rad ignored
  if (datatype.num_prec) {
    os << '(';
    os << static_cast<int>(*datatype.num_prec);
    if (datatype.num_scale && *datatype.num_scale)
      os << ',' << *datatype.num_scale;
    os << ')';
    return true;
  } else
    assert(!datatype.num_scale || !*datatype.num_scale);
  return false;
}

void
ddl_stname_migrator_base::migrate_datatype_ddl(const ddl_datatype& datatype, ostream& os) {
  os << datatype.name.str();
  const bool char_data_len = migrate_char_data_len_ddl(datatype, os);
  const bool num_data_prec = migrate_num_data_prec_ddl(datatype, os);
  assert((!char_data_len && !num_data_prec) || (char_data_len ^ num_data_prec));
}

ddl_datatype_migration
ddl_stname_migrator_base::migrate_table_column_ddl(const ddl_column& column, ddl_table_migrator& app_table_migrator, bool assumed_identity, ostream& os) {
  os << column.cname.str() << ' ';
  ddl_datatype datatype{column.datatype};
  const string default_value{column.default_value ? *column.default_value : ""};
  ddl_datatype_migration datatype_migration{app_table_migrator.migrate_datatype_ddl(column.cname, datatype, column.nullable, default_value, column.identity || assumed_identity)};
  if (!datatype_migration.column_rejected) {
    migrate_datatype_ddl(datatype, os);
    if (!datatype_migration.datatype_comment.empty())
      os << " /* " << datatype_migration.datatype_comment << " */";
    if (!datatype_migration.default_value.empty())
      os << " DEFAULT " << datatype_migration.default_value;
    if (!column.nullable)
      os << " NOT NULL";
    if (!datatype_migration.identity_clause.empty()) {
      os << ' ' << datatype_migration.identity_clause;
      if (assumed_identity) {
        if (column.identity)
          datatype_migration.msgs.push_back(ddl_migration_msg{ddl_migration_msg_level::warning, "assumed identity column: '" + column.cname.str() + "' was already IDENTITY"});
        else
          datatype_migration.msgs.push_back(ddl_migration_msg{ddl_migration_msg_level::remark, "assumed identity column: '" + column.cname.str() + '\''});
      }
    }
  }
  return datatype_migration;
}

string::size_type
next_line(const string& lines, string::size_type next_line_pos, string& line) {
  const string::size_type line_pos{lines.find('\n', next_line_pos)};
  if (line_pos == string::npos)
    return string::npos;
  line = lines.substr(next_line_pos, line_pos - next_line_pos);
  return line_pos + 1;
}

ddl_create_table_migration
ddl_stname_migrator_base::migrate_create_table_ddl(normal_ddl::ddl_table_migrator_factory& app_table_migrator_factory, const ddl_stname& stname, const ddl_table_info& table_info, const ddl_assumed_identities& assumed_idents, ddl_stnames_migration_msgs& stnames_migration_msgs, counter_ochain& coc) {
  vector<ddl_cname> rejected_cnames;
  string_backed_counter_ochain table_sbcoc(coc.counter());
  table_sbcoc.os() << endl << "CREATE TABLE " << stname.str() << '(';
  const unique_ptr<ddl_table_migrator> app_table_migrator{app_table_migrator_factory.make_table_migrator(stname, table_info.pk_cnames, verbose_os)};
  assert(app_table_migrator);
  bool unrejected_column = false, prev = false;
  for (const auto& column : table_info.columns) {
    if (prev)
      table_sbcoc.os() << ',' << endl;
    const ddl_stcname stcname{stname, column.cname.str()};
    const bool assumed_identity{assumed_idents.find(stcname) != assumed_idents.end()};
    const unsigned int datatype_lineno{static_cast<unsigned int>(1 + table_sbcoc.counter().lines())};
    string_backed_counter_ochain column_sbcoc;
    ddl_datatype_migration datatype_migration{migrate_table_column_ddl(column, *app_table_migrator, assumed_identity, column_sbcoc.os())};
    if (datatype_migration.column_rejected) {
#ifdef LEVEL_LOGGING
      BOOST_LOG_TRIVIAL(debug) << "migrate create table: column " << stcname.str() << " rejected";
#endif
      if (verbose_os)
        *verbose_os << "column " << stcname.str() << " rejected" << endl;
      rejected_cnames.push_back(column.cname);
      table_sbcoc.os() << "/* column " << ddl_stcname{stname, column.cname.str()}.str() << ": data type " << column.datatype.name.str() << " rejected! */" << endl;
      prev = false;
    } else {
      column_sbcoc.merge_into(table_sbcoc.os());
      unrejected_column = true;
      prev = true;
    }
    for (auto&& msg : datatype_migration.msgs)
      stnames_migration_msgs.push_back(ddl_stnames_migration_msg{datatype_lineno, stname, column.cname, std::move(msg)});
  }
  const vector<ddl_cname> pk_columns{app_table_migrator->filter_pk_columns()};
  if (!pk_columns.empty()) {
    if (prev)
      table_sbcoc.os() << ',' << endl;
    table_sbcoc.os() << "PRIMARY KEY(";
    bool first = true;
    for (const auto& pk_column : pk_columns) {
      if (!first)
        table_sbcoc.os() << ',';
      table_sbcoc.os() << pk_column.str();
      first = false;
    }
    table_sbcoc.os() << ')' << endl;
  }
  table_sbcoc.os() << ");" << endl;
  if (unrejected_column)
    table_sbcoc.merge_into(coc.os());
  else {
    string line;
    string::size_type line_pos = 0;
    while ((line_pos = next_line(table_sbcoc.str(), line_pos, line)) != string::npos) {
      if (!line.empty())
        coc.os() << "-- " << line;
      coc.os() << endl;
    }
    if (verbose_os)
      *verbose_os << "table " << stname.str() << " emptied" << endl;
    coc.os() << "-- table " << stname.str() << " emptied!" << endl;
  }
  return ddl_create_table_migration{std::move(rejected_cnames), app_table_migrator->uniquify_identity_columns()};
}

void
ddl_stname_migrator_base::migrate_ck_constraint_ddl(const ddl_stcname& stcname, const string& constraint_name, const string& check_clause, ostream& os) {
  os << "ALTER TABLE " << stcname.stname().str() << " ADD CONSTRAINT " << constraint_name << " CHECK(" << make_quotable_ddl(check_clause) << ");" << endl;
}

void
ddl_stname_migrator_base::migrate_uq_constraint_ddl(const ddl_stname& stname, const vector<ddl_cname>& cnames, const string& constraint_name, ostream& os) {
  os << "ALTER TABLE " << stname.str() << " ADD CONSTRAINT " << constraint_name << " UNIQUE(";
  bool first = true;
  for (const auto& cname : cnames) {
    if (!first)
      os << ',';
    os << cname.str();
    first = false;
  }
  os << ");" << endl;
}

void
ddl_stname_migrator_base::migrate_fk_constraint_ddl(const ddl_stcname& stcname, const ddl_stcname& ref_stcname, ostream& os) {
  os << "ALTER TABLE " << stcname.stname().str() << " ADD FOREIGN KEY(" << stcname.column_name() << ") REFERENCES " << ref_stcname.stname().str() << '(' << ref_stcname.column_name() << ");" << endl;
}
}
