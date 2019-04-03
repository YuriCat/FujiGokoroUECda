/*
 wdi_server.cc
 Katsuki Ohto
 */

/////////////////////////////////

// wdiプロトコル大富豪サーバー(UECda版)

/////////////////////////////////

#include "../include.h"
#include "../structure/field/ServerField.hpp"

#include "WDI.hpp"

namespace UECda{
	class WDIServer : public WDI::Server<N_PLAYERS>{
	private:
		ServerField f;
		XorShift64 dice;
		
		//Log< GameLog< PlayLog<PLASUU>>> server_log;
		
	public:
        virtual int recvJoin(const std::string& name, int *pn) {
			// 参加受け入れ
			const int newNum = f.np;
			
			*pn = newNum; // プレーヤー番号設定
			
			f.setPlayerName(newNum, name);
			f.np++;
			
			if (f.np == N_PLAYERS) { // 定員
				return 1;
			}
			return 0;
		}
		// 試合進行
		virtual int recvChange(const Cards& ac) {
			// 交換がプレーヤー番号通りに行われる事が保証されているとする
			if (f.phase.isInitGame()) { return -1; } // 交換がないゲーム
			if (!f.phase.isInChange()) { return -1; } // 交換中でない
			
			int p = f.getChangeTurnPlayer();
			if (!f.isChanging(p)) { return -1; } // このプレーヤーの交換中ではない
			f.procChange(ac);
			f.changing.reset(p);
			
			if ( f.getNChanging()==0 ) {//全員の交換が揃った
				
				//server_log.thisGameLog->setChange(f);//ログに記録
				
				for (int p = 0; p < f.np; ++p) {
#ifdef BROADCAST
					cerr << "change cards" << p << "->"
					<<f.getClassPlayer( getAiteClass(f.np,f.getPlayerClass(p)) )
					<<f.changeCards[p]<<endl;
#endif
				}
				//全員に通達
				for (int p=0;p<f.np;++p) {
					int paite=f.getClassPlayer( getAiteClass( f.np, f.getPlayerClass(p) ) );
					sendChangeCards( p, f.changeCards[paite] );
				}
				f.initPlayPhase();
				
				return f.turn();
			} else {
				return f.getChangeTurnPlayer();
			}
		}
		virtual int recvPlay(const Move& amv,uint64_t at) {
			if ( !f.phase.isInPlay() ) {return -1;}//プレー中でない
			uint32_t tp=f.turn();
			
#ifdef BROADCAST
			f.broadcastBP();
#endif
			f.addUsedTime(tp,at);
			
			if ( f.bd.isNF() && amv.isPASS() ) {
				assert(0);
			}
			
			//server_log.thisGameLog->setBeforePlay(f);
			uint32_t next_tp=f.proc( amv );//ここで局面更新
			//server_log.thisGameLog->setPlay( amv, at );
			//server_log.thisGameLog->setAfterPlay(f);
	
			
#ifdef BROADCAST
			f.broadcastPlay(tp,amv);
#endif
			
			CERR<<"next tp "<<next_tp<<endl;
			
			//着手を全員に通達
			for (int i=0;i<f.np;++i) {
				if ( sendPlay(i,amv)!=0 ) {return -1;}
			}
			
			if ( next_tp==uint32_t(-1) ) {//ゲーム終了
				//この試合の結果と途中経過を流す					
				int max_len=0;
				for (int p=0;p<f.np;++p) {
					int tlen=strlen(f.name[p])+1;
					if ( tlen > max_len ) {max_len=tlen;}
				}
				cerr<<"game "<<f.gameNum<<endl;
				const std::string class_str[4]={"daifugo  ","fugo     ","hinmin   ","daihinmin",};
				for (int c=0;c<f.np;++c) {
					int p=f.getNewClassPlayer(c);
					cerr<<class_str[c]<<"  "<<p<<"."<<f.name[p]<<Space(max_len - strlen(f.name[p]) + 1);
					if ( f.ps.isDownfall(p) ) {
						cerr<<"downfall";
					}else if ( f.ps.isForbidden(p) ) {
						cerr<<"forbidden";
					}
					cerr<<endl;
				}
				
				int ret=f.close1G();
				//server_log.thisGameLog->setAfterGame(f);
				
				//プレーヤー解析
				//analyzePlayerModel(&f,&server_log);
				
				//server_log.close1G();
				
				for (int p=0;p<f.np;++p) {
					cerr<<p<<"."<<f.name[p]<<Space(max_len - strlen(f.name[p]) + 1)<<f.score[p]<<" pts. "<<f.used_time_mics[p]<<" micsec."<<endl;
				}
				if ( ret == -1 ) {//全ゲーム終了
					f.closeAllG();
					//server_log.closeAllG();
					return -2;
				} else {
					//次のゲームへ
					f.init1G();
					//server_log.init1G(f.getGameNum());
					
					//席替え
					if ( f.getGameNum()>0 && f.getGameNum()%4==0 ) {
						f.shuffleSeats(&dice);
						int s[ServerField::N];
						for (int p=0;p<f.np;++p) {
							s[p]=f.getPlayerSeat(p);
						}
						for (int p=0;p<f.np;++p) {
							if ( sendSeats(p,s)==-1 ) {return -1;}
						}
					}
					
                    if (f.firstPrmOrder == ORDER_REVERSED) {
                        for (int p=0;p<f.np;++p) {
                            sendRev(p);
                        }
                    }
#ifdef BROADCAST
					f.broadcastB1G();
#endif
					f.dealCards(&dice);//カードを配る
					//カード送信
					for (int i=0;i<f.np;++i) {
						if ( sendDealtCards(i,f.dealtCards[i])==-1 ) {return -1;}
					}
					if ( f.isChangeGame() ) {
						f.initChangePhase();//交換準備
					}
					return 0;
				}
			} else {
				return (int)next_tp;
			}
		}
		virtual int recvNGames(int n) {
			f.setNGames(n);
			return 0;
		}
		virtual int recvRevMode() {
			f.firstPrmOrder=ORDER_REVERSED;
			return 0;
		}
		virtual int startGame() {
			
			//プレーヤー番号送信
			for (int i=0;i<f.np;++i) {
				if ( sendPlayerNum(i,i)!=0 ) {return -1;}
			}
			
			f.initHalf();
			//f.chooseBlindCards(&dice);
			f.init1G();
			//server_log.init1G(f.getGameNum());
			
			if ( f.firstPrmOrder==ORDER_REVERSED ) {
					for (int p=0;p<f.np;++p) {
						sendRev(p);
					}
					}
			
#ifdef BROADCAST
			f.broadcastB1G();
#endif
			
			f.dealCards(&dice);
			//カード送信
			for (int i = 0; i < f.np; ++i) {
				if (sendDealtCards(i, f.dealtCards[i]) != 0) { return -1; }
			}
			if (!f.isInitGame()) {
				f.initChangePhase(); // 交換準備
				return f.getChangeTurnPlayer();
			} else {
				f.initPlayPhase();
				return f.turn();
			}
		}
		
		int initAll() {
			std::srand((unsigned int)time(NULL));
			dice.srand((unsigned int)time(NULL) * 111);
			
			initCards();
			
			f.initAllG();
			server_log.initAllG();
		}
		
		WDIServer(WDI::DSpace *sp):
		WDI::Server<4>(sp)
		{
			initAll();
		}
		~WDIServer() {}
	};
}


int main(int argc, char* argv[]) {
	
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	
	WDI::DSpace *wdi=new UECda::WDIDSpace();
	UECda::WDIServer server(wdi);
	
	// 引数から諸々を指定
	for (int i = 1; i < argc; ++i) {
		if (strstr(argv[i], "-g")) {
			int ngames = atoi(argv[i + 1]);
			if (server.recvNGames(ngames) == -1) { return -1; }
		}
		if (strstr(argv[i], "-r")) {
			if (server.recvRevMode() == -1) { return -1; }
		}
		/*if ( strstr(argv[i],"-t") ) {
		 tl=atoi(argv[i+1]);
		 }*/
	}

	
	server.start_network_mode();
	
	return 0;
}
