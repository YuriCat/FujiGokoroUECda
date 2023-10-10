
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
    tools.dice.srand(1);

    for (int i = 0; i < match.games.size(); i++) {
        model.update(match, i, -1, shared, tools.mbuf);
    }
    model.stats();
}

bool ModelingTest(const vector<string>& recordFiles, PlayerModel *pmodel) {
    shared.baseChangePolicy.fin(DIRECTORY_PARAMS_IN + "change_policy_param.dat");
    shared.basePlayPolicy.fin(DIRECTORY_PARAMS_IN + "play_policy_param.dat");

    PlayerModel model;
    for (string rf : recordFiles) {
        MatchRecord match(rf);
        testModeling(match, model);
    }
    if (pmodel != nullptr) *pmodel = model; // あとで使えるように保持

    return 0;
}