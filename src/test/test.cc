#include "test.h"

using namespace std;

int main(int argc, char* argv[]) {
    vector<string> recordFiles;

    // ヘッダ読み取り
    for (int c = 1; c < argc; c++) {
        if (!strcmp(argv[c], "-i")) { // input directory
            DIRECTORY_PARAMS_IN = string(argv[c + 1]);
        } else if (!strcmp(argv[c], "-l")) { // log path
            recordFiles.push_back(string(argv[c + 1]));
        } else if (!strcmp(argv[c], "-ld")) { // log directory path
            auto tmp = getFilePathVectorRecursively(string(argv[c + 1]), ".dat");
            recordFiles.insert(recordFiles.end(), tmp.begin(), tmp.end());
        }
    }

    // 機械学習モデルに絡まない部分のテスト
    CardsTest();
    MovegenTest(recordFiles);
    DominanceTest(recordFiles);
    MateTest(recordFiles);
    Last2Test(recordFiles);

    // 機械学習モデルを利用するテスト
    PolicyTest(recordFiles);
    PlayerModel model;
    ModelingTest(recordFiles, &model);
    SimulationTest(recordFiles, &model);
    EstimationTest(recordFiles, &model);

    return 0;
}