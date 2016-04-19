#ifndef LOGGING_H
#define LOGGING_H

#include <string>

namespace middle {

int parse_log_level(const std::string& level);
void setup_log_file(const std::string& logfile, int level);
}
#endif
