#ifndef BUSBOOK_SYSTEM_HPP
#define BUSBOOK_SYSTEM_HPP

#include "concepts.hpp"
#include "models.hpp"
#include "storage.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace busbook {

// Type alias with "using": easier to read than raw type.
using MenuAction = std::function<void()>;

// Generic helper function template.
// Demonstrates templates + higher-order function parameter (predicate).
template <typename Collection, typename Predicate>
std::size_t count_if_custom(const Collection &collection, Predicate predicate) {
  std::size_t count = 0;
  for (const auto &item : collection) {
    if (predicate(item)) {
      ++count;
    }
  }
  return count;
}

class ReservationSystem final {
public:
  // Constructor with dependency injection:
  // caller can pass any IDataStore implementation (file, in-memory, database, etc.).
  explicit ReservationSystem(std::unique_ptr<IDataStore> data_store = std::make_unique<FileDataStore>());
  void run();

  // Test-friendly command surface (no console I/O).
  OperationResult addBusCommand(const BusDetails &details);
  OperationResult reserveSeatCommand(const std::string &bus_number, int seat, const std::string &passenger);
  OperationResult cancelSeatCommand(const std::string &bus_number, int seat);
  OperationResult transferSeatCommand(const std::string &bus_number, int from_seat, int to_seat);
  OperationResult deleteBusCommand(const std::string &bus_number);
  OperationResult loadDataCommand();
  OperationResult saveDataCommand() const;
  std::size_t busCount() const noexcept;

private:
  struct MenuOption {
    int number = 0;
    std::string title;
    MenuAction action;
  };

  void showWelcome() const;
  void showLearningMap() const;
  void showMenu() const;

  void addBus();
  void reserveSeat();
  void showBusSeats() const;
  void listBuses() const;
  void cancelReservation();
  void searchByRoute() const;
  void findPassenger() const;
  void saveDataNow() const;
  void editBusDetails();
  void deleteBus();
  void transferReservation();
  void showOccupancyReport() const;
  void showFareAndRevenueReport() const;
  void showAuditLog() const;
  void clearAuditLog();
  void addPassengerToWaitlist();
  void showBusWaitlist() const;
  void undoLastAction();
  void showRouteCatalog() const;
  void exitAndSave();

  Bus *findBusByNumber(const std::string &number);
  const Bus *findBusByNumber(const std::string &number) const;

  OperationResult addAuditEvent(const std::string &message);
  OperationResult saveToFile() const;
  OperationResult loadFromFile();

  static void printLine(char ch = '=');
  static std::string readLine(const std::string &prompt);
  static int readInt(const std::string &prompt, int min_value, int max_value);
  static std::string toLowerCopy(std::string value);
  static std::string trimCopy(std::string value);
  static bool isValidBusNumberFormat(const std::string &value);

  struct UndoReserve {
    std::string bus_number;
    int seat = 0;
  };
  struct UndoCancel {
    std::string bus_number;
    int seat = 0;
    std::string passenger;
  };
  struct UndoTransfer {
    std::string bus_number;
    int from_seat = 0;
    int to_seat = 0;
  };
  struct UndoAddBus {
    std::string bus_number;
  };
  struct UndoDeleteBus {
    Bus bus_snapshot;
  };
  using UndoAction = std::variant<UndoReserve, UndoCancel, UndoTransfer, UndoAddBus, UndoDeleteBus>;

  // Smart pointer example: owning polymorphic resource.
  // std::unique_ptr guarantees single ownership and automatic cleanup (RAII).
  std::unique_ptr<std::vector<MenuOption>> menu_options_;
  std::unique_ptr<IDataStore> data_store_;
  std::vector<Bus> buses_;
  std::vector<std::string> audit_log_;
  std::vector<UndoAction> undo_stack_;
  int audit_counter_ = 0;
  bool is_running_ = true;
};

} // namespace busbook

#endif // BUSBOOK_SYSTEM_HPP
