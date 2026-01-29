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

enum DECK_INDEX {
     CLUBS_TWO   = 0,
     CLUBS_THREE = 1,
     CLUBS_FOUR  = 2,
     CLUBS_FIVE  = 3,
     CLUBS_SIX   = 4,
     CLUBS_SEVEN = 5,
     CLUBS_EIGHT = 6,
     CLUBS_NINE  = 7,
     CLUBS_TEN   = 8,
     CLUBS_JACK  = 9,
     CLUBS_QUEEN = 10,
     CLUBS_KING  = 11,
     CLUBS_ACE   = 12,

     SPADES_TWO   = 13,
     SPADES_THREE = 14,
     SPADES_FOUR  = 15,
     SPADES_FIVE  = 16,
     SPADES_SIX   = 17,
     SPADES_SEVEN = 18,
     SPADES_EIGHT = 19,
     SPADES_NINE  = 20,
     SPADES_TEN   = 21,
     SPADES_JACK  = 22,
     SPADES_QUEEN = 23,
     SPADES_KING  = 24,
     SPADES_ACE   = 25,

     DIAMONDS_TWO   = 26,
     DIAMONDS_THREE = 27,
     DIAMONDS_FOUR  = 28,
     DIAMONDS_FIVE  = 29,
     DIAMONDS_SIX   = 30,
     DIAMONDS_SEVEN = 31,
     DIAMONDS_EIGHT = 32,
     DIAMONDS_NINE  = 33,
     DIAMONDS_TEN   = 34,
     DIAMONDS_JACK  = 35,
     DIAMONDS_QUEEN = 36,
     DIAMONDS_KING  = 37,
     DIAMONDS_ACE   = 38,

     HEARTS_TWO   = 39,
     HEARTS_THREE = 40,
     HEARTS_FOUR  = 41,
     HEARTS_FIVE  = 42,
     HEARTS_SIX   = 43,
     HEARTS_SEVEN = 44,
     HEARTS_EIGHT = 45,
     HEARTS_NINE  = 46,
     HEARTS_TEN   = 47,
     HEARTS_JACK  = 48,
     HEARTS_QUEEN = 49,
     HEARTS_KING  = 50,
     HEARTS_ACE   = 51,

     BACK_CARD    = 52,
     INVALID_CARD = 127
};

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
    QList<QPixmap> pixImages;
    int current_deck_style = UNSET_DECK;
    QGraphicsSvgItem *loadGraphicsItem(const QString &filePath);
    QSvgRenderer *renderer = nullptr;
    void createPixmap(int cardId, bool suitFirst, int format);
    QString getElementIdForCard(int cardId, bool suitFirst) const;

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
    const QPixmap& get_card_pixmap(int cardId) const;
    QGraphicsSvgItem* createCardItem(int cardId, bool suitFirst);
    QGraphicsSvgItem* createSvgCardElement(int cardId, bool suitFirst);
};

#endif // DECK_H
