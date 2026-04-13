#include "system.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>
#include <regex>
#include <stdexcept>
#include <type_traits>
#include <tuple>
#include <set>
#include <utility>

namespace busbook {

ReservationSystem::ReservationSystem(std::unique_ptr<IDataStore> data_store)
    : data_store_(std::move(data_store)) {}

OperationResult ReservationSystem::addBusCommand(const BusDetails &details) {
  if (trimCopy(details.number).empty()) {
    return {OperationStatus::kInvalidInput, "Bus number cannot be empty."};
  }
  if (!isValidBusNumberFormat(details.number)) {
    return {OperationStatus::kInvalidInput, "Invalid bus number format."};
  }
  if (findBusByNumber(details.number) != nullptr) {
    return {OperationStatus::kConflict, "Bus already exists."};
  }
  buses_.emplace_back(details);
  undo_stack_.push_back(UndoAddBus{details.number});
  (void)addAuditEvent("Added bus " + details.number + " on route " + details.source + " -> " +
                      details.destination + ".");
  return {OperationStatus::kSuccess, "Bus added successfully."};
}

OperationResult ReservationSystem::reserveSeatCommand(const std::string &bus_number, int seat,
                                                      const std::string &passenger) {
  Bus *bus = findBusByNumber(bus_number);
  if (bus == nullptr) {
    return {OperationStatus::kNotFound, "Bus not found."};
  }
  if (!bus->reserveSeat(seat, passenger)) {
    return {OperationStatus::kConflict, "Unable to reserve seat."};
  }
  undo_stack_.push_back(UndoReserve{bus_number, seat});
  (void)addAuditEvent("Reserved seat " + std::to_string(seat) + " on bus " + bus_number +
                      " for passenger " + passenger + ".");
  return {OperationStatus::kSuccess, "Seat reserved successfully."};
}

OperationResult ReservationSystem::cancelSeatCommand(const std::string &bus_number, int seat) {
  Bus *bus = findBusByNumber(bus_number);
  if (bus == nullptr) {
    return {OperationStatus::kNotFound, "Bus not found."};
  }
  const std::string passenger_before = bus->seatPassenger(seat);
  if (!bus->cancelSeat(seat)) {
    return {OperationStatus::kConflict, "Unable to cancel seat."};
  }
  undo_stack_.push_back(UndoCancel{bus_number, seat, passenger_before});
  (void)addAuditEvent("Cancelled reservation for seat " + std::to_string(seat) + " on bus " +
                      bus_number + ".");
  const auto promoted = bus->popWaitlistedPassenger();
  if (promoted.has_value()) {
    (void)bus->reserveSeat(seat, *promoted);
  }
  return {OperationStatus::kSuccess, "Reservation cancelled successfully."};
}

OperationResult ReservationSystem::transferSeatCommand(const std::string &bus_number, int from_seat,
                                                       int to_seat) {
  Bus *bus = findBusByNumber(bus_number);
  if (bus == nullptr) {
    return {OperationStatus::kNotFound, "Bus not found."};
  }
  if (!bus->transferReservation(from_seat, to_seat)) {
    return {OperationStatus::kConflict, "Unable to transfer reservation."};
  }
  undo_stack_.push_back(UndoTransfer{bus_number, from_seat, to_seat});
  (void)addAuditEvent("Transferred reservation on bus " + bus_number + " from seat " +
                      std::to_string(from_seat) + " to seat " + std::to_string(to_seat) + ".");
  return {OperationStatus::kSuccess, "Reservation transferred successfully."};
}

OperationResult ReservationSystem::deleteBusCommand(const std::string &bus_number) {
  for (auto it = buses_.begin(); it != buses_.end(); ++it) {
    if (it->details().number == bus_number) {
      undo_stack_.push_back(UndoDeleteBus{*it});
      buses_.erase(it);
      (void)addAuditEvent("Deleted bus " + bus_number + ".");
      return {OperationStatus::kSuccess, "Bus deleted successfully."};
    }
  }
  return {OperationStatus::kNotFound, "Bus not found."};
}

OperationResult ReservationSystem::loadDataCommand() { return loadFromFile(); }
OperationResult ReservationSystem::saveDataCommand() const { return saveToFile(); }
std::size_t ReservationSystem::busCount() const noexcept { return buses_.size(); }

void ReservationSystem::run() {
  const auto load_result = loadFromFile();
  if (!load_result) {
    std::cout << "Startup note: " << load_result.message << '\n';
  }

  // Dynamic menu registry with lambdas.
  menu_options_ = std::make_unique<std::vector<MenuOption>>(std::vector<MenuOption>{
      {1, "Add new bus details", [this] { addBus(); }},
      {2, "Reserve a seat", [this] { reserveSeat(); }},
      {3, "Show seats in a bus", [this] { showBusSeats(); }},
      {4, "List all buses", [this] { listBuses(); }},
      {5, "Cancel a reservation", [this] { cancelReservation(); }},
      {6, "Search buses by route", [this] { searchByRoute(); }},
      {7, "Find passenger by name", [this] { findPassenger(); }},
      {8, "Save data now", [this] { saveDataNow(); }},
      {9, "Edit bus details", [this] { editBusDetails(); }},
      {10, "Delete a bus", [this] { deleteBus(); }},
      {11, "Transfer reservation to another seat", [this] { transferReservation(); }},
      {12, "Occupancy analytics", [this] { showOccupancyReport(); }},
      {13, "Fare and revenue report", [this] { showFareAndRevenueReport(); }},
      {14, "View audit log", [this] { showAuditLog(); }},
      {15, "Clear audit log", [this] { clearAuditLog(); }},
      {16, "Add passenger to bus waitlist", [this] { addPassengerToWaitlist(); }},
      {17, "Show waitlist for a bus", [this] { showBusWaitlist(); }},
      {18, "Undo last action", [this] { undoLastAction(); }},
      {19, "Show route catalog", [this] { showRouteCatalog(); }},
      {20, "Exit", [this] { exitAndSave(); }},
  });

  showWelcome();
  showLearningMap();

  while (is_running_) {
    showMenu();
    const int choice = readInt("Enter your choice: ", 1, 20);
    const auto it = std::find_if(menu_options_->begin(), menu_options_->end(),
                                 [choice](const MenuOption &option) {
                                   return option.number == choice;
                                 });
    if (it != menu_options_->end()) {
      // Timer object is created here and destroyed when this block ends.
      // The destructor prints elapsed time (RAII in action).
      ScopedTimer timer("Menu action " + std::to_string(choice));
      printLine('*');
      it->action();
    }
  }
}

void ReservationSystem::showWelcome() const {
  printLine('-');
  std::cout << "      BusBook Learning Project (C++ Basics to Advanced)\n";
  printLine('-');
}

void ReservationSystem::showLearningMap() const {
  std::cout << "Concepts used inside this project:\n"
            << " - Basics: variables, loops, branches, validation\n"
            << " - OOP: structs/classes, encapsulation, const correctness\n"
            << " - STL: vector, array, optional, algorithms, function\n"
            << " - Generic programming: function templates\n"
            << " - Polymorphism: IDataStore interface and FileDataStore\n"
            << " - Memory safety: unique_ptr and RAII timer\n"
            << " - Exceptions: robust parsing and guarded conversions\n"
            << " - File persistence: versioned serialization format\n\n";
}

void ReservationSystem::showMenu() const {
  printLine('*');
  for (const auto &option : *menu_options_) {
    std::cout << option.number << ". " << option.title << '\n';
  }
  printLine('*');
}

void ReservationSystem::addBus() {
  BusDetails details;
  details.number = trimCopy(readLine("Enter bus number: "));
  details.driver = trimCopy(readLine("Enter driver's name: "));
  details.arrival = trimCopy(readLine("Enter arrival time: "));
  details.departure = trimCopy(readLine("Enter departure time: "));
  details.source = trimCopy(readLine("Enter source city: "));
  details.destination = trimCopy(readLine("Enter destination city: "));
  details.fare_per_seat = static_cast<double>(readInt("Enter fare per seat (whole number): ", 0, 1000000));
  const auto result = addBusCommand(details);
  std::cout << result.message << '\n';
}

void ReservationSystem::reserveSeat() {
  if (buses_.empty()) {
    std::cout << "No buses available yet.\n";
    return;
  }
  const std::string number = trimCopy(readLine("Enter bus number: "));
  const int seat = readInt("Enter seat number (1-32): ", 1, kSeatCount);
  const std::string passenger = trimCopy(readLine("Enter passenger name: "));
  const auto result = reserveSeatCommand(number, seat, passenger);
  std::cout << result.message << '\n';
}

void ReservationSystem::showBusSeats() const {
  if (buses_.empty()) {
    std::cout << "No buses available yet.\n";
    return;
  }
  const std::string number = trimCopy(readLine("Enter bus number: "));
  const Bus *bus = findBusByNumber(number);
  if (bus == nullptr) {
    std::cout << "Bus not found.\n";
    return;
  }
  bus->printSeatMap(std::cout);
}

void ReservationSystem::listBuses() const {
  if (buses_.empty()) {
    std::cout << "No buses available.\n";
    return;
  }
  for (const auto &bus : buses_) {
    printLine('*');
    bus.printSummary(std::cout);
  }
  printLine('*');
}

void ReservationSystem::cancelReservation() {
  if (buses_.empty()) {
    std::cout << "No buses available yet.\n";
    return;
  }
  const std::string number = trimCopy(readLine("Enter bus number: "));
  const int seat = readInt("Enter seat number to cancel (1-32): ", 1, kSeatCount);
  const auto result = cancelSeatCommand(number, seat);
  std::cout << result.message << '\n';
}

void ReservationSystem::searchByRoute() const {
  const std::string source = toLowerCopy(trimCopy(readLine("Enter source city to search: ")));
  const std::string destination =
      toLowerCopy(trimCopy(readLine("Enter destination city to search: ")));
  bool found = false;
  for (const auto &bus : buses_) {
    const auto &details = bus.details();
    if (toLowerCopy(details.source) == source && toLowerCopy(details.destination) == destination) {
      printLine('*');
      bus.printSummary(std::cout);
      found = true;
    }
  }
  if (!found) {
    std::cout << "No buses found for the selected route.\n";
  }
}

void ReservationSystem::findPassenger() const {
  const std::string passenger = trimCopy(readLine("Enter passenger name to search: "));
  if (passenger.empty()) {
    std::cout << "Passenger name cannot be empty.\n";
    return;
  }

  bool found = false;
  for (const auto &bus : buses_) {
    const auto seat = bus.findPassengerSeat(passenger);
    if (seat.has_value()) {
      std::cout << "Passenger " << passenger << " is in bus " << bus.details().number
                << ", seat " << *seat << ".\n";
      found = true;
    }
  }
  if (!found) {
    std::cout << "Passenger not found in current records.\n";
  }
}

void ReservationSystem::saveDataNow() const {
  const auto save_result = saveToFile();
  std::cout << save_result.message << '\n';
}

void ReservationSystem::editBusDetails() {
  const std::string number = trimCopy(readLine("Enter bus number to edit: "));
  Bus *bus = findBusByNumber(number);
  if (bus == nullptr) {
    std::cout << "Bus not found.\n";
    return;
  }

  BusDetails updated = bus->details();
  std::cout << "Leave a field blank to keep current value.\n";

  std::string input = readLine("Driver name [" + updated.driver + "]: ");
  if (!trimCopy(input).empty()) {
    updated.driver = trimCopy(input);
  }
  input = readLine("Arrival time [" + updated.arrival + "]: ");
  if (!trimCopy(input).empty()) {
    updated.arrival = trimCopy(input);
  }
  input = readLine("Departure time [" + updated.departure + "]: ");
  if (!trimCopy(input).empty()) {
    updated.departure = trimCopy(input);
  }
  input = readLine("Source city [" + updated.source + "]: ");
  if (!trimCopy(input).empty()) {
    updated.source = trimCopy(input);
  }
  input = readLine("Destination city [" + updated.destination + "]: ");
  if (!trimCopy(input).empty()) {
    updated.destination = trimCopy(input);
  }

  input = trimCopy(readLine("Fare per seat [" + std::to_string(static_cast<int>(updated.fare_per_seat)) + "]: "));
  if (!input.empty()) {
    try {
      updated.fare_per_seat = std::stod(input);
      updated.fare_per_seat = clampValue(updated.fare_per_seat, 0.0, 1'000'000.0);
    } catch (const std::exception &) {
      std::cout << "Invalid fare input. Keeping previous fare.\n";
      updated.fare_per_seat = bus->details().fare_per_seat;
    }
  }

  bus->updateDetails(updated);
  std::cout << "Bus details updated.\n";
  (void)addAuditEvent("Updated bus details for " + number + ".");
}

void ReservationSystem::deleteBus() {
  const std::string number = trimCopy(readLine("Enter bus number to delete: "));
  for (auto it = buses_.begin(); it != buses_.end(); ++it) {
    if (it->details().number == number) {
      const std::string confirm = toLowerCopy(trimCopy(readLine("Type 'yes' to confirm deletion: ")));
      if (confirm == "yes") {
        undo_stack_.push_back(UndoDeleteBus{*it});
        buses_.erase(it);
        (void)addAuditEvent("Deleted bus " + number + ".");
        std::cout << "Bus deleted.\n";
      } else {
        std::cout << "Deletion cancelled.\n";
      }
      return;
    }
  }
  std::cout << "Bus not found.\n";
}

void ReservationSystem::transferReservation() {
  const std::string number = trimCopy(readLine("Enter bus number: "));
  const int from_seat = readInt("From seat (1-32): ", 1, kSeatCount);
  const int to_seat = readInt("To seat (1-32): ", 1, kSeatCount);

  const auto result = transferSeatCommand(number, from_seat, to_seat);
  std::cout << result.message << '\n';
}

void ReservationSystem::showOccupancyReport() const {
  if (buses_.empty()) {
    std::cout << "No buses available.\n";
    return;
  }
  const Bus *most_booked = &buses_.front();
  const Bus *least_booked = &buses_.front();
  int total_reserved = 0;

  for (const auto &bus : buses_) {
    const int reserved = bus.reservedSeatCount();
    total_reserved += reserved;
    if (reserved > most_booked->reservedSeatCount()) {
      most_booked = &bus;
    }
    if (reserved < least_booked->reservedSeatCount()) {
      least_booked = &bus;
    }
  }

  const auto full_buses = count_if_custom(
      buses_, [](const Bus &bus) { return bus.reservedSeatCount() == kSeatCount; });
  const auto [bus_count, reserved_count, empty_count] = std::tuple<std::size_t, int, int>{
      buses_.size(),
      total_reserved,
      static_cast<int>(buses_.size() * static_cast<std::size_t>(kSeatCount)) - total_reserved};

  printLine('=');
  std::cout << "Total buses: " << bus_count << '\n';
  std::cout << "Total reserved seats: " << reserved_count << '\n';
  std::cout << "Total empty seats: " << empty_count << '\n';
  std::cout << "Completely full buses: " << full_buses << '\n';
  printLine('-');
  std::cout << "Most booked bus: " << most_booked->details().number << " ("
            << most_booked->reservedSeatCount() << "/" << kSeatCount << " reserved)\n";
  std::cout << "Least booked bus: " << least_booked->details().number << " ("
            << least_booked->reservedSeatCount() << "/" << kSeatCount << " reserved)\n";
  printLine('=');
}

void ReservationSystem::showFareAndRevenueReport() const {
  if (buses_.empty()) {
    std::cout << "No buses available.\n";
    return;
  }
  double total_revenue = 0.0;
  double total_max_revenue = 0.0;
  printLine('=');
  for (const auto &bus : buses_) {
    const auto &details = bus.details();
    total_revenue += bus.currentRevenue();
    total_max_revenue += bus.maxPossibleRevenue();
    std::cout << "Bus " << details.number << " | Fare: " << details.fare_per_seat
              << " | Reserved: " << bus.reservedSeatCount() << "/" << kSeatCount
              << " | Revenue: " << bus.currentRevenue() << "/" << bus.maxPossibleRevenue() << '\n';
  }
  printLine('-');
  std::cout << "Total current revenue: " << total_revenue << '\n';
  std::cout << "Total max possible revenue: " << total_max_revenue << '\n';
  printLine('=');
}

void ReservationSystem::showAuditLog() const {
  if (audit_log_.empty()) {
    std::cout << "Audit log is empty.\n";
    return;
  }
  printLine('=');
  for (const auto &entry : audit_log_) {
    std::cout << entry << '\n';
  }
  printLine('=');
}

void ReservationSystem::clearAuditLog() {
  const std::string confirm = toLowerCopy(trimCopy(readLine("Type 'clear' to erase audit log: ")));
  if (confirm != "clear") {
    std::cout << "Audit log clear cancelled.\n";
    return;
  }
  audit_log_.clear();
  std::cout << "Audit log cleared.\n";
}

void ReservationSystem::addPassengerToWaitlist() {
  const std::string number = trimCopy(readLine("Enter bus number: "));
  Bus *bus = findBusByNumber(number);
  if (bus == nullptr) {
    std::cout << "Bus not found.\n";
    return;
  }
  const std::string passenger = trimCopy(readLine("Enter passenger name for waitlist: "));
  if (!bus->addToWaitlist(passenger)) {
    std::cout << "Could not add passenger to waitlist.\n";
    return;
  }
  std::cout << "Passenger added to waitlist. Current size: " << bus->waitlistSize() << '\n';
  (void)addAuditEvent("Added waitlist passenger " + passenger + " to bus " + number + ".");
}

void ReservationSystem::showBusWaitlist() const {
  const std::string number = trimCopy(readLine("Enter bus number: "));
  const Bus *bus = findBusByNumber(number);
  if (bus == nullptr) {
    std::cout << "Bus not found.\n";
    return;
  }
  bus->printWaitlist(std::cout);
}

void ReservationSystem::undoLastAction() {
  if (undo_stack_.empty()) {
    std::cout << "No action available to undo.\n";
    return;
  }
  const UndoAction action = undo_stack_.back();
  undo_stack_.pop_back();

  std::visit(
      [this](const auto &typed_action) {
        using T = std::decay_t<decltype(typed_action)>;
        if constexpr (std::is_same_v<T, UndoReserve>) {
          if (Bus *bus = findBusByNumber(typed_action.bus_number)) {
            (void)bus->cancelSeat(typed_action.seat);
            std::cout << "Undo: reservation removed.\n";
          }
        } else if constexpr (std::is_same_v<T, UndoCancel>) {
          if (Bus *bus = findBusByNumber(typed_action.bus_number)) {
            (void)bus->reserveSeat(typed_action.seat, typed_action.passenger);
            std::cout << "Undo: cancelled reservation restored.\n";
          }
        } else if constexpr (std::is_same_v<T, UndoTransfer>) {
          if (Bus *bus = findBusByNumber(typed_action.bus_number)) {
            (void)bus->transferReservation(typed_action.to_seat, typed_action.from_seat);
            std::cout << "Undo: transfer reversed.\n";
          }
        } else if constexpr (std::is_same_v<T, UndoAddBus>) {
          buses_.erase(std::remove_if(buses_.begin(), buses_.end(),
                                      [&typed_action](const Bus &bus) {
                                        return bus.details().number == typed_action.bus_number;
                                      }),
                       buses_.end());
          std::cout << "Undo: added bus removed.\n";
        } else if constexpr (std::is_same_v<T, UndoDeleteBus>) {
          buses_.push_back(typed_action.bus_snapshot);
          std::cout << "Undo: deleted bus restored.\n";
        }
      },
      action);
}

void ReservationSystem::showRouteCatalog() const {
  std::map<std::string, std::set<std::string>> catalog;
  for (const auto &bus : buses_) {
    catalog[bus.details().source].insert(bus.details().destination);
  }
  if (catalog.empty()) {
    std::cout << "No routes available.\n";
    return;
  }
  printLine('=');
  for (const auto &[source, destinations] : catalog) {
    std::cout << source << " -> ";
    bool first = true;
    for (const auto &destination : destinations) {
      if (!first) {
        std::cout << ", ";
      }
      std::cout << destination;
      first = false;
    }
    std::cout << '\n';
  }
  printLine('=');
}

void ReservationSystem::exitAndSave() {
  const auto save_result = saveToFile();
  std::cout << save_result.message << '\n';
  std::cout << "Successfully logged out.\n";
  is_running_ = false;
}

Bus *ReservationSystem::findBusByNumber(const std::string &number) {
  for (auto &bus : buses_) {
    if (bus.details().number == number) {
      return &bus;
    }
  }
  return nullptr;
}

const Bus *ReservationSystem::findBusByNumber(const std::string &number) const {
  for (const auto &bus : buses_) {
    if (bus.details().number == number) {
      return &bus;
    }
  }
  return nullptr;
}

OperationResult ReservationSystem::addAuditEvent(const std::string &message) {
  ++audit_counter_;
  const std::string entry = "[" + currentTimestamp() + "] #" + std::to_string(audit_counter_) +
                            " " + message;
  audit_log_.push_back(entry);
  return {OperationStatus::kSuccess, "Audit event appended."};
}

OperationResult ReservationSystem::saveToFile() const {
  PersistedData data{buses_, audit_log_};
  std::string error_message;
  if (!data_store_->save(data, error_message)) {
    return {OperationStatus::kFailure, "Save failed: " + error_message};
  }
  return {OperationStatus::kSuccess, "Data saved successfully."};
}

OperationResult ReservationSystem::loadFromFile() {
  PersistedData data;
  std::string error_message;
  if (!data_store_->load(data, error_message)) {
    return {OperationStatus::kFailure, "Load failed: " + error_message};
  }
  buses_ = std::move(data.buses);
  audit_log_ = std::move(data.audit_log);
  audit_counter_ = static_cast<int>(audit_log_.size());
  return {OperationStatus::kSuccess, "Data loaded successfully."};
}

void ReservationSystem::printLine(char ch) {
  for (int i = 0; i < 85; ++i) {
    std::cout << ch;
  }
  std::cout << '\n';
}

std::string ReservationSystem::readLine(const std::string &prompt) {
  std::string value;
  std::cout << prompt;
  std::getline(std::cin, value);
  return value;
}

int ReservationSystem::readInt(const std::string &prompt, int min_value, int max_value) {
  while (true) {
    std::cout << prompt;
    int choice = 0;
    if (std::cin >> choice && choice >= min_value && choice <= max_value) {
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      return choice;
    }
    std::cout << "Invalid input. Enter a number between " << min_value << " and " << max_value
              << ".\n";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
}

std::string ReservationSystem::toLowerCopy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

std::string ReservationSystem::trimCopy(std::string value) {
  auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
  value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
  value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
  return value;
}

bool ReservationSystem::isValidBusNumberFormat(const std::string &value) {
  // regex example:
  // ^[A-Za-z]{1,3}-?[0-9]{1,4}$
  // 1-3 letters, optional dash, then 1-4 digits.
  static const std::regex kBusFormat("^[A-Za-z]{1,3}-?[0-9]{1,4}$");
  return std::regex_match(value, kBusFormat);
}

} // namespace busbook
