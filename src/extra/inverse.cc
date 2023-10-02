#include <random>
#include "../core/field.hpp"
#include "../core/record.hpp"
#include "../core/action.hpp"
#include "../engine/estimation.hpp"

using namespace std;

struct GradientUpdator {
    static constexpr double lr = 3e-5;
    static constexpr double lr_decay = 1e-6;
    static constexpr double decay = 3e-6;
    static constexpr double stats_decay = 1e-5;
    double sum[EST_FEATURES];
    double sum2[EST_FEATURES];
    double count[EST_FEATURES];
    double totalCount;
    double lr_, decay_;

    GradientUpdator() { init(); }

    double var(int i) const { return sum2[i] / totalCount - (sum[i] / totalCount) * (sum[i] / totalCount); }
    double freq(int i) const { return count[i] / totalCount; }

    void init() {
        totalCount = 1;
        for (int j = 0; j < EST_FEATURES; j++) sum[j] = 0;
        for (int j = 0; j < EST_FEATURES; j++) sum2[j] = count[j] = 1;
        lr_ = lr;
        decay_ = decay;
    }

    void update(float *value, double *prob, int numMoves, int index, const std::vector<std::pair<int, float>> *features);
};

void GradientUpdator::update(float *value, double *prob, int num, int index, const vector<pair<int, float>> *features) {
    // 統計情報を更新
    for (int i = 0; i < num; i++) {
        for (auto f : features[i]) {
            sum[f.first] += f.second;
            sum2[f.first] += f.second * f.second;
            count[f.first] += 1;
        }
        totalCount += 1;
    }
    for (int j = 0; j < EST_FEATURES; j++) {
        sum[j] *= 1 - stats_decay;
        sum2[j] *= 1 - stats_decay;
        count[j] *= 1 - stats_decay;
    }
    totalCount *= 1 - stats_decay;

    // 選ばれた手の確率を上げ、全体の手の確率を下げる
    for (int i = 0; i < num; i++) {
        for (auto f : features[i]) {
            double diff = i == index ? (1 - prob[i]) : -prob[i];
            value[f.first] += lr_ * diff / (1e-3 + var(f.first)) * f.second;
            value[f.first] *= pow(1 - decay_, 1 / (1e-3 + freq(i)));
        }
    }
    lr_ *= 1 - lr_decay;
    decay_ *= 1 - lr_decay;
}

struct GradientUpdatorStats {
    unsigned long long count, correct;
    double ent, cent;

    GradientUpdatorStats() { clear(); }
    void clear() { count = correct = ent = cent = 0; }
    void update(double *prob, int num, int index) {
        count += 1;
        correct += (max_element(prob, prob + num) - prob) == index;
        cent = -log2(prob[index]);
        for (int i = 0; i < num; i++) ent += -prob[i] * log2(prob[i]);
    }
    void stats() {
        cerr << "acc: " << correct / (double)count
        << " (" << correct << " / " << count << ")" << " ent: " << ent / count << " cent: " << cent / count << endl;
    }
};

void updateGame(GradientUpdator *const updator, GradientUpdatorStats *const stats, const GameRecord& record, Dice *const pdice) {
    MoveInfo buf[512];
    Field field;
    field.fromRecord(record, -1, (*pdice)() % record.numPlays);

    int turn = field.turn();

    // 除外ルール
    if (field.numPlayersAlive() <= 2) return;
    int numMoves = genMove(buf, field.hand[turn].cards, field.board);

    // 他のプレーヤーにランダムに配布
    array<Cards, N_PLAYERS> randomCards;
    for (int p = 0; p < N_PLAYERS; p++) {
        randomCards[p] = p == turn ? record.orgCards[p] : field.usedCards[p];
    }
    Cards remained = field.getRemCards() - field.hand[turn].cards;

    // 自分が上位でカード交換した場合には、自分があげたカードは確定
    int partner = -1;
    int turnClass = field.classOf(turn);
    if (!field.isInitGame()) {
        if (turnClass != HEIMIN) partner = field.classPlayer(getChangePartnerClass(turnClass));
        if (turnClass < MIDDLE) {
            Cards sentCards = field.sentCards[turn];
            randomCards[partner] |= sentCards;
            remained = maskCards(remained, sentCards);
        }
    }

    IntCard candidates[N_CARDS];
    int num = 0;
    for (IntCard ic : remained) { candidates[num++] = ic; }
    for (int p = 0; p < N_PLAYERS; p++) {
        if (p == turn) continue;
        for (int i = randomCards[p].count(); i < record.numOrgCards[p]; i++) {
            int index = (*pdice)() % num;
            randomCards[p].insert(candidates[index]);
            candidates[index] = candidates[--num];
        }
    }
    for (int p = 0; p < N_PLAYERS; p++) assert(randomCards[p].count() == record.numOrgCards[p]);

    double score[2] = {0};
    vector<pair<int, float>> features[2];

    // ランダムなカードのスコアを計算
    for (int p = 0; p < N_PLAYERS; p++) {
        if (p != turn && field.isAlive(p)) {
            score[0] += inverseEstimationScore(randomCards[p], field.usedCards[p], field.sentCards[p], field.classOf(p), &features[0]);
        }
    }
    // 実際のカードのスコアを計算
    for (int p = 0; p < N_PLAYERS; p++) {
        if (p != turn && field.isAlive(p)) {
            score[1] += inverseEstimationScore(record.orgCards[p], field.usedCards[p], field.sentCards[p], field.classOf(p), &features[1]);
        }
    }

    SoftmaxSelector<double> selector(score, 2, 1);
    double prob[2];
    //cerr << prob[0] << " " << prob[1] << endl;
    for (int i = 0; i < 2; i++) prob[i] = selector.prob(i);
    // スタッツ更新
    stats->update(prob, 2, 1);
    // パラメータ更新
    updator->update(estimationTable, prob, 2, 1, features);
}

int main(int argc, char* argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    vector<string> recordFiles;
    GradientUpdator updator;
    GradientUpdatorStats stats;
    mt19937 mt((uint32_t)time(NULL));
    Dice dice(1);

    for (int c = 1; c < argc; c++) {
        if (!strcmp(argv[c], "-l")) {
            recordFiles.push_back(string(argv[c + 1]));
        } else if (!strcmp(argv[c], "-ld")) {
            vector<string> tmpRecordFiles = getFilePathVectorRecursively(string(argv[c + 1]), ".dat");
            recordFiles.insert(recordFiles.end(), tmpRecordFiles.begin(), tmpRecordFiles.end());
        }
    }

    Record record(recordFiles);
    int games = record.games();
    record.shuffle(mt);

    for (int i = 0; i < 1000000; i++) {
        updateGame(&updator, &stats, record.rgame(i % games), &dice);
        if ((i + 1) % 10000 == 0) {
            stats.stats();
            stats.clear();
        }
    }

    ofstream ofs("param.bin", ios::out | ios::binary);
    ofs.write(reinterpret_cast<char*>(estimationTable), EST_FEATURES * 4);

    return 0;
}