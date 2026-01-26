#pragma once

#ifndef DEFINE_H
#define DEFINE_H

constexpr const char VERSION[] = "Hearts 2.0";
constexpr const char INSTALL_PATH[] = "/usr/local/Hearts";
constexpr const char FOLDER[] = "/DEV/Hearts2/";

constexpr int MIN_APPL_WIDTH = 500;
constexpr int MIN_APPL_HEIGHT = 460;

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
   SUIT_NONE    = 6
};

constexpr bool isValid(SUIT suit) {
    return (suit >= 0) && (suit < SUIT_COUNT);
}

constexpr bool isValid(PLAYER player) {
    return (player >= 0) && (player < PLAYER_COUNT);
}

#endif // DEFINE_H
