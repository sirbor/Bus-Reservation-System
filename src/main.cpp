#include "system.hpp"

#include <exception>
#include <iostream>

// Entry point:
// main() is where every C++ executable starts.
int main() {
  try {
    busbook::ReservationSystem system;
    system.run();
    return 0;
  } catch (const std::exception &error) {
    // Top-level safety net:
    // if something unexpected throws, we show a meaningful message.
    std::cerr << "Fatal error: " << error.what() << '\n';
    return 1;
  }
}
