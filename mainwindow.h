#pragma once

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QResizeEvent>
#include <QShowEvent>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QEasingCurve>
#include <QRandomGenerator>
#include <QMovie>
#include <QLabel>
#include <QGraphicsProxyWidget>
#include <QList>
#include <QButtonGroup>
#include <QGraphicsTextItem>
#include <QPoint>

#include "sounds.h"
#include "deck.h"
#include "cardscene.h"
#include "turnarrow.h"
#include "background.h"
#include "engine.h"
#include "config.h"
#include "draggablescoregroup.h"
#include "statistics.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

constexpr int MAX_DECK = 8;

/*
enum ScoreAnchorSide {
    LeftSide,
    RightSide,
    TopSide,
    BottomSide
};
*/

enum MESSAGE {
  MESSAGE_ERROR  = 0,
  MESSAGE_SYSTEM = 1,
  MESSAGE_INFO   = 2
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

struct TrickCardInfo {
    int cardId;
    qreal rotation = 0.0;
    QPointF position;
    int ownerPlayer;  // optionnel, pour savoir à qui appartenait la carte
};

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QParallelAnimationGroup *animateCardsOffBoard(const QList<int>& cardsId, int winner);
    void animateCardsToCenter(const QList<int> &cardsId,
                                      QParallelAnimationGroup *masterGroup,
                                      int startDelay,
                                      int perCardStagger);
private slots:
    void onCardClicked(QGraphicsItem *item);
    void onBackgroundPreviewClicked();
    void onDeckChanged(int deckId);
    void onArrowClicked();
    void aboutToQuit();
    void on_opt_animations_clicked();
    void on_pushButton_score_clicked();

private:
/*
    QSize lastViewSize = QSize(0, 0);
    ScoreAnchorSide scoreAnchorSideX = LeftSide;
    ScoreAnchorSide scoreAnchorSideY = BottomSide;
    qreal scoreAnchorDistX = 20.0;
    qreal scoreAnchorDistY = 80.0;
    QPointF savedScorePos = QPointF(20, 100);
    bool scorePositionSaved = false;
*/

    enum class PlayerIcon { Human, Bot, Remote, None };

    Statistics* statistics = nullptr;
    QTimer* newGameDebounceTimer = nullptr;
    QSize lastBoardViewSize = QSize(0, 0);
    bool windowWasResizedWhileAway = false;
    PlayerIcon icons[4] = {PlayerIcon::Human, PlayerIcon::Bot, PlayerIcon::Bot, PlayerIcon::Bot};
    DraggableScoreGroup *m_scoreGroup = nullptr;
    QGraphicsTextItem *m_scoreText = nullptr;
    QGraphicsRectItem *m_scoreBackground = nullptr;
    Ui::MainWindow *ui;
    Sounds *sounds = nullptr;
    Deck *deck = nullptr;
    Background *background = nullptr;
    CardScene *scene = nullptr;
    Config *config = nullptr;
    Engine *engine = nullptr;
    QGraphicsPixmapItem *backgroundItem = nullptr;
    TurnArrow *yourTurnIndicator = nullptr;
    QPixmap originalBgPixmap;
    QMovie *arrowMovie = nullptr;
    QLabel *arrowLabel = nullptr;
    QGraphicsProxyWidget *arrowProxy = nullptr;
    QList<int> currentTrick;               // Cards in current trick
    QList<int> selectedCards;
    QPointF trickCenter;                   // Center position for trick pile
    QPointF getTakenPileCenter(int player, const QSize& viewSize) const;
    QPointF calculatePlayerNamePos(int player) const;
    QSize m_fixedSizeDuringLock;           // current size when first locked
    QButtonGroup *deckGroup;
    QButtonGroup *variantsGroup;
    QButtonGroup *languageGroup;
    QButtonGroup *animationsGroup;
    QGraphicsTextItem *creditsText = nullptr;
    QGraphicsTextItem *playerNamesItem[4] = {nullptr, nullptr, nullptr, nullptr};
    QList<TrickCardInfo> currentTrickInfo;

    bool forced_new_deck = false;
    bool start_engine_dalayed = false;
    bool valid_deck[MAX_DECK] = {true, true, true, true, true, true, true, true};
    int currentTrickZ = Z_TRICKS_BASE;
    int m_animationLockCount = 0;

    void saveCurrentTrickState();
    void restoreTrickCards();
    void disableAllDecks();
    void enableAllDecks();

public:
//    void saveDragPosition(QPointF pos) {savedScorePos = pos; };
    void message(QString mesg, MESSAGE type);
    void updateBackground();
    void updateTrickPile();
    void updateYourTurnIndicator();
    void updateCredits(const QString &text, const QColor &color);
    void updatePlayerNames();
    void set_card_pos(int cardId, int player, int handIndex, bool frontCard);
    void refresh_deck();
    void animateDeal(int dealDuration = 600, int delayBetweenCards = 40);
    void animateYourTurnIndicator();
    void animatePassCards(const QList<int> passedCardsById[4], int direction);
    void animatePlayCard(int cardId, int player);
    void playCard(int cardId, int player);
    void animateCollectTrick(int winner, bool takeAllRemaining);
    void highlightAIPassedCards(int player, const QList<int>& passedCardIds);
    void showTurnArrow(int direction);
    void updateTurnArrow();
    void updateScores(const QString playersName[], const int handScores[4], const int totalScores[4], const PlayerIcon icons[4]);
    void updateScorePosition();
    void create_arrows();
    void createScoreDisplay();
    void adjustGraphicsViewSize();
    void updateSceneRect();
    void updateCreditsPosition();
    void repositionAllCardsAndElements();
    void showTurnIndicator();
    void setAnimationLock();
    void setAnimationUnlock();
    void enableAllCards();
 //   void setCurrentSuit(int legalSuit);
    void disableInvalidCards();
    void removeInvalidCardsEffect();
    void applyAllSettings();
    void createButtonsGroups();
    void createCreditsLabel();
    void loadHelpFile();
    void setCheatMode(bool enabled);

    // Setters (publics)
    /*
    void setScoreAnchorSideX(ScoreAnchorSide side) { scoreAnchorSideX = side; }
    void setScoreAnchorSideY(ScoreAnchorSide side) { scoreAnchorSideY = side; }
    void setScoreAnchorDistX(qreal dist) { scoreAnchorDistX = qMax(0.0, dist); }
    void setScoreAnchorDistY(qreal dist) { scoreAnchorDistY = qMax(0.0, dist); }
    void setLastViewSize(QSize size) { lastViewSize = size; };
    */

/*
    void setSavedScorePos(QPointF pos) { savedScorePos = pos; };
    void setScorePositionSaved(bool saved) { scorePositionSaved = saved; };
    void setDraggingScore(bool drag) { isDragging = drag; };
*/

    void createPlayerNames();
    void disableDeck(int deckId);
    void setAnimationButtons(bool enabled);
    void revealPlayer(int player);
    void clearTricks();
    void pushTrickInfo(int cardId, int owner);
    void import_allCards_to_scene();
    bool loadBackgroundPreview(const QString &image_path);
    int cardZ(int player, int cardID);
    int cardX(int cardId, int player, int handIndex, bool frontCard);
    int cardY(int cardId, int player, int handIndex, bool frontCard);
    qreal anim_rotation_value(int player);
    qreal calc_scale() const;
};
#endif // MAINWINDOW_H
