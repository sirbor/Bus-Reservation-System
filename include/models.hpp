#ifndef BUSBOOK_MODELS_HPP
#define BUSBOOK_MODELS_HPP

#include <array>
#include <deque>
#include <istream>
#include <optional>
#include <ostream>
#include <string>

namespace busbook {

// Compile-time constants: these values are known before runtime.
// They demonstrate:
// 1) constexpr
// 2) strongly scoped constants in a namespace
inline constexpr int kRows = 8;
inline constexpr int kCols = 4;
inline constexpr int kSeatCount = kRows * kCols;

// A struct is a class where members are public by default.
// We use it as a lightweight "data bundle" for bus metadata.
struct BusDetails {
  std::string number;
  std::string driver;
  std::string arrival;
  std::string departure;
  std::string source;
  std::string destination;
  double fare_per_seat = 0.0;
};

// Operator overloading example:
// lets us print BusDetails with `std::cout << details;`
std::ostream &operator<<(std::ostream &out, const BusDetails &details);

// This class models one bus and its seat reservations.
// It demonstrates:
// - encapsulation (private data + public methods)
// - constructors
// - const correctness
// - optional for "value may or may not exist"
// - std::array for fixed-size storage
class Bus final {
public:
  Bus() = default;
  explicit Bus(BusDetails details);

  const BusDetails &details() const noexcept;
  void updateDetails(const BusDetails &new_details);

  bool reserveSeat(int seat_number, const std::string &passenger_name);
  bool cancelSeat(int seat_number);
  bool transferReservation(int from_seat, int to_seat);
  bool addToWaitlist(const std::string &passenger_name);
  std::optional<std::string> popWaitlistedPassenger();
  std::size_t waitlistSize() const noexcept;
  void printWaitlist(std::ostream &out) const;

  bool isSeatReserved(int seat_number) const;
  std::optional<int> findPassengerSeat(const std::string &passenger_name) const;
  const std::string &seatPassenger(int seat_number) const;

  int emptySeatCount() const;
  int reservedSeatCount() const;
  double currentRevenue() const;
  double maxPossibleRevenue() const;

  void printSummary(std::ostream &out) const;
  void printSeatMap(std::ostream &out) const;

  void serialize(std::ostream &out) const;
  static std::optional<Bus> deserialize(std::istream &in, bool with_fare, bool with_waitlist);

private:
  bool isValidSeatNumber(int seat_number) const noexcept;
  static std::string toLowerCopy(std::string value);

  BusDetails details_{};
  std::array<std::string, kSeatCount> seats_{};
  std::deque<std::string> waitlist_{};
};

} // namespace busbook

#endif // BUSBOOK_MODELS_HPP
