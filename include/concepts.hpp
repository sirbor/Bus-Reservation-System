#ifndef BUSBOOK_CONCEPTS_HPP
#define BUSBOOK_CONCEPTS_HPP

#include <chrono>
#include <iostream>
#include <string>

namespace busbook {

// Strongly typed enum class:
// unlike old enum, values do not leak into surrounding scope.
enum class OperationStatus {
  kSuccess,
  kNotFound,
  kInvalidInput,
  kConflict,
  kFailure
};

// A small result object that pairs status + human-readable message.
// This is a common pattern in large C++ systems where "bool" is too limited.
struct OperationResult {
  OperationStatus status = OperationStatus::kFailure;
  std::string message;

  // Conversion operator: lets us write `if (result) { ... }`.
  explicit operator bool() const { return status == OperationStatus::kSuccess; }
};

// Function template with generic type T.
// Works for int/double/etc as long as comparisons are supported.
template <typename T>
T clampValue(T value, T min_value, T max_value) {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return value;
}

// RAII class:
// constructor starts timing, destructor prints duration.
// This is useful for profiling and demonstrates deterministic cleanup in C++.
class ScopedTimer final {
public:
  explicit ScopedTimer(std::string label);
  ~ScopedTimer();

  ScopedTimer(const ScopedTimer &) = delete;
  ScopedTimer &operator=(const ScopedTimer &) = delete;

private:
  std::string label_;
  std::chrono::steady_clock::time_point start_time_;
};

std::string currentTimestamp();

} // namespace busbook

#endif // BUSBOOK_CONCEPTS_HPP
