#ifndef BUSBOOK_STORAGE_HPP
#define BUSBOOK_STORAGE_HPP

#include "models.hpp"

#include <memory>
#include <string>
#include <vector>

namespace busbook {

// Simple aggregate object for persistence transfer.
// It keeps storage concerns separate from UI/business logic.
struct PersistedData {
  std::vector<Bus> buses;
  std::vector<std::string> audit_log;
};

// Interface (abstract base class) for persistence engines.
// Demonstrates runtime polymorphism using virtual methods.
class IDataStore {
public:
  virtual ~IDataStore() = default;

  virtual bool load(PersistedData &data, std::string &error_message) const = 0;
  virtual bool save(const PersistedData &data,
                    std::string &error_message) const = 0;
};

// Concrete implementation that stores data in plain text files.
class FileDataStore final : public IDataStore {
public:
  explicit FileDataStore(std::string storage_file = "bus_data.txt",
                         std::string audit_file = "audit_log.txt");

  bool load(PersistedData &data, std::string &error_message) const override;
  bool save(const PersistedData &data,
            std::string &error_message) const override;

private:
  std::string storage_file_;
  std::string audit_file_;
};

} // namespace busbook

#endif // BUSBOOK_STORAGE_HPP
