
// 相手モデリングのテスト

#include "../core/record.hpp"
#include "../core/action.hpp"
#include "../core/field.hpp"
#include "../engine/data.hpp"
#include "../engine/modeling.hpp"
#include "test.h"

using namespace std;

static SharedData shared;
static ThreadTools tools;
static Clock cl;


void testModeling(const MatchRecord& match) {
    shared.initMatch(-1);
    tools.dice.srand(1);
    PlayerModel model;

    for (int i = 0; i < match.games.size(); i++) {
        model.update(match, i, -1, shared.basePlayPolicy, tools.mbuf);
    }
    model.stats();
}

bool ModelingTest(const vector<string>& recordFiles) {
    shared.baseChangePolicy.fin(DIRECTORY_PARAMS_IN + "change_policy_param.dat");
    shared.basePlayPolicy.fin(DIRECTORY_PARAMS_IN + "play_policy_param.dat");

    for (string rf : recordFiles) {
        MatchRecord match(rf);
        testModeling(match);
    }

    return 0;
}
