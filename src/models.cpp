#include "models.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <istream>
#include <limits>
#include <utility>

namespace busbook {

std::ostream &operator<<(std::ostream &out, const BusDetails &details) {
  out << "Bus No: " << details.number << "\nDriver: " << details.driver
      << "\nFrom: " << details.source << "\nTo: " << details.destination
      << "\nArrival: " << details.arrival << "\nDeparture: " << details.departure
      << "\nFare/Seat: " << details.fare_per_seat;
  return out;
}

Bus::Bus(BusDetails details) : details_(std::move(details)) {}

const BusDetails &Bus::details() const noexcept { return details_; }

void Bus::updateDetails(const BusDetails &new_details) { details_ = new_details; }

bool Bus::reserveSeat(int seat_number, const std::string &passenger_name) {
  if (!isValidSeatNumber(seat_number) || passenger_name.empty()) {
    return false;
  }
  std::string &seat = seats_[static_cast<std::size_t>(seat_number - 1)];
  if (!seat.empty()) {
    return false;
  }
  seat = passenger_name;
  return true;
}

bool Bus::cancelSeat(int seat_number) {
  if (!isValidSeatNumber(seat_number)) {
    return false;
  }
  std::string &seat = seats_[static_cast<std::size_t>(seat_number - 1)];
  if (seat.empty()) {
    return false;
  }
  seat.clear();
  return true;
}

bool Bus::transferReservation(int from_seat, int to_seat) {
  if (!isValidSeatNumber(from_seat) || !isValidSeatNumber(to_seat) || from_seat == to_seat) {
    return false;
  }
  std::string &from = seats_[static_cast<std::size_t>(from_seat - 1)];
  std::string &to = seats_[static_cast<std::size_t>(to_seat - 1)];
  if (from.empty() || !to.empty()) {
    return false;
  }
  to = from;
  from.clear();
  return true;
}

bool Bus::addToWaitlist(const std::string &passenger_name) {
  if (passenger_name.empty()) {
    return false;
  }
  waitlist_.push_back(passenger_name);
  return true;
}

std::optional<std::string> Bus::popWaitlistedPassenger() {
  if (waitlist_.empty()) {
    return std::nullopt;
  }
  std::string passenger = waitlist_.front();
  waitlist_.pop_front();
  return passenger;
}

std::size_t Bus::waitlistSize() const noexcept { return waitlist_.size(); }

void Bus::printWaitlist(std::ostream &out) const {
  if (waitlist_.empty()) {
    out << "Waitlist is empty.\n";
    return;
  }
  out << "Waitlist (" << waitlist_.size() << "):\n";
  int index = 1;
  for (const auto &name : waitlist_) {
    out << "  " << index++ << ". " << name << '\n';
  }
}

bool Bus::isSeatReserved(int seat_number) const {
  return isValidSeatNumber(seat_number) &&
         !seats_[static_cast<std::size_t>(seat_number - 1)].empty();
}

std::optional<int> Bus::findPassengerSeat(const std::string &passenger_name) const {
  const std::string needle = toLowerCopy(passenger_name);
  for (int i = 0; i < kSeatCount; ++i) {
    const std::string &current = seats_[static_cast<std::size_t>(i)];
    if (!current.empty() && toLowerCopy(current) == needle) {
      return i + 1;
    }
  }
  return std::nullopt;
}

const std::string &Bus::seatPassenger(int seat_number) const {
  static const std::string kEmpty;
  if (!isValidSeatNumber(seat_number)) {
    return kEmpty;
  }
  return seats_[static_cast<std::size_t>(seat_number - 1)];
}

int Bus::emptySeatCount() const {
  // Using std::count_if with a lambda:
  // lambda syntax: [captures](params) { body }
  const auto empty_count = std::count_if(seats_.begin(), seats_.end(),
                                         [](const std::string &seat) { return seat.empty(); });
  return static_cast<int>(empty_count);
}

int Bus::reservedSeatCount() const { return kSeatCount - emptySeatCount(); }

double Bus::currentRevenue() const { return reservedSeatCount() * details_.fare_per_seat; }

double Bus::maxPossibleRevenue() const { return kSeatCount * details_.fare_per_seat; }

void Bus::printSummary(std::ostream &out) const {
  out << details_ << "\nEmpty Seats: " << emptySeatCount() << "/" << kSeatCount
      << "\nFare/Seat: " << details_.fare_per_seat
      << "\nCurrent Revenue: " << currentRevenue()
      << "\nWaitlist Size: " << waitlist_.size() << '\n';
}

void Bus::printSeatMap(std::ostream &out) const {
  printSummary(out);
  for (int row = 0; row < kRows; ++row) {
    for (int col = 0; col < kCols; ++col) {
      const int seat_number = row * kCols + col + 1;
      const std::string &passenger = seats_[static_cast<std::size_t>(seat_number - 1)];
      out << std::setw(3) << seat_number << ". " << std::left << std::setw(16)
          << (passenger.empty() ? "Empty" : passenger);
    }
    out << '\n';
  }
}

void Bus::serialize(std::ostream &out) const {
  out << std::quoted(details_.number) << ' ' << std::quoted(details_.driver) << ' '
      << std::quoted(details_.arrival) << ' ' << std::quoted(details_.departure) << ' '
      << std::quoted(details_.source) << ' ' << std::quoted(details_.destination) << ' '
      << details_.fare_per_seat << '\n';
  for (const auto &seat : seats_) {
    out << std::quoted(seat) << '\n';
  }
  out << waitlist_.size() << '\n';
  for (const auto &passenger : waitlist_) {
    out << std::quoted(passenger) << '\n';
  }
}

std::optional<Bus> Bus::deserialize(std::istream &in, bool with_fare, bool with_waitlist) {
  BusDetails details;
  if (!(in >> std::quoted(details.number) >> std::quoted(details.driver) >>
        std::quoted(details.arrival) >> std::quoted(details.departure) >>
        std::quoted(details.source) >> std::quoted(details.destination))) {
    return std::nullopt;
  }
  if (with_fare) {
    if (!(in >> details.fare_per_seat)) {
      return std::nullopt;
    }
  }

  in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  Bus bus(details);
  for (auto &seat : bus.seats_) {
    if (!(in >> std::quoted(seat))) {
      return std::nullopt;
    }
    in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  if (with_waitlist) {
    std::size_t waitlist_count = 0;
    if (!(in >> waitlist_count)) {
      return std::nullopt;
    }
    in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    for (std::size_t i = 0; i < waitlist_count; ++i) {
      std::string passenger;
      if (!(in >> std::quoted(passenger))) {
        return std::nullopt;
      }
      bus.waitlist_.push_back(passenger);
      in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
  }
  return bus;
}

bool Bus::isValidSeatNumber(int seat_number) const noexcept {
  return seat_number >= 1 && seat_number <= kSeatCount;
}

std::string Bus::toLowerCopy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

} // namespace busbook
