CXX      = g++
CXXFLAGS = -std=c++11 -march=native -MMD -MP
LDFLAGS  = -pthread
LIBS     =
INCLUDE  =
SRC_DIR  = ./src
BLD_DIR  = ./out
OBJ_DIR  = $(BLD_DIR)/obj
SRCS     = $(wildcard $(SRC_DIR)/*.cc) $(wildcard $(SRC_DIR)/**/*.cc)
OBJS     = $(subst $(SRC_DIR),$(OBJ_DIR), $(SRCS:.cc=.o))
EXEOBJS  = $(addprefix $(OBJ_DIR)/, client/client.o server/server.o test/test.o extra/learner.o)
SUBOBJS  = $(filter-out $(EXEOBJS), $(OBJS))
SRC_DIRS = $(shell find $(SRC_DIR) -maxdepth 2 -mindepth 1 -type d)
OBJ_DIRS = $(subst $(SRC_DIR),$(OBJ_DIR), $(SRC_DIRS))
TARGET   = $(addprefix $(BLD_DIR)/, client server test learner)
DEPENDS  = $(OBJS:.o=.d)

OPT = -Ofast -DNDEBUG -DMINIMUM
ifdef mode
	ifeq ($(mode),teacher)
		OPT := -Ofast -DNDEBUG -DMINIMUM -DTEACHER
	else ifeq ($(mode),match)	
		OPT := -Ofast -DNDEBUG -DMINIMUM -DMATCH
	else ifeq ($(mode),default)
		OPT := -Ofast -g -ggdb -fno-fast-math
	else ifeq ($(mode),debug)
		OPT := -O0 -g -ggdb -DDEBUG -DBROADCAST -D_GLIBCXX_DEBUG
	endif
endif
CXXFLAGS += $(OPT)

all: $(TARGET)

$(BLD_DIR)/client: $(OBJ_DIR)/client/client.o $(SUBOBJS) $(LIBS)
	$(CXX) -o $@ $(OBJ_DIR)/client/client.o $(SUBOBJS) $(LDFLAGS)

$(BLD_DIR)/server: $(OBJ_DIR)/server/server.o $(SUBOBJS) $(LIBS)
	$(CXX) -o $@ $(OBJ_DIR)/server/server.o $(SUBOBJS) $(LDFLAGS)

$(BLD_DIR)/test: $(OBJ_DIR)/test/test.o $(SUBOBJS) $(LIBS)
	$(CXX) -o $@ $(OBJ_DIR)/test/test.o $(SUBOBJS) $(LDFLAGS)

$(BLD_DIR)/learner: $(OBJ_DIR)/extra/learner.o $(SUBOBJS) $(LIBS)
	$(CXX) -o $@ $(OBJ_DIR)/extra/learner.o $(SUBOBJS) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cc
	@if [ ! -d $(OBJ_DIR) ]; \
		then echo "mkdir -p $(OBJ_DIRS)"; mkdir -p $(OBJ_DIRS); \
		fi
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $@ -c $< 

clean:
	$(RM) -r $(BLD_DIR)

-include $(DEPENDS)

.PHONY: all clean
