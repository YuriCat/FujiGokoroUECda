/*
 daihubc.cc
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/param.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <errno.h>
#include <math.h>
#include <time.h>

#include <cstring>
#include <ctime>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <atomic>
#include <array>
#include <vector>
#include <random>
#include <algorithm>
#include <string>
#include <bitset>

#include "external.hpp"
#include "tn_protocol.hpp"

#include "statistics.h"

#include "../include.h"
#include "../engine/engineSettings.h"
#include "../engine/engineStructure.hpp"
#include "../engine/rating.hpp"

using namespace UECda;

const char* one_to_yes(int n) {
    if (n) return "YES";
    else return "NO";
}

std::string DIR_IN(""), DIR_OUT(""), DIR_LOG("");
std::string record_file = "";

MinMatchLog<MinGameLog<MinPlayLog>> match_log;
MinGameLog<MinPlayLog> game_log;
EngineSharedData shared;
EngineThreadTools threadTools[N_THREADS];

void outputLog() {
    // 棋譜書き出し
    if (record_file.size() == 0) { //ディレクトリが指定されていない時
        int record_num = 0;
        char str_file[256];
        {
            int i,iret;
            FILE *pf;
            //ログの番号を決める
            for (i = 0; i < 999; ++i) {
                snprintf(str_file, 256, std::string(DIR_LOG+"logn%d.dat").c_str(), i);
                pf = fopen(str_file, "r");
                if (pf == NULL) { break; }
                iret = fclose(pf);
                if (iret < 0) { exit(1); }
            }
            record_num = i;
        }
        
        //ログファイルのオープン
        snprintf(str_file, 256, std::string(DIR_LOG+"logn%d.dat").c_str(), record_num);
        record_file = std::string(str_file);
    }
    
    //出力
    match_log.fout(record_file);
}

unsigned int point_sum[100000][N_PLAYERS]={0};  // cards on the stage
unsigned char point[100000][N_PLAYERS]={0};  // cards on the stage

int main(int argc, char *argv[]) {
    
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    
    //cerr << "start" << endl;
    
    /* setting of valuances */
    
    int i,j,k;		      // roop
    char *dat1;		      // roop
    char *dat2;		      // roop
    char dat3[256];		      // roop
    char dat4[256];		      // roop
    
    /* controle game */
    int flag_wait_type=3;
    int graph_mode=0;
    int g_flag=0;
    int tmp;
    
    /* for initialize_windows2 */
    int accept_flag=0;
    
    /* for cards */
    int work_card[8][15]={0};       // i.e. submitted card etc...
    int stage_card[8][15]={0};       // i.e. submitted card etc...
    int old_stage_card[8][15]={0};       // i.e. submitted card etc...
    int players_card[5][8][15]={0}; // players_card
    int tmp_card[2][8][15]={0}; // players_card
    int number_of_card;	     // number_of_card temporal
    int status_of_submitted_card[4]={0}; // now card
    int status_of_stages_card[4]={-1};  // cards on the stage
    
    int sekijun[N_PLAYERS]={0}; // sekijun
    int mibun[N_PLAYERS]={0};  // cards on the stage
    int human[N_PLAYERS]={0};  // cards on the stage
    int error;		      // error check
    
    int now_number_of_games;	// save number of played games
    
    /* flags */
    int now_pass[N_PLAYERS]={0};
    int flash=0;   // flash
    int last_player;
    int now_muki;
    int now_player;  // now playing player number
    int now_kakumei; // flag of kakumei
    int now_11back;  // flag of 11 back
    int now_number_of_goal=0;  // number of person whose card is none
    int goal_flag[N_PLAYERS]={0};      // flags of person
    int now_jocker_single=0;   // flag of jocker
    int now_shibari=0;   // flag of jocker
    int number_renzoku_pass=0;       // flags of passed person
    
    /* for tcp */
    int sockfd, client_sockfd[N_PLAYERS]={0};
    int port_number=42485;
    int protocol_version=20070;
    struct sockaddr_in wait_addr; // waiting port
    struct sockaddr_in client_addr[N_PLAYERS]; // port for each clients
    socklen_t client_len[N_PLAYERS]; // waiting port
    fd_set target_fds;
    fd_set org_target_fds;
    struct timeval waitval;
    
    /* for log */
    FILE *fp,*fp2;
    char cfg_file[100]="tndhms.cfg";
    int debug=0;     // debug_flag
    
    char XPM_PATH[80]="\0";
    char XPM_CARD[100];
    char XPM_TEFUDA[100];
    char XPM_TEFUDA2[100];
    char XPM_MIBUN[100];
    
    int WINDOW_TYPE=2;
    char player_name[N_PLAYERS][15];
    
    /* for rule */
    int RULE_KAKUMEI=1;
    int RULE_SHIBARI=1;
    int RULE_KINSOKU=0;
    int RULE_CHANGE=1;
    int RULE_KAIDAN=1;
    int RULE_5TOBI=0;
    int RULE_6REVERS=0;
    int RULE_8GIRI=1;
    int RULE_11BACK=0;
    int RULE_SEKIGAE=1;
    int RULE_SEKIGAE_NUM=3;
    int RAND_TYPE=1;
    int GAME_NUMBER=1000;
    int FLASH_MIBUN_NUMBER=100;
    
    /* for player statistics */
    int count_turn;
    struct playerStatistics ps[N_PLAYERS];
    for (i=0;i<N_PLAYERS;i++) {
        memset(&ps[i], 0, sizeof(struct playerStatistics));
    }
    int sum_of_turn = 0;
    int game_count = 0;
    
    //added by OHTO
    int broadcast=0;
    const int humanplaying=0;
    const int makelog=0;
    const int yaocho4=0;
    int player_name_length[N_PLAYERS];
    int player_name_maxlength=0;
    unsigned long long changetime[N_PLAYERS]={0};
    unsigned long long playtime[N_PLAYERS]={0};
    struct timeval timevalstart,timevalstart2,timevalstop;
    int rev_mode=0;
    int output_flag=0;
    int output_player_num=0;
    int output_game_span=100;
    std::string output_file = "";
    unsigned int start_time = time(NULL);
    bool rating = false;
    std::array<double, N_PLAYERS> playerRate = {0};
    std::array<double, N_PLAYERS> playerRateMean = {0};
    std::array<double, N_PLAYERS> playerRateExpMean = {0};
    constexpr double rateEpsilon = 0.99;
    
    {
        std::ifstream ifs("blauweregen_config.txt");
        ifs >> DIR_IN;
        ifs >> DIR_OUT;
        ifs >> DIR_LOG;
    }
    //cerr << "test" << endl;
    //ファイルパスの取得
    match_log.init();
    
    FILE *logfile;
    
    /************************************/
    /*  setting initial value	    */
    /************************************/
    
    for (i=0;i<N_PLAYERS;i++) {
        sprintf(player_name[i],"Player %i",i+1);
    }
    
    /************************************/
    /*  setting for argument part1	    */
    /************************************/
    
    for (i=1;i<=argc-1;i++) {
        if (strcmp(argv[i],"-v")==0) { /* print version */
            printf("tndhms version 0.29\n");
            return 0;
        }else if ((strcmp(argv[i],"-h")==0)) { /* print help message */
            printf("tndhms [-vh] [-p port_number] \n");
            printf("   -v version\n");
            printf("   -h help\n");
            printf("   -c config_file\n");
            printf("   -p port_number\n");
            return 0;
        }else if ((strcmp(argv[i],"-c")==0)) { /* config file name */
            if (i+1<=argc-1) {
                fp=fopen(argv[i+1],"r");
                if ((strlen(argv[i+1])>=90) || fp==NULL) {
                    printf("Bad file name. \n");
                    return 0;
                } else {
                    strcpy(cfg_file,argv[i+1]);
                    printf("a use config_file is %s\n", cfg_file);
                }
                fclose(fp);
            } else {
                printf("Bad file name. \n");
                return 0;
            }
        }
    }
    
    /************************************/
    /*  setting for argument part2	    */
    /************************************/
    
    for (i=1;i<=argc-1;i++) {
        if (strcmp(argv[i],"-p")==0) {   /* port number  */
            if (i+1<=argc-1) {
                //if (isint(argv[i+1])) {
                port_number=atoi(argv[i+1]);
                printf("port number is %i\n",port_number);
                //} else {
                //	printf("bad argument\n");
                //	return 0;
                //}
            } else {
                printf("bad argument\n");
                return 0;
            }
        }else if (strcmp(argv[i],"-d")==0) { /* debug flag */
            debug=1;
        }else if (strcmp(argv[i],"-bc")==0) { /* broadcasting flag */
            broadcast=1;
        }else if (strcmp(argv[i],"-g")==0) { /* the number of games */
            GAME_NUMBER=atoi(argv[i+1]);
        }else if (strcmp(argv[i],"-o")==0) { /* output */
            output_flag=1;
            output_file=std::string(argv[i+1]);
            //output_player_num=atoi(argv[i+1]);
            //output_span=atoi(argv[i+2]);
        }else if (strcmp(argv[i],"-nc")==0) { /* no change */
            FLASH_MIBUN_NUMBER=1;
        }else if (strcmp(argv[i],"-r")==0) { /* rev mode */
            rev_mode=1;
        }else if (strcmp(argv[i],"-l")==0) { /* record file path */
            record_file=std::string(argv[i+1]);
        }else if (strcmp(argv[i],"-ra")==0) { /* rating */
            rating = true;
        }
    }
    
    if (rating) {
        // レート計算のためシミュレーションに使用するデータを準備
        shared.setMyPlayerNum(-1);
        shared.basePlayPolicy.fin(DIR_IN + "play_policy_param.dat");
        shared.baseChangePolicy.fin(DIR_IN + "change_policy_param.dat");
        // スレッドごとのデータ初期化
        for (int th = 0; th < N_THREADS; ++th) {
            threadTools[th].init(th);
        }
        XorShift64 tdice;
        tdice.srand((unsigned int)time(NULL));
        for (int th = 0; th < N_THREADS; ++th) {
            threadTools[th].dice.srand(tdice.rand() * (th + 111));
        }
    }
    
    /************************************/
    /*  print setting	            */
    /************************************/
    printf("RAND_TYPE\t=\t%i\n",RAND_TYPE);
    printf("RULE_KAKUMEI\t=\t%s\n",one_to_yes(RULE_KAKUMEI));
    printf("RULE_SHIBARI\t=\t%s\n",one_to_yes(RULE_SHIBARI));
    printf("RULE_KINSOKU\t=\t%s\n",one_to_yes(RULE_KINSOKU));
    printf("RULE_KAIDAN\t=\t%s\n",one_to_yes(RULE_KAIDAN));
    printf("RULE_CHANGE\t=\t%s\n",one_to_yes(RULE_CHANGE));
    printf("RULE_5TOBI\t=\t%s\n",one_to_yes(RULE_5TOBI));
    printf("RULE_6REVERS\t=\t%s\n",one_to_yes(RULE_6REVERS));
    printf("RULE_8GIRI\t=\t%s\n",one_to_yes(RULE_8GIRI));
    printf("RULE_11BACK\t=\t%s\n",one_to_yes(RULE_11BACK));
    printf("RULE_SEKIGAE\t=\t%s\n",one_to_yes(RULE_SEKIGAE));
    printf("RULE_SEKIGAE_NUM\t=\t%i\n",RULE_SEKIGAE_NUM);
    printf("GAME_NUMBER\t=\t%i\n",GAME_NUMBER);
    printf("FLASH_MIBUN_NUMBER\t=\t%i\n",FLASH_MIBUN_NUMBER);
    printf("GAME_PORT\t=\t%i\n",port_number);
    
    if (makelog) {
        if ((logfile = fopen("log.dat", "a")) == NULL) {
            printf("file open error!!\n");
            exit(EXIT_FAILURE);
        }
    }
    
    
    /************************************/
    /*  initialize random seed	    */
    /************************************/
    
    //srand((unsigned)time(NULL));
    tn_rand_init((unsigned long)time(NULL),RAND_TYPE);
    //tn_rand_init((unsigned long)3,RAND_TYPE);
    
    
    /********************************/
    /* setting for client/server	*/
    /*  make soket for each client  */
    /********************************/
    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("client: socket");
        exit(1);
    }
    
    //printf("my sock num : %d\n", client_sockfd[i]);
    
    memset((char *) &wait_addr, 0, sizeof(wait_addr));
    wait_addr.sin_family = PF_INET;
    wait_addr.sin_addr.s_addr = htons(INADDR_ANY);
    wait_addr.sin_port = htons(port_number);
    
    i = 1;
    j = sizeof(i);
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&i, j) < 0) {
        perror("setsockopt");
    }
    if (::bind(sockfd, (struct sockaddr *)&wait_addr, sizeof(wait_addr)) < 0) {
        perror("reader: bind");
        exit(1);
    }
    if (listen(sockfd, 1) < 0) {
        perror("reader: listen");
        close(sockfd);
        exit(1);
    }
    for (i=0;i<N_PLAYERS;i++) {
        printf("now waiting %i \n", i);
        client_len[i]=sizeof(client_addr[i]);
        if ((client_sockfd[i]=accept(sockfd,(struct sockaddr *)&client_addr[i],&client_len[i])) < 0 ) {
            perror("reader: accept");
            exit(1);
        };
        printf("sock num : %d\n",client_sockfd[i]);
        
        FD_ZERO(&org_target_fds);
        FD_SET(client_sockfd[i], &org_target_fds);
        memcpy(&target_fds, &org_target_fds, sizeof(org_target_fds));
        waitval.tv_sec  = 2;
        waitval.tv_usec = 500;
        switch(select(50,&target_fds,NULL,NULL,&waitval)) {
            case -1:
                printf("protocol_version: NONE\n");
                exit(1);
            case 0: /* time out */
                protocol_version=20060;
                printf("protocol_version: 2006a\n");
                break;
            default: /* connect from client */
                tn_card_read(client_sockfd[i], work_card , protocol_version);
                protocol_version=work_card[0][0];
                printf("protocol_version: %d\n",work_card[0][0]);
                
                player_name_length[i]=15;
                
                for (j=0;j<=13;j++) {
                    player_name[i][j]=work_card[1][j];
                    if (j<player_name_length[i] && player_name[i][j]==0) {
                        player_name[i][j]='\0';
                        player_name_length[i]=j;
                    }
                }
                if (player_name_length[i]==15) {
                    player_name[i][14]='\0';
                    player_name_length[i]=j;
                }
                
                if (player_name_length[i] > player_name_maxlength)player_name_maxlength=player_name_length[i];
                
                player_name[i][14]='\0';
                printf("NAME: %s\n", player_name[i]);
                
                match_log.setPlayer(i,player_name[i]);
                
                break;
        }
        tn_int_write(client_sockfd[i], i , protocol_version);
        printf("accepted from %s \n",inet_ntoa(client_addr[i].sin_addr));
    }
    /* end of socket setting */
    
    
    /*********************/
    /* initialize values */
    /*********************/
    
    for (i=0;i<N_PLAYERS;i++) {
        mibun[i]=i;
    }
    for (i=0;i<N_PLAYERS;i++) {
        sekijun[i]=i;
    }
    
    // player statistics
    
    for (i=0;i<N_PLAYERS;i++) {
        memset(&ps[i], 0, sizeof(struct playerStatistics));
    }
    
    /**************/
    /* game start */
    /**************/
    for (now_number_of_games=1;now_number_of_games<=GAME_NUMBER;now_number_of_games++) {
        
        game_log.init();
        
        // shuffle all cards for | N_PLAYERS | players
        memset(stage_card,0,sizeof(stage_card));
        memset(old_stage_card,0,sizeof(old_stage_card));
        memset(players_card,0,sizeof(players_card));
        memset(goal_flag,0,sizeof(goal_flag));
        
        if (RULE_SEKIGAE!=0) { // decide sekigae
            tn_sekigae(now_number_of_games,sekijun,RULE_SEKIGAE,RULE_SEKIGAE_NUM, RAND_TYPE);
            /*printf("sekigae done\n");
             for (i=0;i<=4;i++) {
             printf("-> %i\n",sekijun[i]);
             }*/
        }
        
        //seats
        for (int s = 0; s < N_PLAYERS; ++s) {
            game_log.infoSeat.assign(sekijun[s], s);
        }
        
        if (((now_number_of_games-1 )% FLASH_MIBUN_NUMBER)==0) {
            // decide first member for shuffle
            //shuffle_card(rand()%5,players_card);
            
            shuffle_card((int)(tn_rand_gen(RAND_TYPE)*N_PLAYERS),players_card, RAND_TYPE, sekijun);
            
            game_log.setInitGame();
        } else {
            shuffle_card(mibun[0],players_card , RAND_TYPE, sekijun);
        }
        
        for (int p = 0; p < N_PLAYERS; ++p) {
            Cards c = TableToCards(players_card[p]);
            game_log.setDealtCards(p, c);
        }
        
        for (i=0;i<N_PLAYERS;i++) {
            work_card[6][mibun[i]+5]=i;
            game_log.infoClass.assign(mibun[i],i);//class
        }
        
        for (i=0;i<N_PLAYERS;i++) { // initialize each table[5]
            memcpy(players_card[i][5], work_card[5], 2*sizeof(work_card[5]));
        }
        
        if (makelog) {
            for (i=0;i<N_PLAYERS;i++) {
                fprintf(logfile,"%d,",i);
                int h,w;
                for (h=0;h<5;h++) {
                    for (w=0;w<15;w++) {
                        fprintf(logfile,"%d",players_card[i][h][w]);
                    }
                }
                fprintf(logfile,"\n");
            }
        }
        
        /************************************/
        /* distribute cards to each players */
        /************************************/
        //convert status to table
        work_card[5][0] = 0;
        work_card[5][1] = 0;
        work_card[5][2] = 0;
        work_card[5][3] = 6;
        work_card[5][4] = 0;
        work_card[5][5] = 0;
        // work_card[5][6] = 0;
        work_card[5][6] = rev_mode ? 1 : 0;//最初からオーダー逆の場合
        work_card[5][7] = 0;
        for (int i = 0; i < N_PLAYERS; ++i) {
            work_card[6][i] = count_card_num_r(players_card[i], &error);
        }
        if (((now_number_of_games-1 )% FLASH_MIBUN_NUMBER)==0) {
            for (i=0;i<N_PLAYERS;i++) {
                work_card[6][i+5]=0;
            }
        } else {
            for (i=0;i<N_PLAYERS;i++) {
                work_card[6][mibun[i]+5]=i;
            }
        }
        for (i=0;i<N_PLAYERS;i++) {
            work_card[6][i+10]=sekijun[i];
        }
        for (i=0;i<N_PLAYERS;i++) {
            memcpy(players_card[i][5], work_card[5], 2*sizeof(work_card[5]));
        }
        // distribute
        
        
        // search strong card in hinmin and data setting
        if ((((now_number_of_games - 1) % FLASH_MIBUN_NUMBER) != 0) &&(RULE_CHANGE==1)) {
            for (i=0;i<N_PLAYERS;i++) {  // data setting for all player
                players_card[mibun[i]][5][0]=1;
                players_card[mibun[i]][5][1]=2-i;
            }
            memcpy(tmp_card[0], players_card[mibun[N_PLAYERS - 2]], sizeof(players_card[mibun[N_PLAYERS - 2]])); // hinmin
            memcpy(tmp_card[1], players_card[mibun[N_PLAYERS - 1]], sizeof(players_card[mibun[N_PLAYERS - 1]])); // daihinmin
            
            //search change cards
            {
                Cards c = TableToCards(players_card[mibun[N_PLAYERS - 1]]);
                c=pickHigh<2>(c);
                //cerr<<mibun[4]<<OutCards(pickHigh<2>(c))<<endl;
                game_log.push_change(MinChangeLog(mibun[N_PLAYERS - 1],mibun[0],c));
            }
            {
                Cards c = TableToCards(players_card[mibun[N_PLAYERS - 2]]);
                c=pickHigh<1>(c);
                game_log.push_change(MinChangeLog(mibun[N_PLAYERS - 2],mibun[1],c));
            }
            
            for (int cl = 0; cl <= FUGO; ++cl) { //search strong card
                int oppCl = getChangePartnerClass(cl);
                trans_strong_card(players_card[mibun[oppCl]],players_card[mibun[cl]],N_CHANGE_CARDS(cl));
            }
        } else {  // no change because of it's a 1st game
            for (i=0;i<N_PLAYERS;i++) {  // data setting for all player
                players_card[mibun[i]][5][0]=1;
                players_card[mibun[i]][5][1]=0;
            }
            memcpy(tmp_card[0], players_card[mibun[N_PLAYERS - 2]], sizeof(players_card[mibun[N_PLAYERS - 2]]));
            memcpy(tmp_card[1], players_card[mibun[N_PLAYERS - 1]], sizeof(players_card[mibun[N_PLAYERS - 1]]));
            
        }
        if (debug) {printf("hinmin and daihinmin card change is done\n");} //DEBUG
        
        int mid = (N_PLAYERS + 1) / 2;
        for (int p = 0; p < mid; ++p) {
            tn_card_write(client_sockfd[mibun[p]],players_card[mibun[p]],protocol_version); // tuuchi
        }
        //tn_card_write(client_sockfd[mibun[3]],players_card[mibun[3]],protocol_version);
        //tn_card_write(client_sockfd[mibun[4]],players_card[mibun[4]],protocol_version);
        for (int p = mid; p < N_PLAYERS; ++p) {
            tn_card_write(client_sockfd[mibun[p]],tmp_card[p - mid],protocol_version);
        }
        //printf("tought.*********************************************************************************************************");
        gettimeofday(&timevalstart,NULL);
        
        // change faise of daihugou start
        if ((((now_number_of_games - 1) % FLASH_MIBUN_NUMBER) != 0) &&(RULE_CHANGE==1)) { // change card
            
            //cerr << "change";
            
            tn_card_read(client_sockfd[mibun[0]],work_card,protocol_version); // uketori
            gettimeofday(&timevalstop,NULL);
            changetime[mibun[0]]+=(timevalstop.tv_sec-timevalstart.tv_sec)*1000000+timevalstop.tv_usec-timevalstart.tv_usec;
            error=0;
            error=count_card_num(work_card, &number_of_card); // number check
            if ((check_include_card(players_card[mibun[0]],work_card)==0)&&(number_of_card==2)&&(error==0)) {
                if (debug) {printf("change card - OK \n");}
                trans_work_card(players_card[mibun[0]],players_card[mibun[N_PLAYERS - 1]],work_card);
            } else {
                if (debug) {printf("change card - fault \n");}
                trans_strong_card(players_card[mibun[0]],players_card[mibun[N_PLAYERS - 1]],2);
            } // fi
            
            Cards c = TableToCards(work_card);
            game_log.push_change(MinChangeLog(mibun[0],mibun[N_PLAYERS - 1],c));
            
        }// fi
        if (debug) {printf("change daihugou - OK\n");} //DEBUG
        
        // change faise of hugou start
        if ((((now_number_of_games - 1) % FLASH_MIBUN_NUMBER)!= 0)&&(RULE_CHANGE==1)) { // change card
            tn_card_read(client_sockfd[mibun[1]],work_card,protocol_version); // uketori
            gettimeofday(&timevalstop,NULL);
            changetime[mibun[1]]+=(timevalstop.tv_sec-timevalstart.tv_sec)*1000000+timevalstop.tv_usec-timevalstart.tv_usec;
            error=0;
            error=count_card_num(work_card, &number_of_card); // number check
            if ((check_include_card(players_card[mibun[1]],work_card)==0)&&(number_of_card==1)) {
                if (debug) {printf("change card - OK \n");}
                trans_work_card(players_card[mibun[1]],players_card[mibun[N_PLAYERS - 2]],work_card);
            } else {
                if (debug) {printf("change card - fault \n");}
                trans_strong_card(players_card[mibun[1]],players_card[mibun[N_PLAYERS - 2]],1);
            } // fi
            
            Cards c = TableToCards(work_card);
            game_log.push_change(MinChangeLog(mibun[1], mibun[N_PLAYERS - 2], c));
            
        }// fi
        if (debug) {printf("change hugou - OK\n");} //DEBUG
        if (debug) {printf("end of distribute\n");} //DEBUG
        /* end of distribute */
        
        if (makelog) {
            for (i=0;i<N_PLAYERS;i++) {
                fprintf(logfile,"%d,",i);
                int h,w;
                for (h=0;h<5;h++) {
                    for (w=0;w<15;w++) {
                        fprintf(logfile,"%d",players_card[i][h][w]);
                    }
                }
                fprintf(logfile,"\n");
            }
        }
        
        if (yaocho4) {
            //printf("start");
            //ここでplayer4に全員の手札を教えます
            for (i=0;i<N_PLAYERS;i++) {
                //tn_card_read(client_sockfd[4],work_card,protocol_version);
                tn_card_write(client_sockfd[N_PLAYERS - 1],players_card[i],protocol_version);
                //for (j=0;j<15;j++) {printf("%d.",players_card[i][0][j]);}
                
                //printf("sent.");
            }
        }
        
        
        /****************************/
        /* each players phase start */
        /****************************/
        
        status_of_submitted_card[0]=-1;    // flash of stage
        status_of_stages_card[0]=-1;    // flash of stage
        status_of_stages_card[1]=-1;
        status_of_stages_card[2]=-1;
        status_of_stages_card[3]=-1;
        memset(stage_card,0,sizeof(stage_card));
        memset(work_card,0,sizeof(work_card));
        now_number_of_goal=0;
        now_player=search_card(players_card,2,1);    // search a "dia 3"
        last_player=now_player;
        
        now_muki=1;
        //now_kakumei=0;
        now_kakumei=rev_mode?1:0;//最初からオーダー逆の場合
        now_11back=0;
        now_jocker_single=0;   // flag of jocker
        now_shibari=0;   // flag of jocker
        number_renzoku_pass=0;
        
        flash=0;   // flash
        memset(goal_flag,0,sizeof(goal_flag));      // flags of person
        memset(now_pass,0,sizeof(now_pass));       // flags of passed person
        
        //original cards
        for (int p=0;p<N_PLAYERS;++p) {
            game_log.setOrgCards(p, TableToCards(players_card[p]));
        }
        
        // player statistics
        count_turn = 0;
        
        while(now_number_of_goal<=N_PLAYERS - 2) {
            
            //convert status to table
            work_card[5][0] = 0;
            work_card[5][1] = 0;
            work_card[5][2] = 0;
            work_card[5][3] = now_player;
            work_card[5][4] = (status_of_stages_card[0]==-1);
            work_card[5][5] = now_11back;
            work_card[5][6] = now_kakumei;
            work_card[5][7] = now_shibari;
            for (int i = 0; i < N_PLAYERS; ++i) {
                work_card[6][i] = count_card_num_r(players_card[i], &error);
            }
            if (((now_number_of_games-1 )% FLASH_MIBUN_NUMBER)==0) {
                for (i=0;i<N_PLAYERS;i++) {
                    work_card[6][i+5]=0;
                }
            } else {
                for (i=0;i<N_PLAYERS;i++) {
                    work_card[6][mibun[i]+5]=i;
                }
            }
            for (i=0;i<N_PLAYERS;i++) {
                work_card[6][i+10]=sekijun[i];
            }
            for (i=0;i<N_PLAYERS;i++) {
                memcpy(players_card[i][5], work_card[5], 2*sizeof(work_card[5]));
            }
            
            memcpy(stage_card[5], work_card[5], 2*sizeof(work_card[5]));
            players_card[now_player][5][2]=1;
            
            if (debug) {printf("To send prepared datas for each player now\n");} //DEBUG
            // prepare datas for each player
            // atode
            for (int i = 0; i < N_PLAYERS; ++i) {
                tn_card_write(client_sockfd[i],players_card[i],protocol_version);
            }
            
            //field.setTurnPlayer(now_player);
            
            //cerr<<now_player<<endl;
            //cerr<<(unsigned int)field.infoSpecialPlayer<<endl;
            
            gettimeofday(&timevalstart,NULL);
            
            if (debug) {printf("To send prepared date is done. \n");} //DEBUG
            
            if (debug) {printf("Read from player %i\n", now_player);} //DEBUG
            
            
            
            tn_card_read(client_sockfd[now_player],work_card,protocol_version);
            
            gettimeofday(&timevalstop,NULL);
            uint64_t tmpTime=(timevalstop.tv_sec-timevalstart.tv_sec)*1000000+timevalstop.tv_usec-timevalstart.tv_usec;
            playtime[now_player]+=tmpTime;
            
            Move mv = TableToMove(work_card);
            
            if (mv.isSeq() && mv.jokerRank() == 15) {
                //cerr << toString(work_card) << endl << getchar();
            }
            
            if (debug) {printf("accepted card is \n");} //DEBUG
            if (debug) {print_player_card(work_card);} //DEBUG
            if (debug) {printf("player card is \n");} //DEBUG
            if (debug) {print_player_card(players_card[now_player]);} //DEBUG
            
            //fprintf(logfile,"%d.",now_player);
            
            if (makelog) {
                int h,w;
                for (h=0;h<5;h++) {
                    for (w=0;w<15;w++) {
                        fprintf(logfile,"%d",work_card[h][w]);
                    }
                }
                for (h=5;h<8;h++) {
                    for (w=0;w<15;w++) {
                        fprintf(logfile,"%d",players_card[0][h][w]);
                    }
                }
                fprintf(logfile,"\n");
            }
            
            error=0; // error initialize
            
            error|=check_include_card(players_card[now_player],work_card);  // include check
            if (debug) {printf("error01 = %i: include check\n",error);} //DEBUG
            
            error|=analyze_card(work_card,status_of_submitted_card,0); // analyze check.  the number of joker > 2 then error and etc
            if (debug) {printf("error02 = %i: analyze check \n",error);} //DEBUG
            
            error|=compare_card_status(status_of_stages_card, status_of_submitted_card, (now_kakumei + now_11back)%2); // compare check
            if (debug) {printf("error03 = %i: compare check \n",error);} //DEBUG
            if (debug) {
                printf(
                       "stage value    %i - number %i - suit %i - type %i \n",
                       status_of_stages_card[0],status_of_stages_card[1],status_of_stages_card[2],status_of_stages_card[3]
                       ); //DEBUG
                printf(
                       "submitte value %i - number %i - suit %i - type %i \n",
                       status_of_submitted_card[0],status_of_submitted_card[1],status_of_submitted_card[2],status_of_submitted_card[3]
                       ); //DEBUG
            }
            
            count_turn++;
            if ((error==0)&&(status_of_stages_card[0]==-1)) {
                ps[now_player].getStage++;
                ps[now_player].cardStrength+=status_of_submitted_card[0];
                switch(status_of_submitted_card[3]) {
                    case 1:
                        break;
                    case 2:
                        ps[now_player].fukusuu++;
                        break;
                    case 3:
                        ps[now_player].kaidan++;
                        break;
                    case 4:
                        ps[now_player].jokerCnt++;
                        ps[now_player].jokerTurnSum+=count_turn;
                        break;
                    default:
                        break;
                }
            } else if ((error==0)) {
                switch(status_of_submitted_card[3]) {
                    case 1:
                        break;
                    case 2:
                        break;
                    case 3:
                        break;
                    case 4:
                        ps[now_player].jokerCnt++;
                        ps[now_player].jokerTurnSum+=count_turn;
                        break;
                    default:
                        break;
                }
            }
            
            if (RULE_SHIBARI) {
                if ((error==0)&&(status_of_submitted_card[3]!=4)) {
                    if (now_shibari==1) {
                        if (status_of_submitted_card[2]==status_of_stages_card[2]) {
                            
                        } else {
                            error|=1;
                            if (debug) {printf("error06 = %i: shibari \n", error);} //DEBUG
                        }
                    } else {
                        ps[now_player].shibariCnt++;
                        if (status_of_submitted_card[2]==status_of_stages_card[2]) {
                            now_shibari++;
                            ps[now_player].shibari++;
                        } else {
                            now_shibari=0;
                        }
                    }
                }
            }
            
            if (now_jocker_single) { //spe3
                if ((status_of_submitted_card[0]==1)&&(status_of_submitted_card[1]==1)&&(status_of_submitted_card[2]==0x0001)) {
                    error=0;
                    now_jocker_single=0;
                    flash=1;
                    ps[now_player].spe3++;
                } else {
                    error|=1;
                }
            }
            if (debug) {printf("error04 = %i: jocker \n",error);} //DEBUG
            
            if (error==0) {
                switch (status_of_submitted_card[3]) {
                    case 0: // pass
                        break;
                    case 1: // single
                        break;
                    case 2: // pair
                        break;
                    case 3: // kaidan
                        if (RULE_KAIDAN) {
                            break;
                        } else {
                            error|=1;
                        }
                        break;
                    case 4: // jocker only
                        now_jocker_single=1;
                        now_shibari=0;
                        break;
                }
            }
            if (debug) {printf("error05 = %i: kaidan \n",error);} //DEBUG
            
            
            if (error || (status_of_submitted_card[1]==0)) {
                if (error) {
                    if (debug) {printf("error 01 - error = %i - number of card is %i\n",error,status_of_submitted_card[1]);}
                    i=8;
                } else {
                    if (debug) {printf("OK 02\n");}
                    i=9;
                }
                if (i==8) {//illegal move
                    mv=MOVE_PASS;
                }
                j=0; while(j<=0) {
                    j=tn_int_write(client_sockfd[now_player],i,protocol_version);
                    if (debug) {printf("ack roop %i\n",j);}
                };
                if (debug) {printf("ack to player %i -- number %i\n",now_player,i);} //DEBUG
                now_pass[now_player]=1;
                number_renzoku_pass++;
            } else {
                if (debug) {printf("OK 03\n");}
                memcpy(status_of_stages_card,status_of_submitted_card,sizeof(status_of_submitted_card));
                i=9;
                j=0;while(j<=0) {j=tn_int_write(client_sockfd[now_player],i,protocol_version);if (debug) {printf("ack roop %i",j);}};
                if (debug) {printf("ack to player %i -- number %i\n",now_player,i);} //DEBUG
                memcpy(old_stage_card,stage_card,sizeof(work_card));
                memcpy(stage_card,work_card,sizeof(work_card));
                drop_card_flag(players_card[now_player],work_card);
                last_player=now_player;
                accept_flag=last_player;
                number_renzoku_pass=0;
                
                count_card_num(players_card[now_player], &number_of_card);
                if ((number_of_card==0)&&(goal_flag[now_player]==0)) {
                    j=0;
                    for (i=0;i<N_PLAYERS;i++) {
                        count_card_num(players_card[i], &number_of_card);
                        j=j+(number_of_card==0);
                    }
                    goal_flag[now_player]=1;
                    mibun[j-1]=now_player;
                    now_number_of_goal=j;
                    
                    game_log.infoNewClass.assign(now_player, j-1);
                }
                
                if (now_jocker_single==0) {
                    if (RULE_KAKUMEI) {
                        if ((status_of_submitted_card[3]==2)&&(status_of_submitted_card[1]>=4)) {
                            now_kakumei=(now_kakumei+1)%2;
                            ps[now_player].kakumei++;
                        }
                        if ((status_of_submitted_card[3]==3)&&(status_of_submitted_card[1]>=5)) {
                            now_kakumei=(now_kakumei+1)%2;
                            ps[now_player].kakumei++;
                        }
                    }
                    if (RULE_8GIRI) {
                        if (check_special_card(8-2,status_of_submitted_card,0)) {
                            flash=1;
                            ps[now_player].eightGiriCnt++;
                            ps[now_player].eightGiriTurnSum+=count_turn;
                        }
                    }
                    if (RULE_6REVERS) {
                        if (check_special_card(6-2,status_of_submitted_card,0)) {
                            if (now_muki==1) {
                                now_muki=N_PLAYERS - 1;
                            } else {
                                now_muki=1;
                            }
                        }
                    }
                    if (RULE_5TOBI) {
                        if (check_special_card(5-2,status_of_submitted_card,0)) {
                            i=get_seat(sekijun,now_player);
                            now_player=sekijun[(i+now_muki)%N_PLAYERS];
                        }
                    }
                    if (RULE_11BACK) {
                        if (check_special_card(11-2,status_of_submitted_card,0)) {
                            now_11back=(now_11back+1)%2;
                        }
                    }
                }
                
            }
            
            //CERR<<mv<<endl;
            game_log.push_play(MinPlayLog(mv, tmpTime));
            
            //convert status to table
            work_card[5][0] = 0;
            work_card[5][1] = 0;
            work_card[5][2] = 0;
            work_card[5][3] = now_player;
            work_card[5][4] = (status_of_stages_card[0]==-1);
            work_card[5][5] = now_11back;
            work_card[5][6] = now_kakumei;
            work_card[5][7] = now_shibari;
            for (int i = 0; i < N_PLAYERS; ++i) {
                work_card[6][i] = count_card_num_r(players_card[i], &error);
            }
            if (((now_number_of_games-1 )% FLASH_MIBUN_NUMBER)==0) {
                for (i=0;i<N_PLAYERS;i++) {
                    work_card[6][i+5]=0;
                }
            } else {
                for (i=0;i<N_PLAYERS;i++) {
                    work_card[6][mibun[i]+5]=i;
                }
            }
            for (i=0;i<N_PLAYERS;i++) {
                work_card[6][i+10]=sekijun[i];
            }
            for (i=0;i<N_PLAYERS;i++) {
                memcpy(stage_card[5], work_card[5], 2*sizeof(work_card[5]));
            }
            memcpy(stage_card[5], work_card[5], 2*sizeof(work_card[5]));
            players_card[now_player][5][2]=1;
            
            // send information "stage cards" to clients.
            for (int i = 0; i < N_PLAYERS; ++i) {
                tn_card_write(client_sockfd[i],stage_card,protocol_version);
            }
            
            if (broadcast /*&& work_card[6][4]*/) {
                
                if (status_of_submitted_card[3]==0) {
                    printf("%d.%s played: PASS\n",now_player,player_name[now_player]);
                } else {
                    
                    int h,w,sp;
                    int hidari;
                    int hidarispace=21;
                    for (i=0;i<max(5, N_PLAYERS);i++) {
                        hidari=0;
                        switch(i) {
                            case 0:
                                printf("Turn %d:",count_turn);
                                hidari+=6+1;
                                if (count_turn >= 10) {
                                    hidari++;
                                    if (count_turn >=100) {
                                        hidari++;
                                    }
                                }
                                break;
                            case 1:
                                
                                printf("%d.%s played:",now_player,player_name[now_player]);
                                hidari+=10+player_name_length[now_player];
                                
                                break;
                                
                            case 2:
                                
                                printf(" ");hidari++;
                                
                                if (status_of_submitted_card[3]==0) {
                                    printf("Pass ");hidari+=5;
                                } else {
                                    
                                    for (w=1;w<=13;w++) {
                                        for (h=3;h>=0;h--) {
                                            if (stage_card[h][w]==1) {
                                                switch(h) {
                                                    case 0: printf("S");break;
                                                    case 1: printf("H");break;
                                                    case 2: printf("D");break;
                                                    case 3: printf("C");break;
                                                }
                                                switch(w) {
                                                    case 8: printf("T");break;
                                                    case 9: printf("J");break;
                                                    case 10: printf("Q");break;
                                                    case 11: printf("K");break;
                                                    case 12: printf("A");break;
                                                    case 13: printf("2");break;
                                                    default: printf("%d",w+2);break;
                                                }
                                                printf(" ");
                                                hidari+=3;
                                            }
                                            if (stage_card[h][w]==2) {printf("JO ");hidari+=3;}
                                        }
                                    }
                                    for (h=0;h<=3;h++) {
                                        if (stage_card[h][0]==2) {printf("JO ");hidari+=3;}
                                        if (stage_card[h][14]==2) {printf("JO ");hidari+=3;}
                                    }
                                    for (w=0;w<=14;w++) {
                                        if (stage_card[4][w]==2) {printf("JO ");hidari+=3;}
                                    }
                                }
                                break;
                            case 3:
                                
                                if (status_of_submitted_card[3]==0) {
                                    printf("onStage: ");hidari+=9;
                                    
                                    for (w=1;w<=13;w++) {
                                        for (h=3;h>=0;h--) {
                                            if (stage_card[h][w]==1) {
                                                switch(h) {
                                                    case 0: printf("S");break;
                                                    case 1: printf("H");break;
                                                    case 2: printf("D");break;
                                                    case 3: printf("C");break;
                                                }
                                                switch(w) {
                                                    case 8: printf("T");break;
                                                    case 9: printf("J");break;
                                                    case 10: printf("Q");break;
                                                    case 11: printf("K");break;
                                                    case 12: printf("A");break;
                                                    case 13: printf("2");break;
                                                    default: printf("%d",w+2);break;
                                                }
                                                printf(" ");
                                                hidari+=3;
                                            }
                                            if (stage_card[h][w]==2) {printf("JO ");hidari+=3;}
                                        }
                                    }
                                    for (h=0;h<=3;h++) {
                                        if (stage_card[h][0]==2) {printf("JO ");hidari+=3;}
                                        if (stage_card[h][14]==2) {printf("JO ");hidari+=3;}
                                    }
                                    for (w=0;w<=14;w++) {
                                        if (stage_card[4][w]==2) {printf("JO ");hidari+=3;}
                                    }
                                    
                                }	
                                
                                break;
                            case 4:
                                
                                if (stage_card[5][7]) {
                                    printf("Lock ");hidari+=5;
                                }
                                if (stage_card[5][6]) {
                                    printf("Rev ");hidari+=4;
                                }
                                
                                
                                break;
                        }
                        
                        for (sp=hidari;sp<hidarispace;sp++)printf(" ");
                        
                        if (i < N_PLAYERS) {
                            if (((now_number_of_games-1 )% FLASH_MIBUN_NUMBER)==0) {
                                printf("  ");
                            } else {
                                switch(work_card[6][sekijun[i]+5]) {
                                    case DAIFUGO: printf("++"); break;
                                    case FUGO: printf(" +"); break;
                                    case HINMIN: printf(" -"); break;
                                    case DAIHINMIN: printf("--"); break;
                                    default: printf("  "); break;
                                }
                            }
                            
                            printf(" %d.%s",sekijun[i],player_name[sekijun[i]]);
                            for (sp=player_name_length[sekijun[i]];sp<player_name_maxlength;sp++)printf(" ");
                            if (now_pass[sekijun[i]]) {
                                printf("*");
                            } else {
                                printf(" ");
                            }
                            printf(": ");
                            
                            if (humanplaying && sekijun[i] != N_PLAYERS - 1) {
                                for (w=1;w<=13;w++) {
                                    for (h=3;h>=0;h--) {
                                        if (players_card[sekijun[i]][h][w]) {
                                            printf("**");
                                            printf(" ");
                                        }
                                    }
                                }
                                if (players_card[sekijun[i]][4][1])printf("** ");
                            } else {
                                for (w=1;w<=13;w++) {
                                    for (h=3;h>=0;h--) {
                                        if (players_card[sekijun[i]][h][w]) {
                                            switch(h) {
                                                case 0: printf("S");break;
                                                case 1: printf("H");break;
                                                case 2: printf("D");break;
                                                case 3: printf("C");break;
                                            }
                                            switch(w) {
                                                case 8: printf("T");break;
                                                case 9: printf("J");break;
                                                case 10: printf("Q");break;
                                                case 11: printf("K");break;
                                                case 12: printf("A");break;
                                                case 13: printf("2");break;
                                                default: printf("%d",w+2);break;
                                            }
                                            printf(" ");
                                        }
                                    }
                                }
                                if (players_card[sekijun[i]][4][1])printf("JO ");
                            }
                        }
                        printf("\n");
                    }
                    printf("\n");
                    if (status_of_submitted_card[3]==0) {
                        //Sleep(1000);
                    } else {
                        //system("PAUSE");
                    }
                    if (now_player==N_PLAYERS - 1) {
                        //system("PAUSE");
                        //getchar();
                    }
                    
                }
            }			
            
            if (number_renzoku_pass>=20) {
                
                if (debug) {printf("renzoku pass \n");}
                // srand((unsigned)time(NULL));	
                while(now_number_of_goal<=N_PLAYERS - 2) {
                    j=(int)(tn_rand_gen(RAND_TYPE)*(N_PLAYERS-now_number_of_goal)+1);
                    i=0;
                    k=0;
                    while(i<j) {
                        k++;
                        if (goal_flag[k-1]==0) {
                            i++;
                        }
                    }
                    goal_flag[k-1]=1;
                    
                    game_log.infoNewClass.assign(k-1, now_number_of_goal);
                    
                    mibun[now_number_of_goal]=k-1;
                    
                    now_number_of_goal++;
                }
            }
            
            int n_now_pass = 0;
            for (int p = 0; p < N_PLAYERS; ++p) {
                n_now_pass += now_pass[p];
            }
            
            if (n_now_pass>=(N_PLAYERS-now_number_of_goal)) {
                flash=1;	
            }
            
            if (flash) {
                flash=0;	
                now_11back=0;	
                now_player=last_player;
                now_jocker_single=0;
                now_shibari=0;   // flag of jocker
                memset(now_pass,0,sizeof(now_pass));
                status_of_stages_card[0]=-1;
                status_of_stages_card[1]=-1;
                status_of_stages_card[2]=-1;
                status_of_stages_card[3]=-1;
                memcpy(old_stage_card,stage_card,sizeof(stage_card));
                memset(stage_card,0,sizeof(stage_card));
                
                count_card_num(players_card[now_player], &number_of_card);
                while(number_of_card==0) {
                    if (debug) {printf("now_player search %i \n",now_player);} //DEBUG
                    i=get_seat(sekijun,now_player);
                    now_player=sekijun[(i+now_muki)%N_PLAYERS];
                    count_card_num(players_card[now_player], &number_of_card);
                }
                last_player=now_player;
                if (debug) {printf("flashed 01==>next player is %i \n",now_player);}
            } else {
                if (debug) {printf("no flash==>%i %i %i \n",now_player, now_muki,(now_player+now_muki)%N_PLAYERS);}
                i=get_seat(sekijun,now_player);
                now_player=sekijun[(i+now_muki)%N_PLAYERS];
                
                count_card_num(players_card[now_player], &number_of_card);
                while(((number_of_card==0)||(now_pass[now_player]==1))) {
                    if (debug) {printf("now_player search %i \n",now_player);} //DEBUG
                    i=get_seat(sekijun,now_player);
                    now_player=sekijun[(i+now_muki)%N_PLAYERS];
                    count_card_num(players_card[now_player], &number_of_card);
                }
            }
            if (debug) {printf("game is contineous = %i \n",now_number_of_goal);}
            
            if (now_number_of_goal==N_PLAYERS - 1) {
                
                //l1 player
                for (int p=0;p<N_PLAYERS;++p) {
                    if (!goal_flag[p]) {
                        game_log.infoNewClass.assign(p, now_number_of_goal);
                        break;
                    }
                }
                
                accept_flag=10;
                if (now_number_of_games==GAME_NUMBER) { // send a information "all game is overd" to clients.
                    i=2;
                    for (j=0;j<N_PLAYERS;j++) {
                        tn_int_write(client_sockfd[j],i,protocol_version);
                    }
                } else {  // send a information "One game is overd" to clients.
                    i=1;
                    for (j=0;j<N_PLAYERS;j++) {
                        tn_int_write(client_sockfd[j],i,protocol_version);
                    }
                }
            } else {
                i=0;
                for (j=0;j<N_PLAYERS;j++) {
                    tn_int_write(client_sockfd[j],i,protocol_version);
                }
            }
            sum_of_turn++;
        }// elihw of 1 game
        for (i=0;i<N_PLAYERS;i++) {
            if (goal_flag[i]==0) {
                mibun[N_PLAYERS - 1]=i;
            }
        }
        if (debug) {printf("end of 1 game\n");}
        
        // レーティング計算
        if (rating) {
            std::array<double, N_PLAYERS> dist = UECda::calcDiffRateByRelativeWpWithSimulation(playerRate, game_log, 1500, 16.0, &shared, &threadTools[0]);
            for (int p = 0; p < N_PLAYERS; ++p) {
                playerRate[p] += dist[p];
                playerRateMean[p] = (playerRateMean[p] * (now_number_of_games - 1) + playerRate[p]) / now_number_of_games;
                playerRateExpMean[p] = playerRateExpMean[p] * rateEpsilon + playerRate[p] * (1 - rateEpsilon);
            }
            cerr << toString(playerRate) << endl;
            cerr << toString(playerRateMean) << endl;
            cerr << toString(playerRateExpMean) << endl;
        }
        
        if (WINDOW_TYPE==2) {
            
            const char* class_name[5] = {
                "daihugou  ",
                "fugo      ",
                "heimin    ",
                "himmin    ",
                "daihinmin ",
            };
            
            printf("================ game %i \n",now_number_of_games);
            
            int mid = (N_PLAYERS + 1) / 2;
            for (int p = 0; p < mid; ++p) {
                printf("%s %i.%s\n",class_name[p],mibun[p],player_name[mibun[p]]);
            }
            for (int p = mid; p < N_PLAYERS; ++p) {
                printf("%s %i.%s\n",class_name[5 - (N_PLAYERS - p)],mibun[p],player_name[mibun[p]]);
            }
            printf("--------- total point\n");
        }
        for (i=0;i<N_PLAYERS;i++) {
            human[mibun[i]]=i;
            point[now_number_of_games][mibun[i]]=N_PLAYERS-i;
            point_sum[now_number_of_games][mibun[i]]=point_sum[now_number_of_games-1][mibun[i]]+(N_PLAYERS-i);
        }
        for (i=0;i<N_PLAYERS;i++) {printf("%i (%1.3f) %i.%s\n",point_sum[now_number_of_games][i],point_sum[now_number_of_games][i] / (double)now_number_of_games,i,player_name[i]);}
        game_count++;
        
        //fprintf(logfile,"\n");
        if (!game_log.play(game_log.plays()-1).move().isPASS()) {
            game_log.setTerminated();
        }
        match_log.pushGame(game_log);
        
    } // rof now_number_of_games
    
    if (output_flag) {
        // spanなしのresult outputの出力
        
        std::ofstream ofs;
        {
            std::ostringstream oss;
            oss << "output_nospan_" << start_time << ".csv";
            ofs.open(oss.str(), std::ios::app);
        }
        
        for (int p=0;p<N_PLAYERS;++p) {
            ofs << player_name[p];
            for (int g=0;g<GAME_NUMBER;++g) {
                ofs << "," << (int)point[g+1][p];
            }
            ofs << endl;
        }
        ofs.close();
        
        std::vector<int> output_vector[N_PLAYERS];
        for (int p=0;p<N_PLAYERS;++p) {
            for (int s=0,g=0;s<(GAME_NUMBER/output_game_span);++s) {
                int sum=0;
                for (int sg=0;sg<output_game_span && g<GAME_NUMBER;++sg,++g) {
                    sum+=point[g+1][p];
                }
                output_vector[p].push_back(sum);
            }
        }
        // spanありのresult outputの出力
        if (output_file == "") {
            std::ostringstream oss;
            oss << "output_span_" << start_time << ".csv";
            ofs.open(oss.str(), std::ios::app);
        } else {
            ofs.open(output_file, std::ios::app);
        }
        
        for (int p=0;p<N_PLAYERS;++p) {
            ofs << player_name[p];
            for (int sg=0;sg<output_vector[p].size();++sg) {
                ofs << "," << output_vector[p][sg];
            }
            ofs << endl;
        }
        ofs.close();
    }
    
    outputLog();
    
    if (makelog) {
        fclose(logfile);
    }
    
    // player statistics
    printf("Player Statistics\n");
    for (i=0;i<N_PLAYERS;i++) {
        printf("%s", player_name[i]);printf("  Change Time : %lld micsec / Play Time : %lld micsec\n",changetime[i],playtime[i]);
        if (ps[i].getStage !=0) {	
            printf("average of card strength:\t\t%f\n", (double)ps[i].cardStrength/(double)ps[i].getStage);
            printf("average of shibari:\t\t%f\n", (double)ps[i].shibari/(double)ps[i].shibariCnt);
            printf("average of fukusuu(done/chance):\t\t%f\n", (double)ps[i].fukusuu/(double)ps[i].getStage);
            printf("average of kaidan(done/chance):\t\t%f\n", (double)ps[i].kaidan/(double)ps[i].getStage);
        }	
        if (ps[i].jokerCnt != 0) 
            printf("average of joker turn:\t\t%d\n", ps[i].jokerTurnSum/ps[i].jokerCnt);
        if (ps[i].eightGiriCnt != 0)
            printf("average of 8giri turn:\t\t%d\n", ps[i].eightGiriTurnSum/ps[i].eightGiriCnt);
        printf("average of spe3:\t\t%d\n", ps[i].spe3);
        printf("average of kakumei:\t\t%d\n",ps[i].kakumei);
    }
    printf("\n");
    printf("all turn ;\t\t%d , all game: \t\t%d\n",sum_of_turn, game_count);
    printf("average of turn:\t\t%f\n",(double) sum_of_turn / (double) game_count);
    
    /*************/ 
    /* game over */ 
    /*************/ 
    printf("All games are overed \n");
    for (i=0;i<N_PLAYERS;i++) {
        shutdown(client_sockfd[i], 2);
        close(client_sockfd[i]);
    }
    
    shutdown(sockfd, 2);
    close(sockfd);
    //fclose(fp);
    //fclose(fp2);
    
}//niam

