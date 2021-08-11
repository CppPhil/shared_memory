#
# **************************************************************
# *                Simple C++ Makefile Template                *
# *                                                            *
# * Author: Arash Partow (2003)                                *
# * URL: http://www.partow.net/programming/makefile/index.html *
# *                                                            *
# * Copyright notice:                                          *
# * Free use of this C++ Makefile template is permitted under  *
# * the guidelines and in accordance with the the MIT License  *
# * http://www.opensource.org/licenses/MIT                     *
# *                                                            *
# **************************************************************
#

COMPILER         := c++
CXXFLAGS         := -pedantic-errors -Wall -Wextra -Werror -std=c++17 -pthread
BUILD            := ./build
LIB_DIR          := $(BUILD)/libs
APP_DIR          := $(BUILD)/apps
LDFLAGS          := -pthread -L$(LIB_DIR) -lshared_memory
LIB_INCLUDE      := -Ilib/include
LIB_SRC          := $(wildcard lib/src/*.cpp)
LIB_OBJ_DIR      := $(BUILD)/lib_objects
CLIENT_INCLUDE   := -Iclient/include -Ilib/include
CLIENT_SRC       := $(wildcard client/src/*.cpp)
CLIENT_OBJ_DIR   := $(BUILD)/client_objects
SERVER_INCLUDE   := -Iserver/include -Ilib/include
SERVER_SRC       := $(wildcard server/src/*.cpp)
SERVER_OBJ_DIR   := $(BUILD)/server_objects
LIB_NAME         := $(LIB_DIR)/libshared_memory.a

LIB_OBJECTS    := $(LIB_SRC:%.cpp=$(LIB_OBJ_DIR)/%.o)
CLIENT_OBJECTS := $(CLIENT_SRC:%.cpp=$(CLIENT_OBJ_DIR)/%.o)
SERVER_OBJECTS := $(SERVER_SRC:%.cpp=$(SERVER_OBJ_DIR)/%.o)

all: clean build $(LIB_NAME) $(APP_DIR)/client $(APP_DIR)/server

$(LIB_OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(COMPILER) $(CXXFLAGS) $(LIB_INCLUDE) -c $< -MMD -o $@

$(CLIENT_OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(COMPILER) $(CXXFLAGS) $(CLIENT_INCLUDE) -c $< -MMD -o $@

$(SERVER_OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(COMPILER) $(CXXFLAGS) $(SERVER_INCLUDE) -c $< -MMD -o $@

$(LIB_NAME): $(LIB_OBJECTS)
	@mkdir -p $(@D)
	ar rcs $(LIB_NAME) $^

$(APP_DIR)/client: $(CLIENT_OBJECTS)
	@mkdir -p $(@D)
	$(COMPILER) $(CXXFLAGS) -o $(APP_DIR)/client $^ $(LDFLAGS)

$(APP_DIR)/server: $(SERVER_OBJECTS)
	@mkdir -p $(@D)
	$(COMPILER) $(CXXFLAGS) -o $(APP_DIR)/server $^ $(LDFLAGS)

.PHONY: all build clean debug release info

build:
	@mkdir -p $(LIB_DIR)
	@mkdir -p $(LIB_OBJ_DIR)
	@mkdir -p $(APP_DIR)
	@mkdir -p $(CLIENT_OBJ_DIR)
	@mkdir -p $(SERVER_OBJ_DIR)

debug: CXXFLAGS += -DDEBUG -g
debug: all

release: CXXFLAGS += -O3 -g -DNDEBUG
release: all

clean:
	-@rm -rvf $(LIB_OBJ_DIR)/*
	-@rm -rvf $(LIB_DIR)/*
	-@rm -rvf $(CLIENT_OBJ_DIR)/*
	-@rm -rvf $(SERVER_OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/*
	-@rm -rvf $(BUILD)
