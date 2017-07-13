/*
 UECdaConnector.hpp
 Katsuki Ohto
 */

// UECdaプロトコルのとの変換

#include "../include.h"
#include "WDI.hpp"

namespace UECda{
    void convertClientWDItoSendTable(const std::string& msg, ){
        // クライアント側のWDIメッセージをUECdaの送信用テーブルに変換
        
    }
}


int main(int argc, char* argv[]){
	
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	
	WDI::DSpace *wdi=new UECda::WDIDSpace();
	UECda::WDIServer server(wdi);
	
	// 引数から諸々を指定
	for(int i = 1; i < argc; ++i){
		if(strstr(argv[i], "-g")){
			int ngames = atoi(argv[i + 1]);
			if(server.recvNGames(ngames) == -1){ return -1; }
		}
		if(strstr(argv[i], "-r")){
			if(server.recvRevMode() == -1){ return -1; }
		}
		/*if( strstr(argv[i],"-t") ){
		 tl=atoi(argv[i+1]);
		 }*/
	}

	
	server.start_network_mode();
	
	return 0;
}
