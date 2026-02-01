#ifndef ENGINE_H
#define ENGINE_H

#include "define.h"
#include "statistics.h"
#include "sounds.h"
#include "deck.h"

#include <QObject>
#include <QList>
#include <QString>

enum VARIANT {
     VARIANT_QUEEN_SPADE = 0,
     VARIANT_PERFECT_100 = 1,
     VARIANT_OMNIBUS     = 2,
     VARIANT_NO_TRICKS   = 3,
     VARIANT_NEW_MOON    = 4,
     VARIANT_NO_DRAW     = 5
};

constexpr int PERFECT_100_VALUE   =  50;  // the hand score will be set to this number.
constexpr int NO_TRICK_VALUE      =   5;  // the no trick bonus will substract this number to the hand score.
constexpr int OMNIBUS_VALUE       =  10;  // the bonus for winning the jack of diamond.
constexpr int GAME_OVER_SCORE     = 100;  // reach this score to make the game over.

constexpr char SAVEDGAME_FILENAME[]  = "/.hearts.saved";
constexpr char SAVEDGAME_CORRUPTED[] = "/.hearts.saved.bak";


enum DIRECTION {
    PASS_LEFT     = 0,
    PASS_RIGHT    = 1,
    PASS_ACROSS   = 2,
    PASS_HOLD     = 3
};

enum GAME_STATUS {
   NEW_ROUND        = 0,
   DEAL_CARDS       = 1,
   SELECT_CARDS     = 2,
   TRADE_CARDS      = 3,
   PLAY_TWO_CLUBS   = 4,
   PLAY_A_CARD_1    = 5,
   PLAY_A_CARD_2    = 6,
   PLAY_A_CARD_3    = 7,
   PLAY_A_CARD_4    = 8,
   END_TURN         = 9,
   LOOP_TURN        = 10,
   END_ROUND        = 11,
   GAME_OVER        = 12
};

enum GAME_ERROR {
   NOERROR          = 0,
   ERR2CLUBS        = 1,
   ERRHEARTS        = 2,
   ERRQUEEN         = 3,
   ERRSUIT          = 4,
   ERRINVALID       = 5,
   ERRLOCKED        = 6,
   ERRCORRUPTED     = 7
};

class Engine : public QObject
{
    Q_OBJECT

private:
    struct UNDO {
       bool available = false;
       QList<int> playerHandsById[4];
       QList<int> currentTrick;
       bool hearts_broken = false;
       bool jack_diamond_in_trick = false;
       bool queen_spade_in_trick = false;
       bool cardPlayed[DECK_SIZE];
       bool busy;
       int cpt_played = 0;
       int cards_left;
       int game_score;
       int best_hand = 0;
       int trick_value = 0;
       int hand_score[4] = {};
       int total_score[4] = {};
       DIRECTION direction = PASS_LEFT;
       PLAYER best_hand_owner;
       PLAYER jack_diamond_owner;
       PLAYER turn;
       PLAYER previous_winner;
       GAME_STATUS game_status = SELECT_CARDS;
       SUIT currentSuit = SUIT::CLUBS;
    } undoStack;
    QWidget* mainWindow;
    QList<int> currentTrick;
    QList<int> playerHandsById[4];
    QList<int> validHandsById[4];
    QList<int> cardsSelected[4];
    QList<int> passedCards[4];
    QString playersName[4] = {tr("You"), tr("West"), tr("North"), tr("East")};
    DIRECTION direction = PASS_LEFT;
    PLAYER best_hand_owner;
    PLAYER jack_diamond_owner;
    PLAYER turn;
    PLAYER previous_winner;
    GAME_STATUS game_status = SELECT_CARDS;
    SUIT currentSuit = SUIT::CLUBS;

    bool variant_queen_spade = false;
    bool variant_perfect_100 = false;
    bool variant_omnibus = false;
    bool variant_no_draw = false;
    bool variant_no_tricks = false;
    bool variant_new_moon = false;

    bool locked = false;
    bool hearts_broken = false;
    bool jack_diamond_in_trick = false;
    bool queen_spade_in_trick = false;
    bool detect_tram = true;
    bool firstTime = true;
    bool busy = false;
    bool cardPlayed[DECK_SIZE];

  //  int AI_players[3];
    int playersIndex[4];
    int cpt_played = 0;
    int cards_left;
    int game_score;
    int best_hand = 0;
    int trick_value = 0;
    int hand_score[4] = {};
    int total_score[4] = {};

    void init_variables();
    void shuffle_deck();
    void generate_players_name();
    void completePassCards();
    void completePassCards(const QList<int> passedCards[4], DIRECTION direction);
    void cpus_select_cards();
    void advance_turn();
    void advance_direction();
    void countSuits(PLAYER player, int &clubs, int &spades, int &diamonds, int &hearts) const;

    bool AI_select_To_Moon(PLAYER player);
    bool AI_select_Clubs(PLAYER player);
    bool AI_select_Spades(PLAYER player);
    bool AI_select_Diamonds(PLAYER player);
    bool AI_select_Hearts(PLAYER player);
    bool AI_select_Randoms(PLAYER player);
    bool AI_elim_suit(PLAYER player, SUIT suit);
    bool AI_Ready(PLAYER player) { return passedCards[player].size() >= 3; };
    bool is_moon_an_option();
    bool is_prepass_to_moon(PLAYER player);
    bool AI_pass_friendly(PLAYER player);
    bool trySelectCardId(PLAYER player, int cardId);
    int  AI_eval_lead_hearts(DECK_INDEX cardId);
    int  AI_eval_lead_spade(DECK_INDEX cardId);
    int  AI_eval_lead_diamond(DECK_INDEX cardId);
    int  AI_eval_lead_freesuit(DECK_INDEX cardId);
    int  AI_get_cpu_move();
    int  eval_card_strength(PLAYER player, DECK_INDEX cardId);

    bool can_play_qs_first_hand(PLAYER player);
    bool is_it_draw();
    bool is_game_over();
    bool is_it_TRAM(PLAYER player);
    bool getNewMoonChoice();

    int highestCardInSuitForPlayer(PLAYER player, SUIT suit) const;
    int lowestCardInSuitForPlayer(PLAYER player, SUIT suit) const;
    int countCardsInSuit(PLAYER player, SUIT suit) const;
    int leftInSuit(SUIT suit) const;
    int getRandomNameIndex();
    void sort_players_hand();
    void check_for_best_hand(PLAYER player, int cardId);
    void update_total_scores();
    int calculate_tricks_from_tram();
    void sendGameResult();
    void shoot_moon(int player);
    void pushUndo();
    void popUndo();
    void Loop();

public:
    explicit Engine(QWidget* mainWin, QObject* parent = nullptr);
    void Start();

signals:
    void sig_your_turn();
    void sig_pass_to(DIRECTION);
    void sig_passed();
    void sig_setCurrentSuit(int);
    void sig_enableAllCards();
    void sig_clear_deck();
    void sig_refresh_deck();
    void sig_deal_cards();
    void sig_play_card(int cardId, PLAYER player);
    void sig_collect_tricks(PLAYER winner, bool TRAM);
    void sig_new_players();
    void sig_update_scores_board(const QString names[4], const int hand[4], const int total[4]);
    void sig_update_stat(int player, STATS stat);
    void sig_update_stat_score(int player, int score);
    void sig_play_sound(SOUNDS soundId);
    void sig_message(QString message, MESSAGE msgType);
    void sig_setTrickPile(QList<int> pile);
    void sig_busy(bool b);
    void sig_refresh_cards_played();
    void sig_card_played(int cardId);

public:
    void set_variant(int variant, bool enabled);
    GAME_ERROR start_new_game();
    void set_passedFromSouth(QList<int> &cards);
    void filterValidMoves(PLAYER player);
    void Step();
    void Play(int cardId, PLAYER player = PLAYER_SOUTH);
    void LockedLoop();
    bool load_game();
    bool save_game();
    bool Undo();
    bool isPlaying() { return (game_status == PLAY_A_CARD_1) || (game_status == PLAY_A_CARD_2) ||
                              (game_status == PLAY_A_CARD_3) || (game_status == PLAY_A_CARD_4) ||
                              (game_status == PLAY_TWO_CLUBS); };
    bool isBusy() { return busy || locked; };
    bool isMyTurn() { return isPlaying() && (turn == PLAYER_SOUTH); };
    bool can_break_hearts(PLAYER player);
    bool isCardPlayed(int cardId);
    int get_player_card(PLAYER player, int handIndex);
    int handSize(PLAYER player);
    void set_tram(bool enabled) { detect_tram = enabled; };

    GAME_ERROR validate_move(PLAYER player, int cardId);
    PLAYER Owner(int cardId) const;
    PLAYER Turn() { return turn; };
    GAME_STATUS Status() { return game_status; };
    DIRECTION Direction() { return direction; };
    QList<int> &Hand(int player) { return playerHandsById[player]; };
    QString &PlayerName(int player) { return playersName[player]; };
    QString errorMessage(GAME_ERROR err);
    const QList<int>& getPassedCards(PLAYER player) const { return passedCards[static_cast<int>(player)]; }
};

#endif // ENGINE_H
