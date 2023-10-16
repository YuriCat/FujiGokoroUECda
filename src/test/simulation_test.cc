
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

int testSimulation(const MatchRecord& match, PlayerModel *pmodel = nullptr) {
    // 棋譜を読んでシミュレーションを行う
    if (pmodel != nullptr) shared.playerModel = *pmodel;
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
            int n = max(1.0, 10 / sqrt(match.games.size() / 100.0));
            for (int j = 0; j < n; j++) {
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

bool SimulationTest(const vector<string>& recordFiles, PlayerModel *pmodel) {
    shared.baseChangePolicy.bin(DIRECTORY_PARAMS_IN + "change_policy.bin");
    shared.basePlayPolicy.bin(DIRECTORY_PARAMS_IN + "play_policy.bin");

    for (string rf : recordFiles) {
        MatchRecord mrecord(rf);
        testSimulation(mrecord);
        if (pmodel->trained) testSimulation(mrecord, pmodel);
    }
    return 0;
}