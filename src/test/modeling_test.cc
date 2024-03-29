
// 相手モデリングのテスト

#include "../core/record.hpp"
#include "../engine/data.hpp"
#include "../engine/modeling.hpp"
#include "test.h"

using namespace std;

static SharedData shared;
static ThreadTools tools;
static Clock cl;


void testModeling(const MatchRecord& match, PlayerModel& model) {
    shared.initMatch(-1);
    tools.dice.seed(1);
    long long time = 0;

    for (int i = 0; i < match.games.size(); i++) {
        cl.start();
        model.update(match, i, -1, shared, tools.mbuf);
        time += cl.stop();
    }
    model.stats();
    cerr << "time = " << time / match.games.size() << " clock" << endl;
}

bool ModelingTest(const vector<string>& recordFiles, PlayerModel *pmodel) {
    shared.baseChangePolicy.bin(DIRECTORY_PARAMS_IN + "change_policy.bin");
    shared.basePlayPolicy.bin(DIRECTORY_PARAMS_IN + "play_policy.bin");

    PlayerModel model;
    for (string rf : recordFiles) {
        MatchRecord match(rf);
        testModeling(match, model);
    }
    if (pmodel != nullptr) *pmodel = model; // あとで使えるように保持

    return 0;
}