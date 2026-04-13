#include "system.hpp"

#include <iostream>
#include <memory>
#include <string>

namespace {

int g_failures = 0;

void expect(bool condition, const std::string &message) {
  if (!condition) {
    ++g_failures;
    std::cerr << "[FAIL] " << message << '\n';
  }
}

class InMemoryDataStore final : public busbook::IDataStore {
public:
  bool load(busbook::PersistedData &data, std::string &error_message) const override {
    (void)error_message;
    data = shared_data_;
    return true;
  }

  bool save(const busbook::PersistedData &data, std::string &error_message) const override {
    (void)error_message;
    shared_data_ = data;
    return true;
  }

private:
  inline static busbook::PersistedData shared_data_{};
};

void testCommandFlow() {
  busbook::ReservationSystem system(std::make_unique<InMemoryDataStore>());

  const busbook::BusDetails details{"AB-123", "Driver", "11:00", "09:00", "A", "B", 100.0};
  expect(static_cast<bool>(system.addBusCommand(details)), "Add bus command should succeed");
  expect(system.busCount() == 1, "Bus count should be 1");
  expect(!system.addBusCommand(details), "Duplicate bus should fail");

  expect(static_cast<bool>(system.reserveSeatCommand("AB-123", 1, "Alice")),
         "Reserve seat should succeed");
  expect(static_cast<bool>(system.transferSeatCommand("AB-123", 1, 2)),
         "Transfer seat should succeed");
  expect(static_cast<bool>(system.cancelSeatCommand("AB-123", 2)), "Cancel seat should succeed");
  expect(!static_cast<bool>(system.cancelSeatCommand("AB-123", 2)),
         "Cancel empty seat should fail");

  expect(static_cast<bool>(system.saveDataCommand()), "Save command should succeed");

  // Fresh system loads from same in-memory store snapshot.
  busbook::ReservationSystem loaded_system(std::make_unique<InMemoryDataStore>());
  expect(static_cast<bool>(loaded_system.loadDataCommand()), "Load command should succeed");
  expect(loaded_system.busCount() == 1, "Loaded system should contain one bus");

  expect(static_cast<bool>(loaded_system.deleteBusCommand("AB-123")), "Delete bus should succeed");
  expect(loaded_system.busCount() == 0, "Bus count should be 0 after delete");
  expect(!static_cast<bool>(loaded_system.deleteBusCommand("AB-123")),
         "Delete unknown bus should fail");
}

} // namespace

int main() {
  testCommandFlow();
  if (g_failures == 0) {
    std::cout << "[PASS] System flow tests passed.\n";
    return 0;
  }
  std::cerr << "[FAIL] " << g_failures << " system flow test(s) failed.\n";
  return 1;
}
