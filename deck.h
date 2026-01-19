#pragma once

#ifndef DECK_H
#define DECK_H

#include <QGraphicsItem>
#include <QGraphicsSvgItem>
#include <QSize>
#include <QList>
#include <QObject>
#include <QSvgRenderer>

constexpr int UNSET_DECK                  = -1;
constexpr int STANDARD_DECK               = 0;
constexpr int NICU_WHITE_DECK             = 1;
constexpr int ENGLISH_DECK                = 2;
constexpr int RUSSIAN_DECK                = 3;
constexpr int KAISER_JUBILAUM             = 4;
constexpr int TIGULLIO_INTERNATIONAL_DECK = 5;
constexpr int MITTELALTER_DECK            = 6;
constexpr int NEOCLASSICAL_DECK           = 7;
constexpr int TIGULLIO_MODERN_DECK        = 8;  // unsuported --> LAST_DECK is 7
constexpr int LAST_DECK                   = 7;

constexpr int LEAD_SUIT_CLUB              = 0;
constexpr int LEAD_SUIT_SPADE             = 1;
constexpr int LEAD_SUIT_DIAMOND           = 2;
constexpr int LEAD_SUIT_HEART             = 3;

constexpr int  CLUBS_TWO   = 0;
constexpr int  CLUBS_THREE = 1;
constexpr int  CLUBS_FOUR  = 2;
constexpr int  CLUBS_FIVE  = 3;
constexpr int  CLUBS_SIX   = 4;
constexpr int  CLUBS_SEVEN = 5;
constexpr int  CLUBS_EIGHT = 6;
constexpr int  CLUBS_NINE  = 7;
constexpr int  CLUBS_TEN   = 8;
constexpr int  CLUBS_JACK  = 9;
constexpr int  CLUBS_QUEEN = 10;
constexpr int  CLUBS_KING  = 11;
constexpr int  CLUBS_ACE   = 12;

constexpr int  SPADES_TWO   = 13;
constexpr int  SPADES_THREE = 14;
constexpr int  SPADES_FOUR  = 15;
constexpr int  SPADES_FIVE  = 16;
constexpr int  SPADES_SIX   = 17;
constexpr int  SPADES_SEVEN = 18;
constexpr int  SPADES_EIGHT = 19;
constexpr int  SPADES_NINE  = 20;
constexpr int  SPADES_TEN   = 21;
constexpr int  SPADES_JACK  = 22;
constexpr int  SPADES_QUEEN = 23;
constexpr int  SPADES_KING  = 24;
constexpr int  SPADES_ACE   = 25;

constexpr int  DIAMONDS_TWO   = 26;
constexpr int  DIAMONDS_THREE = 27;
constexpr int  DIAMONDS_FOUR  = 28;
constexpr int  DIAMONDS_FIVE  = 29;
constexpr int  DIAMONDS_SIX   = 30;
constexpr int  DIAMONDS_SEVEN = 31;
constexpr int  DIAMONDS_EIGHT = 32;
constexpr int  DIAMONDS_NINE  = 33;
constexpr int  DIAMONDS_TEN   = 34;
constexpr int  DIAMONDS_JACK  = 35;
constexpr int  DIAMONDS_QUEEN = 36;
constexpr int  DIAMONDS_KING  = 37;
constexpr int  DIAMONDS_ACE   = 38;

constexpr int  HEARTS_TWO   = 39;
constexpr int  HEARTS_THREE = 40;
constexpr int  HEARTS_FOUR  = 41;
constexpr int  HEARTS_FIVE  = 42;
constexpr int  HEARTS_SIX   = 43;
constexpr int  HEARTS_SEVEN = 44;
constexpr int  HEARTS_EIGHT = 45;
constexpr int  HEARTS_NINE  = 46;
constexpr int  HEARTS_TEN   = 47;
constexpr int  HEARTS_JACK  = 48;
constexpr int  HEARTS_QUEEN = 49;
constexpr int  HEARTS_KING  = 50;
constexpr int  HEARTS_ACE   = 51;

constexpr int  BACK_CARD    = 52;
constexpr int  INVALID_CARD = 127;

constexpr int  DECK_SIZE    = 52;

constexpr int TARGET_CARD_WIDTH   = 90;

constexpr int PNG_FILE_FORMAT     = 0;
constexpr int SVG_53_FILES_FORMAT = 1;
constexpr int SVG_1_FILE_FORMAT   = 2;

class Deck : public QObject
{
    Q_OBJECT

private:
    QList<QGraphicsSvgItem *> deck;
    int current_deck_style = UNSET_DECK;
    QGraphicsSvgItem *loadGraphicsItem(const QString &filePath);
    QSvgRenderer *renderer = nullptr;

signals:
    void deckChange(int deckId);

public:
    explicit Deck(QObject *parent = nullptr);
    ~Deck();
    bool check_file(QString &filename);
    bool set_deck(int style);
    void delete_current_deck();
    void apply_settings(QGraphicsItem *item, int cardId);
    void reset_selections();
    int Style() { return current_deck_style; };
    QPixmap loadToPixmap(const QString &filePath);
    QGraphicsSvgItem* get_card_item(int cardId, bool revealed);
    QGraphicsSvgItem* createCardItem(int cardId, bool suitFirst);
    QGraphicsSvgItem* createSvgCardElement(int cardId, bool suitFirst);
};

#endif // DECK_H
