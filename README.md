# BusBook - C++ Learning Project (Basic to Advanced)
<!-- cspell:ignore Wextra Iinclude waitlisted waitlist busbook RAII -->

This project is remodeled as a **learn-by-building C++ codebase** using a Bus Reservation System.
It is intentionally organized and heavily commented so a learner can walk from beginner ideas to
advanced modern C++ patterns.

## Quick Start

```bash
make
make test
make run
```

---

## Learning Goals Covered

The code demonstrates and explains:

- C++ basics: variables, loops, conditions, user input, functions
- OOP: `struct`, `class`, encapsulation, constructors, const methods
- STL: `std::vector`, `std::array`, `std::string`, `std::optional`
- Algorithms and lambdas: `std::find_if`, `std::count_if`, custom predicates
- Generic programming: template utility `count_if_custom`
- Functional style: menu actions stored as `std::function<void()>`
- RAII and smart pointers: `std::unique_ptr`
- Error handling: `try/catch`, `std::exception`, safe numeric parsing
- File handling and persistence: serialization + versioned data files
- Clean architecture: headers in `include/`, implementation in `src/`

---

## Project Structure

```text
Bus-Reservation-System/
  include/
    concepts.hpp        # Reusable concept demos (enum class, templates, RAII timer)
    models.hpp          # Domain models + declarations
    storage.hpp         # Persistence interface + file-based implementation contract
    system.hpp          # Reservation system interface + template helper
  src/
    concepts.cpp        # RAII timer + timestamp utilities
    models.cpp          # Bus behavior implementation
    storage.cpp         # File persistence implementation
    system.cpp          # Menu flow, business logic, reports
    main.cpp            # Program entry point + top-level exception handling
  bus_data.txt          # Runtime storage (auto-generated/updated)
  audit_log.txt         # Runtime audit trail (auto-generated/updated)
  CMakeLists.txt
```

---

## Build and Run

### Makefile (fastest for daily use)

```bash
make            # release build
make run        # build and run app
make test       # run all tests
make debug      # debug build
make clean      # remove build artifacts
```

### CMake (recommended)

```bash
cmake -S . -B build
cmake --build build
./build/BusBook
```

Run tests:

```bash
ctest --test-dir build --output-on-failure
```

### Direct Compile

```bash
g++ -std=c++20 -Wall -Wextra -pedantic \
  src/main.cpp src/models.cpp src/storage.cpp src/concepts.cpp src/system.cpp \
  -Iinclude -o BusBook
./BusBook
```

Direct test compile/run:

```bash
g++ -std=c++20 -Wall -Wextra -pedantic \
  tests/busbook_tests.cpp src/models.cpp src/storage.cpp src/concepts.cpp \
  -Iinclude -o BusBookTests
./BusBookTests
```

System-flow test compile/run (mock datastore, no console input):

```bash
g++ -std=c++20 -Wall -Wextra -pedantic \
  tests/system_flow_tests.cpp src/system.cpp src/models.cpp src/storage.cpp src/concepts.cpp \
  -Iinclude -o BusBookSystemFlowTests
./BusBookSystemFlowTests
```

---

## Concept -> File Map

- Basics (variables, loops, input, conditions): `src/system.cpp`
- Functions and decomposition: `include/system.hpp`, `src/system.cpp`
- Structs and classes (OOP fundamentals): `include/models.hpp`, `src/models.cpp`
- Encapsulation and const correctness: `include/models.hpp`, `src/models.cpp`
- STL containers (`vector`, `array`, `string`, `optional`): `include/models.hpp`, `src/models.cpp`, `src/system.cpp`
- STL algorithms + lambdas: `src/models.cpp`, `src/system.cpp`
- Function templates: `include/system.hpp` (`count_if_custom`), `include/concepts.hpp` (`clampValue`)
- Smart pointers (`unique_ptr`) and ownership: `include/system.hpp`, `src/system.cpp`
- Enums and result object pattern: `include/concepts.hpp`, `src/system.cpp`
- RAII pattern (`ScopedTimer`): `include/concepts.hpp`, `src/concepts.cpp`, `src/system.cpp`
- Exceptions and error handling: `src/main.cpp`, `src/system.cpp`, `src/storage.cpp`
- Interface and polymorphism (`IDataStore`): `include/storage.hpp`, `src/storage.cpp`, `src/system.cpp`
- File I/O + serialization/versioned persistence: `src/storage.cpp`, `src/models.cpp`
- Regex validation: `src/system.cpp`
- Variant + visit (undo command model): `include/system.hpp`, `src/system.cpp`
- Tuple + structured bindings: `src/system.cpp`
- Associative containers (`map`, `set`) for route catalog: `src/system.cpp`
- Operator overloading (`operator<<`): `include/models.hpp`, `src/models.cpp`

---

## Learning Roadmap (How to Study This Project)

1. Start from `src/main.cpp` to understand program entry and exception safety.
2. Read `include/models.hpp` and `src/models.cpp` for OOP + STL fundamentals.
3. Read `include/system.hpp` for template + functional menu action declarations.
4. Read `src/system.cpp` to see the full feature flow and file persistence.
5. Run the app, test menu options, and map each feature to the concept used.

---

## Features in the Application

1. Add new bus details  
2. Reserve a seat  
3. Show seats in a bus  
4. List all buses  
5. Cancel a reservation (auto-promotes waitlisted passenger)  
6. Search buses by route  
7. Find passenger by name  
8. Save data now  
9. Edit bus details  
10. Delete a bus  
11. Transfer reservation to another seat  
12. Occupancy analytics  
13. Fare and revenue report  
14. View audit log  
15. Clear audit log  
16. Add passenger to bus waitlist  
17. Show waitlist for a bus  
18. Undo last action  
19. Show route catalog  
20. Exit  

---

## Notes

- The remodeled code is now optimized for learning and maintainability.
