#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <boost/lexical_cast.hpp>

#include <uc_string.h>

#include "ddl_column_rules.h"
#include "ddl_default_value_rule_macro.h"
#include "ddl_migration_msg.h"
#include "ddl_names.h"
#include "ddl_table_migrator.h"
#include "ddl_table_migrator_prefs.h"

namespace normal_ddl {
using namespace std;
using namespace boost;
using namespace universals;

int
migrate_char_length_size(ddl_datatype& datatype, const ddl_map_size& map_size, string& datatype_comment, vector<ddl_migration_msg>& migration_msgs) {
  if (datatype.char_max_len) {
    if (*datatype.char_max_len >= map_size.range_min && *datatype.char_max_len <= map_size.range_max) {
      switch (map_size.map_size_type) {
        case ddl_map_size_type::reject_defined:
          migration_msgs.push_back(ddl_migration_msg{ddl_migration_msg_level::error, string{"rejected defined char length: "} + lexical_cast<string>(*datatype.char_max_len)});
          return -1;

        case ddl_map_size_type::suppress:
          if (!datatype_comment.empty())
            datatype_comment += "/ ";
          datatype_comment += "char length " + lexical_cast<string>(*datatype.char_max_len) + " suppressed";
          datatype.char_max_len.reset();
          datatype.char_oct_len.reset();
          return 1;

        case ddl_map_size_type::narrow:
          return 0;

        case ddl_map_size_type::assign:
          if (!datatype_comment.empty())
            datatype_comment += "/ ";
          datatype_comment += "char length " + lexical_cast<string>(*datatype.char_max_len) + " assigned " + lexical_cast<string>(map_size.new_value);
          datatype.char_max_len.reset(new int32_t{map_size.new_value});
          datatype.char_oct_len.reset();
          return 1;

        default:
          break;
      }
    } else if (map_size.map_size_type == ddl_map_size_type::narrow) {
      if (*datatype.char_max_len < map_size.range_min) {
        if (!datatype_comment.empty())
          datatype_comment += "/ ";
        datatype_comment += "char length " + lexical_cast<string>(*datatype.char_max_len) + " narrowed to " + lexical_cast<string>(map_size.range_min);
        datatype.char_max_len.reset(new int32_t{map_size.range_min});
        datatype.char_oct_len.reset();
      } else {
        if (!datatype_comment.empty())
          datatype_comment += "/ ";
        datatype_comment += "char length " + lexical_cast<string>(*datatype.char_max_len) + " narrowed to " + lexical_cast<string>(map_size.range_max);
        datatype.char_max_len.reset(new int32_t{map_size.range_max});
        datatype.char_oct_len.reset();
      }
      return 1;
    }
  } else {
    switch (map_size.map_size_type) {
      case ddl_map_size_type::reject_undefined:
        migration_msgs.push_back(ddl_migration_msg{ddl_migration_msg_level::remark, string{"rejected undefined char length"}});
        return -1;

      case ddl_map_size_type::assign_undefined:
        if (!datatype_comment.empty())
          datatype_comment += "/ ";
        datatype_comment += "undefined char length assigned " + lexical_cast<string>(map_size.new_value);
        datatype.char_max_len.reset(new int32_t{map_size.new_value});
        datatype.char_oct_len.reset();
        return 1;

      default:
        break;
    }
  }
  return 0;
}

int
migrate_numeric_precision_size(ddl_datatype& datatype, const ddl_map_size& map_size, string& datatype_comment, vector<ddl_migration_msg>& migration_msgs) {
  if (datatype.num_prec) {
    const int32_t num_prec{static_cast<int32_t>(*datatype.num_prec)};
    if (num_prec >= map_size.range_min && num_prec <= map_size.range_max) {
      switch (map_size.map_size_type) {
        case ddl_map_size_type::reject_defined:
          migration_msgs.push_back(ddl_migration_msg{ddl_migration_msg_level::remark, string{"rejected defined numeric precision: "} + lexical_cast<string>(num_prec)});
          return -1;

        case ddl_map_size_type::suppress:
          if (!datatype_comment.empty())
            datatype_comment += "/ ";
          datatype_comment += "numeric precision " + lexical_cast<string>(num_prec) + " suppressed";
          datatype.num_prec.reset();
          return 1;

        case ddl_map_size_type::narrow:
          return 0;

        case ddl_map_size_type::assign:
          if (!datatype_comment.empty())
            datatype_comment += "/ ";
          datatype_comment += "numeric precision " + lexical_cast<string>(num_prec) + " assigned " + lexical_cast<string>(map_size.new_value);
          datatype.num_prec.reset(new uint8_t{static_cast<uint8_t>(map_size.new_value)});
          return 1;

        default:
          break;
      }
    } else if (map_size.map_size_type == ddl_map_size_type::narrow) {
      const int32_t num_prec{static_cast<int32_t>(*datatype.num_prec)};
      if (num_prec < map_size.range_min) {
        if (!datatype_comment.empty())
          datatype_comment += "/ ";
        datatype_comment += "numeric precision " + lexical_cast<string>(num_prec) + " narrowed to " + lexical_cast<string>(map_size.range_min);
        datatype.num_prec.reset(new uint8_t{static_cast<uint8_t>(map_size.range_min)});
      } else {
        if (!datatype_comment.empty())
          datatype_comment += "/ ";
        datatype_comment += "numeric precision " + lexical_cast<string>(num_prec) + " narrowed to " + lexical_cast<string>(map_size.range_max);
        datatype.num_prec.reset(new uint8_t{static_cast<uint8_t>(map_size.range_max)});
      }
      return 1;
    }
  } else {
    switch (map_size.map_size_type) {
      case ddl_map_size_type::reject_undefined:
        migration_msgs.push_back(ddl_migration_msg{ddl_migration_msg_level::remark, string{"rejected undefined numeric precision"}});
        return -1;

      case ddl_map_size_type::assign_undefined:
        if (!datatype_comment.empty())
          datatype_comment += "/ ";
        datatype_comment += "undefined numeric precision assigned " + lexical_cast<string>(map_size.new_value);
        datatype.num_prec.reset(new uint8_t{static_cast<uint8_t>(map_size.new_value)});
        return 1;

      default:
        break;
    }
  }
  return 0;
}

int
migrate_numeric_scale_size(ddl_datatype& datatype, const ddl_map_size& map_size, string& datatype_comment, vector<ddl_migration_msg>& migration_msgs) {
  if (datatype.num_scale) {
    if (*datatype.num_scale >= map_size.range_min && *datatype.num_scale <= map_size.range_max) {
      switch (map_size.map_size_type) {
        case ddl_map_size_type::scale_reject_defined:
          migration_msgs.push_back(ddl_migration_msg{ddl_migration_msg_level::remark, string{"rejected defined numeric scale: "} + lexical_cast<string>(*datatype.num_scale)});
          return -1;

        case ddl_map_size_type::scale_suppress:
          if (!datatype_comment.empty())
            datatype_comment += "/ ";
          datatype_comment += "numeric scale " + lexical_cast<string>(*datatype.num_scale) + " suppressed";
          datatype.num_scale.reset();
          return 1;

        case ddl_map_size_type::scale_narrow:
          return 0;

        case ddl_map_size_type::scale_assign:
          if (!datatype_comment.empty())
            datatype_comment += "/ ";
          datatype_comment += "numeric scale " + lexical_cast<string>(*datatype.num_scale) + " assigned " + lexical_cast<string>(map_size.new_value);
          datatype.num_scale.reset(new int32_t{map_size.new_value});
          return 1;

        default:
          break;
      }
    } else if (map_size.map_size_type == ddl_map_size_type::scale_narrow) {
      if (*datatype.num_scale < map_size.range_min) {
        if (!datatype_comment.empty())
          datatype_comment += "/ ";
        datatype_comment += "numeric scale " + lexical_cast<string>(*datatype.num_scale) + " narrowed to " + lexical_cast<string>(map_size.range_min);
        datatype.num_scale.reset(new int32_t{map_size.range_min});
      } else {
        if (!datatype_comment.empty())
          datatype_comment += "/ ";
        datatype_comment += "numeric scale " + lexical_cast<string>(*datatype.num_scale) + " narrowed to " + lexical_cast<string>(map_size.range_max);
        datatype.num_scale.reset(new int32_t{map_size.range_max});
      }
      return 1;
    }
  } else {
    switch (map_size.map_size_type) {
      case ddl_map_size_type::scale_reject_undefined:
        migration_msgs.push_back(ddl_migration_msg{ddl_migration_msg_level::remark, string{"rejected undefined numeric scale"}});
        return -1;

      case ddl_map_size_type::scale_assign_undefined:
        if (!datatype_comment.empty())
          datatype_comment += "/ ";
        datatype_comment += "undefined numeric scale assigned " + lexical_cast<string>(map_size.new_value);
        datatype.num_scale.reset(new int32_t{map_size.new_value});
        return 1;

      default:
        break;
    }
  }
  return 0;
}

int
migrate_dt_precision_size(ddl_datatype& datatype, const ddl_map_size& map_size, string& datatype_comment, vector<ddl_migration_msg>& migration_msgs) {
  if (datatype.dt_prec) {
    const int32_t dt_prec{static_cast<int32_t>(*datatype.dt_prec)};
    if (dt_prec >= map_size.range_min && dt_prec <= map_size.range_max) {
      switch (map_size.map_size_type) {
        case ddl_map_size_type::reject_defined:
          migration_msgs.push_back(ddl_migration_msg{ddl_migration_msg_level::remark, string{"rejected defined datetime precision: "} + lexical_cast<string>(dt_prec)});
          return -1;

        case ddl_map_size_type::suppress:
          if (!datatype_comment.empty())
            datatype_comment += "/ ";
          datatype_comment += "datetime precision " + lexical_cast<string>(dt_prec) + " suppressed";
          datatype.dt_prec.reset();
          return 1;

        case ddl_map_size_type::narrow:
          return 0;

        case ddl_map_size_type::assign:
          if (!datatype_comment.empty())
            datatype_comment += "/ ";
          datatype_comment += "datetime precision " + lexical_cast<string>(dt_prec) + " assigned " + lexical_cast<string>(map_size.new_value);
          datatype.dt_prec.reset(new int16_t{static_cast<int16_t>(map_size.new_value)});
          return 1;

        default:
          break;
      }
    } else if (map_size.map_size_type == ddl_map_size_type::narrow) {
      const int32_t dt_prec{static_cast<int32_t>(*datatype.dt_prec)};
      if (dt_prec < map_size.range_min) {
        if (!datatype_comment.empty())
          datatype_comment += "/ ";
        datatype_comment += "datetime precision " + lexical_cast<string>(dt_prec) + " narrowed to " + lexical_cast<string>(map_size.range_min);
        datatype.dt_prec.reset(new int16_t{static_cast<int16_t>(map_size.range_min)});
      } else {
        if (!datatype_comment.empty())
          datatype_comment += "/ ";
        datatype_comment += "datetime precision " + lexical_cast<string>(dt_prec) + " narrowed to " + lexical_cast<string>(map_size.range_max);
        datatype.dt_prec.reset(new int16_t{static_cast<int16_t>(map_size.range_max)});
      }
      return 1;
    }
  } else {
    switch (map_size.map_size_type) {
      case ddl_map_size_type::reject_undefined:
        migration_msgs.push_back(ddl_migration_msg{ddl_migration_msg_level::remark, string{"rejected undefined datetime precision"}});
        return -1;

      case ddl_map_size_type::assign_undefined:
        if (!datatype_comment.empty())
          datatype_comment += "/ ";
        datatype_comment += "undefined datetime precision assigned " + lexical_cast<string>(map_size.new_value);
        datatype.dt_prec.reset(new int16_t{static_cast<int16_t>(map_size.new_value)});
        return 1;

      default:
        break;
    }
  }
  return 0;
}

string
ddl_table_migrator::expand_default_value_macro(const string& old_default_value) {
  string default_value{old_default_value};
  while (default_value.size() > 2 && *default_value.cbegin() == '(' && *default_value.crbegin() == ')')
    default_value = default_value.substr(1, default_value.size() - 2);
  if (default_value.empty())
    return "";
  ddl_default_value_rule_macro_expander macro_expander{table_migrator_prefs.default_value_rules};
  const unique_ptr<const string> new_default_value{macro_expander.expand_all_macros(default_value)};
  return !new_default_value ? old_default_value : string{*new_default_value};
}

ddl_datatype_migration
ddl_table_migrator::migrate_datatype_ddl(const ddl_cname& cname, ddl_datatype& datatype, bool nullable, const string& default_value, bool identity) {
  vector<ddl_migration_msg> migration_msgs;
  string datatype_comment;
  bool column_rejected = false;

  const auto it = table_migrator_prefs.datatype_rules.find(datatype.name);
  if (it != table_migrator_prefs.datatype_rules.end()) {
    if (!it->second.map_name.empty()) {
      const string datatype_name{to_upper(it->second.map_name)};
      datatype_comment = "data type " + datatype.name.str() + " replaced by " + datatype_name;
      datatype.name = datatype_name;
    }

    switch (it->second.datatype_rule_type) {
      case ddl_datatype_rule_type::rejected:
        datatype_comment = "data type " + datatype.name.str() + " rejected";
        column_rejected = true;
        break;

      case ddl_datatype_rule_type::unsized:
        assert(!datatype.char_max_len && !datatype.char_oct_len);
        assert(!datatype.num_prec && !datatype.num_prec_rad && !datatype.num_scale);
        assert(!datatype.dt_prec);
        break;

      case ddl_datatype_rule_type::char_length:
        // TODO char_oct_len, charset_name unused
        assert(!datatype.num_prec && !datatype.num_prec_rad && !datatype.num_scale);
        assert(!datatype.dt_prec);
        for (const auto& map_size : it->second.map_sizes) {
          const auto ret = migrate_char_length_size(datatype, map_size, datatype_comment, migration_msgs);
          if (ret != 0) {
            if (ret < 0)
              column_rejected = true;
            break;
          }
        }
        break;

      case ddl_datatype_rule_type::precision_scale:
        // TODO num_prec_rad unused
        assert(!datatype.char_max_len && !datatype.char_oct_len);
        assert(!datatype.dt_prec);
        {
          auto cit = it->second.map_sizes.cbegin();
          const auto cend = it->second.map_sizes.cend();
          while (cit != cend && !scale_map_size_type(cit->map_size_type)) {
            const auto ret = migrate_numeric_precision_size(datatype, *cit, datatype_comment, migration_msgs);
            ++cit;
            if (ret != 0) {
              if (ret < 0)
                column_rejected = true;
              break;
            }
          }
          if (column_rejected && cit != cend) {
            do {
              const bool ok = scale_map_size_type(cit->map_size_type);
              assert(ok);
              const auto ret = migrate_numeric_scale_size(datatype, *cit, datatype_comment, migration_msgs);
              ++cit;
              if (ret != 0) {
                if (ret < 0)
                  column_rejected = true;
                break;
              }
            } while (cit != cend);
          }
        }
        break;

      case ddl_datatype_rule_type::dt_precision:
        assert(!datatype.char_max_len && !datatype.char_oct_len);
        assert(!datatype.num_prec && !datatype.num_prec_rad && !datatype.num_scale);
        for (const auto& map_size : it->second.map_sizes) {
          const auto ret = migrate_dt_precision_size(datatype, map_size, datatype_comment, migration_msgs);
          if (ret != 0) {
            if (ret < 0)
              column_rejected = true;
            break;
          }
        }
        break;
    }
  } else {
    datatype_comment = "data type " + datatype.name.str() + " rejected";
    column_rejected = true;
  }

  string new_default_value;
  if (!column_rejected && !default_value.empty()) {
    new_default_value = expand_default_value_macro(default_value);
    if (!new_default_value.empty() && new_default_value != default_value) {
      if (!datatype_comment.empty())
        datatype_comment += "/ ";
      datatype_comment += string{"default value rule macro expanded"};
    }
  } else
    new_default_value = default_value;

  if (!column_rejected && !datatype_comment.empty())
    migration_msgs.push_back(ddl_migration_msg{ddl_migration_msg_level::remark, datatype_comment});  // TODO

  return ddl_datatype_migration{column_rejected, std::move(new_default_value), string{}, std::move(datatype_comment), std::move(migration_msgs)};
}
}
