#include "log_window_sink.h"

namespace Frasy {
std::string (*LogWindowSink::TimestampToString)(spdlog::log_clock::time_point time) = TimestampToFullString;
}