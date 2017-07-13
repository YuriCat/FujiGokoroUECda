#ifndef UECDA_SERVER_DEFINE_H_
#define UECDA_SERVER_DEFINE_H_

/* rule setting */
/*#define RULE_KAKUMEI
#define RULE_SHIBARI
#undef  RULE_KINSOKU
#define RULE_KAIDAN
#undef  RULE_5TOBI
#undef  RULE_6REVERS
#define RULE_8GIRI
#undef  RULE_11BASK*/

/* setting values*/ 
/*#define  GAME_NUMBER 10*/

// header

#include<stdio.h>
#include<math.h>
#include<stdlib.h>
#include<time.h>
#include<string.h>
#include<strings.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <sys/param.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <errno.h>
#include<math.h>
#include<time.h>

#include <netinet/in.h>
#include <inttypes.h>

int tn_card_write(int client_sockfd, int send_card[8][15], int protocol_version);
int tn_int_write(int client_sockfd, int i , int protocol_version);
int tn_card_read(int client_sockfd, int read_card[8][15], int protocol_version);

char* num_to_str(char* str,int flag);
int isint(char* a);
void strupr2(char *dat);
int count_card_num(int tmp_card[8][15],int *card_num);
int count_card_num_r(int tmp_card[8][15],int *error);
void shuffle_card(int initial_person,int player_card[N_PLAYERS][8][15], int RAND_TYPE, int sekijun[N_PLAYERS]);
void print_player_card(int player_card[8][15]);
int analyze_card(int player_card[8][15], int status_of_submitted_card[4], int revers);
int check_include_card(int org_card[8][15], int subset_card[8][15]);
int drop_card_flag(int org_card[8][15], int subset_card[8][15]);
int check_special_card(int special_card, int status_of_submitted_card[4], int revers);
int search_card(int players_card[N_PLAYERS][8][15], int num, int suit);
int compare_card_status(int stage_card[4], int now_card[4], int revers);
int marge_card(int target_card[8][15], int add_card[8][15]);
int trans_work_card(int source_card[8][15], int target_card[8][15], int work_card[8][15]);
int trans_strong_card(int source_card[8][15], int target_card[8][15], int number_of_card);
int search_strong_card(int player_card[8][15], int strong_card[8][15], int number_of_card);
void tn_rand_init(unsigned long seed, int type);
double tn_rand_gen(int type);
void tn_sekigae(int now_number_of_games, int sekijun[N_PLAYERS], int RULE_SEKIGAE, int RULE_SEKIGAE_NUM, int RAND_TYPE);
int get_seat(int sekijun[N_PLAYERS], int now_player);

#endif
