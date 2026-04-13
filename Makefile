# BusBook Makefile
# cspell:ignore Wextra Iinclude busbook fsyntax
# ------------------------------------------------------------
# A practical developer Makefile for building/running/testing
# the BusBook C++ project without requiring CMake.

PROJECT_NAME := BusBook
BUILD_DIR := build

CXX ?= g++
STD := -std=c++20
WARN := -Wall -Wextra -pedantic
INCLUDES := -Iinclude
OPT := -O2
DBG := -g -O0

CXXFLAGS := $(STD) $(WARN) $(INCLUDES) $(OPT)
DEBUG_FLAGS := $(STD) $(WARN) $(INCLUDES) $(DBG)

APP := $(BUILD_DIR)/$(PROJECT_NAME)
UNIT_TEST_BIN := $(BUILD_DIR)/BusBookTests
FLOW_TEST_BIN := $(BUILD_DIR)/BusBookSystemFlowTests

APP_SRCS := \
	src/main.cpp \
	src/models.cpp \
	src/storage.cpp \
	src/concepts.cpp \
	src/system.cpp

UNIT_TEST_SRCS := \
	tests/busbook_tests.cpp \
	src/models.cpp \
	src/storage.cpp \
	src/concepts.cpp

FLOW_TEST_SRCS := \
	tests/system_flow_tests.cpp \
	src/system.cpp \
	src/models.cpp \
	src/storage.cpp \
	src/concepts.cpp

.PHONY: help all release debug run test test-unit test-flow clean rebuild lint

help:
	@echo "BusBook Makefile targets:"
	@echo "  make              -> Build release app"
	@echo "  make release      -> Build release app"
	@echo "  make debug        -> Build debug app"
	@echo "  make run          -> Build and run app"
	@echo "  make test         -> Run all tests"
	@echo "  make test-unit    -> Run unit tests only"
	@echo "  make test-flow    -> Run system-flow tests only"
	@echo "  make clean        -> Remove build artifacts"
	@echo "  make rebuild      -> Clean and build release"
	@echo "  make lint         -> Compile-only check app sources"

all: release

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

release: CXXFLAGS := $(STD) $(WARN) $(INCLUDES) $(OPT)
release: $(APP)

debug: CXXFLAGS := $(DEBUG_FLAGS)
debug: $(APP)

$(APP): $(APP_SRCS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(APP_SRCS) -o $(APP)

run: release
	./$(APP)

$(UNIT_TEST_BIN): $(UNIT_TEST_SRCS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(UNIT_TEST_SRCS) -o $(UNIT_TEST_BIN)

$(FLOW_TEST_BIN): $(FLOW_TEST_SRCS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(FLOW_TEST_SRCS) -o $(FLOW_TEST_BIN)

test: test-unit test-flow

test-unit: $(UNIT_TEST_BIN)
	./$(UNIT_TEST_BIN)

test-flow: $(FLOW_TEST_BIN)
	./$(FLOW_TEST_BIN)

lint: | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -fsyntax-only $(APP_SRCS)

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean release
