/*
  EXTERNAL.H
 */

#ifndef _TN_PROTOCOL_H_
#define _TN_PROTOCOL_H_

static int num[65536];

int tn_card_write(int client_sockfd, int send_card[8][15], int protocol_version) {
    uint32_t tmp_card[8][15];
    int i,j;
    
    switch(protocol_version) {
        case 20060:
            return send(client_sockfd, send_card, sizeof(i)*8*15, 0);
            break;
        case 20070:
            for (i=0;i<=7;i++) {
                for (j=0;j<=14;j++) {
                    tmp_card[i][j]=htonl((uint32_t)send_card[i][j]*(send_card[i][j]>=0));
                }
            }
            return send(client_sockfd, tmp_card, sizeof(uint32_t)*8*15, 0);
            break;
    }
    return -1;
}

int tn_int_write(int client_sockfd, int i, int protocol_version) {
    uint32_t tmp;
    
    switch(protocol_version) {
        case 20060:
            return send(client_sockfd, &i, sizeof(i), 0);
            break;
        case 20070:
            tmp=htonl((uint32_t)i*(i>=0));
            return send(client_sockfd, &tmp, sizeof(uint32_t), 0);
            break;
    }
    return -1;
}

int tn_card_read(int client_sockfd, int read_card[8][15], int protocol_version) {
    uint32_t tmp_card[8][15];
    int tmp_card2[8][15];
    int i,j;
    
    memset(tmp_card,0,sizeof(tmp_card));
    switch(protocol_version) {
        case 20060:
            return recv(client_sockfd, read_card, sizeof(i)*8*15, 0);
            break;
        case 20070:
            int ret = recv(client_sockfd, tmp_card, sizeof(uint32_t)*8*15, 0);
            for (i=0;i<=7;i++) {
                for (j=0;j<=14;j++) {
                    read_card[i][j]=(int)ntohl(tmp_card[i][j]);
                }
            }
            return ret;
            break;
    }
    return -1;
}

#endif 

