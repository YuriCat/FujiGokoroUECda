#include "record.hpp"

using namespace std;

namespace Recorder {
    const vector<string> commandList = {
        "//", "/*", "*/"
        "player", "game", "score", "seat", "class",
        "dealt", "changed", "original",
        "play", "result",
    };

    map<string, int> commandMap;

    bool isCommand(const string& str) {
        return commandMap.find(str) != commandMap.end();
    }
    void initCommandSet() {
        commandMap.clear();
        int cnt = 0;
        for (string command : commandList) commandMap[command] = cnt++;
    }
}

string PlayRecord::toString() const {
    ostringstream oss;
    oss << toRecordString(move) << "[" << time << "]";
    return oss.str();
}
    
int StringQueueToCardsM(queue<string>& q, Cards *const dst) {
    *dst = CARDS_NULL;
    const string& str = q.front();
    if (Recorder::isCommand(str)) {
        cerr << "com" << endl; return -1;
    }
    if (str != "{") {
        cerr << "not {" << endl; return -1;
    }
    q.pop();
    while (q.front() != "}") {
        if (!q.size()) {
            cerr << "not q size" << endl; return -1;
        }
        const string& str = q.front();
        if (Recorder::isCommand(str)) {
            cerr << "com" << endl; return -1;
        }
        IntCard ic = StringToIntCard(str);
        if (!examIntCard(ic)) {
            cerr << "bad intcard " << ic << " by " << str << endl; return -1;
        }
        dst->insert(ic);
        q.pop();
    }
    q.pop();
    return 0;
}

bool StringToMoveTimeM(const string& str, Move *const dstMove, uint64_t *const dstTime) {
    *dstMove = MOVE_PASS;
    *dstTime = 0;

    vector<string> v = split(str, " []\n");
    DERR << v << endl;

    if (v.size() <= 0) {
        DERR << "failed to read move-time : length = 0" << endl;
        return false;
    }

    *dstMove = StringToMoveM(v[0]);
    if (*dstMove == MOVE_NONE) {
        DERR << "illegal move" << endl;
        return false;
    }

    if (v.size() <= 1) {
        DERR << "failed to read move-time : length = 1" << endl;
        return true; // 時間なくてもOK
    }
    *dstTime = atoll(v[1].c_str());

    return true;
}

void GameRecord::init(int playerNum) {
    changes.clear();
    plays.clear();
    flags_.reset();
    infoClass.clear();
    infoSeat.clear();
    infoNewClass.clear();
    infoPosition.clear();

    dealtCards.fill(CARDS_NULL);
    orgCards.fill(CARDS_NULL);

    numDealtCards.fill(-1);
    numOrgCards.fill(-1);

    firstTurn = -1;
    myPlayerNum = playerNum;
}

string GameRecord::toString() const {
    ostringstream oss;

    oss << "/* " << endl;
    oss << "score " << endl;
    oss << "class ";
    for (int p = 0; p < N_PLAYERS; p++) {
        oss << classOf(p) << " ";
    }
    oss << endl;
    oss << "seat ";
    for (int p = 0; p < N_PLAYERS; p++) {
        oss << seatOf(p) << " ";
    }
    oss << endl;
    oss << "dealt ";
    for (int p = 0; p < N_PLAYERS; p++) {
        oss << tolower(dealtCards[p].toString()) << " ";
    }
    oss << endl;
    oss << "changed ";

    Cards changeCards[N_PLAYERS];
    for (int p = 0; p < N_PLAYERS; p++) changeCards[p].clear();
    for (auto change : changes) {
        changeCards[change.from] = change.cards;
    }
    for (int p = 0; p < N_PLAYERS; p++) {
        oss << tolower(changeCards[p].toString()) << " ";
    }
    oss << endl;
    oss << "original ";
    for (int p = 0; p < N_PLAYERS; p++) {
        oss << tolower(orgCards[p].toString()) << " ";
    }
    oss << endl;
    oss << "play ";
    for (auto play : plays) {
        oss << play.toString() << " ";
    }
    oss << endl;
    oss << "result ";
    for (int p = 0; p < N_PLAYERS; p++) {
        oss << newClassOf(p) << " ";
    }
    oss << endl;
    oss << "*/ " << endl;

    return oss.str();
}

#define Foo() {DERR << "unexpected command : " << q.front() << endl; goto NEXT;}
#define ToI(str) stoll(str)

struct Queue : public queue<string> {
    string pop() {
        string s = front();
        queue<string>::pop();
        return s;
    }
};

int loadMatchRecord(const string& path, MatchRecord *const pmatch) {
    Recorder::initCommandSet();

    ifstream ifs(path);
    if (!ifs) {
        cerr << "no record file " << path << endl;
        return false;
    }

    pmatch->init();
    GameRecord game;
    map<int, ChangeRecord> changeMap;
    Move lastMove;
    int startedGame = -1;
    int failedGames = 0;

    Queue q;

    // ここから読み込みループ
    cerr << "start loading record..." << endl;
    while (1) {
        while (q.size() < 1000) {
            // コマンドを行ごとに読み込み
            string str;
            if (!getline(ifs, str, '\n')) {
                q.push("//");
                break;
            }
            vector<string> v = split(str, ' ');
            for (const string& str : v) {
                if (str.size() > 0) q.push(str);
            }
        }

        const string cmd = q.front();
        q.pop();

        if (cmd == "//") break; // 全試合終了
        else if (cmd == "/*") { // game開始合図
            if (startedGame >= 0) {
                cerr << "failed to read game " << (startedGame + failedGames) << endl;
                failedGames += 1;
            }
            startedGame = pmatch->games.size();
            game.init();
            changeMap.clear();
            lastMove = MOVE_PASS;
        } else if (cmd == "*/") { // game終了合図
            // 棋譜として必要なデータが揃っているか確認
            if (!lastMove.isPASS()) game.setTerminated();
            pmatch->pushGame(game);
            startedGame = -1;
        } else if (cmd == "match") {
            if (Recorder::isCommand(q.front())) Foo();
            q.pop();
        } else if (cmd == "player") {
            for (int p = 0; p < N_PLAYERS; p++) {
                if (Recorder::isCommand(q.front())) Foo();
                pmatch->playerName[p] = q.pop();
            }
        } else if (cmd == "game") {
            if (Recorder::isCommand(q.front())) Foo();
            long long gn = atoll(q.pop().c_str());
        } else if (cmd == "score") {
            for (int p = 0; p < N_PLAYERS; p++) {
                if (Recorder::isCommand(q.front())) Foo();
            }
        } else if (cmd == "class") {
            for (int p = 0; p < N_PLAYERS; p++) {
                if (Recorder::isCommand(q.front())) Foo();
                game.setClassOf(p, ToI(q.pop()));
            }
        } else if (cmd == "seat") {
            for (int p = 0; p < N_PLAYERS; p++) {
                if (Recorder::isCommand(q.front())) Foo();
                game.setSeatOf(p, ToI(q.pop()));
            }
        } else if (cmd == "dealt") {
            for (int p = 0; p < N_PLAYERS; p++) {
                Cards c;
                if (StringQueueToCardsM(q, &c) < 0) Foo();
                game.dealtCards[p] = c;
            }
        } else if (cmd == "changed") {
            bool anyChange = false;
            auto infoClassPlayer = invert(game.infoClass);
            for (int p = 0; p < N_PLAYERS; p++) {
                Cards c;
                if (StringQueueToCardsM(q, &c) < 0) Foo();
                if (anyCards(c)) {
                    anyChange = true;
                    ChangeRecord change;
                    bool already = game.classOf(p) > MIDDLE;
                    change.set(p, infoClassPlayer[getChangePartnerClass(game.classOf(p))],
                               c.count(), c, already);
                    changeMap[-game.classOf(p)] = change;
                }
            }
            if (!anyChange) {
                game.setInitGame();
            } else {
                for (auto c : changeMap) game.pushChange(c.second);
            }
        } else if (cmd == "original") {
            for (int p = 0; p < N_PLAYERS; p++) {
                Cards c;
                if (StringQueueToCardsM(q, &c) < 0) Foo(); 
                game.orgCards[p] = c;
            }
        } else if (cmd == "play") {
            while (1) {
                Move move; uint64_t time;
                if (Recorder::isCommand(q.front())) Foo();
                if (!StringToMoveTimeM(q.pop(), &move, &time)) Foo();
                PlayRecord play;
                play.set(move, time);
                game.pushPlay(play);
                lastMove = move;
            }
        } else if (cmd == "result") {
            for (int p = 0; p < N_PLAYERS; p++) {
                if (Recorder::isCommand(q.front())) Foo();
                game.setNewClassOf(p, ToI(q.pop()));
            }
        } else q.pop(); // いかなるコマンドでもないので読み飛ばし
    NEXT:;
    }

END:
    cerr << pmatch->games.size() << " games were loaded." << endl;
    ifs.close();
    return true;
}
#undef ToI
#undef Foo

string MatchRecord::toHeaderString() const {
    // ヘッダ部分の出力
    ostringstream oss;
    oss << "player";
    for (int p = 0; p < N_PLAYERS; p++) oss << " " << playerName[p];
    oss << endl;
    return oss.str();
}
int MatchRecord::fin(string path) {
    // ファイルから読み込み
    filePath = path;
    return loadMatchRecord(filePath, this) ? 0 : -1;
}
int MatchRecord::fout(string path) {
    // ファイルに書き込み
    filePath = path;
    ofstream ofs(filePath, ios::out);
    if (!ofs) return -1;
    ofs << toHeaderString();
    for (const auto& game : games) ofs << game.toString();
    return 0;
}

int Record::fin(string path) {
    // 棋譜ファイルを1つ読み込み
    cerr << "using log file [ " << path << " ]" << endl;
    match.resize(match.size() + 1);
    int err = match.back().fin(path);
    if (err < 0) {
        cerr << "failed to read " << path << "." << endl;
        match.resize(match.size() - 1);
        return err;
    }
    // インデックスの準備
    int g = match.back().games.size();
    for (int i = 0; i < g; i++) rindex.push_back(startIndex.back() + i);
    startIndex.push_back(startIndex.back() + g);
    return 0;
}
int Record::fin(const vector<string>& paths) {
    // 棋譜ファイルを複数読み込み
    for (string path : paths) fin(path);
    cerr << matches() << " matches were loaded." << endl;
    return 0;
}

void Field::setBeforeGame(const GameRecord& game, int playerNum) {
    // 棋譜を読んでの初期設定
    initGame();
    myPlayerNum = playerNum;
    setMoveBuffer(nullptr);
    if (game.isInitGame()) setInitGame();
    infoNewClass.fill(-1);
    infoNewClassPlayer.fill(-1);
    for (int p = 0; p < N_PLAYERS; p++) {
        setClassOf(p, game.classOf(p));
        setSeatOf(p, game.seatOf(p));
        setPositionOf(p, game.positionOf(p));
    }
    phase = PHASE_INIT;
}

void Field::passPresent(const GameRecord& game, int playerNum) {
    if (phase < PHASE_INIT) setBeforeGame(game, playerNum);
    // 初期手札の設定
    for (int p = 0; p < N_PLAYERS; p++) {
        if (know(p)) {
            dealtCards[p] = game.dealtCards[p];
            setBothHand(p, dealtCards[p]);
        } else setBothHandQty(p, game.numDealtCards[p]);
    }
    // 献上の処理
    for (const auto& change : game.changes) {
        // 自分が受け取る側では無い献上を処理
        if (change.already && change.to != myPlayerNum) {
            makeChange(change.from, change.to, change.qty, change.cards, false, true);
        }
    }
    phase = PHASE_PRESENT;
}

void Field::passChange(const GameRecord& game, int playerNum) {
    if (phase < PHASE_PRESENT) passPresent(game, playerNum);
    // 交換の処理
    for (const auto& change : game.changes) {
        makeChange(change.from, change.to, change.qty, change.cards, change.already);
    }
    phase = PHASE_CHANGE;
    prepareAfterChange();
}

void Field::setAfterChange(const GameRecord& game,
                           const array<Cards, N_PLAYERS>& cards) {
    // カード交換が終わった後から棋譜を読み始める時の初期設定
    // 全体初期化はされていると仮定する
    for (int p = 0; p < N_PLAYERS; p++) {
        setBothHand(p, cards[p]);
        addAttractedPlayer(p);
    }
    myPlayerNum = -1; // 全員の手札情報を得たので必ずサーバー視点
    prepareAfterChange();
}

void Field::fromRecord(const GameRecord& game, int playerNum, int tcnt) {
    myPlayerNum = game.myPlayerNum;
    if (phase < PHASE_PRESENT) passPresent(game, playerNum);
    if (tcnt < 0) return; // tcnt < 0で交換中まで

    common.turn = common.owner = common.firstTurn = game.firstTurn; // prepareForPlayで反映するためここで
    if (phase < PHASE_CHANGE) passChange(game, playerNum);
    prepareForPlay();

    // 役提出の処理
    tcnt = min((int)game.plays.size(), tcnt);
    for (int t = 0; t < tcnt; t++) proceed(game.plays[t].move);
}