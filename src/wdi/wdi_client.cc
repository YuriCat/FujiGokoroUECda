/*
 wdi_client.cc
 Katsuki Ohto
 */

/////////////////////////////////

// 大富豪クライアント
// 藤心(FujiGokoro)
// WDI通信クライアント(UECda版)

/////////////////////////////////

#include "../include.h"

std::string DIRECTORY_PARAMS_IN(""), DIRECTORY_PARAMS_OUT(""), DIRECTORY_LOGS("");

#include "../fuji/FujiGokoro.hpp"
#include "wdi.hpp"

UECda::Fuji::Client client;

using namespace std;
using namespace UECda::Fuji;

namespace UECda{
	class WDIClient : public WDI::Client{
	private:
		
		ClientField gameField; // このゲームの開始時点での情報
		ClientField fSaved[16]; // 保存する情報
		ClientField f; // 今いじる情報
		int ffirst;
		int fnow;
		int flast;
		// WDIが指定するプレーヤー番号と、自分が使うプレーヤー番号の間の変換
		
	public:
		// オーバーロード関数
		virtual int undo() { // 局面をコマンド1つ分戻す
			if (fnow <= ffirst) {
				return -1; // もう戻せない
			}
			--fnow;
			f = fSaved[fnow % 16];
#ifdef BROADCAST
			f.broadcastBP();
#endif
			return 0;
		}
		virtual int redo() { // 局面をコマンド1つ分進める
			if (fnow >= flast) {
				return -1; // もう進められない
			}
			++fnow;
			f = fSaved[fnow % 16];
#ifdef BROADCAST
            f.broadcastBP();
#endif
			return 0;
		}
		virtual int rev() { // 局面の永続オーダーを強引に反転させる
			f.bd.flipPrmOrder();
			save();
			return 0;
		}
		virtual int recvSeats(const int* as) {
			for (int p = 0; p < N_PLAYERS; ++p) {
				f.setPlayerSeat(p, as[p]);
			}
			return 0;
		}
		/*virtual int recvClasses(const int* acl) {
			for (int p = 0; p < ClientField::N; ++p) {
				cerr << p << "," << acl[p] << " ";
				f.infoClass.assign(p, acl[p]);
				f.infoClass.assign(N_PLAYERS + acl[p], p);
			}
			f.phase.setChangeGame();
			f.setMyClass(f.getPlayerClass(f.getMyPlayerNum()));
			return 0;
		}*/
		virtual int recvDealtCards(const Cards& ac) {
            
            if ((int)countCards(ac) != f.infoNDealtCards[f.getMyPlayerNum()]) {
                cerr << ac << "(" << (int)countCards(ac) << ")";
                cerr << f.infoNDealtCards[f.getMyPlayerNum()] << endl;
                cerr << "ClientField : illegal dealt cards." << endl;
                return -1;
            }
            
			if (f.setMyDealtCards(ac) == -1) {
				return -1;
			}
			if (f.setMyCards(ac) == -1) {
				return -1;
			}
            
            // もしD3を持っている場合にはFTP確定
            if (containsD3(f.getMyCards())) {
                if (f.setFirstTurnPlayer(f.getMyPlayerNum()) != 0) {
                    return -1;
                }
                f.setTurnPlayer(f.getMyPlayerNum());
                f.phase.setInPlay(); // プレー中フラグを立てる
            }
            f.infoNCards = f.infoNDealtCards;
            
#ifdef BROADCAST
            f.broadcastBP();
#endif
            
			if (!f.isInitGame()) {
				CERR << "start change phase" << endl;
				//f.initChangePhase();
				CERR << "next command... cc" << endl;
			} else {
				CERR << "start init play" << endl;
				//f.initPlayPhase();
                
                client.replaceField(f);
				client.prepareForGame();
                f = client.field;
                
				CERR << "end init play" << endl;
				
				CERR << "next command... " << f.turn() << "'s play " << endl;
			}
			save();
			return 0;
		}
		virtual int recvChange(const Cards& ac) {
			if (f.setMyRecvCards(ac) == -1) {
				return -1;
			}
			//f.initPlayPhase();
            
            client.replaceField(f);
            client.prepareForGame();
            f = client.field;
            
			CERR << "next command... " << f.turn() << "'s play " << endl;
			
			save();
			return 0;
		}
		virtual int recvPlay(const Move& amv, const uint64_t at) {
			if (f.isInChange()) {
				// 交換中
				return -1;
			}else if (!f.isInPlay()) {
				// プレー中でない
				return -1;
			}
			uint32_t tp = f.turn();
			
			if (!isSbjValid(f.bd, amv, f.getRemCards(), f.getNCards(tp))) {
				// 非合法手。入力モードでミスがあった場合など
				cerr << "UECda::WDIClient : subjectively illegal move." << endl;
				return -1;
			}
			
#ifdef LOGGING
			//game_log.thisGameLog->setBeforePlay(f);
#endif
			uint32_t next_tp = f.proc(amv);
			
#ifdef LOGGING
			//game_log.thisGameLog->setPlay(amv, at);
			//game_log.thisGameLog->setAfterPlay(f);
#endif
			CERR << "next_tp " << next_tp << endl;
			
#ifdef BROADCAST
			f.broadcastPlay(tp, amv); // 実況
#endif
			if (tp == f.getMyPlayerNum()) {
                client.replaceField(f);
				client.afterMyPlay();
                f = client.field;
			} else {
                client.replaceField(f);
				client.afterOthersPlay();
                f = client.field;
			}
			
			if (next_tp == (uint32_t)(-1)) { // 試合終了
				close1G();
				CERR << "game end." << endl;
				init1G();
				
#ifdef BROADCAST
				f.broadcastB1G(); // 実況
#endif
				if (!isNetWorkMode()) {
					CERR << "next command... chbc seats dc" << endl;
				}
			} else {
				if (!isNetWorkMode()) {
#ifdef BROADCAST
                    f.broadcastBP();
#endif
					CERR << "next command... " << next_tp << "'s play ";
					if (!f.ps.isAlive(f.getMyPlayerNum())) {
						CERR << "res ";
					}
					CERR << endl;
				}
			}
			save();
			return 0;
		}
		virtual int recvAllPass() {
			// 流れる、または自分の手番が来るまで全員パス
			if (f.isInChange()) {
				// 交換中
				return -1;
			}else if (!f.isInPlay()) {
				// プレー中でない
				return -1;
			}
			if (f.bd.isNF()) { return -1; } // 空場からの全員パスは考慮しない
			uint32_t tp = f.turn();
			while (tp != f.getMyPlayerNum()) {
				
#ifdef LOGGING
				//game_log.thisGameLog->setBeforePlay(f);
#endif
				tp = f.proc(MOVE_PASS);
#ifdef LOGGING
				//game_log.thisGameLog->setPlay(MOVE_PASS, 0);
				//game_log.thisGameLog->setAfterPlay(f);
#endif			
				if (f.bd.isNF()) {break;}
			}
#ifdef BROADCAS
			f.broadcastAP();
#endif
			CERR << "next command... " << f.turn() << "'s play ";
			if (!f.ps.isAlive(f.getMyPlayerNum())) {
				CERR << "res ";
			}
			CERR << endl;
			
			save();
			return 0;
		}
        virtual int recvFirstTurnPlayer(int p) {
            if (f.setFirstTurnPlayer(p) != 0) {
                return -1;
            }
            f.setTurnPlayer(p);
            f.phase.setInPlay(); // プレー中フラグを立てる
            save();
            return 0;
        }
        
		virtual int go() {
			// 自分が何かするべき状況か考える
			if (f.isInChange() && 0 /*f.isMyChangeReq()*/) {
				CERR << "my change turn." << endl;
                
                client.replaceField(f);
				Cards c = client.change(N_CHANGE_CARDS(f.getPlayerClass(f.getMyPlayerNum())));
                f = client.field;
                
				f.setMySentCards(c);
				if (sendChangeCards(c) == -1) {
					return -1;
				}
				if (!isNetWorkMode()) {
#ifdef BROADCAST
                    f.broadcastBP();
#endif
				}
				return 1;
			}else if (f.isInPlay() && f.turn() == f.getMyPlayerNum()) {
				CERR << "my play turn." << endl;
#ifdef BROADCAST
                f.broadcastB1G(); // 実況
				f.broadcastBP();
#endif
				//CERR << f.getMyCards() << endl;
				//CERR << f.getRemCards() << endl;
                
                client.replaceField(f);
				move_t mv = client.play();
                f = client.field;
                
				//CERR<<mv<<endl;
				if (sendPlay(mv) == -1) {
					CERR << "illegal play" << endl;
					return -1;
				}
				if (!isNetWorkMode()) {
#ifdef BROADCAST
                    f.broadcastBP();
#endif
				}
				return 1;
			} else {
				// 自分が何かする状況ではない
				CERR << "not my turn." << endl;
				return 0;
			}
		}
		virtual int getMyName(string *const str) {
			*str = string(MY_NAME) + string(MY_VERSION);
            return 0;
		}
		virtual int recvMyPlayerNumber(int n) {
			if (isNetWorkMode() || f.getMyPlayerNum() == 0) {
				CERR << "MyPlayerNum..." << n << endl;
			}
			init1G();
			if (f.setMyPlayerNum(n) == -1) {
				return -1;
			}
			
			if (isNetWorkMode()) {
#ifdef BROADCAST
                f.broadcastBAllG();
#endif
			}
			
			CERR << "next command... chbc seats dc" << endl;
			
			save();
			return 0;
		}
		/*virtual int recvNewClass(int ap, int acl) {
			// 観戦によって結果を報告する
			if (f.isInChange()) {
				// 交換中
				return -1;
			}else if (!f.isInPlay()) {
				// プレー中でない
				return -1;
			}
			if (!f.ps.isAlive(ap)) {
				cerr << "WDIClient : is not alive" << endl;
				return -1;
			}
			if (acl != f.ps.getBestClass()) {
				return -1;
			}
			//if ( !isNetWorkMode() || f.getMyPlayerNum()==0 ) { CERR<<"watching player "<<ap<<" class "<<acl<<endl; }
			if (f.setPlayerNewClass(ap, acl) == -1) {
				return -1;
			}
			f.ps.setAgari(ap);

			if (!isNetWorkMode()) {
				f.print();
			}
			
			if (f.ps.isSoloAlive()) {
				//最後の1人のあがり処理
				f.settleGameResult();
				close1G();
				CERR << "game end." << endl;
				init1G();
#ifdef BROADCAST
				f.broadcastB1G();//実況
#endif
				CERR << "next command... seats dc" << endl;
			} else {
				CERR << "next command... res" << endl;
			}
			save();
			return 0;
		}*/
		virtual int initAll() {
            CERR << "WDIClient::initAll()" << endl;
			initCards();
			//initHash();
			f.initAllG();
#ifdef LOGGING
			//game_log.initAllG();
#endif
            client.replaceField(f);
            client.initAllG();
            f = client.field;
			
			ffirst = 0;
			fnow = -1;
			flast = -1;
			
			save();
			
			return 0;
		}
		virtual int closeAll() {
            
            client.replaceField(f);
			client.closeAllG();
            f = client.field;
			f.closeAllG();
#ifdef LOGGING
			//game_log.closeAllG();
#endif
			return 0;
		}
		virtual int printState() {
			// コマンド1つ入力するごとに局面情報を出力
			return 0;
		}
		
		
		// 自分用関数
		int save() {
			// fをセーブ
			//CERR<<"SAVED "<<fnow+1<<endl;
			++fnow;
			fSaved[fnow % 16] = f;
			if (fnow-16 > ffirst) {
				// ffirstが消えた
				++ffirst;
			}
			if (fnow >= flast) {
				flast = fnow;
			}
			
			// 局面ログ
			// その試合のそこまでのデータを読み込む
			
			return 0;
		}
		int close1G() {
            
            client.replaceField(f);
			client.close1G();
            f = client.field;
			f.close1G();
			
#ifdef LOGGING
			//game_log.thisGameLog->setAfterGame(f);
			//game_log.close1G();
#endif
			return 0;
		}
		int init1G() {
            CERR << "WDIClient::init1G()" << endl;
			f.init1G();
#ifdef LOGGING
			//game_log.init1G(f.getGameNum());
#endif
            client.replaceField(f);
			client.init1G();
            f = client.field;
            
            f.setInitGame();
            
            // 配布枚数はプレーヤー番号に従って設定
            f.infoNDealtCards.clear();
            for (int c = 0; c < N_CARDS; ++c) {
                f.infoNDealtCards.plus(c % N_PLAYERS, 1);
            }
            int seat[N_PLAYERS];
            for (int p = 0; p < N_PLAYERS; ++p) {
                seat[p] = p;
            }
            recvSeats(seat);
            
            game_log.init();
            game_log.infoClass() = f.infoClass;
            game_log.infoSeat() = f.infoSeat;
            if (f.isInitGame()) {
                game_log.setInitGame();
            }
            
            //f.broadcastB1G(); // 実況
            
			return 0;
		}
		
		WDIClient(WDI::DSpace *sp):
		WDI::Client(sp),
		f()
		{
			initAll();
		}
		~WDIClient()
		{
			closeAll();
		}
	};
}

int main(int argc, char *argv[]) {
	
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
    
    // ファイルパスの取得
    {
        std::ifstream ifs("blauweregen_config.txt");
        if (ifs) { ifs >> DIRECTORY_PARAMS_IN; }
        if (ifs) { ifs >> DIRECTORY_PARAMS_OUT; }
        if (ifs) { ifs >> DIRECTORY_LOGS; }
    }
	
	WDI::DSpace *wdi = new UECda::WDIDSpace();
	UECda::WDIClient wdiClient(wdi);
    
    int mode = 0;
    
    for (int c = 1; c < argc; ++c) {
        if (strstr(argv[c], "-n")) {
            mode = 1;
        }else if (strstr(argv[c], "-p")) {
            mode = 2;
        }
    }
	
    if (mode == 1) {
        wdiClient.start_network_mode();
    }else if (mode == 2) {
        wdiClient.start_practice_mode();
    } else {
        wdiClient.start_input_mode();
    }
	
	return 0;
}
