#pragma once

#ifndef CONFIG_H
#define CONFIG_H

#include "deck.h"
#include "background.h"
#include <QString>
#include <QList>

enum CONFIG {
     CONFIG_AUTO_CENTERING          = 0,
     CONFIG_CHEAT_MODE              = 1,
     CONFIG_INFO_CHANNEL            = 2,
     CONFIG_SOUNDS                  = 3,
     CONFIG_DETECT_TRAM             = 4,
     CONFIG_PERFECT_100             = 5,
     CONFIG_OMNIBUS                 = 6,
     CONFIG_QUEEN_SPADE             = 7,
     CONFIG_NO_TRICK                = 8,
     CONFIG_NEW_MOON                = 9,
     CONFIG_NO_DRAW                 = 10,
     CONFIG_SAVE_GAME               = 11,
     CONFIG_LANGUAGE                = 12,
     CONFIG_EASY_CARD_SELECTION     = 13,
     CONFIG_CARD_DISPLAY            = 14,
     CONFIG_AUTO_START              = 15,
     CONFIG_DECK_STYLE              = 16,
     CONFIG_ANIMATED_PLAY           = 17,
     CONFIG_USERNAME                = 18,
     CONFIG_PASSWORD                = 19,
     CONFIG_WARNING                 = 20,
     CONFIG_SHOW_DIRECTION          = 21,
     CONFIG_EMPTY_SLOT              = 22,
     CONFIG_ANIM_DEAL_CARDS         = 23,
     CONFIG_ANIM_PLAY_CARD          = 24,
     CONFIG_ANIM_COLLECT_TRICKS     = 25,
     CONFIG_ANIM_PASS_CARDS         = 26,
     CONFIG_ANIMATED_ARROW          = 27,
     CONFIG_ANIM_TURN_INDICATOR     = 28,
     CONFIG_CONFIRM_EXIT            = 29,
     CONFIG_CHEAT_REVEAL            = 30
};

constexpr int HEARTS_TEXT_ONLY               = 0;
constexpr int HEARTS_ICONS_PINK              = 1;
constexpr int HEARTS_ICONS_GREY              = 2;
constexpr int HEARTS_ICONS_SUIT              = 3;
constexpr int HEARTS_ICONS_CPU               = 4;

constexpr int SPEED_SLOW                     = 0;
constexpr int SPEED_NORMAL                   = 1;
constexpr int SPEED_FAST                     = 2;
constexpr int SPEED_EXPERT                   = 3;

constexpr int SPEED_PLAY_CARD                = 0;
constexpr int SPEED_PLAY_TWO_CLUBS           = 1;
constexpr int SPEED_SHUFFLE                  = 2;
constexpr int SPEED_CLEAR_TABLE              = 3;
constexpr int SPEED_YOUR_TURN                = 4;
constexpr int SPEED_PASS_CARDS               = 5;
constexpr int SPEED_ANIMATE_PASS_CARDS       = 6;
constexpr int SPEED_ANIMATE_PLAY_CARD        = 7;
constexpr int MAX_SPEEDS                     = 8;

constexpr int LANG_ENGLISH                   = 0;
constexpr int LANG_FRENCH                    = 1;
constexpr int LANG_RUSSIAN                   = 2;

constexpr int SPEED_PLAY_CARD_VALUES[3]      = {550, 400, 200};
constexpr int SPEED_PLAY_TWO_CLUBS_VALUES[3] = {1000, 700, 350};
constexpr int SPEED_SHUFFLE_VALUES[3]        = {1550, 1200, 1050};
constexpr int SPEED_CLEAR_TABLE_VALUES[3]    = {350, 200, 100};
constexpr int SPEED_YOUR_TURN_VALUES[3]      = {300, 200, 100};
constexpr int SPEED_PASS_CARDS_VALUES[3]     = {2500, 2000, 1000};
constexpr int SPEED_ANIMATE_PASS_CARDS_VALUES[3] = {22, 10, 4};
constexpr int SPEED_ANIMATE_PLAY_CARD_VALUES[3]  = {200, 170, 130};

constexpr char CONFIG_FILENAME[10]  = "/.hearts";

class Config
{
private:
    QString background_path = "";
    QString username = "";
    QString password = "";

    bool save_config_file();
    bool load_config_file();

    int language = LANG_ENGLISH;
    int deck_style = NICU_WHITE_DECK;
    int speed = SPEED_NORMAL;
    int background_index = BACKGROUND_GREEN;
    int hearts_style = HEARTS_TEXT_ONLY;
    int expert_speeds[MAX_SPEEDS] = {550, 1000, 1550, 350, 300, 2500, 22, 200};
    int appWidth = 825;
    int appHeight = 600;
    int appPosX = 200;
    int appPosY = 200;

    bool warning = true;

    // game variants
    bool perfect_100 = false;
    bool omnibus = false;
    bool queen_spade_break_heart = false;
    bool no_trick_bonus = false;
    bool new_moon = false;
    bool no_draw = false;
    bool auto_start = false;

    // settings
    bool animated_play = true;
    bool auto_centering = true;
    bool cheat_mode = false;
    bool info_channel = false;
    bool show_direction = true;
    bool sounds = true;
    bool detect_tram = true;
    bool easy_card_selection = true;
    bool card_display = false;
    bool save_game = true;
    bool empty_slot_opaque = true;
    bool confirm_exit = true;
    bool cheat_reveal = false;

    // Animations
    bool animations = true;
    bool anim_deal_cards = true;
    bool anim_play_card = true;
    bool anim_collect_tricks = true;
    bool anim_pass_cards = true;
    bool animated_arrow = true;
    bool anim_turn_indication = true;

public:
    Config();

public:
    void set_online(QString u, QString p);
    bool set_config_file(int param, bool enable);
    bool set_language(int lang);
    bool set_deck_style(int style);
    bool set_hearts_style(int style);
    bool set_speed(int s);
    bool set_expert_speeds(QList<int> values);
    bool set_background_path(const QString &path);
    bool set_width(int w);
    bool set_height(int h);
    bool set_posX(int x);
    bool set_posY(int y);

    QString &Username() { return username; };
    QString &Password() { return password; };
    QString &Path() { return background_path; };

    int get_language() { return language; };
    int get_background_index() { return background_index; };
    int get_hearts_style() { return deck_style; };
    int get_speed() { return speed; };
    int get_deck_style() { return deck_style; };
    int get_speed(int type);
    int width() { return appWidth; };
    int height() { return appHeight; };
    int posX() { return appPosX; };
    int posY() { return appPosY; };

    bool Warning() { return warning; };

    bool is_auto_centering() { return auto_centering; };
    bool is_cheat_mode() { return cheat_mode; };
    bool is_info_channel() { return info_channel; };
    bool is_sounds() { return sounds; };
    bool is_detect_tram() { return detect_tram; };

    bool is_perfect_100() { return perfect_100; };
    bool is_omnibus() { return omnibus; };
    bool is_queen_spade_break_heart() { return queen_spade_break_heart; };
    bool is_no_trick_bonus() { return no_trick_bonus; };
    bool is_new_moon() { return new_moon; };
    bool is_no_draw() { return no_draw; };
    bool is_save_game() { return save_game; };
    bool is_easy_card_selection() { return easy_card_selection; };
    bool is_card_display() { return card_display; };
    bool is_auto_start() { return auto_start; };
    bool is_animated_play() { return animated_play; };
    bool is_show_direction() { return show_direction; };
    bool is_empty_slot_opaque() { return empty_slot_opaque; };
    bool is_anim_deal_cards() { return anim_deal_cards; };
    bool is_anim_play_card() { return anim_play_card; };
    bool is_anim_collect_tricks() { return anim_collect_tricks; };
    bool is_anim_pass_cards() { return anim_pass_cards; };
    bool is_animated_arrow() { return animated_arrow; };
    bool is_anim_turn_indicator() { return anim_turn_indication; };
    bool is_confirm_exit() { return confirm_exit; };
    bool is_cheat_reveal() { return cheat_reveal; };
};

#endif // CONFIG_H
