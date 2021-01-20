// SyslogBackend.hpp

#pragma once

#include <simple_logs/logs.hpp>

namespace logs {
class SyslogBackend final : public BasicBackend {
public:
  enum class Priority { Emerg, Alert, Crit, Err, Warning, Notice, Info, Debug };

  explicit SyslogBackend(SyslogBackend::Priority priority);
  SyslogBackend(std::string_view ident, SyslogBackend::Priority priority);

  void consume(std::string_view record) noexcept override;

private:
  std::string ident_;
  int         priority_;
};
} // namespace logs
