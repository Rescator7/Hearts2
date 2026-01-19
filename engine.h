#ifndef ENGINE_H
#define ENGINE_H

#include "define.h"

#include <QObject>
#include <QList>
#include <QString>

constexpr int VARIANT_QUEEN_SPADE = 0;
constexpr int VARIANT_PERFECT_100 = 1;
constexpr int VARIANT_OMNIBUS     = 2;
constexpr int VARIANT_NO_TRICKS   = 3;
constexpr int VARIANT_NEW_MOON    = 4;
constexpr int VARIANT_NO_DRAW     = 5;

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
   ERRINVALID       = 5
};

class Engine : public QObject
{
    Q_OBJECT

private:
    QList<int> currentTrick;
    QList<int> playerHandsById[4];
    QList<int> validHandsById[4];
    QList<int> cardsSelected[4];
    QList<int> passedCards[4];
    QString playersName[4] = {"South", "West", "North", "East"};
    DIRECTION direction = PASS_LEFT;
    bool variant_queen_spade = false;
    bool variant_perfect_100 = false;
    bool variant_omnibus = false;
    bool variant_no_draw = false;
    bool variant_no_tricks = false;
    bool variant_new_moon = false;
    GAME_STATUS game_status = SELECT_CARDS;
    SUIT currentSuit = SUIT::CLUBS;
    int AI_players[3];
    int cpt_played = 0;
    int cards_left;
    int game_score;
    int best_hand = 0;
    int trick_value = 0;
    int hand_score[4] = {};
    int total_score[4] = {};
    PLAYER best_hand_owner;
    bool hearts_broken = false;
    PLAYER turn;

    void init_variables();
    void shuffle_deck();
    void generate_players_name();
    void completePassCards();
    void completePassCards(const QList<int> passedCards[4], DIRECTION direction);
    void cpus_select_cards();
    void advance_turn();
    void countSuits(PLAYER player, int &clubs, int &spades, int &diamonds, int &hearts) const;

    bool AI_select_To_Moon(PLAYER player);
    bool AI_select_Clubs(PLAYER player);
    bool AI_select_Spades(PLAYER player);
    bool AI_select_Diamonds(PLAYER player);
    bool AI_select_Hearts(PLAYER player);
    bool AI_select_Randoms(PLAYER player);
    bool can_play_qs_first_hand(PLAYER player);

    int countCardsInSuit(PLAYER player, SUIT suit) const;
    int getRandomNameIndex();
    bool load_saved_game();
    void sort_players_hand();
    bool is_it_TRAM();
    void check_for_best_hand(PLAYER player, int cardId);
    void update_total_scores();
    void Loop();

public:
    Engine(QObject *parent);
    void Start();

signals:
    void sig_your_turn();
    void sig_pass_to(DIRECTION);
    void sig_passed();
    void sig_setCurrentSuit(int);
    void sig_enableAllCards();
    void sig_clear_deck();
    void sig_shuffle_deck();
    void sig_refresh_deck();
    void sig_deal_cards();
    void sig_hearts_broken();
    void sig_queen_spade();
    void sig_play_card(int cardId, PLAYER player);
    void sig_collect_tricks(PLAYER winner, bool TRAM);
    void sig_new_players(const QString names[4]);
    void sig_update_scores_board(const QString names[4], const int hand[4], const int total[4]);

public:
    void set_variant(int variant, bool enabled);
    void start_new_game();
    void set_passedFromSouth(QList<int> &cards);
    void filterValidMoves(PLAYER player);
    void Step();
    void Play(int cardId, PLAYER player = PLAYER_SOUTH);
    bool undo();
    bool isPlaying() { return (game_status == PLAY_A_CARD_1) || (game_status == PLAY_A_CARD_2) ||
                              (game_status == PLAY_A_CARD_3) || (game_status == PLAY_A_CARD_4) ||
                              (game_status == PLAY_TWO_CLUBS); };
    bool can_break_hearts(PLAYER player);
    int get_player_card(PLAYER player, int handIndex);
    int player_hand_size(PLAYER player);
    int find_card_owner(int cardId);
    GAME_ERROR validate_move(PLAYER player, int cardId);

    PLAYER Owner(int cardId) const;
    PLAYER Turn() { return turn; };
    GAME_STATUS Status() { return game_status; };
    DIRECTION Direction() { return direction; };
    QList<int> &Hand(int player) { return playerHandsById[player]; };
    QString &PlayerName(int player) { return playersName[player]; };
    QString errorMessage(GAME_ERROR err) const;
    const QList<int>& getPassedCards(PLAYER player) const { return passedCards[static_cast<int>(player)]; }
};

#endif // ENGINE_H
