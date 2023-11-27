BUILD_DIR ?= $(PWD)/build
BIN_DIR ?= $(BUILD_DIR)/bin
OBJ_DIR ?= $(BUILD_DIR)/obj
SRC_DIR := $(PWD)/src

BUILD_TYPE ?= Debug

EXEC := $(BIN_DIR)/sat-lang

CXX := clang++
CXXFLAGS += -std=c++17 -Wall -Wpedantic -Wextra -Werror
CXXFLAGS += -Wsign-conversion

ifeq (${BUILD_TYPE},Debug)
CXXFLAGS += -g
else
CXXFLAGS += -O3 -DNDEBUG
endif

CXXFLAGS += -I$(SRC_DIR)

SRC_FILES := $(shell ls $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))
DEPS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.d,$(SRC_FILES))
-include ${DEPS}

.PHONY: build clean

build: $(EXEC)

$(EXEC): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ -o $@

${OBJ_DIR}/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -MMD -MF $(@:.o=.d) -o $@

clean:
	rm -rf $(BUILD_DIR)
