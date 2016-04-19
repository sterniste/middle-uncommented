#ifdef LEVEL_LOGGING
#include <iomanip>
#include <string>

#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/log/core/core.hpp>
#include <boost/log/expressions/message.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/keywords/file_name.hpp>
#include <boost/log/keywords/format.hpp>
#include <boost/log/keywords/rotation_size.hpp>
#include <boost/log/keywords/start_thread.hpp>
#include <boost/log/keywords/time_based_rotation.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/phoenix/operator.hpp>
#endif

namespace middle {
#ifdef LEVEL_LOGGING
using namespace std;
using namespace boost::log;
namespace log_expr = boost::log::expressions;
namespace log_keywords = boost::log::keywords;
namespace log_sinks = boost::log::sinks;
namespace log_trivial = boost::log::trivial;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "Timestamp", boost::posix_time::ptime)

int
parse_log_level(const string& level) {
  if (level == "fatal")
    return log_trivial::fatal;
  if (level == "error")
    return log_trivial::error;
  if (level == "warn")
    return log_trivial::warning;
  if (level == "info")
    return log_trivial::info;
  if (level == "debug")
    return log_trivial::debug;
  if (level == "trace")
    return log_trivial::trace;
  return -1;
}

void
setup_log_file(const string& logfile, int level) {
  add_common_attributes();
  if (!logfile.empty()) {
    add_file_log(log_keywords::file_name = logfile.c_str(), log_keywords::rotation_size = 10 * 1024 * 1024, log_keywords::time_based_rotation = log_sinks::file::rotation_at_time_point(0, 0, 0), log_keywords::format = (log_expr::stream << log_expr::format_date_time(timestamp, "%Y-%m-%d %H:%M:%S.%f") << ": [" << setw(7) << log_trivial::severity << "] " << log_expr::smessage));
    core::get()->set_filter(log_trivial::severity >= level);
  } else
    core::get()->set_logging_enabled(false);
}
#endif
}
