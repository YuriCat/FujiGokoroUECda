
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

int testSimulation(const MatchRecord& match) {
    // 棋譜を読んでシミュレーションを行う
    tools.dice.srand(1);
    mt19937 dice(0);

    long long matrix[N_PLAYERS][N_PLAYERS] = {0};
    long long time = 0;

    for (const auto& game : match.games) {
        shared.initGame();
        auto newClasses = game.infoNewClass; // 対局結果
        int tc = dice() % game.plays.size();
        Field field;
        for (Move move : PlayRoller(field, game)) {
            // この局面からシミュレーションを行う
            Field f = field;
            f.attractedPlayers.reset();
            f.addAttractedPlayer(field.turn());
            f.setMoveBuffer(tools.mbuf);
            // 一致度計測
            for (int j = 0; j < 8; j++) {
                cl.start();
                Field ff = f;
                simulation(ff, &shared, &tools);
                time += cl.stop();
                // 実際の試合結果との一致
                matrix[newClasses[field.turn()]][ff.newClassOf(field.turn())]++;
            }
            // TODO: 多様性計測
        }
    }

    double all = 0, same = 0;
    cerr << "real - simulation matrix" << endl;
    for (int p0 = 0; p0 < N_PLAYERS; p0++) {
        for (int p1 = 0; p1 < N_PLAYERS; p1++) {
            long long count = matrix[p0][p1];
            cerr << setw(6) << count << " ";
            if (p0 == p1) same += count;
            all += count;
        }
        cerr << endl;
    }
    cerr << "accuracy = " << same / all << endl;

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
