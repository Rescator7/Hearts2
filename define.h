#pragma once

#ifndef DEFINE_H
#define DEFINE_H

constexpr const char VERSION[] = "Hearts 2.0";

constexpr int MIN_APPL_WIDTH = 500;
constexpr int MIN_APPL_HEIGHT = 460;

enum MESSAGE {
  MESSAGE_ERROR  = 0,
  MESSAGE_SYSTEM = 1,
  MESSAGE_INFO   = 2
};

enum TABS {
   TAB_BOARD        = 0,
   TAB_CHANNEL      = 1,
   TAB_STATISTICS   = 2,
   TAB_CARDS_PLAYED = 3,
   TAB_HELP         = 4,
   TAB_SETTINGS     = 5
};

constexpr int SELECTED_CARD_OFFSET = 40;

enum PLAYER  {
   PLAYER_SOUTH     = 0,
   PLAYER_WEST      = 1,
   PLAYER_NORTH     = 2,
   PLAYER_EAST      = 3,
   PLAYER_COUNT     = 4,
   PLAYER_NOBODY    = -1
};

enum ZLayer {
   Z_BACKGROUND     = -1000,
   Z_CREDITS        =     5,
   Z_CARDS_BASE     =   100,
   Z_TRICKS_BASE    =   200,
   Z_PLAYERS_NAME   =   300,
   Z_YOUR_TURN      =   400,
   Z_ARROW          =   500,
   Z_PASSED_BASE    =   600,
   Z_OFFBOARD       =   800,
   Z_SCORE          =  1000
};

enum SUIT {
   CLUBS        = 0,
   SPADES       = 1,
   DIAMONDS     = 2,
   HEARTS       = 3,
   SUIT_COUNT   = 4,
   SUIT_ALL     = 5,
};

enum TocIndex {
    TOC_Rules = 0,
    TOC_Settings,
    TOC_Variants,
    TOC_Miscellaneous,
    TOC_Scoreboard,
    TOC_Shortcuts,
    TOC_Online,
    TOC_Credits,
    TOC_Decks,
    TOC_Icons,
    TOC_Backgrounds,
    TOC_Sounds,
    TOC_Special
};

constexpr bool isValid(SUIT suit) {
    return (suit >= 0) && (suit < SUIT_COUNT);
}

constexpr bool isValid(PLAYER player) {
    return (player >= 0) && (player < PLAYER_COUNT);
}

#endif // DEFINE_H
