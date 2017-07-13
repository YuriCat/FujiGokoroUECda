/*
 logIteration.hpp
 Katsuki Ohto
 */

#ifndef UECDA_STRUCTURE_LOGITERATION_HPP_
#define UECDA_STRUCTURE_LOGITERATION_HPP_

#include "../primitive/prim.hpp"

// ログから局面を正方向に読む
// 交換やプレー時のコールバッックには返り値を要求する
// 0 が通常
// -1 でそのフェーズ終了
// -2 で試合ログ読み終了
// -3 で1マッチ(連続対戦)終了
// -4 で全マッチ終了

namespace UECda{
    
    // 1試合全体
    
    template<
    class field_t,
    class gameLog_t,
    typename firstCallback_t,
    typename dealtCallback_t,
    typename changeCallback_t,
    typename afterChangeCallback_t,
    typename playCallback_t,
    typename lastCallback_t>
    int iterateGameLog
    (const gameLog_t& gLog,
     const firstCallback_t& firstCallback = [](const field_t&)->void{},
     const dealtCallback_t& dealtCallback = [](const field_t&)->void{},
     const changeCallback_t& changeCallback = [](const field_t&, const int, const int, const Cards)->int{ return 0; },
     const afterChangeCallback_t& afterChangeCallback = [](const field_t&)->void{},
     const playCallback_t& playCallback = [](const field_t&, const Move&, const uint64_t)->int{ return 0; },
     const lastCallback_t& lastCallback = [](const field_t&)->void{}
     ){
        
        using playLog_t = typename gameLog_t::playLog_t;
        using changeLog_t = typename gameLog_t::changeLog_t;
        
        field_t field;
        
        field.init1G();
        field.setMoveBuffer(nullptr);
        field.setDice(nullptr);
        
        if(gLog.isInitGame()){
            field.setInitGame();
            field.infoClass.fill(Class::MIDDLE);
        }else{
            field.infoClass = gLog.infoClass();
        }
        
        field.infoSeat = gLog.infoSeat();
        field.infoSeatPlayer = invert(gLog.infoSeat());
        
        
        if(!field.isInitGame()){
            field.infoClassPlayer = invert(gLog.infoClass());
        }
        
        firstCallback(field);
        
        // deal cards
        for(int p = 0; p < gLog.players(); ++p){
            Cards tmp = gLog.dealtCards(p);
            field.hand[p].cards = tmp;
        }
        // present
        for(int t = 0, tend = gLog.changes(); t < tend; ++t){
            const changeLog_t& change = gLog.change(t);
            if(field.getPlayerClass(change.to()) <= Class::FUGO){
                Cards present = change.cards();
                addCards(&field.hand[change.to()].cards, present);
            }
        }
        // set card info
        for(int p = 0; p < gLog.players(); ++p){
            Cards tmp = field.hand[p].cards;
            
            //cerr << p << " " << OutCards(tmp) << endl;
            field.hand[p].set(tmp);
            field.opsHand[p].set(subtrCards(CARDS_ALL, tmp));
            
            uint64_t hash = CardsToHashKey(tmp);
            
            field.hand[p].setHash(hash);
            field.opsHand[p].setHash(HASH_CARDS_ALL ^ hash);
            
            field.addAttractedPlayer(p);
        }
        field.fillRemHand(CARDS_ALL);
        
        dealtCallback(field);

        // change
        for(int t = 0, tend = gLog.changes(); t < tend; ++t){
            const changeLog_t& change = gLog.change(t);
            if(field.getPlayerClass(change.from()) >= Class::HINMIN){
                // subtr present cards
                field.hand[change.from()].subtrAll(change.cards());
                field.opsHand[change.from()].addAll(change.cards());
            }else{
                int ret = changeCallback(field, change.from(), change.to(), change.cards());
                if(ret <= -2){
                    cerr << "error on change "
                    << change.from() << " -> " << change.to() << endl;
                    return ret;
                }else if(ret == -1){
                    break;
                }
                // proceed field
                field.makeChange(change.from(), change.to(), change.cards());
            }
        }
        field.prepareAfterChange();
        
        afterChangeCallback(field);
        
        // play
        for(int t = 0, tend = gLog.plays(); t < tend; ++t){
            field.prepareForPlay();
            const playLog_t& play = gLog.play(t);
            int ret = playCallback(field, play.move(), play.time());
            if(ret <= -2){
                cerr << "error on play turn " << t << endl;
                return ret;
            }else if(ret == -1){
                break;
            }
            // proceed field
            field.procSlowest(play.move());
        }
        
        field.infoNewClass = gLog.infoNewClass();
        field.infoNewClassPlayer = invert(gLog.infoNewClass());
        
        lastCallback(field);
        return 0;
    }
    
    // 役提出が始まる前まで
    
    template<
    class field_t,
    class gameLog_t,
    typename firstCallback_t,
    typename dealtCallback_t,
    typename changeCallback_t,
    typename lastCallback_t>
    int iterateGameLogBeforePlay
    (const gameLog_t& gLog,
     const firstCallback_t& firstCallback = [](const field_t&)->void{},
     const dealtCallback_t& dealtCallback = [](const field_t&)->void{},
     const changeCallback_t& changeCallback = [](const field_t&, const int, const int, const Cards)->int{ return 0; },
     const lastCallback_t& lastCallback = [](const field_t&)->void{}
     ){
        using changeLog_t = typename gameLog_t::changeLog_t;
        
        field_t field;
        
        field.init1G();
        field.setMoveBuffer(nullptr);
        field.setDice(nullptr);
        
        if(gLog.isInitGame()){
            field.setInitGame();
            field.infoClass.fill(Class::MIDDLE);
        }else{
            field.infoClass = gLog.infoClass();
        }
        
        field.infoSeat = gLog.infoSeat();
        field.infoSeatPlayer = invert(gLog.infoSeat());
        
        if(!field.isInitGame()){
            field.infoClassPlayer = invert(gLog.infoClass());
        }
        
        firstCallback(field);
        
        // deal cards
        for(int p = 0; p < gLog.players(); ++p){
            Cards tmp = gLog.dealtCards(p);
            field.hand[p].cards = tmp;
        }
        // present
        for(int t = 0, tend = gLog.changes(); t < tend; ++t){
            const changeLog_t& change = gLog.change(t);
            if(field.getPlayerClass(change.to()) <= Class::FUGO){
                Cards present = change.cards();
                addCards(&field.hand[change.to()].cards, present);
            }
        }
        // set card info
        for(int p = 0; p < gLog.players(); ++p){
            Cards tmp = field.hand[p].cards;
            
            //cerr << p << " " << OutCards(tmp) << endl;
            field.hand[p].set(tmp);
            field.opsHand[p].set(subtrCards(CARDS_ALL, tmp));
            
            uint64_t hash = CardsToHashKey(tmp);
            
            field.hand[p].setHash(hash);
            field.opsHand[p].setHash(HASH_CARDS_ALL ^ hash);
            
            field.addAttractedPlayer(p);
        }
        field.fillRemHand(CARDS_ALL);
        
        dealtCallback(field);
        
        // change
        for(int t = 0, tend = gLog.changes(); t < tend; ++t){
            const changeLog_t& change = gLog.change(t);
            if(field.getPlayerClass(change.from()) >= Class::HINMIN){
                // subtr present cards
                field.hand[change.from()].subtrAll(change.cards());
                field.opsHand[change.from()].addAll(change.cards());
            }else{
                int ret = changeCallback(field, change.from(), change.to(), change.cards());
                if(ret <= -2){
                    cerr << "error on change "
                    << change.from() << " -> " << change.to() << endl;
                    return ret;
                }else if(ret == -1){
                    break;
                }
                // proceed field
                field.makeChange(change.from(), change.to(), change.cards());
            }
        }
        lastCallback(field);
        return 0;
    }
    
    // カード交換が終わった後から
    
    template<
    class field_t,
    class gameLog_t,
    typename firstCallback_t,
    typename playCallback_t,
    typename lastCallback_t>
    int iterateGameLogAfterChange
    (const gameLog_t& gLog,
     const firstCallback_t& firstCallback = [](const field_t&)->void{},
     const playCallback_t& playCallback = [](const field_t&, const Move, const uint64_t)->int{ return 0; },
     const lastCallback_t& lastCallback = [](const field_t&)->void{}
     ){
        
        using playLog_t = typename gameLog_t::playLog_t;
        
        field_t field;
        
        field.init1G();
        field.setMoveBuffer(nullptr);
        field.setDice(nullptr);
        
        if(gLog.isInitGame()){
            field.setInitGame();
            field.infoClass.fill(Class::MIDDLE);
        }else{
            field.infoClass = gLog.infoClass();
        }
        
        field.infoSeat = gLog.infoSeat();
        field.infoSeatPlayer = invert(gLog.infoSeat());
        
        if(!field.isInitGame()){
            field.infoClassPlayer = invert(gLog.infoClass());
        }
        
        // set for play
        for(int p = 0; p < gLog.players(); ++p){
            
            Cards tmp = gLog.orgCards(p);
            
            field.hand[p].set(tmp);
            field.opsHand[p].set(subtrCards(CARDS_ALL, tmp));
            
            uint64_t hash = CardsToHashKey(tmp);
            
            field.hand[p].setHash(hash);
            field.opsHand[p].setHash(HASH_CARDS_ALL ^ hash);
            
            field.addAttractedPlayer(p);
        }
        field.fillRemHand(CARDS_ALL);
        field.prepareAfterChange();
        
        firstCallback(field);
        
        // play
        for(int t = 0, tend = gLog.plays(); t < tend; ++t){
            field.prepareForPlay();
            const playLog_t& play = gLog.play(t);
            int ret = playCallback(field, play.move(), play.time());
            if(ret <= -2){
                cerr << "error on play turn " << t << endl;
                return ret;
            }else if(ret == -1){
                break;
            }
            // proceed field
            field.procSlowest(play.move());
        }
        
        field.infoNewClass = gLog.infoNewClass();
        field.infoNewClassPlayer = invert(gLog.infoNewClass());
        
        lastCallback(field);
        return 0;
    }
    
    // 試合中のプレーヤーがこれまでの試合(交換後)を振り返る場合
    // 相手手札がわからないのでhandとして外部から与える
    
    template<
    class field_t,
    class gameLog_t,
    class hand_t,
    typename firstCallback_t,
    typename playCallback_t,
    typename lastCallback_t>
    int iterateGameLogInGame
    (const gameLog_t& gLog,
     const std::array<hand_t, gameLog_t::players()>& hand,
     const firstCallback_t& firstCallback = [](const field_t&)->void{},
     const playCallback_t& playCallback = [](const field_t&, const Move, const uint64_t)->int{ return 0; },
     const lastCallback_t& lastCallback = [](const field_t&)->void{}
     ){
        using playLog_t = typename gameLog_t::playLog_t;
        
        field_t field;
        
        field.init1G();
        field.setMoveBuffer(nullptr);
        field.setDice(nullptr);
        
        if(gLog.isInitGame()){
            //cerr << "init-game!!" << endl;
            field.setInitGame();
            field.infoClass.fill(Class::MIDDLE);
        }else{
            field.infoClass = gLog.infoClass();
        }
        
        field.infoSeat = gLog.infoSeat();
        field.infoSeatPlayer = invert(gLog.infoSeat());
        
        if(!field.isInitGame()){
            field.infoClassPlayer = invert(gLog.infoClass());
        }
        
        // set for play
        for(int p = 0; p < gLog.players(); ++p){
            
            Cards tmp = Cards(hand[p]);
            
            field.hand[p].set(tmp);
            field.opsHand[p].set(subtrCards(CARDS_ALL, tmp));
            
            uint64_t hash = CardsToHashKey(tmp);
            
            field.hand[p].setHash(hash);
            field.opsHand[p].setHash(HASH_CARDS_ALL ^ hash);
            
            field.addAttractedPlayer(p);
        }
        
        field.fillRemHand(CARDS_ALL);
        field.prepareAfterChange();
        
        firstCallback(field);
        
        // play
        for(int t = 0, tend = gLog.plays(); t < tend; ++t){
            field.prepareForPlay();
            const playLog_t& play = gLog.play(t);
            int ret = playCallback(field, play.move(), play.time());
            if(ret <= -2){
                cerr << "error on play turn " << t << endl;
                return ret;
            }else if(ret == -1){
                break;
            }
            // proceed field
            field.procSlowest(play.move());
        }
        lastCallback(field);
        return 0;
    }
}

#endif // UECDA_STRUCTURE_LOGITERATION_HPP_
