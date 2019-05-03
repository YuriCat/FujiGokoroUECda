TARGET    = all
CXX       = g++
CXXFLAGS  = -std=c++14 -march=native -Wall -Wextra -Wcast-qual -Wno-sign-compare -Wno-unused-value -Wno-unused-label -Wno-unused-variable -Wno-unused-parameter -Wno-unused-function -Wno-missing-field-initializers -fno-exceptions -fno-rtti
INCLUDES  =
LIBRARIES = -lpthread

ifdef mode
	ifeq ($(mode),teacher)
		CXXFLAGS += -Ofast -DNDEBUG -DMINIMUM -DTEACHER
	else ifeq ($(mode),match)	
		CXXFLAGS += -Ofast -DNDEBUG -DMINIMUM -DMATCH
	else ifeq ($(mode),default)
		CXXFLAGS += -Ofast -g -ggdb -fno-fast-math
	else ifeq ($(mode),debug)
		CXXFLAGS += -O0 -g -ggdb -DDEBUG -DBROADCAST -D_GLIBCXX_DEBUG
	else
		CXXFLAGS += -Ofast -DNDEBUG -DMINIMUM
	endif
else
	CXXFLAGS += -Ofast -DNDEBUG -DMINIMUM
endif

OBJ_DIR = out/obj

BASE_HEADERS   = $(wildcard src/base/*.hpp)
CORE_HEADERS   = $(wildcard src/core/*.hpp)
ENGINE_HEADERS = $(wildcard src/engine/*.hpp) $(wildcard src/engine/*.h)
SERVER_HEADERS = $(wildcard src/server/*.hpp) $(wildcard src/server/*.h)
ALL_HEADERS    = $(BASE_HEADERS) $(CORE_HEADERS) $(ENGINE_HEADERS) $(SERVER_HEADERS)

CORE_SOURCES   = $(wildcard src/core/*.cc)
ENGINE_SOURCES = $(wildcard src/engine/*.cc)

ALL_SOURCES    = $(shell ls src/*.cc) $(shell ls src/*.c) $(shell ls src/server/*.cc) $(shell ls src/server/*.c)

CORE_OBJECTS   = $(subst .cc,.o,$(subst src/,$(OBJ_DIR)/,$(CORE_SOURCES)))
ENGINE_OBJECTS = $(subst .cc,.o,$(subst src/,$(OBJ_DIR)/,$(ENGINE_SOURCES)))
CLIENT_OBJECTS = $(patsubst %,$(OBJ_DIR)/%,client.o connection.o)
SERVER_OBJECTS = $(patsubst %,$(OBJ_DIR)/%,server/server.o server/mt19937ar.o)

OBJECTS        = $(CORE_OBJECTS) $(ENGINE_OBJECTS) $(CLIENT_OBJECTS) $(SERVER_OBJECTS)

all: $(OBJ_DIR) $(patsubst %,out/%,client server)

clean:
	rm -rf out

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR) $(OBJ_DIR)/core $(OBJ_DIR)/engine $(OBJ_DIR)/server

out/client: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o out/client $(CLIENT_OBJECTS) $(CORE_OBJECTS)

out/server: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o out/server $(SERVER_OBJECTS) $(CORE_OBJECTS)

$(OBJ_DIR)/core/daifugo.o: src/core/daifugo.cc src/core/daifugo.hpp $(BASE_HEADERS)
	$(CXX) -c $(CXXFLAGS) -o $@ $<


$(OBJ_DIR)/client.o: src/client.cc $(ALL_HEADERS)
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(OBJ_DIR)/connection.o: src/connection.c src/connection.h
	$(CXX) -c $(CXXFLAGS) -o $@ $<


$(OBJ_DIR)/server/server.o: src/server/daihubc.cc $(ALL_HEADERS)
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(OBJ_DIR)/server/mt19937ar.o: src/server/mt19937ar.c src/server/mt19937ar.h
	$(CXX) -c $(CXXFLAGS) -o $@ $<
