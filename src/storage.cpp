#include "storage.hpp"

#include <fstream>
#include <limits>
#include <string>
#include <utility>

namespace busbook {

namespace {
inline constexpr const char *kDataVersion = "BUSBOOK_V5";
inline constexpr const char *kDataVersionV4 = "BUSBOOK_V4";
}

FileDataStore::FileDataStore(std::string storage_file, std::string audit_file)
    : storage_file_(std::move(storage_file)), audit_file_(std::move(audit_file)) {}

bool FileDataStore::load(PersistedData &data, std::string &error_message) const {
  data = PersistedData{};

  // Load buses.
  std::ifstream input(storage_file_);
  if (input) {
    std::string first_line;
    if (std::getline(input, first_line)) {
      bool with_fare = false;
      bool with_waitlist = false;
      std::size_t count = 0;

      if (first_line == kDataVersion) {
        with_fare = true;
        with_waitlist = true;
        if (!(input >> count)) {
          error_message = "Invalid bus count in storage file.";
          return false;
        }
        input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      } else if (first_line == kDataVersionV4) {
        with_fare = true;
        if (!(input >> count)) {
          error_message = "Invalid bus count in storage file.";
          return false;
        }
        input.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      } else {
        try {
          count = static_cast<std::size_t>(std::stoul(first_line));
        } catch (const std::exception &) {
          error_message = "Could not parse storage header.";
          return false;
        }
      }

      data.buses.reserve(count);
      for (std::size_t i = 0; i < count; ++i) {
        auto bus = Bus::deserialize(input, with_fare, with_waitlist);
        if (!bus.has_value()) {
          error_message = "Storage file appears corrupted.";
          return false;
        }
        data.buses.push_back(std::move(*bus));
      }
    }
  }

  // Load audit log.
  std::ifstream audit_input(audit_file_);
  if (audit_input) {
    std::string line;
    while (std::getline(audit_input, line)) {
      if (!line.empty()) {
        data.audit_log.push_back(line);
      }
    }
  }

  return true;
}

bool FileDataStore::save(const PersistedData &data, std::string &error_message) const {
  std::ofstream output(storage_file_);
  if (!output) {
    error_message = "Unable to open storage file for writing.";
    return false;
  }

  output << kDataVersion << '\n';
  output << data.buses.size() << '\n';
  for (const auto &bus : data.buses) {
    bus.serialize(output);
  }

  std::ofstream audit_output(audit_file_, std::ios::trunc);
  if (!audit_output) {
    error_message = "Unable to open audit file for writing.";
    return false;
  }
  for (const auto &entry : data.audit_log) {
    audit_output << entry << '\n';
  }
  return true;
}

} // namespace busbook
