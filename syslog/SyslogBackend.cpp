// SyslogBackend.cpp

#include "SyslogBackend.hpp"
// XXX syslog.h must be include after our logs.hpp, because we use defines with
// same names
#include <syslog.h>

namespace logs {
int toSyslotPriority(SyslogBackend::Priority priority) {
  switch (priority) {
  case SyslogBackend::Priority::Emerg:
    return LOG_EMERG;
  case SyslogBackend::Priority::Alert:
    return LOG_ALERT;
  case SyslogBackend::Priority::Crit:
    return LOG_CRIT;
  case SyslogBackend::Priority::Err:
    return LOG_ERR;
  case SyslogBackend::Priority::Warning:
    return LOG_WARNING;
  case SyslogBackend::Priority::Notice:
    return LOG_NOTICE;
  case SyslogBackend::Priority::Info:
    return LOG_INFO;
  case SyslogBackend::Priority::Debug:
    return LOG_DEBUG;

  default:
    throw std::runtime_error{"unknown syslog priority"};
  }
}

SyslogBackend::SyslogBackend(SyslogBackend::Priority priority)
    : priority_{toSyslotPriority(priority)} {
  openlog(NULL, LOG_CONS | LOG_PID, 0);
}

SyslogBackend::SyslogBackend(std::string_view        ident,
                             SyslogBackend::Priority priority)
    : ident_{ident} // XXX string with syslog ident must be accessible any time,
                    // because syslog don't store this string
    , priority_{toSyslotPriority(priority)} {
  openlog(ident_.c_str(), LOG_CONS | LOG_PID, 0);
}

void SyslogBackend::consume(std::string_view record) noexcept {
  syslog(priority_, "%s", std::string{record}.c_str());
}
} // namespace logs
