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

int StringToMoveTimeM(const string& str, Move *const dstMv, uint64_t *const dstTime) {
    *dstMv = MOVE_PASS;
    *dstTime = 0;
    
    Move mv;
    vector<string> v = split(str, " []\n");
    
    for (const string& tstr : v) DERR << tstr << ", ";
    DERR << endl;
    
    if (v.size() <= 0) {
        DERR << "failed to read move-time : length = 0" << endl;
        return -1;
    }
    mv = StringToMoveM(v[0]);

    if (mv == MOVE_NONE) {
        DERR << "illegal move" << endl;
        return -1;
    }
    *dstMv = mv;

    if (v.size() <= 1) {
        DERR << "failed to read move-time : length = 1" << endl;
        return 0;
    }
    char *end;
    *dstTime = strtol(v[1].c_str(), &end, 10);
    return 0;
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

int readMatchLogFile(const string& fName, MatchRecord *const pmLog) {
    Recorder::initCommandSet();
    
    ifstream ifs(fName);
    if (!ifs) {
        cerr << "readMatchLogFile() : no log file." << endl;
        return -1;
    }
    
    pmLog->init();
    GameRecord game;
    BitArray32<4, N_PLAYERS> infoClass, infoSeat, infoNewClass;
    map<int, ChangeRecord> changeMap;
    Move lastMove;
    
    bitset<32> flagGame, flagMatch;
    int startedGame = -1;
    int failedGames = 0;
    
    queue<string> q;
    string str;
    
    // ここから読み込みループ
    cerr << "start reading log..." << endl;
    while (1) {
        while (q.size() < 1000) {
            // コマンドを行ごとに読み込み
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
        
        DERR << "command = " << cmd << endl;
        
        if (cmd == "//") break; // 全試合終了
        else if (cmd == "/*") { // game開始合図
            if (startedGame >= 0) {
                cerr << "failed to read game " << (startedGame + failedGames) << endl;
                failedGames += 1;
            }
            startedGame = pmLog->games.size();
            game.init();
            infoClass.clear();
            infoSeat.clear();
            infoNewClass.clear();
            changeMap.clear();
            lastMove = MOVE_PASS;
            flagGame.reset();
        } else if (cmd == "*/") { // game終了合図
            // ログとして必要なデータが揃っているか確認
            if (!lastMove.isPASS()) {
                game.setTerminated();
            }
            game.infoClass = infoClass;
            game.infoSeat = infoSeat;
            game.infoNewClass = infoNewClass;
            pmLog->pushGame(game);
            startedGame = -1;
        } else if (cmd == "match") {
            const string& str = q.front();
            if (Recorder::isCommand(str)) Foo();
            q.pop();
            flagMatch.set(0);
        } else if (cmd == "player") {
            for (int p = 0; p < N_PLAYERS; p++) {
                const string& str = q.front();
                if (Recorder::isCommand(str)) Foo();
                DERR << "pname : " << str << endl;
                pmLog->playerName[p] = str;
                q.pop();
            }
            flagMatch.set(1);
        } else if (cmd == "game") {
            const string& str = q.front();
            if (Recorder::isCommand(str)) Foo();
            char *end;
            int gn = strtol(str.c_str(), &end, 10);
            DERR << "game " << gn << endl;
            q.pop();
            flagGame.set(0);
        } else if (cmd == "score") {
            for (int p = 0; p < N_PLAYERS; p++) {
                const string& str = q.front();
                if (Recorder::isCommand(str)) Foo();
                q.pop();
            }
            flagGame.set(1);
        } else if (cmd == "class") {
            for (int p = 0; p < N_PLAYERS; p++) {
                const string& str = q.front();
                if (Recorder::isCommand(str)) Foo();
                infoClass.assign(p, ToI(str));
                q.pop();
            }
            flagGame.set(2);
        } else if (cmd == "seat") {
            for (int p = 0; p < N_PLAYERS; p++) {
                const string& str = q.front();
                if (Recorder::isCommand(str)) Foo();
                infoSeat.assign(p, ToI(str));
                q.pop();
            }
            flagGame.set(3);
        } else if (cmd == "dealt") {
            for (int p = 0; p < N_PLAYERS; p++) {
                Cards c;
                if (StringQueueToCardsM(q, &c) < 0) Foo();
                game.dealtCards[p] = c;
                DERR << c << endl;
            }
            flagGame.set(4);
        } else if (cmd == "changed") {
            bool anyChange = false;
            auto infoClassPlayer = invert(infoClass);
            for (int p = 0; p < N_PLAYERS; p++) {
                Cards c;
                if (StringQueueToCardsM(q, &c) < 0) Foo();
                if (anyCards(c)) {
                    anyChange = true;
                    ChangeRecord change;
                    bool already = infoClass[p] > MIDDLE;
                    change.set(p, infoClassPlayer[getChangePartnerClass(infoClass[p])],
                               c.count(), c, already);
                    changeMap[-infoClass[p]] = change;
                }
            }
            if (!anyChange) {
                game.setInitGame();
                DERR << "init game." << endl;
            } else {
                for (auto c : changeMap) game.pushChange(c.second);
            }
            flagGame.set(5);
        } else if (cmd == "original") {
            for (int p = 0; p < N_PLAYERS; p++) {
                Cards c;
                if (StringQueueToCardsM(q, &c) < 0) Foo(); 
                game.orgCards[p] = c;
                DERR << c << endl;
            }
            flagGame.set(6);
        } else if (cmd == "play") {
            while (1) {
                Move mv; uint64_t time;
                const string& str = q.front();
                if (Recorder::isCommand(str)) Foo();
                if (StringToMoveTimeM(str, &mv, &time) < 0) Foo();
                PlayRecord play;
                play.set(mv, time);
                game.pushPlay(play);
                lastMove = mv;
                q.pop();
            }
            flagGame.set(7);
        } else if (cmd == "result") {
            for (int p = 0; p < N_PLAYERS; p++) {
                const string& str = q.front();
                if (Recorder::isCommand(str)) Foo();
                infoNewClass.assign(p, ToI(str));
                q.pop();
            }
            flagGame.set(8);
        } else q.pop(); // いかなるコマンドでもないので読み飛ばし
    NEXT:;
    }
END:;
    
    cerr << pmLog->games.size() << " games were loaded." << endl;
    ifs.close();
    return 0;
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
int MatchRecord::fin(std::string path) {
    // ファイルから読み込み
    filePath = path;
    return readMatchLogFile(filePath, this);
}
int MatchRecord::fout(std::string path) {
    // ファイルに書き込み
    filePath = path;
    std::ofstream ofs(filePath, std::ios::out);
    if (!ofs) return -1;
    ofs << toHeaderString();
    for (const auto& game : games) ofs << game.toString();
    return 0;
}

int Record::fin(std::string path) {
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
int Record::fin(const std::vector<std::string>& paths) {
    // 棋譜ファイルを複数読み込み
    for (std::string path : paths) fin(path);
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
    if (tcnt < 0) { // tcnt < 0で交換中まで
        if (phase < PHASE_PRESENT) passPresent(game, playerNum);
        return;
    }
    if (phase < PHASE_CHANGE) passChange(game, playerNum);
    // 役提出の処理
    common.turn = common.owner = common.firstTurn = game.firstTurn;
    tcnt = min((int)game.plays.size(), tcnt);
    for (int t = 0; t < tcnt; t++) proceed(game.plays[t].move);
}