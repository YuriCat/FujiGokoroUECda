/*connection*/

void checkArg(int argc,char* argv[]);
int  startGame(int table[8][15]);
int  entryToGame(const char*);
void sendChangingCards(int cards[8][15]);
int  receiveCards(int cards[8][15]);
int  sendCards(int cards[8][15]);
void lookField(int cards[8][15]);
int  beGameEnd(void);
int  closeSocket();