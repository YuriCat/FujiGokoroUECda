/*connection*/

// daihinmin.cより必要な関数を追加、一部書き換え

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#else

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#endif

#include "connection.h"

#include "settings.h"

#define PROTOCOL_VERSION    20070       //プロトコルヴァージョンを表す整数
#define DEFAULT_SERVER      "127.0.0.1" //デフォルトのサーバのアドレス 文字列で指定
#define DEFAULT_PORT        42485       //デフォルトのポート番号 整数で指定
#define DEFAULT_NAME        MY_NAME     //デフォルトのクライアント名 文字列で指定

//using protocol_int_t = int; // デフォルトの通信設定
using protocol_int_t = int32_t; // intの長さが矛盾する場合にはこちらを設定

const int g_logging = 0;

/* 静的関数の関数プロトタイプ宣言 */
static int refreshTable(int table_val[8][15]);
static int sendTable(int table_val[8][15]);
static int openSocket(const char ip_addr[], uint16_t portnum_data);
static int sendProfile(const char user[15]);

/*connection内大域変数*/

//ソケット関連の変数を静的グローバル変数として宣言
static int g_sockfd;
static int g_buf_len;
static struct sockaddr_in g_client_addr;

//接続するサーバ、ポートを格納する変数
static char     server_name[256]= DEFAULT_SERVER;
static uint16_t port            = DEFAULT_PORT;
//サーバに通知するクライアント名
static char     user_name[15]   = DEFAULT_NAME;

//テーブルを受信した回数をカウント
static int table_count=0;

//daihinmin.cより
void outputTable(int table[8][15]){
    int i,j;
    for(i=0;i<8;i++){
        for(j=0;j<15;j++){
            printf("%i ",table[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}


//hostに接続し ゲームに参加する プレーヤー番号を返す
int entryToGame(void){
    protocol_int_t my_playernum;  //プレイヤー番号を記憶する
#if defined(_WIN32)
    // Windows 独自の設定
    WSADATA data;
    if(SOCKET_ERROR == WSAStartup(MAKEWORD(2, 0), &data)){
        printf("failed to initialize WSA-data.\n");
        exit(1);
    }
#endif // _WIN32
    //サーバに対してソケットを用意し、connectする
    if((openSocket(server_name,port))== -1){
        printf("failed to open socket to server[%s:%d].\n",server_name,port);
        exit (1);
    }
    if(g_logging==1){
        printf("connectiong to server was finished successfully[%s:%d].\n",server_name,port);
    }
    
    sendProfile(user_name);     //クライアントの情報を送信
    
    if(g_logging==1){
        printf("send profile .\n");
    }
    //自身のプレイヤー番号をサーバからもらう
    if(recv(g_sockfd, &my_playernum, sizeof(protocol_int_t), 0) > 0){
        my_playernum=ntohl(my_playernum);
        if(g_logging==1){
            printf("my player number is %d\n",my_playernum);
        }
    }
    else{
        printf("failed to get player number.\n");
        exit (1);
    }
    
    return my_playernum; //自身のプレイヤー番号を返す。
}


//ソケットのcloseを行う 成功した場合は 0 を返す。エラーが発生した場合は -1 を返す
int closeSocket(){
    int err;
#ifdef _WIN32
    err = closesocket(g_sockfd);
    WSACleanup();
#else
    err = close(g_sockfd);
#endif // _WIN32
    return err;
}

//シャッフルされたカードを受け取る 何回目のゲームかを返す
int startGame(int table[8][15]){
    static int game_count=0;
    //ログ取りが有効ならゲーム番号を表示
    if(g_logging == 1){
        printf("game number is %d.\n",game_count);
    }
    table_count=0;// ゲーム開始と同時にテーブルのカウントを0にする。
    //初回のカードを受け取る
    if(((refreshTable(table)))== -1) {
        printf("failed to get initial cards table for exchange.\n");
        exit(1) ;
    }
    
    //ログ取りが有効なら、配布されたカード(というかテーブル)を表示
    if(g_logging ==1){
        printf("initial cards table is follow.\n");
        outputTable(table);
    }
    
    game_count++;
    
    return game_count;
}

//カード交換時のカードの提出
void sendChangingCards(int cards[8][15]){
    if(sendTable(cards)==-1){
        //失敗したらエラーをはいて停止
        printf("sending cards table was failed.\n");
        exit (1);
    }
    else{
        //成功時はlogginフラグが立っていたら、メッセージを吐く
        if(g_logging==1){
            printf("sending cards-table was done successfully.\n");
        }
    }
    //logginフラグが立っていたら、テーブルの内容を出力
    if(g_logging == 1){
        //テーブルの内容を出力
        printf("sent card table was......\n");
        outputTable(cards);
    }
}

//カードを受け取り 自分のターンであれば1を返す
int receiveCards(int cards[8][15]){
    //カードを受け取る
    if(((refreshTable(cards)))== -1 ){
        //失敗した場合のエラー処理
        printf("failed to get my cards table.\n");
        exit (1);
    }
    else{
        //成功時 loggingフラグに応じログの表示
        if(g_logging == 1){
            printf("recieved my cards table.\n");
            printf("  table count=%d\n",table_count);
            outputTable(cards);
        }
    }
    //getState(cards);   //場の状態の読み出し
    return cards[5][2];
}

//カードを提出し受理されたか否かを返す
int sendCards(int cards[8][15]){
    protocol_int_t accept_flag; //提出したカードが受理されたかを判別する変数
    //カードを送信する
    if(g_logging==1){
        printf("send card table was following.\n");
        outputTable(cards);
    }
    
    if((sendTable(cards)) == -1 ){ //カードを送信し 失敗時はメッセージを表示
        printf("failed to send card sending table. \n");
        exit(1);
    }
    
    //accept_flagをサーバから受け取る
    if((recv(g_sockfd, &accept_flag, sizeof(protocol_int_t), 0)) < 0 ){
        printf("failed to get accept sign.\n");
        exit(1);
    }
    else{
        if(g_logging==1){
            printf("accept_flag=%d\n",(int)ntohl(accept_flag));
        }
    }
    return ntohl(accept_flag);
}

//ラウンドの最後にゲームか終ったかを サーバから受けとり その値を返す。
int beGameEnd(void){
    protocol_int_t one_gameend_flag;
    //ゲーム終了のフラグを受け取る
    if ((recv(g_sockfd, &one_gameend_flag, sizeof(protocol_int_t), 0)) < 0 ){
        //受信失敗時 メッセージを表示し停止
        printf("failed to check if the game was finished.\n");
        exit(1);
    }else{
        //受信成功時 値のバイトオーダーを直す
        one_gameend_flag=ntohl( one_gameend_flag);
    }
    return one_gameend_flag;
}

void lookField(int cards[8][15]){
    if(((refreshTable(cards)))== -1 ){
        //失敗した場合のエラー処理
        printf("failed to get result table.\n");
        exit (1);
    }else{
        //成功時ログを表示
        if(g_logging == 1){
            printf("received result table.\n");
            outputTable(cards);
            printf("end bacards\n");
        }
    }
    //getField(cards); //場に出ているカードの情報を読み出す
}

void checkArg(int argc,char* argv[]){
    /*
     渡されたコマンドラインから与えられた引数の情報を解析し、必要に応じて
     サーバアドレス、ポート番号、クライアント名を変更する。
     */
    const char Option[]="[-h server_adress] [-p port] [-n user_name]";
    int        arg_count=1;
    
    while(arg_count<argc){
        if( strcmp(argv[arg_count],"--help")==0){
            printf("usage : %s %s\n",argv[0],Option);
            exit(0);
        }else if (strcmp(argv[arg_count],"-h")==0){
            arg_count++;
            if (arg_count<argc){
                strcpy(server_name,argv[arg_count]);
            }else{
                printf("%s -h hostname\n",argv[0]);
                exit(1);
            }
        }else if ((strcmp(argv[arg_count],"-p"))==0){
            arg_count++;
            if (arg_count<argc){
                port = (uint16_t)atoi(argv[arg_count]);
            }else{
                printf("%s -p port\n",argv[0]);
                exit(1);
            }
        }else if ((strcmp(argv[arg_count],"-n"))==0){
            arg_count++;
            if (arg_count<argc){
                strcpy(user_name ,argv[arg_count]);
            }else{
                printf("%s -n user_name\n",argv[0]);
                exit(1);
            }
        }else{
            //Ohto : オプション追加のため以下をコメントアウト
            /*printf("%s : unknown option : %s \n",argv[0],argv[arg_count]);
            printf("usage : %s %s\n",argv[0],Option);
            exit(1);*/
        }
        arg_count++;
    }
}

//サーバからテーブル情報を受け取り、成功なら0失敗なら-1を返す
static int refreshTable(int table_val[8][15]){
    protocol_int_t net_table[8][15];
    if ((g_buf_len = recv(g_sockfd,net_table, 8 * 15 * sizeof(protocol_int_t), 0)) > 0){
        int i,j;
        //全てのテーブルの要素をネットワークオーダーからホストオーダーに変換
        for(i=0;i<8;i++)
            for(j=0;j<15;j++)
                table_val[i][j]=ntohl(net_table[i][j]);
        table_count++;
        return 0;
    }
    else{
        return(-1);
    }
}

//サーバにテーブル情報を投げる関数。成功なら0　失敗時-1を返す
static int sendTable(int table_val[8][15]){
    protocol_int_t net_table[8][15];
    int i,j;
    //全てのテーブルの要素をホストオーダーからネットワークオーダーに変換
    for(i=0;i<8;i++)
        for(j=0;j<15;j++)
            net_table[i][j]=htonl(table_val[i][j]);
    //変換したテーブルを送信
    if((g_buf_len = send(g_sockfd, net_table, 8 * 15 * sizeof(protocol_int_t), 0))> 0){
        return (0);
    }
    else{
        return (-1);
    }
}

//ソケットの設定・接続を行う 成功時0、失敗時-1を返す
static int openSocket(const char addr[], uint16_t port_num){
    //ソケットの生成
    if ((g_sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        return(-1);
    }
    
    /* ポートとアドレスの設定 */
    memset(&g_client_addr,0,sizeof(g_client_addr));
    g_client_addr.sin_family = PF_INET;
    g_client_addr.sin_port = htons(port_num);
    g_client_addr.sin_addr.s_addr = inet_addr(addr);
    
    //IPアドレスで指定されていないとき、ホスト名の解決を試みる
    if (g_client_addr.sin_addr.s_addr == 0xffffffff) {
        struct hostent *host;
        host = gethostbyname(addr);
        if (host == NULL) {
            printf("failed to gethostbyname() : %s.\n",addr);
            return -1;//ホスト名解決に失敗したとき、-1を返す
        }
        g_client_addr.sin_addr.s_addr=
        *(unsigned int *)host->h_addr_list[0];
    }
    
    /* サーバにコネクトする */
    if (connect(g_sockfd,(struct sockaddr *)&g_client_addr, sizeof(g_client_addr)) == 0){
        return 0;
    }
    return -1;
}

//クライアントの情報を送信
static int sendProfile(const char user[15]){
    int profile[8][15];
    int i;
    
    memset(&profile,0,sizeof(profile));        //テーブルを0でうめる
    profile[0][0]=PROTOCOL_VERSION;                   //2007年度版を宣言
    for(i=0;i<15;i++) profile[1][i]=(int)user[i];     //制作者名をテーブルに格納
    
    //送信
    if(sendTable(profile)==-1){                       //失敗したらエラーを出力し停止
        printf("sending profile table was failed.\n");
        exit (1);
    }
    
    //成功時はloggingフラグが立っていたら、メッセージとテーブルの内容を出力する
    if(g_logging==1){
        printf("sending profile was done successfully.\n");
        printf("sent profile table was......\n");
        outputTable(profile);
    }
    return 0;
}
