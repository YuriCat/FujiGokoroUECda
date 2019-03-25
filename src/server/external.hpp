/*
  EXTERNAL.H
 */

#ifndef _EXTERNAL_H_
#define _EXTERNAL_H_

/* prototypes */

/*#include<X11/Intrinsic.h>
#include<X11/StringDefs.h>
#include<X11/Xaw/Label.h>
#include<X11/xpm.h>
#include<X11/Xutil.h>*/
#include "mt19937ar.h"

#include "../include.h"
#include "define.h"

using namespace UECda;
using namespace std;

int count_card_num(int tmp_card[8][15],int *card_num) {
    /***********************************************************/
    /* input  : tmp_card : player's card table.                */
    /*          card_num : dust                                */
    /* return : If there are some error then program return 1  */
    /*          other wise 0.                                  */
    /*          where errors are                               */
    /*          1) the number of joker is over 2               */
    /*          2) There are any flags                         */
    /*             in row 0 and colom 0 and 14                 */
    /* destroy: card_number : a number of tmp_card             */
    /***********************************************************/
    int i,j;
    int tmp_card_num=0;
    int tmp_joker_num=0;
    int collectly=0;
    
    for (i=0;i<=4;i++) {
        for (j=0;j<=14;j++) {
            switch (tmp_card[i][j]) {
                case 0:
                    break;
                case 1:
                    tmp_card_num++;
                    if (j==0 || j==14) {
                        collectly=1;
                    };
                    if (i==4) {
                        collectly=1;
                    };
                    break;
                case 2:
                    tmp_card_num++;
                    tmp_joker_num++;
                    break;
                default:
                    collectly=1;
                    
            }
        }
    }
    *card_num=tmp_card_num;
    return ((collectly + (tmp_joker_num >= 2)) >=1);
}

int count_card_num_r(int tmp_card[8][15], int *error) {
    /***********************************************************/
    /* input  : tmp_card : player's card table.                */
    /*          card_num : dust                                */
    /* return : If there are some error then program return 1  */
    /*          other wise 0.                                  */
    /*          where errors are                               */
    /*          1) the number of joker is over 2               */
    /*          2) There are any flags                         */
    /*             in row 0 and colom 0 and 14                 */
    /* destroy: card_number : a number of tmp_card             */
    /***********************************************************/
    int i,j;
    int tmp_card_num=0;
    int tmp_joker_num=0;
    int collectly=0;
    
    for (i=0;i<=4;i++) {
        for (j=0;j<=14;j++) {
            switch (tmp_card[i][j]) {
                case 0:
                    break;
                case 1:
                    tmp_card_num++;
                    if (j==0 || j==14) {
                        collectly=1;
                    };
                    if (i==4) {
                        collectly=1;
                    };
                    break;
                case 2:
                    tmp_card_num++;
                    tmp_joker_num++;
                    break;
                default:
                    collectly=1;
                    
            }
        }
    }
    *error = ((collectly + (tmp_joker_num >= 2)) >=1);
    return tmp_card_num;
}

void shuffle_card(int initial_person,int player_card[N_PLAYERS][8][15], int RAND_TYPE, int sekijun[N_PLAYERS]) {
    /***********************************************************/
    /* input  : initial_person : DAIHUGO or first player       */
    /*          player_card    : dust                          */
    /* return : none                                           */
    /* destroy: player_card    : cards table of all players    */
    /***********************************************************/
    int i,j;
    int initial_seat;
    int tmp;
    int tmp_number;
    int card_table[54]={0};
    
    memset(card_table,0,sizeof(card_table));
    
    initial_seat=get_seat(sekijun, initial_person);
    
    for (i=0;i<=52;i++) {
        tmp=(int)(tn_rand_gen(RAND_TYPE) * (53 - i));
        tmp_number=0;
        for (j=0;tmp_number<=tmp;j++) {
            tmp_number=tmp_number+(card_table[j] ==0 );
        }
        j--;
        card_table[j]=1;
        if (j==52) {
            player_card[sekijun[(i+initial_seat) % N_PLAYERS]][j / 13][j % 13 + 1]=2;
        } else {
            player_card[sekijun[(i+initial_seat) % N_PLAYERS]][j / 13][j % 13 + 1]=1;
        }
    }
}

void print_player_card(int player_card[8][15]) {
    /***********************************************************/
    /* input  : player_card : table of one player's card       */
    /* return : none                                           */
    /* destroy: none                                           */
    /* etc    : print card_table to standard output            */
    /***********************************************************/
    int i,j;
    
    for (i=0;i<=7;i++) {
        for (j=0;j<=14;j++) {
            printf("%i ",player_card[i][j]);
        }
        printf("\n");
    }
}

int analyze_card(int player_card[8][15], int status_of_submitted_card[4], int revers) {
    /***************************************************************/
    /* input  : player_card : table of published cards table       */
    /*        : status_of_submitted_card : dust                    */
    /*        : revers : revers FLAG                               */
    /* return : If thare are any error then return 1 else return 0 */
    /*            1) the number of joker is over 2                 */
    /*            2) There are any flags                           */
    /*               in row 0 and colom 0 and 14                   */
    /*            3) There are 2 cards when kaidan.                */
    /*            4) Abnormal suit                                 */
    /* destroy: status_of_submitted_card :                         */
    /*            [0]: minimam value of cards                      */
    /*            [1]: number of cards                             */
    /*            [2]: suit, lower 4 bits was used                 */
    /*            [3]: type                                        */
    /*                 1 -> single,                                */
    /*                 2 -> pair,                                  */
    /*                 3 -> kaidan,                                */
    /*                 4 -> joker tanki                            */
    /***************************************************************/
    int i,k,j;
    int number_of_submitted_cards;
    int error=0;
    
    status_of_submitted_card[0]=0;
    status_of_submitted_card[1]=0;
    status_of_submitted_card[2]=0;
    status_of_submitted_card[3]=0;
    
    
    j=-1; // search minimum value of cards
    while((j==-1)||(player_card[i][j]==0 && j<=14)) {
        i=0;
        j=j+1;
        while(player_card[i][j]==0 && i<=3) {
            //printf("%i %i %i \n", i,j,player_card[i][j]);
            i++;
        }
    }
    status_of_submitted_card[0]=j; // set a minimam value of cards
    
    error+=count_card_num(player_card,&number_of_submitted_cards);
    switch ((int)number_of_submitted_cards) {
        case 0:
            status_of_submitted_card[3]=0;
            return 0;
        case 1:
            if (player_card[i][j]==2) {
                status_of_submitted_card[3]=4;
            } else {
                status_of_submitted_card[3]=1;
            }
    }
    
    if (j!=14 && player_card[i][j+1]!=0) { // kaidan haitei
        status_of_submitted_card[2]=0x0001<<i;
        status_of_submitted_card[1]=1;
        while(j<=14 && player_card[i][j++]!=0) {
            status_of_submitted_card[1]++;
        }
        status_of_submitted_card[1]--;
        status_of_submitted_card[3]=3;
        if (revers) {
            status_of_submitted_card[0]=status_of_submitted_card[0]+status_of_submitted_card[1]-1;
        }
        error+=(status_of_submitted_card[1]<=2);
    } else { // pair hantei
        for (i=0;i<=4;i++) {
            if (player_card[i][j]!=0) {
                status_of_submitted_card[1]++;
                status_of_submitted_card[2] |= (0x0001 << i);
            }
        }
        if (status_of_submitted_card[3]==0) {
            status_of_submitted_card[3]=2;
        }
    }
    
    if (status_of_submitted_card[2]>=16 && status_of_submitted_card[2]!=31) {
        error+=1;
    }
    
    error=error || (number_of_submitted_cards != status_of_submitted_card[1]);
    return error;
}

int check_special_card(int special_card, int status_of_submitted_card[4], int revers) {
    /*****************************************************************************/
    /* input  : special_card : a special card, which is checked in this function */
    /*        : status_of_submitted_card : a analyze table                       */
    /*        : recers : If KAKUME then 1 otherwise 0                            */
    /* return : If a status of submitted card includes specias card then 1       */
    /* destroy: none                                                             */
    /*****************************************************************************/
    if (status_of_submitted_card[3]==3) { // kaidan
        if (revers) {
            if ((status_of_submitted_card[0]>=special_card) && (status_of_submitted_card[0]-status_of_submitted_card[1]+1<=special_card)) {
                return 1;
            }
            return 0;
        } else {
            if ((status_of_submitted_card[0]<=special_card) && (status_of_submitted_card[0]+status_of_submitted_card[1]-1>=special_card)) {
                return 1;
            }
            return 0;
        }
    } else {  // etc
        if (status_of_submitted_card[0]==special_card) {
            return 1;
        }
        return 0;
    }
}

int search_card(int players_card[N_PLAYERS][8][15], int num, int suit) {
    /********************************************************************************/
    /* input  : players_card : all player's all cards                               */
    /*        : num : a card's number which is wanted to search                     */
    /*        : suit : a card's suit which is wanted to search                      */
    /* return : If a plaer has the card then this functiron returns player's number */
    /*        : else this function returns 255.                                     */
    /* destroy: none                                                                */
    /********************************************************************************/
    int i;
    
    for (i=0;i<=4;i++) {
        if (players_card[i][num][suit]!=0) {
            return i;
        }
    }
    return 255;
}

int compare_card_status(int stage_card[4], int now_card[4], int revers) {
    /****************************************************************/
    /* input  : stage_card : a analyze table of cards on the stage  */
    /*        : now_card : a analyze table of submitted card        */
    /*        : recers : If KAKUME then 1 otherwise 0               */
    /* return : If now card is correct then this function returns 1 */
    /*        : else this function returns 0                        */
    /* destroy: none                                                */
    /* note   : This function can't compare suit                    */
    /****************************************************************/
    if (stage_card[0]==-1) { // if there are no cards on the stage then any cards are accepted.
        return 0;
    }
    
    if (stage_card[1]!=now_card[1]) { // compare number of cards
        return 1;
    }
    
    if ((now_card[3]==4) && stage_card[3]==1) { // when joker tanki
        return 0;
    }
    
    if (stage_card[3]!=now_card[3]) { //if type are different between accepted cards and stages cards then return value is 1.
        return 1;
    }
    
    if (stage_card[3]==2) { // when the type is pair
        if (revers) { // compare value of cards on kakumei
            return ((now_card[0]-stage_card[0])>=0);
        } else {
            //printf("pair hantei %i\n",((now_card[0]-stage_card[0]+1)<=0));
            return ((now_card[0]-stage_card[0])<=0);
        }
    }
    
    // when the type is kaidan or tanki.
    if (revers) { //compare value of cards on kakumei
        return ((now_card[0]-stage_card[0]+stage_card[1]-1)>=0);
    } else {
        return ((now_card[0]-stage_card[0]-stage_card[1]+1)<=0);
    }
}

int check_include_card(int org_card[8][15], int subset_card[8][15]) {
    /***************************************************************/
    /* input  : org_card :card table of player's all cards         */
    /*        : subset_card :card table of published cards         */
    /* return : If there are error then it return 1.               */
    /*            1) org_card don't include subset_card            */
    /*            2) When player dose't have joker,                */
    /*               plyaer use joker.                             */
    /* destroy: none                                               */
    /***************************************************************/
    int i,j;
    
    for (i=0;i<=4;i++) {
        for (j=0;j<=14;j++) {
            switch (subset_card[i][j]) {
                case 1:
                    if (org_card[i][j]!=1) {
                        return 1;
                    }
                    break;
                case 2:
                    if (org_card[4][1]!=2) {
                        return 1;
                    }
            }
        }
    }
    return 0;
}

int marge_card(int target_card[8][15], int add_card[8][15]) {
    /****************************************************************/
    /* input  : target_card: a card table of one player's all cards */
    /*        : add_card : a card table of published cards          */
    /* retuen : 0                                                   */
    /* destroy: target_card: target_card - add_card.                */
    /****************************************************************/
    int i,j;
    
    for (i=0;i<=4;i++) {
        for (j=0;j<=14;j++) {
            switch(add_card[i][j]) {
                case 0:
                    break;
                case 1:
                    target_card[i][j]=1;
                    break;
                case 2:
                    target_card[4][1]=2;
                    break;
            }
        }//rof j
    }// rof i
    return 0;
}

int drop_card_flag(int org_card[8][15], int subset_card[8][15]) {
    /**************************************************************************/
    /* input  : org_card: a card table of one player's all cards              */
    /*        : subset_card : a card table of published cards                 */
    /* retuen : 0                                                             */
    /* destroy: org_card: org_card - subset_card.                             */
    /* note   : This function can't check that org_card includes subset_card. */
    /*        : Please combine check_include_card.                            */
    /**************************************************************************/
    int i,j;
    
    for (i=0;i<=4;i++) {
        for (j=0;j<=14;j++) {
            switch (subset_card[i][j]) {
                case 1:
                    org_card[i][j]=0;
                    break;
                case 2:
                    org_card[4][1]=0;
            }
        }
    }
    return 0;
}

int search_strong_card(int player_card[8][15], int strong_card[8][15], int number_of_card) {
    /**************************************************************************/
    /* input  : player_card: a card table of one player's all cards           */
    /*        : strong_card: dump                                             */
    /*        : number_of_card : the number of strong cards                   */
    /* retuen : 0                                                             */
    /* destroy: strong_cards has strong cards.                                */
    /**************************************************************************/
    int i,j;
    int k=0;
    int card_num=0;
    
    memset(strong_card,0,8*15*strong_card[0][0]);
    
    if (count_card_num(player_card,&card_num)) {
        return 1;
    }
    if (card_num < number_of_card) {
        number_of_card=card_num;
    }
    
    if (player_card[4][1]==2 && k<number_of_card) {
        k++;
        strong_card[4][1]=2;
    }
    
    i=0; //suit
    j=14;
    while(k<number_of_card) {
        if (player_card[i][j]) {
            k++;
            strong_card[i][j]=1;
        }
        i=(i+1)%4;
        if (i==0) {
            j--;
        }
    }
    return 0;
}

int trans_strong_card(int source_card[8][15], int target_card[8][15], int number_of_card) {
    /**************************************************************************/
    /* input  : source_card: a card table of one player's all cards           */
    /*        : target_card: a card table of one player's all cards           */
    /*        : number_of_card : the number of strong cards                   */
    /* retuen : 0                                                             */
    /* destroy: souce_card is taken number_of_card strong cards.              */
    /*        : target_card get number_of_card strong cards.                  */
    /**************************************************************************/
    int i,j;
    int k=0;
    int card_num=0;
    
    if (count_card_num(source_card,&card_num)) {
        return 1;
    }
    if (card_num < number_of_card) {
        number_of_card=card_num;
    }
    
    if (source_card[4][1]==2 && k<number_of_card) {
        //print_player_card(source_card);
        k++;
        source_card[4][1]=0;
        target_card[4][1]=2;
        //printf("trans joker\n");
    }
    
    i=0; //suit
    j=14;
    while(k<number_of_card) {
        if (source_card[i][j]) {
            k++;
            source_card[i][j]=0;
            target_card[i][j]=1;
        }
        i=(i+1)%4;
        if (i==0) {
            j--;
        }
    }
    return 0;
}

int trans_work_card(int source_card[8][15], int target_card[8][15], int work_card[8][15]) {
    /**************************************************************************/
    /* input  : target_card: a card table of one player's all cards           */
    /*        : souce_card : a card table of published cards                  */
    /*        : work_card  : a card table of published cards                  */
    /* retuen : 0                                                             */
    /* destroy: source_card: source_card - work_card.                         */
    /*        : target_card: target_card + work_card.                         */
    /* note   : This function can't check that org_card includes subset_card. */
    /*        : Please combine check_include_card.                            */
    /**************************************************************************/
    int i,j;
    
    for (i=0;i<=4;i++) {
        for (j=0;j<=14;j++) {
            switch(work_card[i][j]) {
                case 0:
                    break;
                case 1:
                    target_card[i][j]=1;
                    source_card[i][j]=0;
                    break;
                case 2:
                    target_card[4][1]=2;
                    source_card[4][1]=0;
                    source_card[i][j]=0;
                    break;
            }
        }//rof j
    }// rof i
    return 0;
}

void tn_rand_init(unsigned long seed, int type) {
    switch(type) {
        case 0 :
            srand((int)seed);
            break;
        case 1 :
            init_genrand((unsigned long)seed);
            break;
    }
}

double  tn_rand_gen(int type) {
    switch(type) {
        case 0 :
            return (double)rand()/((double)RAND_MAX-1.0);
        case 1 :
            return genrand_real1();
    }
    return 0;
}

int double_cmp(const void *a, const void *b) {
    int tmp;
    const double *tmp_a=(const double*)a;
    const double *tmp_b=(const double*)b;
    
    if ((tmp_a[1]-tmp_b[1])>0) {
        tmp=1;
    }else if ((tmp_a[1]-tmp_b[1]<0)) {
        tmp=-1;
    } else {
        tmp=0;
    }
    return tmp;
}


void tn_sekigae(int now_number_of_games, int sekijun[N_PLAYERS], int RULE_SEKIGAE, int RULE_SEKIGAE_NUM, int RAND_TYPE) {
    int i,j;
    int flag;
    double tmp_seki[N_PLAYERS][2];
    
    switch(RULE_SEKIGAE) {
        case 1:
            if ((now_number_of_games % RULE_SEKIGAE_NUM)==1) {
                for (i=0;i<N_PLAYERS;i++) {
                    tmp_seki[i][0]=i;
                    flag=0;
                    while(flag==0) {
                        flag=1;
                        tmp_seki[i][1]=tn_rand_gen(RAND_TYPE);
                        for (j=0;j<i;j++) {
                            if (tmp_seki[i][1]==tmp_seki[j][1]) {
                                flag=0;
                            }
                        }
                    }
                }
                qsort(tmp_seki,N_PLAYERS,sizeof(tmp_seki[0]),double_cmp);
                for (i=0;i<N_PLAYERS;i++) {
                    sekijun[i]=(int)tmp_seki[i][0];
                }
            }
            return ;
        case 0:
            return;
    }
    
}

int get_seat(int sekijun[N_PLAYERS], int now_player) {
    int i;
    
    i=0;
    while(sekijun[i]!=now_player) {
        i++;
    }
    return i;
}


#endif 

