#pragma once

#include "sink.hpp"

namespace hage {

class ConsoleSink final : public Sink
{
public:

  void receive(const LogLevel level, const timestamp_type& ts, const std::string_view line) override;
};

}
