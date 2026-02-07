#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "define.h"
#include "resourcepaths.h"

#include <QList>
#include <QDir>
#include <QFileDialog>
#include <QTimer>
#include <QVariantAnimation>
#include <QRandomGenerator>
#include <QSequentialAnimationGroup>
#include <QPauseAnimation>
#include <QButtonGroup>
#include <QGraphicsColorizeEffect>
#include <QGraphicsItemAnimation>
#include <QTimeLine>
#include <QPropertyAnimation>  // Pas obligatoire ici, mais au cas où
#include <QMessageBox>
#include <QWindow>
#include <QListWidget>
#include <QShortcut>
#include <QKeySequence>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Protect side players' cards from clipping
    setMinimumHeight(MIN_APPL_HEIGHT);

    ui->graphicsView->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    ui->graphicsView->setCacheMode(QGraphicsView::CacheBackground);

    ui->splitterHelp->setSizes(QList<int>() << 250 << 750);

    setWindowTitle(VERSION);

    message(tr("Welcome to ") + QString(VERSION), MESSAGE_SYSTEM);

    config = new Config();

    statistics = new Statistics(this);
    ui->statisticsLayout->addWidget(statistics);

    sounds = new Sounds(this);

    scene = new CardScene(this);

    // scene->setItemIndexMethod(QGraphicsScene::NoIndex);  // Moins de repaint inutiles
    //ui->graphicsView->setRenderHint(QPainter::Antialiasing, false);

    deck = new Deck(this);

    engine = new Engine(this, this);

    ui->graphicsView->setScene(scene);

    background = new Background(scene, this);
    scene->protectItem(background);

    initCardsPlayedPointers();

    createButtonsGroups();

    create_arrows();

    createPlayerNames();

    applyAllSettings();

    ui->channel->setStyleSheet(
      "color: yellow;"              // Text color
      "background-color: black;"    // Background of the widget itself
      "selection-background-color: black;"  // Optional: keeps selection bg black
    );
    ui->channel->setReadOnly(true);

    ui->labelBackgroundPreview->setScaledContents(true);
    ui->labelBackgroundPreview->setAlignment(Qt::AlignCenter);
    ui->labelBackgroundPreview->setMouseTracking(true);
    ui->labelBackgroundPreview->setCursor(Qt::PointingHandCursor);
    ui->labelBackgroundPreview->installEventFilter(this);
    ui->labelBackgroundPreview->setStyleSheet("QLabel { border-radius: 10px; background: lightgray; border: 2px solid gray; }"
                                              "QLabel:hover { border: 4px solid white; }");

// Overlay label (top)
    ui->labelPreviewOverlay->setAlignment(Qt::AlignCenter);
    ui->labelPreviewOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    ui->labelPreviewOverlay->setVisible(false);  // Initially hidden
    ui->labelPreviewOverlay->setStyleSheet("border-radius: 10px;"
                                           "background-color: gray;"
                                           "color: white;"
                                           "font-size: 12px;"
//                                         "font-weight: bold;"
                                           "background-color: rgba(0,0,0,180);"
//                                         "padding: 15px;"
                                           );

    // We must delay the creation of the credits label or it won't be added to the scene.
    // We must delay the creation of the scores board, or it size and position will be wrong.
    QTimer::singleShot(0, this, [this]() {
      createCreditsLabel();

      updatePlayerNames();

      createScoreDisplay();

      setLanguage();  // creditsLabel must be created first.

      int handScores[4] = {0, 0, 0, 0};
      int totalScores[4] = {0, 0, 0, 0};
      QString names[4] = {"South", "West", "North", "East"};

      scene->protectItem(m_scoreGroup);
      scene->protectItem(m_scoreText);
      scene->protectItem(m_scoreBackground);
      updateScores(names, handScores, totalScores, icons);
    });

// Optional: disable scrollbars for clean look
    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

// Optional: nicer rendering when scaled
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);
    ui->graphicsView->setRenderHint(QPainter::SmoothPixmapTransform);

// Important anchors
    ui->graphicsView->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    ui->graphicsView->setAlignment(Qt::AlignCenter);
    ui->graphicsView->setSceneRect(scene->itemsBoundingRect());

// Solid green background (no repeat)
    ui->graphicsView->setBackgroundBrush(QBrush(QColor(0, 100, 0)));  // dark green felt

    connect(scene, &CardScene::cardClicked, this, &MainWindow::onCardClicked);
    connect(scene, &CardScene::arrowClicked, this, &MainWindow::onArrowClicked);

    connect(deck, &Deck::deckChange, this, &MainWindow::onDeckChanged);

    connect(ui->pushButton_reveal, &QPushButton::clicked, this, [this](bool checked) {
      refresh_deck();
      config->set_config_file(CONFIG_CHEAT_REVEAL, checked);
    });

    connect(ui->checkBox_confirm_exit, &QCheckBox::clicked, this, [this](bool checked) {
      config->set_config_file(CONFIG_CONFIRM_EXIT, checked);
    });

    connect(ui->checkBox_tram, &QCheckBox::clicked, this, [this](bool checked) {
      config->set_config_file(CONFIG_DETECT_TRAM, checked);
      engine->set_tram(checked);
    });

    connect(ui->checkBox_cheat, &QCheckBox::clicked, this, [this](bool checked) {
      config->set_config_file(CONFIG_CHEAT_MODE, checked);
      setCheatMode(checked);
    });

    connect(ui->pushButton_sound, &QPushButton::clicked, this, [this](bool checked) {
      sounds->setEnabled(checked);
      config->set_config_file(CONFIG_SOUNDS, checked);
    });

    connect(ui->pushButton_new, &QPushButton::clicked, this, &MainWindow::onNewGame);

    connect(ui->checkBox_new_game, &QCheckBox::clicked, this, [this](bool checked) {
      config->set_config_file(CONFIG_COMFIRM_NEW_GAME, checked);
    });

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    connect(background, &Background::backgroundChanged, this, [this](const QString &path) {
        if (loadBackgroundPreview(path)) {
          updateCredits(background->Credits(), background->CreditTextColor());
          config->set_background_path(path);
        }
    });

    connect(statistics, &Statistics::sig_message, this, &MainWindow::message);

    connect(engine, &Engine::sig_your_turn, this, &MainWindow::onYourTurn);
    connect(engine, &Engine::sig_play_sound, this, &MainWindow::onPlaySound);
    connect(engine, &Engine::sig_update_stat, this, &MainWindow::onUpdateStat);
    connect(engine, &Engine::sig_update_stat_score, this, &MainWindow::onUpdateStatScore);
    connect(engine, &Engine::sig_message, this, &MainWindow::message);
    connect(engine, &Engine::sig_setTrickPile, this, &MainWindow::onSetTrickPile);
    connect(engine, &Engine::sig_enableAllCards, this, &MainWindow::enableAllCards);
    connect(engine, &Engine::sig_refresh_deck, this, &MainWindow::refresh_deck);
    connect(engine, &Engine::sig_clear_deck, this, &MainWindow::onClearDeck);
    connect(engine, &Engine::sig_new_players, this, &MainWindow::onNewPlayers);
    connect(engine, &Engine::sig_update_scores_board, this, &MainWindow::onUpdateScoresBoard);
    connect(engine, &Engine::sig_collect_tricks, this, &MainWindow::onCollectTricks);
    connect(engine, &Engine::sig_play_card, this, &MainWindow::onPlayCard);
    connect(engine, &Engine::sig_deal_cards, this, &MainWindow::onDealCards);
    connect(engine, &Engine::sig_passed, this, &MainWindow::onPassed);
    connect(engine, &Engine::sig_pass_to, this, &MainWindow::onPassTo);
    connect(engine, &Engine::sig_refresh_cards_played, this, &MainWindow::onRefreshCardsPlayed);
    connect(engine, &Engine::sig_card_played, this, &MainWindow::onCardPlayed);

    connect(qApp, &QCoreApplication::aboutToQuit, this, &MainWindow::aboutToQuit);

    connect(ui->treeHelpTOC, &QTreeWidget::currentItemChanged,
        this, [this](QTreeWidgetItem *current, QTreeWidgetItem *previous) {
            if (!current) return;

            QString anchor = current->data(0, Qt::UserRole).toString();
            if (!anchor.isEmpty()) {
                ui->textHelpContent->setSource(QUrl("#" + anchor));
            }
   });

   // Global quick Quit with shortcut Ctrl+Q
    QShortcut *quitShortcut = new QShortcut(QKeySequence("Ctrl+Q"), this);
    quitShortcut->setContext(Qt::ApplicationShortcut);
    connect(quitShortcut, &QShortcut::activated, this, &MainWindow::tryQuit);

    connect(ui->pushButton_exit, &QPushButton::clicked, this, [this]() {
       if (!ui->checkBox_confirm_exit->isChecked()) {
         qApp->quit();
         return;
       }
       QMessageBox::StandardButton reply = QMessageBox::question(this,
       tr("Exit the game?"),
       tr("Do you really want to leave the game?"),
       QMessageBox::Yes | QMessageBox::No,
       QMessageBox::No);
       if (reply == QMessageBox::Yes) {
         qApp->quit();
       }
    });
}

MainWindow::~MainWindow()
{
    disconnect(this, nullptr, nullptr, nullptr);
    delete config; config = nullptr; // no parent

    qDebug() << "~MainWindow() terminé";
}

// ************************************************************************************************
// Section 1- [ EVENTS OVERRIDE ]
//            resizeEvent(QResizeEvent *event)
//            eventFilter(QObject *obj, QEvent *event)
//            showEvent(QShowEvent *event)
//            closeEvent(QCloseEvent *event) override
//            tryQuit()
//            aboutToQuit()
//
// Section 2- [ Initialisation / Set / Loads ]
//            initCardsPlayedPointers()
//            loadHelpFile()
//            loadBackgroundPreview(const QString &image_path)
//            loadCardsPlayed()
//            applyAllSettings()
//            setAnimationLock()
//            setAnimationUnlock()
//            setCheatMode(bool enabled)
//            setAnimationButtons(bool enabled)
//            setLanguage()
//
// Section 3- [ Private Slots ]
//            onDeckChanged(int deckId)
//            onBackgroundPreviewClicked()
//            onCardClicked(QGraphicsItem *item)
//            onArrowClicked
//            onYourTurn();
//            onPlaySound(SOUNDS soundId);
//            onUpdateStat(int player, STATS stat);
//            onEngineMessage(const QString &msg);
//            onSetTrickPile(const QList<int> &pile);
//            onClearDeck();
//            onCollectTricks(PLAYER winner, bool tram);
//            onPlayCard(int cardId, PLAYER player);
//            onDealCards();
//            onPassed();
//            onPassTo(int direction);
//            onNewPlayers(const QString names[4]);
//            onUpdateScoresBoard(const QString names[4], const int hand[4], const int total[4]);
//            onUpdateStatScore(int player, int score);
//            onNewGame();
//            onTabChanged(int index);
//            onDeckStyle(int id);
//            onVariantToggled(int id, bool checked);
//            onAnimationToggled(int id, bool checked);
//            onRefreshCardsPlayed()
//            onCardPlayed()
//            on_opt_animations_clicked()
//            on_pushButton_undo_clicked()
//            on_opt_anim_arrow_clicked(bool checked)
//            on_pushButton_score_clicked()
//
// Section 4- [ Create Functions ]
//            createCreditsLabel()
//            create_arrows()
//            createButtonsGroups()
//            createPlayerNames()
//            createScoreDisplay()
//            createTOC()
//
// Section 5- [ Update Functions ]
//            updateTurnArrow()
//            updateYourTurnIndicator()
//            updateTrickPile()
//            updateSceneRect()
//            updateBackground()
//            updateCreditsPosition()
//            updateCredits()
//            refresh_deck()
//            updatePlayerNames()
//            updateScores(const int handScores[4], const int totalScores[4], const QString playersName[4], const PlayerIcon icons[4])
//            repositionAllCardsAndElements()
//
// Section 6- [ Animate Functions ]
//            animatePlayCard(int cardId, int player)
//            animatePassCards(const QList<int> passedCards[4], int direction)
//            animateCardsToCenter(const QList<int> &cardsId, QParallelAnimationGroup *masterGroup, int startDelay, int perCardStagger)
//            animateCardsToPlayer(const QList<int> &cardsId, int targetPlayer, int duration, QParallelAnimationGroup *masterGroup, int startDelay, int perCardStagger, bool takeAllRemaining)
//            animateCardsOffBoard(const QList<int> &cardsId, int winner)
//            animateCollectTrick(int winner, bool takeAllRemaining = false)
//            animateDeal(int dealDuration, int delayBetweenCards)
//            animateYourTurnIndicator()
//            highlightAIPassedCards(int player, const QList<int>& passedCardIds)
//
// Section 7- [ Mathematic Functions ]
//            cardX(int cardId, int player, int handIndex)
//            cardY(int cardId, int player, int handIndex)
//            getTakenPileCenter(int player, const QSize& viewSize) const
//            anim_rotation_value(int player)
//            calc_scale()
//            cardZ(int player, int cardId)
//            calculatePlayerNamePos(int player)
//
// Section 8- [ UNSORTED FUNCTIONS ]
//
// ************************************************************************************************


// ************************************[ 1- EVENTS OVERRIDE ] *************************************
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);

    if (ui->graphicsView->scene()) {
        ui->graphicsView->scene()->invalidate(QRectF(), QGraphicsScene::AllLayers); // ← invalide TOUT
        ui->graphicsView->scene()->update();           // scène globale
        ui->graphicsView->viewport()->update();        // ← le plus important : force le widget viewport
        ui->graphicsView->viewport()->repaint();       // ← repaint immédiat, bypass le paint event queue
    }

    // Only update if Board tab is visible (avoids unnecessary work)
    if (ui->tabWidget->currentIndex() == TAB_BOARD) {
      updateSceneRect();
      repositionAllCardsAndElements();
      ui->graphicsView->scene()->update();
      lastBoardViewSize = ui->graphicsView->size();
    }  else {
         windowWasResizedWhileAway = true;
       }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui->labelBackgroundPreview) {
        if (event->type() == QEvent::Enter) {
            ui->labelPreviewOverlay->setVisible(true);
            return false;
        }
        if (event->type() == QEvent::Leave) {
            ui->labelPreviewOverlay->setVisible(false);
            return false;
        }
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                onBackgroundPreviewClicked();
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);

    // Only do this the first time the window is shown
    static bool firstShow = true;
    if (firstShow) {
        firstShow = false;

        // this will prevent start or load game...
        // No valid deck loaded... DON'T refresh or it will crash.
       if (forced_new_deck) {
         start_engine_dalayed = true;
         return;
       }

        // Let the layout fully settle, then reposition everything and start the engine.
        QTimer::singleShot(0, this, [this]() {
            updateSceneRect();
            repositionAllCardsAndElements();
            engine->Start();
        });
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    qDebug() << "closeEvent appelé, busy =" << engine->isBusy();

    QCoreApplication::processEvents(QEventLoop::AllEvents, 800);

    tryQuit();

    event->ignore();
}

void MainWindow::tryQuit()
{
    if (!engine->isBusy()) {
        qDebug() << "tryQuit : idle → fermeture immédiate";
        qApp->quit();
        return;
    }

    sounds->setEnabled(false);
    sounds->stopAllSounds();

    ui->opt_animations->setChecked(false);
    hide();

    qDebug() << "tryQuit : busy → attente sig_busy(false)";

    static QMetaObject::Connection busyConnection;

    // On déconnecte l'ancienne si elle existe (pour éviter accumulation)
    if (busyConnection) {
        disconnect(busyConnection);
    }

    busyConnection = connect(engine, &Engine::sig_busy, this,
        [this](bool busy) {
            qDebug() << "sig_busy pendant fermeture :" << busy;
            if (!busy) {
                qDebug() << "→ idle détecté → on quitte";
                qApp->quit();

                // Optionnel : on peut déconnecter ici, mais pas obligatoire
                // (la static reste mais la connexion est inactive)
                if (busyConnection) {
                    disconnect(busyConnection);
                    busyConnection = {};  // reset pour la prochaine fois
                }
            }
        });

    QTimer::singleShot(10000, this, [this]() {
        qWarning() << "Timeout → fermeture forcée";
        qApp->quit();

        // Nettoyage aussi dans le timeout
        if (busyConnection) {
            disconnect(busyConnection);
            busyConnection = {};
        }
    });
}

void MainWindow::aboutToQuit()
{
  statistics->save_stats_file();
  engine->save_game();

  config->set_width(size().width());
  config->set_height(size().height());
  config->set_posX(pos().x());
  config->set_posY(pos().y());

  // Give Pulse audio time to clean stuff to avoid crashes on exit()
  sounds->stopAllSounds();
  QTimer::singleShot(200, qApp, &QApplication::quit);
}

// ************************************************************************************************

// *****************************[ 2- Initialisation / Set / Loads ] *******************************
void MainWindow::initCardsPlayedPointers()
{
  cardLabels[CLUBS_TWO] =   ui->label_clubs_2;
  cardLabels[CLUBS_THREE] = ui->label_clubs_3;
  cardLabels[CLUBS_FOUR] =  ui->label_clubs_4;
  cardLabels[CLUBS_FIVE] =  ui->label_clubs_5;
  cardLabels[CLUBS_SIX] =   ui->label_clubs_6;
  cardLabels[CLUBS_SEVEN] = ui->label_clubs_7;
  cardLabels[CLUBS_EIGHT] = ui->label_clubs_8;
  cardLabels[CLUBS_NINE] =  ui->label_clubs_9;
  cardLabels[CLUBS_TEN] =   ui->label_clubs_10;
  cardLabels[CLUBS_JACK] =  ui->label_clubs_jack;
  cardLabels[CLUBS_QUEEN] = ui->label_clubs_queen;
  cardLabels[CLUBS_KING] =  ui->label_clubs_king;
  cardLabels[CLUBS_ACE] =   ui->label_clubs_ace;

  cardLabels[SPADES_TWO] =   ui->label_spades_2;
  cardLabels[SPADES_THREE] = ui->label_spades_3;
  cardLabels[SPADES_FOUR] =  ui->label_spades_4;
  cardLabels[SPADES_FIVE] =  ui->label_spades_5;
  cardLabels[SPADES_SIX] =   ui->label_spades_6;
  cardLabels[SPADES_SEVEN] = ui->label_spades_7;
  cardLabels[SPADES_EIGHT] = ui->label_spades_8;
  cardLabels[SPADES_NINE] =  ui->label_spades_9;
  cardLabels[SPADES_TEN] =   ui->label_spades_10;
  cardLabels[SPADES_JACK] =  ui->label_spades_jack;
  cardLabels[SPADES_QUEEN] = ui->label_spades_queen;
  cardLabels[SPADES_KING] =  ui->label_spades_king;
  cardLabels[SPADES_ACE] =   ui->label_spades_ace;

  cardLabels[DIAMONDS_TWO] =   ui->label_diamonds_2;
  cardLabels[DIAMONDS_THREE] = ui->label_diamonds_3;
  cardLabels[DIAMONDS_FOUR] =  ui->label_diamonds_4;
  cardLabels[DIAMONDS_FIVE] =  ui->label_diamonds_5;
  cardLabels[DIAMONDS_SIX] =   ui->label_diamonds_6;
  cardLabels[DIAMONDS_SEVEN] = ui->label_diamonds_7;
  cardLabels[DIAMONDS_EIGHT] = ui->label_diamonds_8;
  cardLabels[DIAMONDS_NINE] =  ui->label_diamonds_9;
  cardLabels[DIAMONDS_TEN] =   ui->label_diamonds_10;
  cardLabels[DIAMONDS_JACK] =  ui->label_diamonds_jack;
  cardLabels[DIAMONDS_QUEEN] = ui->label_diamonds_queen;
  cardLabels[DIAMONDS_KING] =  ui->label_diamonds_king;
  cardLabels[DIAMONDS_ACE] =   ui->label_diamonds_ace;

  cardLabels[HEARTS_TWO] =   ui->label_hearts_2;
  cardLabels[HEARTS_THREE] = ui->label_hearts_3;
  cardLabels[HEARTS_FOUR] =  ui->label_hearts_4;
  cardLabels[HEARTS_FIVE] =  ui->label_hearts_5;
  cardLabels[HEARTS_SIX] =   ui->label_hearts_6;
  cardLabels[HEARTS_SEVEN] = ui->label_hearts_7;
  cardLabels[HEARTS_EIGHT] = ui->label_hearts_8;
  cardLabels[HEARTS_NINE] =  ui->label_hearts_9;
  cardLabels[HEARTS_TEN] =   ui->label_hearts_10;
  cardLabels[HEARTS_JACK] =  ui->label_hearts_jack;
  cardLabels[HEARTS_QUEEN] = ui->label_hearts_queen;
  cardLabels[HEARTS_KING] =  ui->label_hearts_king;
  cardLabels[HEARTS_ACE] =   ui->label_hearts_ace;
}

void MainWindow::loadHelpFile()
{
  QFile file; // (":/translations/help.html");
  if (ui->opt_english->isChecked()) {
    file.setFileName(":/translations/help.html");
  } else
    if (ui->opt_french->isChecked()) {
      qDebug() << "Set french help.";
      file.setFileName(":/translations/help_fr.html");
    } else
      if (ui->opt_russian->isChecked()) {
        file.setFileName(":/translations/help_ru.html");
      }

  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QTextStream in(&file);
    QString html = in.readAll();
    ui->textHelpContent->setHtml(html);
    file.close();
  } else {
      qWarning() << "Impossible de charger help.html";
      ui->textHelpContent->setPlainText(tr("Error : help file not found."));
    }
  ui->textHelpContent->setOpenExternalLinks(true);
  ui->textHelpContent->setOpenLinks(true);
}

bool MainWindow::loadBackgroundPreview(const QString &image_path)
{
  QPixmap preview(image_path);

  if (preview.isNull())
    return false;

  ui->labelBackgroundPreview->setPixmap(preview.scaled(ui->labelBackgroundPreview->size(),
                                        Qt::KeepAspectRatioByExpanding,
                                        Qt::SmoothTransformation ));

  return true;
}

void MainWindow::loadCardsPlayed()
{
  static const QStringList ranks = { "2", "3", "4", "5", "6", "7", "8", "9", "10", "Jack", "Queen", "King", "Ace" };
  static const QStringList suits = { " ♣", " ♠", " ♦", " ♥" };

  QPixmap pix;
  QString sRank;

  for (int cardId = 0; cardId < DECK_SIZE; cardId++) {
    int suit = cardId / 13;
    int rank = cardId % 13;
    switch (rank) {
       case 9  : sRank = tr("Jack"); break;
       case 10 : sRank = tr("Queen"); break;
       case 11 : sRank = tr("King"); break;
       case 12 : sRank = tr("Ace"); break;
       default : sRank = ranks[rank];
    }

    pix = deck->get_card_pixmap(cardId);
    cardLabels[cardId]->setPixmap(pix);
    cardLabels[cardId]->setToolTip(sRank + QString(suits[suit]));
    cardLabels[cardId]->setDisabled(engine->isCardPlayed(cardId));
  }
}

void MainWindow::applyAllSettings()
{
  int deck_style = config->get_deck_style();
  bool deck_loaded = deck->set_deck(deck_style);
  bool enabled;

  if (!deck_loaded)
    disableDeck(deck_style);
  else {
    deckGroup->button(deck_style)->setChecked(true);
    import_allCards_to_scene();
  }

  enabled = config->is_sounds();
  ui->pushButton_sound->setChecked(enabled);
  sounds->setEnabled(enabled);

  enabled = config->is_queen_spade_break_heart();
  ui->opt_queen_spade->setChecked(enabled);
  engine->set_variant(VARIANT_QUEEN_SPADE, enabled);

  enabled = config->is_perfect_100();
  ui->opt_perfect_100->setChecked(enabled);
  engine->set_variant(VARIANT_PERFECT_100, enabled);

  enabled = config->is_omnibus();
  ui->opt_omnibus->setChecked(enabled);
  engine->set_variant(VARIANT_OMNIBUS, enabled);

  enabled = config->is_no_trick_bonus();
  ui->opt_no_tricks->setChecked(enabled);
  engine->set_variant(VARIANT_NO_TRICKS, enabled);

  enabled = config->is_new_moon();
  ui->opt_new_moon->setChecked(enabled);
  engine->set_variant(VARIANT_NEW_MOON, enabled);

  enabled = config->is_no_draw();
  ui->opt_no_draw->setChecked(enabled);
  engine->set_variant(VARIANT_NO_DRAW, enabled);

  enabled = config->is_animated_play();
  ui->opt_animations->setChecked(enabled);
  setAnimationButtons(enabled);

  enabled = config->is_confirm_exit();
  ui->checkBox_confirm_exit->setChecked(enabled);

  enabled = config->is_detect_tram();
  ui->checkBox_tram->setChecked(enabled);
  engine->set_tram(enabled);

  enabled = config->is_cheat_reveal();
  ui->pushButton_reveal->setChecked(enabled);

  enabled = config->is_confirm_new_game();
  ui->checkBox_new_game->setChecked(enabled);

  // This settings must be applied after is_cheat_reveal(), so setCheatMode will unset
  // reveal pushButton if the overall cheat mode has been set to false.
  enabled = config->is_cheat_mode();
  ui->checkBox_cheat->setChecked(enabled);
  setCheatMode(enabled);

  ui->opt_anim_deal_cards->setChecked(config->is_anim_deal_cards());
  ui->opt_anim_play_card->setChecked(config->is_anim_play_card());
  ui->opt_anim_collect_tricks->setChecked(config->is_anim_collect_tricks());
  ui->opt_anim_pass_cards->setChecked(config->is_anim_pass_cards());

  enabled = config->is_animated_arrow();
  ui->opt_anim_arrow->setChecked(enabled);
  on_opt_anim_arrow_clicked(enabled);

  ui->opt_anim_triangle->setChecked(config->is_anim_turn_indicator());

  switch (config->get_language()) {
    case LANG_ENGLISH: ui->opt_english->setChecked(true); break;
    case LANG_FRENCH: ui->opt_french->setChecked(true); break;
    case LANG_RUSSIAN: ui->opt_russian->setChecked(true); break;
  }

  QSize savedSize(config->width(), config->height());
  QPoint savedPos(config->posX(), config->posY());

  // If there is no background path. We load the legacy background save by index.
  if (config->Path().isEmpty()) {
    qDebug() << "applyAllSettings loading a legacy background!";
    background->setBackground(config->get_background_index());
  } else {
    background->setBackgroundPixmap(config->Path());
  }

  resize(savedSize);

// With all the Linux window managers THAT doesn't works.
// move(savedPos);

  loadBackgroundPreview(background->FullPath());
}

void MainWindow::setAnimationLock()
{
    m_animationLockCount++;

    if (m_animationLockCount == 1) {  // First lock (was 0 → 1)
        // Remember current size before fixing
        m_fixedSizeDuringLock = size();

        // Lock window size
        setFixedSize(m_fixedSizeDuringLock);

        // Disable deck switch tab
        ui->tabWidget->setTabEnabled(TAB_SETTINGS, false);

        // Disable cards reveal
        ui->pushButton_reveal->setDisabled(true);

        // Disable new game
        ui->pushButton_new->setDisabled(true);

        // Disable undo
        ui->pushButton_undo->setDisabled(true);

        // Visual feedback
        //  ui->graphicsView->setCursor(Qt::WaitCursor);
        QApplication::setOverrideCursor(Qt::WaitCursor);
    }
}

void MainWindow::setAnimationUnlock()
{
    if (m_animationLockCount <= 0) {
        m_animationLockCount = 0;
        return;
    }

    m_animationLockCount--;

    if (m_animationLockCount == 0) {
        // Fully unlocked
        ui->tabWidget->setTabEnabled(TAB_SETTINGS, true);

        // TODO don't enable reveal and undo, if we're playing ONLINE
        ui->pushButton_reveal->setDisabled(false);
        ui->pushButton_undo->setDisabled(false);
        ui->pushButton_new->setDisabled(false);

        // ui->graphicsView->unsetCursor();
        QApplication::restoreOverrideCursor();

        // Restore full resizability
        setMinimumSize(QSize(MIN_APPL_WIDTH, MIN_APPL_HEIGHT));
        setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
    }
}

void MainWindow::setCheatMode(bool enabled)
{
  if (!enabled) {
    ui->pushButton_reveal->setChecked(false);
    config->set_config_file(CONFIG_CHEAT_REVEAL, false);
  }
  ui->tabWidget->setTabVisible(TAB_CARDS_PLAYED, enabled);
  ui->pushButton_reveal->setVisible(enabled);
}

void MainWindow::setAnimationButtons(bool enabled)
{
  ui->opt_anim_deal_cards->setEnabled(enabled);
  ui->opt_anim_play_card->setEnabled(enabled);
  ui->opt_anim_collect_tricks->setEnabled(enabled);
  ui->opt_anim_pass_cards->setEnabled(enabled);
  ui->opt_anim_arrow->setEnabled(enabled);
  ui->opt_anim_triangle->setEnabled(enabled);
}

void MainWindow::setLanguage()
{
  // Retirer l’ancien translator s’il existe
  if (currentTranslator) {
   qApp->removeTranslator(currentTranslator);
   delete currentTranslator;
   currentTranslator = nullptr;
  }

  currentTranslator = new QTranslator(this);

  QString translation(":/translations/Hearts_en_CA.qm");

  if (ui->opt_french->isChecked()) {
    translation = ":/translations/Hearts_fr_CA.qm";
  } else
      if (ui->opt_russian->isChecked()) {
        translation = ":/translations/Hearts_ru_RU.qm";
      }

  // Chargement depuis les ressources embarquées
  if (currentTranslator->load(translation)) {
    qApp->installTranslator(currentTranslator);
  } else {
    qDebug() << "Échec chargement traduction :" << translation;
  }

  ui->retranslateUi(this);
  statistics->Translate();

  ui->treeHelpTOC->clear();
  qDebug() << "Create new TOC";
  createTOC();
  loadHelpFile();
  loadCardsPlayed();
  background->setCredits();
  updateCredits(background->Credits(), background->CreditTextColor());
  updateCreditsPosition();
}
// ************************************************************************************************


// ************************************[ 3- Private Slots ] ***************************************
void MainWindow::onDeckChanged(int deckId)
{
  qDebug() << "onDeckChanged: " << deckId;
  qDebug() << "onDeckChanged: Scene size: " << scene->items().size();
  QGraphicsItem *item;

  for (int i = 0; i < selectedCards.size(); i++) {
    item = deck->get_card_item(selectedCards.at(i), true);
    item->setData(1, true);
  }
  loadCardsPlayed();
}

void MainWindow::onBackgroundPreviewClicked()
{
    QString path = getResourceFile(QString("backgrounds"));

    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Choose Background Image"),
        path,
        tr("Images (*.png *.jpg *.jpeg *.bmp *.gif *.svg)"));

    if (fileName.isEmpty()) return;

    QPixmap pix(fileName);
    if (pix.isNull()) {
        qDebug() << "Failed to load background:" << fileName;
        return;
    }

    // Update the actual scene background
    background->setBackgroundPixmap(fileName);
    updateCreditsPosition();

    // Update preview label
    loadBackgroundPreview(fileName);
}

void MainWindow::onCardClicked(QGraphicsItem *item)
{
    if (!item) return;

    QVariant idVar = item->data(0);
    if (!idVar.isValid() || !idVar.canConvert<int>()) return;

    int cardId = idVar.toInt();
    if (cardId < 0 || cardId >= DECK_SIZE) return;

    int player = engine->Owner(cardId);
    if (player != PLAYER_SOUTH)
      return;

    idVar = item->data(1);
    if (!idVar.isValid() || !idVar.canConvert<bool>()) return;

    bool selected = idVar.toBool();

    if (engine->Status() == SELECT_CARDS) {
      if (selected) {
        selectedCards.removeAll(cardId);
        item->setData(1, false);
        item->setPos(item->pos().x(), item->pos().y() + SELECTED_CARD_OFFSET);
      } else {
          if (selectedCards.size() >= 3)
           return;

          // Must saved those selected cards. Switching to a new deck will destroy all the previous items and information.
          selectedCards.append(cardId);

          item->setData(1, true);
          item->setPos(item->pos().x(), item->pos().y() - SELECTED_CARD_OFFSET);
          sounds->play(SOUND_CONTACT);
      }
    } else {
        if (!isProcessingTurn && engine->isPlaying() && (engine->Turn() == PLAYER_SOUTH)) {
          isProcessingTurn = true;
          engine->Play(cardId);
          sounds->play(SOUND_DEALING_CARD);
          yourTurnIndicator->hide();
          removeInvalidCardsEffect();
          playCard(cardId, PLAYER_SOUTH);
        }
      }
    if (player == PLAYER_NOBODY) {
      qDebug() << "Warning: Clicked card" << cardId << "not found in any player's hand!";
      return;
    }
}

void MainWindow::onArrowClicked()
{
  if (selectedCards.size() != 3) {
    message(tr("You must select 3 cards to pass!"), MESSAGE_ERROR);
  } else {
    arrowLabel->hide();

    engine->set_passedFromSouth(selectedCards);
    selectedCards.clear();
    engine->Step();
  }
}

void MainWindow::onYourTurn() {
  isProcessingTurn = false;
  disableInvalidCards();
  showTurnIndicator();
}

void MainWindow::onPlaySound(SOUNDS soundId) {
  sounds->play(soundId);
}

void MainWindow::onUpdateStat(int player, STATS stat) {
  statistics->increase_stats(player, stat);
}

void MainWindow::onSetTrickPile(const QList<int> &pile) {
  int cpt = 0;
  currentTrick = pile;
  for (int i = pile.size() - 1; i >= 0; --i) {
    QGraphicsSvgItem *item = deck->get_card_item(currentTrick.at(i), true);
    switch (cpt) {
      case 0: item->setRotation(-90); break;
      case 1: item->setRotation(180); break;
      case 2: item->setRotation(90); break;
              break;
    }
    cpt++;
    item->setZValue(Z_TRICKS_BASE - cpt);
  }
  updateTrickPile();
}

void MainWindow::onClearDeck() {
  scene->clear();
  clearTricks();
  yourTurnIndicator->hide();
  arrowLabel->hide();
}

void MainWindow::onCollectTricks(PLAYER winner, bool tram) {
  if (ui->opt_animations->isChecked() && ui->opt_anim_collect_tricks->isChecked()) {
    animateCollectTrick(winner, tram);
  } else {
      QTimer::singleShot(1000, this, [this]() {
        clearTricks();
        engine->Step();
      });
    }
}

void MainWindow::onPlayCard(int cardId, PLAYER player) {
  sounds->play(SOUND_DEALING_CARD);
  playCard(cardId, player);
}

void MainWindow::onDealCards() {
  if (ui->opt_animations->isChecked() && ui->opt_anim_deal_cards->isChecked()) {
    animateDeal();
  } else {
      engine->Step();
    }
}

void MainWindow::onPassed() {
  if (ui->opt_animations->isChecked() && (ui->opt_anim_pass_cards->isChecked())) {
    QList<int> passedCards[4];
    passedCards[PLAYER_SOUTH] = engine->getPassedCards(PLAYER_SOUTH);
    passedCards[PLAYER_WEST] = engine->getPassedCards(PLAYER_WEST);
    passedCards[PLAYER_NORTH] = engine->getPassedCards(PLAYER_NORTH);
    passedCards[PLAYER_EAST] = engine->getPassedCards(PLAYER_EAST);

    setAnimationLock();
    //    ui->graphicsView->setCursor(Qt::ArrowCursor);

    //    2. Avance les cartes de l'IA (West, North, East) vers le centre
    highlightAIPassedCards(PLAYER_WEST,  passedCards[PLAYER_WEST]);
    highlightAIPassedCards(PLAYER_NORTH, passedCards[PLAYER_NORTH]);
    highlightAIPassedCards(PLAYER_EAST,  passedCards[PLAYER_EAST]);

    // 3. Attends un court instant (ex. 1 seconde) pour que le joueur voie
    QTimer::singleShot(1000, this, [this, passedCards]() {
    // Une fois le délai passé, lance l'animation réelle
      sounds->play(SOUND_PASSING_CARDS);
      animatePassCards(passedCards, engine->Direction());
    });
  } else {
      deck->reset_selections();
      engine->Step();
    }
}

void MainWindow::onPassTo(int direction) {
  showTurnArrow(direction);
}

void MainWindow::onNewPlayers() {
  updatePlayerNames();
}

void MainWindow::onUpdateScoresBoard(const QString names[4], const int hand[4], const int total[4]) {
  updateScores(names, hand, total, icons);
}

void MainWindow::onUpdateStatScore(int player, int score) {
  statistics->set_score(player, score);
}

void MainWindow::onNewGame() {
  if (ui->checkBox_new_game->isChecked()) {
    QMessageBox::StandardButton reply = QMessageBox::question(this,
    tr("Start a new game?"),
    tr("Are you ready to start a new game?"),
    QMessageBox::Yes | QMessageBox::No,
    QMessageBox::No);
    if (reply == QMessageBox::No) {
       return;
    }
  }

  // unselect any selected cards.
  selectedCards.clear();
  deck->reset_selections();

  // Anti-spam
  if (newGameDebounceTimer && newGameDebounceTimer->isActive()) {
    qDebug() << "New game ignoré (debounce)";
    return;
  }

  if (!newGameDebounceTimer) {
    newGameDebounceTimer = new QTimer(this);
    newGameDebounceTimer->setSingleShot(true);
  }
  newGameDebounceTimer->start(1000);

  qDebug() << "new game";
  GAME_ERROR err = engine->start_new_game();
  if (err) {
    qDebug() << "pushButton_new";
    message(engine->errorMessage(err), MESSAGE_ERROR);
  }
  // Note: engine will emit sig_clear_deck() --> scene->clear() + clearTricks() + yourTurnIndicator->hide() = arrowLabel->hide()
}

void MainWindow::onTabChanged(int index) {
  if (index == 0) {  // Switched TO Board tab
    QTimer::singleShot(0, this, [this]() {
      QSize currentSize = ui->graphicsView->size();

      bool sizeChanged = (currentSize != lastBoardViewSize);
      bool needsFullReposition = sizeChanged && windowWasResizedWhileAway;

      updateSceneRect();
      if (needsFullReposition) {
        repositionAllCardsAndElements();
        lastBoardViewSize = currentSize;
      } else {
          refresh_deck();
          updateTrickPile();
        }

      windowWasResizedWhileAway = false;
    });
  }
}

void MainWindow::onDeckStyleClicked(int id) {
  if (deck->Style() == id)
   return;

  saveCurrentTrickState();
  disableAllDecks();
  if (deck->set_deck(id)) {
    config->set_deck_style(id);
    import_allCards_to_scene();

    if (forced_new_deck) {
      ui->tabWidget->setTabEnabled(TAB_BOARD, true);
      ui->tabWidget->setCurrentIndex(TAB_BOARD);
      forced_new_deck = false;
      if (start_engine_dalayed) {
        start_engine_dalayed = false;
        engine->Start();
      }
    }
  } else {
      disableDeck(id);
      valid_deck[id] = false;
    }

  enableAllDecks();
  restoreTrickCards();
  if (engine->isMyTurn()) {
    disableInvalidCards();
  }

  scene->update();
}

void MainWindow::onVariantToggled(int id, bool checked) {
  switch (id) {
    case 0: config->set_config_file(CONFIG_QUEEN_SPADE, checked);
            engine->set_variant(VARIANT_QUEEN_SPADE, checked);
            break;
    case 1: config->set_config_file(CONFIG_PERFECT_100, checked);
            engine->set_variant(VARIANT_PERFECT_100, checked);
            break;
    case 2: config->set_config_file(CONFIG_OMNIBUS, checked);
            engine->set_variant(VARIANT_OMNIBUS, checked);
            break;
    case 3: config->set_config_file(CONFIG_NO_TRICK, checked);
            engine->set_variant(VARIANT_NO_TRICKS, checked);
            break;
    case 4: config->set_config_file(CONFIG_NEW_MOON, checked);
            engine->set_variant(VARIANT_NEW_MOON, checked);
            break;
    case 5: config->set_config_file(CONFIG_NO_DRAW, checked);
            engine->set_variant(VARIANT_NO_DRAW, checked);
            break;
    }
}

void MainWindow::onAnimationToggled(int id, bool checked) {
  switch (id) {
    case 0: config->set_config_file(CONFIG_ANIM_DEAL_CARDS, checked); break;
    case 1: config->set_config_file(CONFIG_ANIM_PLAY_CARD, checked); break;
    case 2: config->set_config_file(CONFIG_ANIM_COLLECT_TRICKS, checked); break;
    case 3: config->set_config_file(CONFIG_ANIM_PASS_CARDS, checked); break;
    case 4: config->set_config_file(CONFIG_ANIMATED_ARROW, checked); break;
    case 5: config->set_config_file(CONFIG_ANIM_TURN_INDICATOR, checked); break;
  }
}

void MainWindow::onRefreshCardsPlayed()
{
  for (int cardId = 0; cardId < DECK_SIZE; cardId++) {
    cardLabels[cardId]->setDisabled(engine->isCardPlayed(cardId));
  }
}

void MainWindow::onCardPlayed(int cardId)
{
  cardLabels[cardId]->setDisabled(true);
}

void MainWindow::on_opt_animations_clicked()
{
  bool enabled = ui->opt_animations->isChecked();

  config->set_config_file(CONFIG_ANIMATED_PLAY, enabled);
  setAnimationButtons(enabled);
  on_opt_anim_arrow_clicked(enabled);
}

void MainWindow::on_pushButton_undo_clicked()
{
  if (engine->Undo()) {
    sounds->play(SOUND_UNDO);
    statistics->increase_stats(PLAYER_SOUTH, STATS_UNDO);
  }
}

void MainWindow::on_opt_anim_arrow_clicked(bool checked)
{
  if (!arrowMovie || !arrowMovie->isValid()) {
   return;
  }

  int totalFrames = arrowMovie->frameCount();
  if (totalFrames <= 0) {
    return;
  }

  if (!checked || !ui->opt_animations->isChecked()) {
    arrowMovie->jumpToFrame(totalFrames - 1);
    arrowMovie->setPaused(true);
    updateTurnArrow();
  } else {
      arrowMovie->jumpToFrame(0);
      arrowMovie->start();
    }
}

void MainWindow::on_pushButton_score_clicked()
{
  bool checked = ui->pushButton_score->isChecked();
  m_scoreGroup->setVisible(checked);
}

// ************************************************************************************************



// ************************************[ 4- Update Functions **************************************
void MainWindow::updateTurnArrow()
{
    if (!arrowProxy) return;

    QSize viewSize = ui->graphicsView->size();
    qreal scale = viewSize.width() / 1200.0;
    scale = qBound(0.5, scale, 2.0);

    // Base size at normal scale
    qreal baseSize = 120.0;  // Adjust for desired size at normal window
    qreal arrowSize = baseSize * scale;

    arrowLabel->setFixedSize(arrowSize, arrowSize);

    // TRUE CENTER: use transform origin + center position
    arrowProxy->setTransformOriginPoint(arrowSize / 2, arrowSize / 2);  // Center of proxy
    arrowProxy->setPos(viewSize.width() / 2.0 - arrowSize / 2, viewSize.height() / 2.0 - arrowSize / 2);
}

void MainWindow::updateYourTurnIndicator()
{
    if (!yourTurnIndicator) return;

    QSize viewSize = ui->graphicsView->size();

    // Get a reference card (first from deck is perfect for testing)

    // TODO: eventually, use the height from player SOUTH card, instead of card id #0
    QGraphicsItem *refCard = deck->get_card_item(0, true);
    if (!refCard) return;

    // Current scaled height of a card as it appears in the view
    qreal scaledCardHeight = refCard->boundingRect().height() * calc_scale();

    // Estimate how much vertical space the bottom hand occupies
    // Typically: scaled height + some overlap/fan offset
    // Safe approximation: hand height ≈ scaledCardHeight + 30% extra for fan
    qreal estimatedHandHeight = scaledCardHeight * 1.05;

    // Spacing between top of hand and bottom of arrow (tweak 40–80)
    qreal spacing = 50.0;

    // Final Y: bottom of view minus hand height minus spacing
    qreal arrowY = viewSize.height() - estimatedHandHeight - spacing;

    // Center horizontally
    yourTurnIndicator->setPos(viewSize.width() / 2.0, arrowY);

    // For testing: always show, or tie to a test flag
    // yourTurnIndicator->setVisible(true);  // or your test condition
}

void MainWindow::updateTrickPile()
{
    if (currentTrick.isEmpty()) return;

    QSize viewSize = ui->graphicsView->size();

    int count = currentTrick.size();

    QGraphicsSvgItem *item;
    for (int i = 0; i < count; ++i) {
        int cardId = currentTrick[i];
        item = deck->get_card_item(cardId, true);
        item->show();

        QPointF center(viewSize.width() / 2.0 - item->boundingRect().width() / 2, viewSize.height() / 2);
        item->setPos(center);
        item->setZValue(ZLayer::Z_TRICKS_BASE + i);  // Ensure on top (100 reserved for Arrow)
        item->setScale(calc_scale());
        qDebug() << "scene: " << item->scene() << "Z: " << item->zValue() << "Visibility: " << item->isVisible() << "Pos: " << item->pos() << "Rotation: " << item->rotation();
    }
}

void MainWindow::updateSceneRect()
{
    if (!ui->graphicsView) return;

    // Make the scene exactly match the current viewport size
    QRectF viewportRect = ui->graphicsView->viewport()->rect();
    ui->graphicsView->setSceneRect(viewportRect);

    // Optional: re-center everything if needed (your cards use view size anyway)
    // But usually not needed if you position cards relative to viewSize.width()/height()
}

void MainWindow::updateBackground()
{
    if (backgroundItem && !originalBgPixmap.isNull()) {
        QPixmap scaled = originalBgPixmap.scaled(ui->graphicsView->size(),
                                                Qt::KeepAspectRatioByExpanding,
                                                Qt::SmoothTransformation);

        backgroundItem->setPixmap(scaled);
    }
}

void MainWindow::updateCreditsPosition()
{
   if (!creditsText) return;

   // Get current view rect in scene coordinates
   QRectF viewRect = ui->graphicsView->mapToScene(ui->graphicsView->viewport()->rect()).boundingRect();

   // Add margin
   qreal margin = 10.0;

   qreal x = viewRect.right() - creditsText->boundingRect().width() - margin;
   qreal y = viewRect.bottom() - creditsText->boundingRect().height() - margin;

   creditsText->setPos(x, y);
}

void MainWindow::updateCredits(const QString &text, const QColor &color)
{
  creditsText->setPlainText(text);
  creditsText->setDefaultTextColor(background->CreditTextColor());

  QFont font("Noto Sans", 10);
  font.setItalic(true);

  creditsText->setFont(font);
}

void MainWindow::refresh_deck()
{
    QGraphicsSvgItem *item, *hidden;

    bool revealed;
    int rot, z;

    for (int player = PLAYER_SOUTH; player < PLAYER_COUNT; player++) {
      revealed = ui->pushButton_reveal->isChecked() || (player == PLAYER_SOUTH);

//qDebug() << "Player: " << player << "Hand: " << engine->Hand(player) << "Size: " << engine->Hand(player).size();
      int handSize = engine->handSize((PLAYER)player);
      for (int handIndex = 0; handIndex < handSize; handIndex++) {
         int cardId = engine->get_player_card((PLAYER)player, handIndex);
         if (cardId == INVALID_CARD) continue;

         item = deck->get_card_item(cardId, revealed);
         hidden = deck->get_card_item(cardId, !revealed);

//qDebug() << "Empty: " << currentTrick.isEmpty() << "Index: " << currentTrick.indexOf(cardId) << "CardId : " << cardId;

         if (!currentTrick.isEmpty() && (currentTrick.indexOf(cardId) != -1)) {
  //        qDebug() << "refresh continue";
          continue; // updateTrickPile()
         }

         switch (player) {
           case PLAYER_WEST:
                  rot = anim_rotation_value(PLAYER_WEST);
                  z = ZLayer::Z_CARDS_BASE - handIndex;
                  item->setRotation(rot);
                  item->setZValue(z);
                  hidden->setRotation(rot);
                  hidden->setZValue(z);
                  set_card_pos(cardId, PLAYER_WEST, handIndex, revealed);
                  break;
           case PLAYER_NORTH:
                  rot = anim_rotation_value(PLAYER_NORTH);
                  z = ZLayer::Z_CARDS_BASE + handIndex;
                  item->setRotation(rot);
                  item->setZValue(z);
                  hidden->setRotation(rot);
                  hidden->setZValue(z);
                  set_card_pos(cardId, PLAYER_NORTH, handIndex, revealed);
                  break;
           case PLAYER_EAST:
                  rot = anim_rotation_value(PLAYER_EAST);
                  z = ZLayer::Z_CARDS_BASE + handIndex;
                  item->setRotation(rot);
                  item->setZValue(z);
                  hidden->setRotation(rot);
                  hidden->setZValue(z);
                  set_card_pos(cardId, PLAYER_EAST, handIndex, revealed);
                  break;
           case PLAYER_SOUTH:
                  rot = anim_rotation_value(PLAYER_SOUTH);
                  z = ZLayer::Z_CARDS_BASE - handIndex;
                  item->setRotation(rot);
                  item->setZValue(z);
                  hidden->setRotation(rot);
                  hidden->setZValue(z);
                  set_card_pos(cardId, PLAYER_SOUTH, handIndex, revealed);
         }
      }
    }
}

void MainWindow::updatePlayerNames()
{
   QString nameHtml;
    // Skipping PLAYER_SOUTH "You".
    for (int p = 1; p < 4; ++p) {
        // Position par joueur
        QPointF pos = calculatePlayerNamePos(p);

        // Rotation selon position
        switch (p) {
        case PLAYER_WEST:  // Gauche → vertical, lisible de gauche
            playerNamesItem[p]->setRotation(90);  // Tourne à 90° antihoraire
            break;
        case PLAYER_EAST:  // Droite → vertical, lisible de droite
            playerNamesItem[p]->setRotation(-90);   // Tourne à 90° horaire
            break;
        case PLAYER_NORTH: // Horizontal, origine centre
            break;

        default:
        case PLAYER_SOUTH: continue;
        }

        nameHtml = QString(
                           "<div style='width: 140px; text-align: center; background-color:rgba(60,80,120,90); "
                           "border-radius:6px; padding:6px 0; color:#f0f0ff; font-weight:bold; font-size:14px; "
                           "-webkit-text-stroke: 0.6px black; text-stroke: 0.6px black;'>"
                           "%1"
                           "</div>").arg(engine->PlayerName(p).toHtmlEscaped());

        playerNamesItem[p]->setHtml(nameHtml);
        playerNamesItem[p]->setPos(pos);
    }
}

void MainWindow::updateScores(const QString playersName[4],
                              const int handScores[4],
                              const int totalScores[4],
                              const PlayerIcon icons[4]
                              )
{
    if (!m_scoreText) return;

    qreal maxWidth = qMin(200.0, ui->graphicsView->width() * 0.32);

    // En-tête + tableau compact
    QString html = QString(
        "<table width='%1' cellspacing='3' cellpadding='2' style='color:white; font-size:11px; margin-bottom: -12px; marging: 0; padding: 0;'>"
        "<tr style='font-weight:bold; color:#ddd;'>"
        "<td>Name</td><td align='right'>Score</td><td align='right'>Total</td>"
        "</tr>"
    )
    .arg(maxWidth);

    for (int p = 0; p < 4; ++p) {
        QString iconHtml = "";
        if (icons[p] == PlayerIcon::Human) {
            iconHtml = "<img src=':/icons/persongeneric.svg' width='16' height='16' style='vertical-align: bottom; margin-right: 4px;'> ";
        } else if (icons[p] == PlayerIcon::Bot) {
            iconHtml = "<img src=':/icons/computer-icon.png' width='16' height='16' style='vertical-align: bottom; margin-right: 4px;'> ";
        } else if (icons[p] == PlayerIcon::Remote) {
            iconHtml = "<img src=':/icons/remote.png' width='16' height='16' style='vertical-align: bottom; margin-right: 4px;'> ";
        }

        QString handColor = (handScores[p] > 0) ? "color:#ffcc00;" : "color:#aaffaa;";

        html += QString(
            "<tr>"
            "<td>%1%2</td>"                           // icône + nom
            "<td align='right' style='%3'>%4</td>"    // hand score coloré
            "<td align='right'>%5</td>"               // total score
            "</tr>"
        )
        .arg(iconHtml)
        .arg(playersName[p].toHtmlEscaped())
        .arg(handColor)
        .arg(handScores[p])
        .arg(totalScores[p]);
    }

    html += "</table>";

    m_scoreText->setHtml(html);

    // Ajustement automatique du fond au contenu réel
    if (m_scoreBackground) {
        QRectF textRect = m_scoreText->boundingRect();
        m_scoreBackground->setRect(0, 0,
                                   textRect.width() + 24,
                                   textRect.height() + 24);
    }
}

void MainWindow::updateScorePosition()
{
    if (!m_scoreGroup) return;

    QSize viewSize = ui->graphicsView->size();
    if (viewSize.width() <= 0 || viewSize.height() <= 0) return;

    qreal x = 40.0;
    qreal y = viewSize.height() - m_scoreGroup->boundingRect().height() - 45;

    // Sécurité (même si resize très petit)
    x = safeBound(10.0, x, viewSize.width() - m_scoreGroup->boundingRect().width() - 10);
    y = safeBound(20.0, y, viewSize.height() - m_scoreGroup->boundingRect().height() - 10);

    m_scoreGroup->setPos(x, y);

    // Optionnel : petit log pour debug
    // qDebug() << "Boîte repositionnée en bas-gauche après resize :" << x << y;
}

/*
void MainWindow::updateScorePosition()
{
    if (isDragging) {
    qDebug() << "updateScorePosition ignorée car drag en cours";
     return;  // Ne rien faire pendant le drag
    }

    if (!m_scoreGroup) return;

    QSize viewSize = ui->graphicsView->size();
    if (viewSize.width() <= 0 || viewSize.height() <= 0) return;

    qreal x, y;

    if (scorePositionSaved) {
        x = savedScorePos.x();
        y = savedScorePos.y();
    } else {
        x = 20;
        y = viewSize.height() - m_scoreGroup->boundingRect().height() - 80;
    }

    x = safeBound(10.0, x, viewSize.width() - m_scoreGroup->boundingRect().width() - 10);
    y = safeBound(120.0, y, viewSize.height() - m_scoreGroup->boundingRect().height() - 10);

    m_scoreGroup->setPos(x, y);

    qDebug() << "UPDATE POS - saved?" << scorePositionSaved << "Pos finale :" << x << y;
}
*/


void MainWindow::repositionAllCardsAndElements()
{
    updateBackground();
    updateTurnArrow();
    refresh_deck();
    updateTrickPile();
    updateYourTurnIndicator();
    updatePlayerNames();
    updateCreditsPosition();
    updateScorePosition();

/*
    QSize newSize = ui->graphicsView->size();
    if (newSize != lastViewSize) {
        lastViewSize = newSize;
        updateScorePosition();
        qDebug() << "VRAI RESIZE détecté - appel updateScorePosition";
    }
*/

      // Toujours ajuster la taille du fond au texte (important après resize)
    if (m_scoreBackground && m_scoreText) {
        QRectF textRect = m_scoreText->boundingRect();
        m_scoreBackground->setRect(0, 0,
                                   textRect.width() + 30,
                                   textRect.height() + 30);
    }

/*
    qDebug() << "View size:" << ui->graphicsView->size();
qDebug() << "Scene rect:" << scene->sceneRect();
qDebug() << "Score pos finale:" << m_scoreGroup->pos();
qDebug() << "scoreScroup Z: " << m_scoreGroup->zValue();
*/
}
// ************************************************************************************************



// ************************************[ 5- Create Functions ] ************************************
void MainWindow::createCreditsLabel()
{
// Create credits label
  creditsText = new QGraphicsTextItem(background->Credits());
  creditsText->setDefaultTextColor(background->CreditTextColor());

  QFont font("Noto Sans", 10);
  font.setItalic(true);
  creditsText->setFont(font);

  scene->addItem(creditsText);
  scene->protectItem(creditsText);
  creditsText->setZValue(ZLayer::Z_CREDITS);

  updateCreditsPosition();
}

void MainWindow::create_arrows()
{
// Create the animated arrow label
    arrowLabel = new QLabel();

    arrowLabel->setAttribute(Qt::WA_NoSystemBackground, true);
    arrowLabel->setAttribute(Qt::WA_TranslucentBackground, true);
    arrowLabel->setAlignment(Qt::AlignCenter);
    arrowLabel->setFocusPolicy(Qt::NoFocus);

    arrowLabel->setStyleSheet(QString(
         "QLabel:focus, QLabel:active {"
         "outline: none !important;"
         "background: transparent !important; }"
    ));

    arrowLabel->setScaledContents(true);  // KEY: Movie scales to label size
//    arrowLabel->setStyleSheet("background: transparent;");  // Extra safety

    arrowMovie = new QMovie(":/icons/arrow-11610_128.gif", QByteArray(), arrowLabel);
    arrowMovie->setSpeed(150);
    arrowMovie->setCacheMode(QMovie::CacheAll);

    if (arrowMovie->isValid()) {
       arrowLabel->setMovie(arrowMovie);
       arrowLabel->setAlignment(Qt::AlignCenter);
    } else {
        arrowLabel->setText("→");  // Fallback
      }

// Add to scene as proxy widget
   arrowProxy = scene->addWidget(arrowLabel);
//   scene->setArrowLabel(arrowProxy);
   scene->protectItem(arrowProxy);

   arrowProxy->setZValue(ZLayer::Z_ARROW);  // Above cards
   arrowProxy->setVisible(false);  // Hidden until turn

   arrowProxy->setFlag(QGraphicsItem::ItemIsSelectable, true);  // Optional highlight
   arrowProxy->setFlag(QGraphicsItem::ItemIsMovable, false);    // Prevent drag
   arrowProxy->setAcceptedMouseButtons(Qt::LeftButton);

   // Create once (e.g., in your setup function)
   yourTurnIndicator = new TurnArrow(20);     // try 40, 45, or 50 to find your favorite size
   yourTurnIndicator->setZValue(ZLayer::Z_YOUR_TURN); // above cards and trick pile
   scene->addItem(yourTurnIndicator);
   //scene->setTurnIndicatorItem(yourTurnIndicator);
   scene->protectItem(yourTurnIndicator);

   yourTurnIndicator->hide();   // hidden by default
}

void MainWindow::createPlayerNames()
{
  for (int p=1; p < 4; p++) {
    playerNamesItem[p] = new QGraphicsTextItem(engine->PlayerName(p));
    playerNamesItem[p]->setZValue(ZLayer::Z_PLAYERS_NAME);

    scene->addItem(playerNamesItem[p]);
    scene->protectItem(playerNamesItem[p]);
  }
  updatePlayerNames();
  // first update is delayed in a QTimer QMainWindow::QMainWindow
}

void MainWindow::createButtonsGroups()
{
// Deck style buttons
    deckGroup = new QButtonGroup(this);

    deckGroup->addButton(ui->button_Deck_Standard, 0);
    deckGroup->addButton(ui->button_Deck_Nicu_White, 1);
    deckGroup->addButton(ui->button_Deck_English, 2);
    deckGroup->addButton(ui->button_Deck_Russian, 3);
    deckGroup->addButton(ui->button_Deck_Kaiser_jubilaum, 4);
    deckGroup->addButton(ui->button_Deck_Tigullio_inter, 5);
    deckGroup->addButton(ui->button_Deck_Mittelalter, 6);
    deckGroup->addButton(ui->button_Deck_Neo_Classical, 7);
    deckGroup->setExclusive(true);

    connect(deckGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, &MainWindow::onDeckStyleClicked);

// Game Variants buttons
    variantsGroup = new QButtonGroup(this);

    variantsGroup->addButton(ui->opt_queen_spade, 0);
    variantsGroup->addButton(ui->opt_perfect_100, 1);
    variantsGroup->addButton(ui->opt_omnibus, 2);
    variantsGroup->addButton(ui->opt_no_tricks, 3);
    variantsGroup->addButton(ui->opt_new_moon, 4);
    variantsGroup->addButton(ui->opt_no_draw, 5);
    variantsGroup->setExclusive(false);

    connect(variantsGroup, &QButtonGroup::idToggled, this, &MainWindow::onVariantToggled);

// Language buttons
   languageGroup = new QButtonGroup(this);

   languageGroup->addButton(ui->opt_english, 0);
   languageGroup->addButton(ui->opt_french, 1);
   languageGroup->addButton(ui->opt_russian, 2);
   languageGroup->setExclusive(true);

    connect(languageGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, [this](int id) {
      if (id == config->get_language()) {
        return;
      }
      config->set_language(id);
      setLanguage();
    });

// Animations buttons
   animationsGroup = new QButtonGroup(this);

   animationsGroup->addButton(ui->opt_anim_deal_cards, 0);
   animationsGroup->addButton(ui->opt_anim_play_card, 1);
   animationsGroup->addButton(ui->opt_anim_collect_tricks, 2);
   animationsGroup->addButton(ui->opt_anim_pass_cards, 3);
   animationsGroup->addButton(ui->opt_anim_arrow, 4);
   animationsGroup->addButton(ui->opt_anim_triangle, 5);
   animationsGroup->setExclusive(false);

   connect(animationsGroup, &QButtonGroup::idToggled, this, &MainWindow::onAnimationToggled);
}

void MainWindow::createScoreDisplay()
{
    if (m_scoreGroup) {
        // Si déjà créé, on ne fait rien (évite les doublons)
        return;
    }

    m_scoreGroup = new DraggableScoreGroup(this);
    m_scoreGroup->setZValue(Z_SCORE);

    m_scoreGroup->setFlag(QGraphicsItem::ItemIsMovable, true);
    m_scoreGroup->setFlag(QGraphicsItem::ItemIsSelectable, false);
    m_scoreGroup->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    m_scoreGroup->setCursor(Qt::OpenHandCursor);
    m_scoreGroup->setAcceptHoverEvents(true);

    // Fond et texte...
    m_scoreBackground = new QGraphicsRectItem(0, 0, 320, 110);
    m_scoreBackground->setBrush(QColor(30, 30, 50, 180));
    m_scoreBackground->setPen(QPen(QColor(180, 180, 220, 200), 2));
    m_scoreBackground->setOpacity(0.88);
    m_scoreGroup->addToGroup(m_scoreBackground);

    m_scoreText = new QGraphicsTextItem();
    m_scoreText->setDefaultTextColor(Qt::white);
    m_scoreText->setFont(QFont("Arial", qBound(10, qRound(ui->graphicsView->height() * 0.015), 12)));
    m_scoreText->setPos(12, 8);
    m_scoreGroup->addToGroup(m_scoreText);

    scene->addItem(m_scoreGroup);

/*
    // ────────── RESTAURATION DE LA POSITION SAUVEGARDÉE ──────────
    QSize viewSize = ui->graphicsView->size();

    if (scorePositionSaved && savedScorePos != QPointF(0, 0)) {
        // On restaure la dernière position connue (même après clear)
        qreal x = savedScorePos.x();
        qreal y = savedScorePos.y();

        // Sécurité
        x = qBound(10.0, x, viewSize.width() - m_scoreGroup->boundingRect().width() - 10);
        y = qBound(120.0, y, viewSize.height() - m_scoreGroup->boundingRect().height() - 10);

        m_scoreGroup->setPos(x, y);

        qDebug() << "Restauration après recreate : pos" << x << y << "(saved)";
    } else {
        // Première fois ou pas de sauvegarde : bas-gauche
        qreal x = 20.0;
        qreal y = viewSize.height() - m_scoreGroup->boundingRect().height() - 80;
        m_scoreGroup->setPos(x, y);

        // Et on sauvegarde cette position par défaut pour les prochains tours
        setSavedScorePos(QPointF(x, y));
        setScorePositionSaved(true);

        qDebug() << "Position par défaut bas-gauche appliquée et sauvegardée";
    }
*/
    updateScorePosition();  // au cas où
}

void MainWindow::createTOC()
{
  struct tocEntry {
    QString anchor;
    bool isSubsection;
  };

  static const tocEntry TOC[] =  {
                           {"rules", false },
                           {"settings", false },
                           {"variants", true },
                           {"miscellaneous", true },
                           {"scoreboard", true },
                           {"shortcuts", false },
                           {"online", false },
                           {"credits", false },
                           {"decks", true },
                           {"icons", true },
                           {"backgrounds", true },
                           {"sounds", true },
                           {"special", true}
                         };

  ui->treeHelpTOC->setHeaderHidden(true);
  ui->treeHelpTOC->setColumnCount(1);
  ui->treeHelpTOC->setStyleSheet("QTreeWidget { background: #2e5c3e; color: #e0ffe0; font-size: 12px; }"
                                 "QTreeWidget::item { padding: 8px; }"
                                 "QTreeWidget::item:selected { background: #5a9f5a; color: white; }"
  );

  QTreeWidgetItem *lastParent = nullptr;

  int cpt = 0;
  for (const auto &entry : TOC) {
    QTreeWidgetItem *item;

    if (entry.isSubsection && lastParent != nullptr) {
        item = new QTreeWidgetItem(lastParent);
    } else {
        item = new QTreeWidgetItem(ui->treeHelpTOC);
        lastParent = item;
    }

    QString title;
    switch (static_cast<TocIndex>(cpt)) {
       case TOC_Rules :         title = tr("1. Basic rules of the game"); break;
       case TOC_Settings :      title = tr("2. Game settings"); break;
       case TOC_Variants :      title = tr("2.1 Game variants"); break;
       case TOC_Miscellaneous : title = tr("2.2 Miscellaneous"); break;
       case TOC_Scoreboard :    title = tr("2.3 Scoreboard"); break;
       case TOC_Shortcuts :     title = tr("3. Game shortcuts"); break;
       case TOC_Online :        title = tr("4. Playing online"); break;
       case TOC_Credits :       title = tr("5. Credits"); break;
       case TOC_Decks :         title = tr("5.1 Playing card decks"); break;
       case TOC_Icons :         title = tr("5.2 Icons"); break;
       case TOC_Backgrounds :   title = tr("5.3 Backgrounds Images"); break;
       case TOC_Sounds :        title = tr("5.4 Sounds"); break;
       case TOC_Special :       title = tr("5.5 Special thanks"); break;
    }

    item->setText(0, title);
    item->setData(0, Qt::UserRole, entry.anchor);
    if (entry.isSubsection) {
        QFont font = item->font(0);
        font.setPointSize(font.pointSize() - 1);
        item->setFont(0, font);
    }
    cpt++;
  }

  ui->treeHelpTOC->expandAll();
}

// ************************************************************************************************


// ************************************[ 6- Animate Functions ] ***********************************
void MainWindow::animatePlayCard(int cardId, int player)
{
    setAnimationLock();

    QGraphicsSvgItem *item = deck->get_card_item(cardId, true);
    QGraphicsSvgItem *back = deck->get_card_item(cardId, false);

    item->show();
    back->hide();

    QSize viewSize = ui->graphicsView->size();

    QPointF deckCenter(viewSize.width() / 2.0 - item->boundingRect().width() / 2, viewSize.height() / 2);

    currentTrick.append(cardId);

    // Bring to front
    item->setZValue(++currentTrickZ + ZLayer::Z_TRICKS_BASE);

    // Calculate offset for trick pile (fan slightly)
    QPointF targetPos = deckCenter;

    // Animation group
    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);

    QVariantAnimation *posAnim = new QVariantAnimation(group);
    posAnim->setDuration(600);
    posAnim->setStartValue(item->pos());
    posAnim->setEndValue(targetPos);
    posAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(posAnim, &QVariantAnimation::valueChanged, this, [item](const QVariant &v) {
          item->setPos(v.toPointF());
    });

    QVariantAnimation *rotAnim = new QVariantAnimation(group);
    rotAnim->setDuration(600);
    rotAnim->setStartValue(item->rotation());
    rotAnim->setEndValue(0);  // Straight in center
    rotAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(rotAnim, &QVariantAnimation::valueChanged, this, [item](const QVariant &v) {
          item->setRotation(v.toReal());
    });

    group->addAnimation(posAnim);
    group->addAnimation(rotAnim);

    connect(group, &QParallelAnimationGroup::finished, this, [this, group, player, cardId]() {
        group->deleteLater();

        setAnimationUnlock();
        pushTrickInfo(cardId, player);
        engine->Step();

    });

    group->start();
}

void MainWindow::animatePassCards(const QList<int> passedCards[4], int direction)
{
    if (direction == PASS_HOLD) return;

   // setAnimationLock();

    int x, y;

    QSize viewSize = ui->graphicsView->size();
    qreal scale = calc_scale();

    // Define target positions for each player (where cards arrive)
    QPointF targetPos[4];

    x = cardX(passedCards[PLAYER_WEST].at(0), PLAYER_WEST, 0, true);
    targetPos[PLAYER_WEST] = QPointF(x + SELECTED_CARD_OFFSET, viewSize.height() / 2.0);

    y = cardY(passedCards[PLAYER_NORTH].at(0), PLAYER_NORTH, 0, true);
    targetPos[PLAYER_NORTH] = QPointF(viewSize.width() / 2.0, y + SELECTED_CARD_OFFSET);

    x = cardX(passedCards[PLAYER_EAST].at(0), PLAYER_EAST, 0, true);
    targetPos[PLAYER_EAST] = QPointF(x - SELECTED_CARD_OFFSET, viewSize.height() / 2.0);

    y = cardY(passedCards[PLAYER_SOUTH].at(0), PLAYER_SOUTH, 0, true);
    targetPos[PLAYER_SOUTH] = QPointF(viewSize.width() / 2.0, y - SELECTED_CARD_OFFSET);

    int delay = 0;
    const int duration = 800;
    const int stagger = 100;  // Cards from same player fly one after another

    // Mapping: who receives from whom based on direction
    int receiver[4];

    switch (direction) {
      case PASS_LEFT : // value(0) PASS_LEFT  = pass right in real life (clockwise)
            receiver[PLAYER_SOUTH] = PLAYER_WEST;
            receiver[PLAYER_WEST] = PLAYER_NORTH;
            receiver[PLAYER_NORTH] = PLAYER_EAST;
            receiver[PLAYER_EAST] = PLAYER_SOUTH;
            break;
      case PASS_RIGHT : // value(1)  PASS_RIGHT = pass left in real life (counter-clockwise)
            receiver[PLAYER_SOUTH] = PLAYER_EAST;
            receiver[PLAYER_WEST] = PLAYER_SOUTH;
            receiver[PLAYER_NORTH] = PLAYER_WEST;
            receiver[PLAYER_EAST] = PLAYER_NORTH;
            break;
       case PASS_ACROSS : // value(2)
            receiver[PLAYER_SOUTH] = PLAYER_NORTH;
            receiver[PLAYER_WEST] = PLAYER_EAST;
            receiver[PLAYER_NORTH] = PLAYER_SOUTH;
            receiver[PLAYER_EAST] = PLAYER_WEST;
            break;
       default :
            qDebug() << "Error: What are we doing here?";
            return;
    }

// ALL cards start animating at the same time (no delay/stagger)
    for (int giver = 0; giver < 4; ++giver) {
      int recv = receiver[giver];
      int cardId;

      bool reveal = ui->pushButton_reveal->isChecked() || (recv == PLAYER_SOUTH);
      QGraphicsItem *card, *hideCard;

      for (int c = 0; c < 3; ++c) {
        cardId = passedCards[giver].at(c);

        card = deck->get_card_item(cardId, reveal);
        hideCard = deck->get_card_item(cardId, !reveal);

        card->show();
        hideCard->hide();

        // get the correct 3 cards Z add a 200 bonus to be above the player current hand
        card->setZValue(cardZ(recv, c) + ZLayer::Z_PASSED_BASE);

        // Base target + fan
        QPointF baseTarget = targetPos[recv];
        qreal fanOffset = (c - 1) * (30.0 * scale);

        QPointF finalTarget;

        if ((recv == PLAYER_SOUTH) || (recv == PLAYER_NORTH))
          finalTarget = baseTarget + QPointF(fanOffset, 0);
        else
          finalTarget = baseTarget - QPointF(0, fanOffset);

        // Current rotation (from giver)
        qreal startRotation = card->rotation();

        // Target rotation (receiver's hand orientation)
        qreal targetRotation = anim_rotation_value(recv);

        // Parallel group for position + rotation
        QParallelAnimationGroup *group = new QParallelAnimationGroup(this);

        // Position animation
        QVariantAnimation *posAnim = new QVariantAnimation(group);
        posAnim->setDuration(duration);
        posAnim->setStartValue(card->pos());
        posAnim->setEndValue(finalTarget);
        posAnim->setEasingCurve(QEasingCurve::OutCubic);
        connect(posAnim, &QVariantAnimation::valueChanged, this, [card](const QVariant &v) {
              card->setPos(v.toPointF());
        });

        // Rotation animation — smooth reorientation
        QVariantAnimation *rotAnim = new QVariantAnimation(group);
        rotAnim->setDuration(duration);
        rotAnim->setStartValue(startRotation);
        rotAnim->setEndValue(targetRotation);
        rotAnim->setEasingCurve(QEasingCurve::OutCubic);
        connect(rotAnim, &QVariantAnimation::valueChanged, this, [card](const QVariant &v) {
             card->setRotation(v.toReal());
        });

        group->addAnimation(posAnim);
        group->addAnimation(rotAnim);
        group->start(QAbstractAnimation::DeleteWhenStopped);
      }
    }

    // Enough time to let all animations finish + 1-2 seconds to see the cards that were passed.
    QTimer::singleShot(3000, this, [this, direction, passedCards]() {
      deck->reset_selections();
      engine->Step();
      setAnimationUnlock();
    });
}

void MainWindow::animateCardsToCenter(const QList<int> &cardsId,
                                      QParallelAnimationGroup *masterGroup,
                                      int startDelay,
                                      int perCardStagger)
{
    if (cardsId.isEmpty()) return;

    QSize viewSize = ui->graphicsView->size();
    QPointF center(viewSize.width() / 2.0, viewSize.height() / 2.0);

    int cardCount = cardsId.size();
    for (int i = 0; i < cardCount; ++i) {
        QGraphicsItem *card = deck->get_card_item(cardsId.at(i), true);
        card->setZValue(ZLayer::Z_TRICKS_BASE + i);

        // Small random offset so they don't perfectly overlap
        QPointF offset((i % 5 - 2) * 15, (i / 5 - 1) * 15);
        QPointF endPos = center + offset;

        QSequentialAnimationGroup *seq = new QSequentialAnimationGroup(this);

        QPauseAnimation *pause = new QPauseAnimation(startDelay + i * perCardStagger, seq);
        seq->addAnimation(pause);

        QVariantAnimation *posAnim = new QVariantAnimation(seq);
        posAnim->setDuration(600);
        posAnim->setStartValue(card->pos());
        posAnim->setEndValue(endPos);
        posAnim->setEasingCurve(QEasingCurve::OutQuad);
        connect(posAnim, &QVariantAnimation::valueChanged, [card](const QVariant &v) {
              card->setPos(v.toPointF());
        });
        seq->addAnimation(posAnim);

        // Optional: small rotation during flight to center
        QVariantAnimation *rotAnim = new QVariantAnimation(seq);
        rotAnim->setDuration(600);
        rotAnim->setStartValue(card->rotation());
        rotAnim->setEndValue(card->rotation() + 180);  // half flip
        connect(rotAnim, &QVariantAnimation::valueChanged, [card](const QVariant &v) {
              card->setRotation(v.toReal());
        });
        seq->addAnimation(rotAnim);

        if (masterGroup) {
            masterGroup->addAnimation(seq);
        } else {
            seq->start(QAbstractAnimation::DeleteWhenStopped);
        }
    }
}

QParallelAnimationGroup *MainWindow::animateCardsOffBoard(const QList<int> &cardsId, int winner)
{
    if (cardsId.isEmpty()) return nullptr;

    QSize viewSize = ui->graphicsView->size();
    int viewWidth  = viewSize.width();
    int viewHeight = viewSize.height();

    QParallelAnimationGroup *offGroup = new QParallelAnimationGroup(this);

    // Target direction (edge of screen)
    QPointF baseDirection;
    switch (winner) {
      case PLAYER_WEST:  baseDirection = QPointF(0, viewHeight / 2.0);                break; // Left
      case PLAYER_NORTH: baseDirection = QPointF(viewWidth / 2.0, 0);                 break; // Top
      case PLAYER_EAST:  baseDirection = QPointF(viewWidth, viewHeight / 2.0);   break; // Right
      case PLAYER_SOUTH: baseDirection = QPointF(viewWidth / 2.0, viewHeight);   break; // Bottom
      default:           baseDirection = QPointF(viewWidth / 2.0, viewHeight / 2.0);   break;
    }

    QRandomGenerator *gen = QRandomGenerator::global();

    for (int i = 0; i < cardsId.size(); ++i) {
        int cardId = cardsId.at(i);

        QGraphicsItem *card = deck->get_card_item(cardId, true);
        card->setZValue(ZLayer::Z_OFFBOARD + i);

        QSequentialAnimationGroup *seq = new QSequentialAnimationGroup(offGroup);

        // Short staggered pause
        QPauseAnimation *pause = new QPauseAnimation(50 + i * 30, seq);
        seq->addAnimation(pause);

        QPointF startPos = card->pos();
        QPointF randomOffset(gen->bounded(-200, 201), gen->bounded(-200, 201));
        QPointF endPos = baseDirection + randomOffset;

        // Only position animation — clean and fast
        QVariantAnimation *posAnim = new QVariantAnimation(seq);
        posAnim->setDuration(500);  // fast and snappy
        posAnim->setStartValue(startPos);
        posAnim->setEndValue(endPos);
        posAnim->setEasingCurve(QEasingCurve::InBack);  // Nice "sucked in" feel

        connect(posAnim, &QVariantAnimation::valueChanged, [card](const QVariant &v) {
              card->setPos(v.toPointF());
        });
        seq->addAnimation(posAnim);
    }

    connect(offGroup, &QParallelAnimationGroup::finished, this, [this, cardsId, offGroup]() {
        for (int cardId : cardsId) {
            QGraphicsItem *card = deck->get_card_item(cardId, true);
            card->hide();
        }
        offGroup->deleteLater();
    });

    offGroup->start();
    return offGroup;
}

void MainWindow::revealPlayer(int player)
{
  int cardId;
  QGraphicsItem *frontCard, *backCard;

  for (int handIndex = 0; handIndex < engine->handSize((PLAYER)player); handIndex++) {
    cardId = engine->get_player_card((PLAYER)player, handIndex);
    if (cardId == INVALID_CARD) continue;

    frontCard = deck->get_card_item(cardId, true);
    backCard = deck->get_card_item(cardId, false);

    backCard->hide();

    // demain -> ne marche pas
    set_card_pos(cardId, player, handIndex, true);
    frontCard->show();
  }
}

void MainWindow::animateCollectTrick(int winner, bool takeAllRemaining = false)
{
    setAnimationLock();

    QParallelAnimationGroup *toCenterGroup = new QParallelAnimationGroup(this);

    int perCardStagger = takeAllRemaining ? 40 : 80;

    // Capture opponent hands for TRAM
    QList<int> capturedOpponentHands;

    if (takeAllRemaining) {
        int handDelayBase = 300;
        for (int p = 0; p < 4; ++p) {
            if (p != PLAYER_SOUTH) {
              revealPlayer(p);
            }
            if (p == winner) continue;
            capturedOpponentHands += engine->Hand(p);

            int handStartDelay = handDelayBase + p * 150;
            animateCardsToCenter(engine->Hand(p), toCenterGroup, handStartDelay, perCardStagger);
        }
    }

    connect(toCenterGroup, &QParallelAnimationGroup::finished, this,
        [this, winner, takeAllRemaining, toCenterGroup, capturedOpponentHands]() mutable {
    // Build full list of cards now in center
    QList<int> allInCenter = currentTrick;

    if (takeAllRemaining) {
        allInCenter += capturedOpponentHands;
    }

    // Short pause → then fly off to winner
    QTimer::singleShot(takeAllRemaining ? 800 : 300, this, [this, allInCenter, winner, toCenterGroup]() {
        // Start the final off-board animation
        QParallelAnimationGroup *offGroup = animateCardsOffBoard(allInCenter, winner);

        // Unlock ONLY when the final animation finishes
        if (offGroup) {
            connect(offGroup, &QParallelAnimationGroup::finished, this, [this, offGroup, toCenterGroup]() {
                setAnimationUnlock();  // ← Now at the very end
                clearTricks();
                engine->Step();
                offGroup->deleteLater();
                toCenterGroup->deleteLater();
            });
        } else {
            // Fallback if no group (shouldn't happen)
            setAnimationUnlock();
            toCenterGroup->deleteLater();
        }
    });

    // DO NOT unlock here!
});

    toCenterGroup->start();
}

void MainWindow::animateDeal(int dealDuration, int delayBetweenCards)
{
    QSize viewSize = ui->graphicsView->size();
    qreal scale = calc_scale();
    QPointF deckCenter(viewSize.width() / 2.0 - 65 * scale, viewSize.height() / 2.0 - 45 * scale);

    int delay = 0;
    int player = 0;
    int handIndex = 0;

    setAnimationLock(); // Locks manual resize, it would be messy during animation

    int totalDelay = DECK_SIZE * delayBetweenCards + dealDuration;

    bool revealed;
    for (int i = 0; i < DECK_SIZE; ++i) {
        int cardId = engine->get_player_card((PLAYER)player, handIndex);
        if (cardId == INVALID_CARD) continue;

        revealed = ui->pushButton_reveal->isChecked() || (player == PLAYER_SOUTH);
        QGraphicsItem *item = deck->get_card_item(cardId, revealed);

        // Initial setup: place in center pile
        item->setPos(deckCenter);
        item->setScale(scale);
        item->setRotation(QRandomGenerator::global()->bounded(-8, 9));
        item->setZValue(ZLayer::Z_CARDS_BASE - i);  // Back card on top
        item->setVisible(true);

        // Calculate final position
        set_card_pos(cardId, player, handIndex, revealed);
        QPointF targetPos = item->pos();

        // Define correct final rotation (independent of set_card_pos)
        qreal targetRotation = anim_rotation_value(player);

        // Reset for animation start
        item->setPos(deckCenter);
        item->setRotation(QRandomGenerator::global()->bounded(-8, 9));

        // Create parallel group
        QParallelAnimationGroup *group = new QParallelAnimationGroup(this);

        // Position animation
        QVariantAnimation *posAnim = new QVariantAnimation(group);
        posAnim->setDuration(dealDuration);
        posAnim->setStartValue(deckCenter);
        posAnim->setEndValue(targetPos);
        posAnim->setEasingCurve(QEasingCurve::OutCubic);
        connect(posAnim, &QVariantAnimation::valueChanged, this, [item](const QVariant &v) {
              item->setPos(v.toPointF());
        });

        // Rotation animation
        QVariantAnimation *rotAnim = new QVariantAnimation(group);
        rotAnim->setDuration(dealDuration);
        rotAnim->setStartValue(item->rotation());
        rotAnim->setEndValue(targetRotation);
        rotAnim->setEasingCurve(QEasingCurve::OutCubic);
        connect(rotAnim, &QVariantAnimation::valueChanged, this, [item](const QVariant &v) {
              item->setRotation(v.toReal());
        });

        group->addAnimation(posAnim);
        group->addAnimation(rotAnim);

        // Final Z-value when animation finishes
        connect(group, &QParallelAnimationGroup::finished, this, [item, player, handIndex]() {
            int finalZ = handIndex;
            if (player == PLAYER_SOUTH || player == PLAYER_WEST) {  // Bottom or left — reverse stack
                finalZ = ZLayer::Z_CARDS_BASE - handIndex;
            }
            item->setZValue(finalZ);
        });

        // Start the entire group after delay
        QTimer::singleShot(delay, group, [group]() {
            group->start();
        });

        // Auto cleanup
        connect(group, &QParallelAnimationGroup::finished, group, &QObject::deleteLater);

        delay += delayBetweenCards;

        if (++player > 3) {
          player = 0;
          handIndex++;
        }
    }

    // re-enable resizing
    QTimer::singleShot(totalDelay + 400, this, [this]() {
      engine->Step();
      setAnimationUnlock();
    });
}

void MainWindow::animateYourTurnIndicator()
{
    if (!yourTurnIndicator) return;

    setAnimationLock();

    // Reset to starting state
    yourTurnIndicator->setOpacity(0.0);
    yourTurnIndicator->setScale(0.8);
    yourTurnIndicator->setPos(yourTurnIndicator->x(),
                              yourTurnIndicator->y() - 30);  // start 30px higher

    // Parallel group for smooth combo
    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);

    // 1. Fade in + pulse
    QPropertyAnimation *opacityAnim = new QPropertyAnimation(yourTurnIndicator, "opacity");
    opacityAnim->setDuration(600);
    opacityAnim->setKeyValues({
        {0.0, 0.0},
        {0.3, 1.0},
        {0.7, 0.8},
        {1.0, 1.0}
    });
    opacityAnim->setEasingCurve(QEasingCurve::OutQuad);

    // 2. Scale pulse (grow slightly then settle)
    QPropertyAnimation *scaleAnim = new QPropertyAnimation(yourTurnIndicator, "scale");
    scaleAnim->setDuration(600);
    scaleAnim->setKeyValues({
        {0.0, 0.8},
        {0.4, 1.15},
        {1.0, 1.0}
    });
    scaleAnim->setEasingCurve(QEasingCurve::OutElastic);

    // 3. Gentle drop/bounce
    QPropertyAnimation *posAnim = new QPropertyAnimation(yourTurnIndicator, "pos");
    posAnim->setDuration(600);
    posAnim->setEndValue(yourTurnIndicator->pos() + QPointF(0, 30));  // final correct pos
    posAnim->setEasingCurve(QEasingCurve::OutBounce);

    group->addAnimation(opacityAnim);
    group->addAnimation(scaleAnim);
    group->addAnimation(posAnim);

    connect(group, &QParallelAnimationGroup::finished, this, [this, group]() {
        setAnimationUnlock();
    });

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::highlightAIPassedCards(int player, const QList<int>& passedCardIds)
{
    for (int cardId : passedCardIds) {
        QGraphicsSvgItem *cardItem = deck->get_card_item(cardId, ui->pushButton_reveal->isChecked());
        if (!cardItem) continue;

        QPointF startPos = cardItem->pos();
        QPointF endPos = startPos;

        switch (player) {
            case PLAYER_WEST:  endPos += QPointF(SELECTED_CARD_OFFSET, 0); break;   // Vers la droite
            case PLAYER_NORTH: endPos += QPointF(0, SELECTED_CARD_OFFSET); break;   // Vers le bas
            case PLAYER_EAST:  endPos -= QPointF(SELECTED_CARD_OFFSET, 0); break;   // Vers la gauche
            default: continue;
        }

        // Animation de position seulement (pas de scale)
        QPropertyAnimation *anim = new QPropertyAnimation(cardItem, "pos");
        anim->setDuration(600);
        anim->setStartValue(startPos);
        anim->setEndValue(endPos);
        anim->setEasingCurve(QEasingCurve::OutQuad);
        anim->start(QAbstractAnimation::DeleteWhenStopped);

        // Nettoyage auto
        connect(anim, &QPropertyAnimation::finished, anim, &QObject::deleteLater);
    }
}
// ************************************************************************************************


// ************************************[ 7- MATHEMATIC FUNCTIONS ] ********************************
int MainWindow::cardX(int cardId, int player, int handIndex, bool frontCard)
{
  QSize viewSize = ui->graphicsView->size();
  int viewWidth  = viewSize.width();

  QRectF bounds = deck->get_card_item(cardId, frontCard)->boundingRect();

  qreal cardHeight = bounds.height();

  qreal scale = calc_scale();
  qreal margin  = 55 * scale;
  qreal spacing = 25 * scale;

  // Extra margin that grows only when scale > 1.0 (prevents truncation in fullscreen)
  qreal extraMargin = 80 * (scale - 1.0);

  int x,
      c = 0,
      hand_size = engine->handSize((PLAYER)player);

  switch (player) {
    case PLAYER_WEST: // Left player (rotated +90°)
       x = margin + extraMargin + cardHeight * 0.3;  // Fine-tuned offset to avoid clipping
       break;

    case PLAYER_NORTH: // Top player (rotated 180°)
        c = (viewWidth - (hand_size * spacing)) / 2;
        x = viewWidth - c - margin - (handIndex * spacing);
        break;

    case PLAYER_EAST: // Right player (rotated -90°)
        x = viewWidth - margin - extraMargin - cardHeight;  // Symmetric to left
        break;

    case PLAYER_SOUTH: // Bottom player (you)
        c = (viewWidth - (hand_size * spacing)) / 2;
        x = viewWidth - c - margin - (handIndex * spacing);
        break;
    }

  return x;
}

int MainWindow::cardY(int cardId, int player, int handIndex, bool frontCard)
{
  QSize viewSize = ui->graphicsView->size();

  QRectF bounds = deck->get_card_item(cardId, frontCard)->boundingRect();

  qreal cardHeight = bounds.height();
  int viewHeight = viewSize.height();

  qreal scale = calc_scale();
  qreal margin  = 55 * scale;
  qreal spacing = 25 * scale;

  // Extra margin that grows only when scale > 1.0 (prevents truncation in fullscreen)
  qreal extraMargin = 80 * (scale - 1.0);

  int y,
      c = 0,
      hand_size = engine->handSize((PLAYER)player);

  switch (player) {
    case PLAYER_WEST: // Left player (rotated +90°)
       c = (viewHeight - (hand_size * spacing)) / 2;
       y = viewHeight - c - margin - (handIndex * spacing);
       break;

    case PLAYER_NORTH: // Top player (rotated 180°)
        y = margin + extraMargin + cardHeight * 0.65;  // Pushes down from top
        break;

    case PLAYER_EAST: // Right player (rotated -90°)
        c = (viewHeight - (hand_size * spacing)) / 2;
        y = viewHeight - c - margin - (handIndex * spacing);
        break;

    case PLAYER_SOUTH: // Bottom player (you)
        y = viewHeight - margin - extraMargin - cardHeight * 0.68;  // Pushes up from bottom
        break;
    }

  return y;
}

qreal MainWindow::calc_scale() const
{
  int viewWidth = ui->graphicsView->width();

  // Base scale when view is 1200px wide (adjust this "reference width" to your taste)
  int REFERENCE_WIDTH = 1200;

  qreal baseScale = 1.0;

  // Calculate scale: clamp between 0.5 and 2.0
  qreal scale = static_cast<qreal>(viewWidth) / REFERENCE_WIDTH;
  scale = qBound(0.5, scale, 2.0);  // Clamps to 0.5–2.0 range

  return scale;
}

QPointF MainWindow::getTakenPileCenter(int player, const QSize& viewSize) const
{
    QPointF center;

    switch (player) {
    case PLAYER_SOUTH:
        center = QPointF(viewSize.width() / 2.0, viewSize.height() - 300);
        break;
    case PLAYER_WEST:
        center = QPointF(250, viewSize.height() / 2.0);
        break;
    case PLAYER_NORTH:
        center = QPointF(viewSize.width() / 2.0, 300);
        break;
    case PLAYER_EAST:
        center = QPointF(viewSize.width() - 250, viewSize.height() / 2.0);
        break;
    }

    return center;
}

qreal MainWindow::anim_rotation_value(int player)
{
   switch (player) {
     case PLAYER_WEST:  return 90;
     case PLAYER_NORTH: return 180;
     case PLAYER_EAST:  return -90;
     case PLAYER_SOUTH:
     default:           return 0;
   }
}

int MainWindow::cardZ(int player, int cardId)
{
  if ((player == PLAYER_WEST) || (player == PLAYER_NORTH))
    return 114 - cardId;

  return 101 + cardId;
}

QPointF MainWindow::calculatePlayerNamePos(int player) const
{
    QSizeF viewSize = ui->graphicsView->mapToScene(ui->graphicsView->viewport()->rect()).boundingRect().size();

    switch (player) {
    case PLAYER_SOUTH: return QPointF(viewSize.width() / 2, viewSize.height() - 100 * calc_scale());  // Bas centre
    case PLAYER_NORTH: return QPointF(viewSize.width() / 2, 65 * calc_scale() - 10);  // Haut centre
    case PLAYER_WEST:  return QPointF(90 * calc_scale() - 10, viewSize.height() / 2 - 20 * 4);  // Gauche centre
    case PLAYER_EAST:  return QPointF(viewSize.width() - 90 * calc_scale() + 10, viewSize.height() / 2 - 20 * 1);  // Droite centre
    }
    return QPointF();
}
// ************************************************************************************************


// ***********************************[ 8- UNSORTED FUNCTIONS ]************************************
void MainWindow::enableAllCards()
{
  for (int c = 0; c < DECK_SIZE; c++) {
    QGraphicsItem *card = deck->get_card_item(c, true);

    card->setData(10, false);
    card->setOpacity(1.0);
    card->setGraphicsEffect(nullptr);  // Clear any colorize
  }
}

void MainWindow::removeInvalidCardsEffect()
{
  const QList<int> &hand = engine->Hand(PLAYER_SOUTH);
  for (int cardId : hand) {
    QGraphicsItem *card = deck->get_card_item(cardId, true);
    if (!card) continue;

    card->setOpacity(1.0);
    card->setGraphicsEffect(nullptr);
    card->setData(10, false);  // Enabled
    card->setToolTip("");
  }
}

void MainWindow::disableInvalidCards()
{
    const QList<int> &hand = engine->Hand(PLAYER_SOUTH);
    const QColor tintColor = background->getAverageColor();

    for (int cardId : hand) {
        QGraphicsItem *card = deck->get_card_item(cardId, true);
        if (!card) continue;

        // On demande au moteur si cette carte est jouable dans l'état actuel
        GAME_ERROR err = engine->validate_move(PLAYER_SOUTH, cardId);
        bool isLegal = (err == NOERROR);

        if (isLegal) {
            card->setOpacity(1.0);
            card->setGraphicsEffect(nullptr);
            card->setData(10, false);  // Enabled
        } else {
            card->setOpacity(0.9);
            QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect(this);
            effect->setColor(tintColor);
            effect->setStrength(1.0);
            card->setGraphicsEffect(effect);
            card->setData(10, true);   // Disabled
            card->setToolTip(engine->errorMessage(err));
        }
    }

    scene->update();
}

void MainWindow::set_card_pos(int cardId, int player, int handIndex, bool frontCard)
{
  int x = cardX(cardId, player, handIndex, frontCard);
  int y = cardY(cardId, player, handIndex, frontCard);

  QGraphicsSvgItem *frontCard_item = deck->get_card_item(cardId, true);
  QGraphicsSvgItem *backCard_item = deck->get_card_item(cardId, false);

  QVariant idVar = frontCard_item->data(1);
  if (!idVar.isValid() || !idVar.canConvert<bool>()) return;

  int selected = idVar.toBool() ? SELECTED_CARD_OFFSET : 0;

  frontCard_item->setPos(x, y - selected);
  frontCard_item->setScale(calc_scale());

  backCard_item->setPos(x, y);
  backCard_item->setScale(calc_scale());

  if (frontCard) {
    frontCard_item->show();
    backCard_item->hide();
  } else {
      frontCard_item->hide();
      backCard_item->show();
  }
}

void MainWindow::saveCurrentTrickState()
{
    currentTrickInfo.clear();
    for (int index : currentTrick) {
        QGraphicsSvgItem *item = deck->get_card_item(index, true);
        TrickCardInfo info;
        info.cardId = item->data(0).toInt();
        info.rotation = item->rotation();
        info.position = item->pos();
        currentTrickInfo.append(info);
    }
    currentTrick.clear();
}

void MainWindow::restoreTrickCards()
{
    for (int i = 0; i < currentTrickInfo.size(); ++i) {
        const TrickCardInfo &info = currentTrickInfo[i];
        QGraphicsSvgItem *newItem = deck->get_card_item(info.cardId, true);
        if (newItem) {
            newItem->setZValue(Z_TRICKS_BASE + i);  // 200 + 0, 201 + 1, etc.

            newItem->setPos(info.position);
            newItem->setRotation(info.rotation);

            currentTrick.append(info.cardId);
        }
    }
}

void MainWindow::clearTricks()
{
  for (int cardId : currentTrick) {
    QGraphicsSvgItem *card = deck->get_card_item(cardId, true);
    card->hide();
  }
  currentTrick.clear();
  currentTrickInfo.clear();
  currentTrickZ = Z_TRICKS_BASE;
}

void MainWindow::import_allCards_to_scene()
{
  QGraphicsSvgItem *front, *back;

  for (int i = 0; i < DECK_SIZE; i++) {
    front = deck->get_card_item(i, true);
    back = deck->get_card_item(i, false);

    scene->addItem(front);
    scene->addItem(back);
  }
}

void MainWindow::disableAllDecks()
{
  for (QAbstractButton *btn : deckGroup->buttons()) {
       btn->setEnabled(false);
    }
}

void MainWindow::enableAllDecks()
{
   QTimer::singleShot(500, this, [this]() {
     int index = 0;
     for (QAbstractButton *btn : deckGroup->buttons()) {
       if (valid_deck[index]) {
         btn->setEnabled(true);
       };
       index++;
     }
   });
}

void MainWindow::disableDeck(int deckId) {
  QAbstractButton *button = deckGroup->button(deckId);
  if (button) {
    button->setChecked(false);
    button->setEnabled(false);
  }

  QMessageBox::warning(this, "Deck Load Failed",
            tr("The selected deck could not be loaded.\n\n"
            "It may be unsupported, missing files or corrupted.\n"
            "Please select a different deck in Settings."));

  if (!forced_new_deck) {
    ui->tabWidget->setTabEnabled(TAB_BOARD, false);
    ui->tabWidget->setCurrentIndex(TAB_SETTINGS);
    forced_new_deck = true;
  }
}

void MainWindow::message(QString mesg, MESSAGE type)
{
  QString complete;
  switch (type) {
    case MESSAGE_ERROR: complete = tr("[Error]: ");
                        sounds->play(SOUND_ERROR);
                        break;
    case MESSAGE_INFO:  complete = tr("[Info]: " );
                        break;
    case MESSAGE_SYSTEM:
    default: break;
  }

  complete += mesg;
  ui->channel->append(complete);
}

void MainWindow::adjustGraphicsViewSize()
{
    if (!ui->graphicsView) return;

    // Option A: Fit scene to view (recommended for card games)
    ui->graphicsView->fitInView(ui->graphicsView->sceneRect(), Qt::KeepAspectRatio);

    // Option B: If you want the green background to fill exactly, no letterboxing:
    // ui->graphicsView->setSceneRect(ui->graphicsView->rect());
    // But then you must manually reposition cards on resize — more work.

    // Ensure the view updates immediately
    ui->graphicsView->viewport()->update();
}

void MainWindow::showTurnArrow(int direction)
{
    if (!arrowProxy) return;

    qreal rot = 0;

    if (direction == PASS_LEFT)
      rot = -90;
    else if (direction == PASS_RIGHT)
          rot = 90;

    arrowProxy->setRotation(rot);

    // set visibility to false, if direction = PASS_HOLD or invalid direction
    arrowProxy->setVisible((direction == PASS_LEFT) || (direction == PASS_RIGHT) || (direction == PASS_ACROSS));

    updateTurnArrow();
}

// In your updateTurnArrow() or similar function
void MainWindow::showTurnIndicator()
{
    sounds->play(SOUND_YOUR_TURN);

    if (ui->opt_animations->isChecked() && ui->opt_anim_triangle->isChecked()) {
      yourTurnIndicator->show();
      animateYourTurnIndicator();
    } else
      yourTurnIndicator->hide();
  }

void MainWindow::pushTrickInfo(int cardId, int owner)
{
   QGraphicsSvgItem *card = deck->get_card_item(cardId, true);

   TrickCardInfo info;
   info.cardId = cardId;
   info.rotation = card->rotation();
   info.position = card->pos();
   info.ownerPlayer = owner;

   currentTrickInfo.append(info);
}

void MainWindow::playCard(int cardId, int player)
{
    QGraphicsSvgItem *frontItem = deck->get_card_item(cardId, true);

    if (ui->opt_animations->isChecked() && ui->opt_anim_play_card->isChecked()) {
      animatePlayCard(cardId, player);
      return;
    }

    QGraphicsSvgItem *backItem = deck->get_card_item(cardId, false);

    QSize viewSize = ui->graphicsView->size();
    QPointF FdeckCenter(viewSize.width() / 2.0 - frontItem->boundingRect().width() / 2, viewSize.height() / 2);
    QPointF BdeckCenter(viewSize.width() / 2.0 - backItem->boundingRect().width() / 2, viewSize.height() / 2);

    frontItem->setPos(FdeckCenter);
    backItem->setPos(BdeckCenter);

    currentTrick.append(cardId);
    currentTrickZ++;
    pushTrickInfo(cardId, player);

    frontItem->setZValue(currentTrickZ + ZLayer::Z_TRICKS_BASE);
    frontItem->show();

    backItem->setZValue(currentTrickZ + ZLayer::Z_TRICKS_BASE);
    backItem->hide();

    engine->Step();
}
