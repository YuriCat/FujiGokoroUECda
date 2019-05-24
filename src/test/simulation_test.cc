 
// シミュレーションの性能チェック

#include "../core/record.hpp"
#include "../core/action.hpp"
#include "../core/field.hpp"
#include "../engine/simulation.hpp"
#include "test.h"

using namespace std;

static SharedData shared;
static ThreadTools tools;
static Clock cl;

int testSimulation(const MatchRecord& mrecord) {
    // 棋譜を読んでシミュレーションを行う
    Field field;
    tools.init(0);
    tools.dice.srand(1);
    mt19937 dice(0);

    long long matrix[N_PLAYERS][N_PLAYERS] = {0};
    long long time = 0;

    for (int i = 0; i < mrecord.games(); i++) {
        shared.initGame();
        const auto& grecord = mrecord.game(i);
        // 対局結果
        auto newClasses = grecord.infoNewClass;
        int tc = dice() % grecord.plays.size();
        iterateGameLogAfterChange
        (field, grecord,
        [](const Field& field)->void{}, // first callback
        [&](const Field& field, Move pl, uint32_t tm)->int{ // play callback
            // この局面からシミュレーションを行う
            Field f = field;
            f.attractedPlayers.reset();
            f.addAttractedPlayer(field.turn());
            f.setMoveBuffer(tools.mbuf);
            // 一致度計測
            for (int j = 0; j < 1; j++) {
                cl.start();
                Field ff;
                copyField(f, &ff);
                cerr << "start simu" << endl;
                simulation(ff, &shared, &tools);
                time += cl.stop();
                // 実際の試合結果との一致
                cerr << ff.newClassOf(field.turn()) << endl;
                matrix[newClasses[field.turn()]][ff.newClassOf(field.turn())]++;
            }
            // 多様性計測
            if (field.turnCount() == tc) {
            }
            return 0;
        },
        [](const Field& field)->void{}); // last callback
    }
    return 0;
}

bool SimulationTest(const vector<string>& recordFiles) {

    shared.baseChangePolicy.fin(DIRECTORY_PARAMS_IN + "change_policy_param.dat");
    shared.basePlayPolicy.fin(DIRECTORY_PARAMS_IN + "play_policy_param.dat");

    for (string rf : recordFiles) {
        MatchRecord mrecord(rf);
        testSimulation(mrecord);
    }
    return 0;
}
