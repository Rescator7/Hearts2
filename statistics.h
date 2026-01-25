#ifndef CSTATISTICS_H
#define CSTATISTICS_H

#include <QWidget>
#include <QTableWidgetItem>

#include "cpus.h"

constexpr int FNOERR     =  0;
constexpr int FCORRUPTED =  1;
constexpr int ERROPENRO  =  2;
constexpr int ERROPENWO  =  3;

const char STATS_FILENAME[20] = "/.hearts.stats";
const char STATS_BACKUP_FILE[20] = "/.hearts.stats.bak";

enum STATS {
     STATS_GAME_STARTED        = 0,
     STATS_GAME_FINISHED       = 1,
     STATS_HANDS_PLAYED        = 2,
     STATS_SCORES              = 3,
     STATS_FIRST_PLACE         = 4,
     STATS_SECOND_PLACE        = 5,
     STATS_THIRD_PLACE         = 6,
     STATS_FOURTH_PLACE        = 7,
     STATS_SHOOT_MOON          = 8,
     STATS_QUEEN_SPADE         = 9,
     STATS_OMNIBUS             = 10,
     STATS_NO_TRICKS           = 11,
     STATS_PERFECT_100         = 12,
     STATS_UNDO                = 13
};

namespace Ui {
class Statistics;
}

class Statistics : public QWidget
{
    Q_OBJECT

public:
    explicit Statistics(QWidget *parent = nullptr);
    ~Statistics();

signals:
    void sig_message(const QString &message);

private:
    Ui::Statistics *ui;
    bool file_corrupted;

    QTableWidgetItem *item_names[MAX_PLAYERS_NAME];
    QTableWidgetItem *item_first_place[MAX_PLAYERS_NAME];
    QTableWidgetItem *item_second_place[MAX_PLAYERS_NAME];
    QTableWidgetItem *item_third_place[MAX_PLAYERS_NAME];
    QTableWidgetItem *item_fourth_place[MAX_PLAYERS_NAME];
    QTableWidgetItem *item_avg_score[MAX_PLAYERS_NAME];
    QTableWidgetItem *item_best_score[MAX_PLAYERS_NAME];
    QTableWidgetItem *item_worst_score[MAX_PLAYERS_NAME];
    QTableWidgetItem *item_shoot_moon[MAX_PLAYERS_NAME];
    QTableWidgetItem *item_queen_spade[MAX_PLAYERS_NAME];
    QTableWidgetItem *item_omnibus[MAX_PLAYERS_NAME];
    QTableWidgetItem *item_no_tricks[MAX_PLAYERS_NAME];
    QTableWidgetItem *item_perfect_100[MAX_PLAYERS_NAME];

    int game_started;
    int game_finished;
    int hands_played;
    int undo;

    int first_place[MAX_PLAYERS_NAME];
    int second_place[MAX_PLAYERS_NAME];
    int third_place[MAX_PLAYERS_NAME];
    int fourth_place[MAX_PLAYERS_NAME];

    int total_score[MAX_PLAYERS_NAME];
    int best_score[MAX_PLAYERS_NAME];
    int worst_score[MAX_PLAYERS_NAME];
    int shoot_moon[MAX_PLAYERS_NAME];
    int queen_spade_taken[MAX_PLAYERS_NAME];
    int omnibus_bonus_taken[MAX_PLAYERS_NAME];
    int no_tricks_bonus_taken[MAX_PLAYERS_NAME];
    int perfect_100_bonus_taken[MAX_PLAYERS_NAME];

private: // functions
    void init_vars();
    void initTableContent();
    void update_window(int plr, int stats);

public: // functions
    bool is_file_corrupted();

    int  load_stats_file();
    int  save_stats_file();

    void increase_stats(int plr, int stats);
    void set_score(int plr, int score);
    void reset();
    void show_stats();
    void Translate();
};

#endif // CSTATISTICS_H
