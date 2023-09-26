#include <string>
#include <iostream>
#include <fstream>
#include "settings.h"

using namespace std;

namespace Settings {
    bool policyMode = false;
    int numPlayThreads = max(1, N_THREADS);
    int numChangeThreads = max(1, (numPlayThreads + 1) / 2);
    int fixedSimulationCount = -1;
    bool maximizePosition = false;
    int thinkingLevel = THINKING_LEVEL;
}

string DIRECTORY_PARAMS_IN = "";
string DIRECTORY_PARAMS_OUT = "";
string DIRECTORY_LOGS = "";

ConfigReader::ConfigReader(string cfile) {
    ifstream ifs(cfile);
    if (!ifs) cerr << "failed to open config file." << std::endl;
    if (ifs) ifs >> DIRECTORY_PARAMS_IN;
    if (ifs) ifs >> DIRECTORY_PARAMS_OUT;
    if (ifs) ifs >> DIRECTORY_LOGS;
};

ConfigReader configReader("lilovyy_config.txt");