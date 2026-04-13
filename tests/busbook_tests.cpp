#include "concepts.hpp"
#include "models.hpp"
#include "storage.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace {

int g_failures = 0;

void expect(bool condition, const std::string &message) {
  if (!condition) {
    ++g_failures;
    std::cerr << "[FAIL] " << message << '\n';
  }
}

void testBusSeatLifecycle() {
  busbook::BusDetails details;
  details.number = "AB-101";
  details.driver = "John";
  details.arrival = "10:00";
  details.departure = "08:00";
  details.source = "CityA";
  details.destination = "CityB";
  details.fare_per_seat = 100.0;

  busbook::Bus bus(details);

  expect(bus.reserveSeat(1, "Alice"), "Reserve seat should succeed");
  expect(!bus.reserveSeat(1, "Bob"), "Reserve occupied seat should fail");
  expect(bus.isSeatReserved(1), "Seat 1 should be reserved");
  expect(bus.transferReservation(1, 2), "Transfer from seat 1 to 2 should succeed");
  expect(!bus.isSeatReserved(1), "Seat 1 should be empty after transfer");
  expect(bus.isSeatReserved(2), "Seat 2 should be reserved after transfer");
  expect(bus.cancelSeat(2), "Cancel seat 2 should succeed");
  expect(!bus.isSeatReserved(2), "Seat 2 should be empty after cancellation");
}

void testWaitlist() {
  busbook::Bus bus(busbook::BusDetails{"AB-102", "D", "11:00", "09:00", "X", "Y", 50.0});
  expect(bus.addToWaitlist("P1"), "Add first waitlisted passenger");
  expect(bus.addToWaitlist("P2"), "Add second waitlisted passenger");
  expect(bus.waitlistSize() == 2, "Waitlist size should be 2");

  auto first = bus.popWaitlistedPassenger();
  expect(first.has_value() && *first == "P1", "Waitlist pop order should be FIFO");
  auto second = bus.popWaitlistedPassenger();
  expect(second.has_value() && *second == "P2", "Second pop should return second passenger");
  auto none = bus.popWaitlistedPassenger();
  expect(!none.has_value(), "Popping empty waitlist should return nullopt");
}

void testSerializeDeserializeRoundTrip() {
  busbook::Bus bus(busbook::BusDetails{"ZX-7", "Driver", "13:00", "10:30", "M", "N", 75.0});
  bus.reserveSeat(1, "Alex");
  bus.reserveSeat(3, "Chris");
  bus.addToWaitlist("W1");
  bus.addToWaitlist("W2");

  std::stringstream stream;
  bus.serialize(stream);
  auto loaded = busbook::Bus::deserialize(stream, true, true);
  expect(loaded.has_value(), "Deserialize should succeed for serialized bus");
  if (loaded.has_value()) {
    expect(loaded->isSeatReserved(1), "Seat 1 should remain reserved after deserialize");
    expect(loaded->isSeatReserved(3), "Seat 3 should remain reserved after deserialize");
    expect(loaded->waitlistSize() == 2, "Waitlist should survive serialization");
  }
}

void testStorageLoadSave() {
  const std::string storage_file = "test_bus_data.txt";
  const std::string audit_file = "test_audit_log.txt";

  busbook::PersistedData original;
  busbook::Bus bus(busbook::BusDetails{"ST-1", "Drv", "08:00", "06:00", "S", "T", 30.0});
  bus.reserveSeat(1, "Neo");
  bus.addToWaitlist("Morpheus");
  original.buses.push_back(bus);
  original.audit_log.push_back("entry-1");

  busbook::FileDataStore store(storage_file, audit_file);
  std::string error;
  expect(store.save(original, error), "Storage save should succeed");

  busbook::PersistedData loaded;
  error.clear();
  expect(store.load(loaded, error), "Storage load should succeed");
  expect(loaded.buses.size() == 1, "Loaded buses size should be 1");
  expect(!loaded.audit_log.empty(), "Loaded audit log should not be empty");
  if (!loaded.buses.empty()) {
    expect(loaded.buses.front().isSeatReserved(1), "Loaded seat reservation should remain");
    expect(loaded.buses.front().waitlistSize() == 1, "Loaded waitlist size should remain");
  }

  std::remove(storage_file.c_str());
  std::remove(audit_file.c_str());
}

void testTemplateClamp() {
  expect(busbook::clampValue(5, 0, 10) == 5, "Clamp should keep value in range");
  expect(busbook::clampValue(-3, 0, 10) == 0, "Clamp should cap low value");
  expect(busbook::clampValue(99, 0, 10) == 10, "Clamp should cap high value");
}

} // namespace

int main() {
  testBusSeatLifecycle();
  testWaitlist();
  testSerializeDeserializeRoundTrip();
  testStorageLoadSave();
  testTemplateClamp();

  if (g_failures == 0) {
    std::cout << "[PASS] All unit tests passed.\n";
    return 0;
  }

  std::cerr << "[FAIL] " << g_failures << " test(s) failed.\n";
  return 1;
}
