# 
# 1. General Compiler Settings
#
CXX       = g++
CXXFLAGS  = -std=c++14 -Wall -Wextra -Wcast-qual -Wno-unused-function -Wno-sign-compare -Wno-unused-value -Wno-unused-label -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-parameter -fno-exceptions -fno-rtti \
            -pedantic -Wno-long-long -msse4.2 -D__STDC_CONSTANT_MACROS
INCLUDES  =
LIBRARIES = -lpthread

#
# 2. Target Specific Settings
#
ifeq ($(TARGET),teacher)
	CXXFLAGS += -Ofast -DNDEBUG -DMINIMUM -DTEACHER
        output_dir := out/teacher/
endif
ifeq ($(TARGET),match)
	CXXFLAGS += -Ofast -DNDEBUG -DMINIMUM -DMATCH
        output_dir := out/match/
endif
ifeq ($(TARGET),release)
	CXXFLAGS += -Ofast -DNDEBUG -DMINIMUM
        output_dir := out/release/
endif
ifeq ($(TARGET),debug)
	CXXFLAGS += -O0 -g -ggdb -DDEBUG -DBROADCAST -D_GLIBCXX_DEBUG
        output_dir := out/debug/
endif
ifeq ($(TARGET),default)
	CXXFLAGS += -Ofast -g -ggdb -fno-fast-math
        output_dir := out/default/
endif

#
# 2. Default Settings (applied if there is no target-specific settings)
#
sources      ?= $(shell ls -R src/*.cc)
sources_dir  ?= src/
exec_dir     ?= src/exec/
objects      ?=
directories  ?= $(output_dir)

#
# 4. Public Targets
#
default release debug development profile test coverage:
	$(MAKE) TARGET=$@ preparation estimator_learner l2_test modeling_test policy_test value_generator dominance_test cards_test movegen_test policy_client mate_test client server policy_learner record_analyzer rating_calculator

match:
	$(MAKE) TARGET=$@ preparation client policy_client

teacher:
	$(MAKE) TARGET=$@ preparation client

run-coverage: coverage
	out/coverage --gtest_output=xml

clean:
	rm -rf out/*

scaffold:
	mkdir -p out test out/data doc lib obj resource

#
# 5. Private Targets
#
preparation $(directories):
	mkdir -p $(directories)

record_analyzer :
	$(CXX) $(CXXFLAGS) -o $(output_dir)record_analyzer $(exec_dir)/record_analyzer.cc $(LIBRARIES)

rating_calculator :
	$(CXX) $(CXXFLAGS) -o $(output_dir)rating_calculator $(exec_dir)/rating_calculator.cc $(LIBRARIES)

value_generator :
	$(CXX) $(CXXFLAGS) -o $(output_dir)value_generator $(exec_dir)/value_generator.cc $(LIBRARIES)

cards_test :
	$(CXX) $(CXXFLAGS) -o $(output_dir)cards_test $(exec_dir)/cards_test.cc $(LIBRARIES)

movegen_test :
	$(CXX) $(CXXFLAGS) -o $(output_dir)movegen_test $(exec_dir)/movegen_test.cc $(LIBRARIES)

dominance_test :
	$(CXX) $(CXXFLAGS) -o $(output_dir)dominance_test $(exec_dir)/dominance_test.cc $(LIBRARIES)

mate_test :
	$(CXX) $(CXXFLAGS) -o $(output_dir)mate_test $(exec_dir)/mate_test.cc $(LIBRARIES)

l2_test :
	$(CXX) $(CXXFLAGS) -o $(output_dir)l2_test $(exec_dir)/l2_test.cc $(LIBRARIES)

maxn_test :
	$(CXX) $(CXXFLAGS) -o $(output_dir)maxn_test $(exec_dir)/maxn_test.cc $(LIBRARIES)

policy_test :
	$(CXX) $(CXXFLAGS) -o $(output_dir)policy_test $(exec_dir)/policy_test.cc $(LIBRARIES)

modeling_test :
	$(CXX) $(CXXFLAGS) -o $(output_dir)modeling_test $(exec_dir)/modeling_test.cc $(LIBRARIES)

estimator_learner :
	$(CXX) $(CXXFLAGS) -o $(output_dir)estimator_learner $(exec_dir)/estimator_learner.cc $(LIBRARIES)

client :
	$(CXX) $(CXXFLAGS) -o $(output_dir)client $(sources_dir)client.cc $(sources_dir)connection.c $(LIBRARIES)

policy_client :
	$(CXX) $(CXXFLAGS) -o $(output_dir)policy_client $(sources_dir)client.cc $(sources_dir)connection.c $(LIBRARIES) -DPOLICY_ONLY

policy_rl_client :
	$(CXX) $(CXXFLAGS) -o $(output_dir)policy_rl_client $(sources_dir)client.cc $(sources_dir)connection.c $(LIBRARIES) -DPOLICY_ONLY -DRL_POLICY

server :
	$(CXX) $(CXXFLAGS) -o $(output_dir)server $(sources_dir)server/daihubc.cc $(sources_dir)server/mt19937ar.c $(LIBRARIES)

policy_learner :
	$(CXX) $(CXXFLAGS) -o $(output_dir)policy_learner $(exec_dir)policy_learner.cc $(LIBRARIES)

random_client :
	$(CXX) $(CXXFLAGS) -o $(output_dir)random_client $(sources_dir)client.cc $(sources_dir)connection.c $(LIBRARIES) -DRANDOM_MODE

human_client :
	$(CXX) $(CXXFLAGS) -o $(output_dir)human_client $(sources_dir)client.cc $(sources_dir)connection.c $(LIBRARIES) -DHUMAN_MODE -DBROADCAST

-include $(dependencies)
