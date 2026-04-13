#include "concepts.hpp"

#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>

namespace busbook {

ScopedTimer::ScopedTimer(std::string label)
    : label_(std::move(label)), start_time_(std::chrono::steady_clock::now()) {}

ScopedTimer::~ScopedTimer() {
  const auto end_time = std::chrono::steady_clock::now();
  const auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_).count();
  std::cout << "[Timer] " << label_ << " took " << elapsed_ms << " ms\n";
}

std::string currentTimestamp() {
  const auto now = std::chrono::system_clock::now();
  const std::time_t now_time = std::chrono::system_clock::to_time_t(now);
  std::tm local_tm{};
#if defined(_WIN32)
  localtime_s(&local_tm, &now_time);
#else
  local_tm = *std::localtime(&now_time);
#endif
  std::ostringstream output;
  output << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S");
  return output.str();
}

} // namespace busbook
