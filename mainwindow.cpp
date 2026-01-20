#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "define.h"

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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

// Protect side players' cards from clipping
// setMinimumHeight(MIN_APPL_HEIGHT);

    ui->graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    ui->graphicsView->setCacheMode(QGraphicsView::CacheNone);

    setWindowTitle(VERSION);

    message(tr("Welcome to ") + QString(VERSION));

    config = new Config();

    sounds = new Sounds(this);

    scene = new CardScene(this);
   // scene->setItemIndexMethod(QGraphicsScene::NoIndex);  // Moins de repaint inutiles
//ui->graphicsView->setRenderHint(QPainter::Antialiasing, false);

    deck = new Deck(this);

    engine = new Engine(this);

    ui->graphicsView->setScene(scene);

    background = new Background(scene, this);
    scene->protectItem(background);

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

  //  ui->graphicsView->setAlignment(Qt::AlignCenter);  // Critical for proper centering
    ui->graphicsView->setSceneRect(scene->itemsBoundingRect());  // Or set a large fixed rect

// Solid green background (no repeat)
    ui->graphicsView->setBackgroundBrush(QBrush(QColor(0, 100, 0)));  // dark green felt

    connect(scene, &CardScene::cardClicked, this, &MainWindow::onCardClicked);
    connect(scene, &CardScene::arrowClicked, this, &MainWindow::onArrowClicked);

    connect(deck, &Deck::deckChange, this, &MainWindow::onDeckChanged);

    connect(ui->pushButton_reveal, &QPushButton::clicked, this, [this](bool checked) {
      refresh_deck();
      config->set_config_file(CONFIG_CHEAT_MODE, checked);
    });

    connect(ui->pushButton_sound, &QPushButton::clicked, this, [this](bool checked) {
      sounds->setEnabled(checked);
      config->set_config_file(CONFIG_SOUNDS, checked);
    });

    connect(ui->pushButton_new, &QPushButton::clicked, this, [this]() {
      // unselect any selected cards.
      selectedCards.clear();
      deck->reset_selections();

      engine->start_new_game();
      // Note: engine will emit sig_clear_deck() --> scene->clear() + clearTricks() + yourTurnIndicator->hide() = arrowLabel->hide()
    });

    connect(ui->pushButton_undo, &QPushButton::clicked, this, [this]() {
      engine->undo() ? sounds->play(SOUND_UNDO) : sounds->play(SOUND_ERROR);
    });

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
      if (index == 0) {  // Switched TO Board tab
        QTimer::singleShot(0, this, [this]() {
          QSize currentSize = ui->graphicsView->size();

          bool sizeChanged = (currentSize != lastBoardViewSize);
          bool needsFullReposition = sizeChanged && windowWasResizedWhileAway;

          updateSceneRect();
          if (needsFullReposition) {
            repositionAllCardsAndElements();
            lastBoardViewSize = currentSize;
          }
          else {
            refresh_deck();
            updateTrickPile();
          }

          windowWasResizedWhileAway = false;
        });
      }
    });

    connect(background, &Background::backgroundChanged, this, [this](const QString &path) {
        if (loadBackgroundPreview(path)) {
          updateCredits(background->Credits(), background->CreditTextColor());
          config->set_background_path(path);
        }
    });

    connect(engine, &Engine::sig_your_turn, this, [this]() {
       disableInvalidCards();
       showTurnIndicator();
    });

    connect(engine, &Engine::sig_hearts_broken, this, [this]() {
      sounds->play(SOUND_BREAKING_HEARTS);
    });

    connect(engine, &Engine::sig_queen_spade, this, [this]() {
      sounds->play(SOUND_QUEEN_SPADE);
    });

    connect(engine, &Engine::sig_shuffle_deck, this, [this]() {
      sounds->play(SOUND_SHUFFLING_CARDS);
    });

    connect(engine, &Engine::sig_game_over, this, [this]() {
      sounds->play(SOUND_GAME_OVER);
    });

 //   connect(engine, &Engine::sig_setCurrentSuit, this, &MainWindow::setCurrentSuit);
    connect(engine, &Engine::sig_enableAllCards, this, &MainWindow::enableAllCards);
    connect(engine, &Engine::sig_refresh_deck, this, &MainWindow::refresh_deck);
    connect(engine, &Engine::sig_clear_deck, this, [this]() {
       scene->clear();
       clearTricks();
       yourTurnIndicator->hide();
       arrowLabel->hide();
    });

    connect(engine, &Engine::sig_new_players, this, [this](const QString names[4]) {
       updatePlayerNames();
    });

    connect(engine, &Engine::sig_update_scores_board, this, [this](const QString names[4], const int hand[4], const int total[4]) {
       updateScores(names, hand, total, icons);
    });

    connect(engine, &Engine::sig_collect_tricks, this, [this](PLAYER winner, bool TRAM) {
       if (ui->opt_animations->isChecked() && ui->opt_anim_collect_tricks->isChecked()) {
         animateCollectTrick(winner, TRAM);
       } else {
           QTimer::singleShot(1000, this, [this]() {
             clearTricks();
             engine->Step();
           });
         }
    });

    connect(engine, &Engine::sig_play_card, this, [this](int cardId, PLAYER player) {
       sounds->play(SOUND_DEALING_CARD);
qDebug() << "sig_play_card **** player: " << player << "cardId: " << cardId;
       playCard(cardId, player);
    });

    connect(engine, &Engine::sig_deal_cards, this, [this]() {
      if (ui->opt_animations->isChecked() && ui->opt_anim_deal_cards->isChecked()) {
        animateDeal();
      } else {
          engine->Step();
      }
    });

    connect(engine, &Engine::sig_passed, this, [this]() {
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
    });

    connect(engine, &Engine::sig_pass_to, this, [this](int direction) {
       showTurnArrow(direction);
    });

    connect(qApp, &QCoreApplication::aboutToQuit, this, &MainWindow::aboutToQuit);

    connect(ui->pushButton_exit, &QPushButton::clicked, qApp, &QApplication::quit);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete config;
    delete deckGroup;
    delete variantsGroup;
    delete languageGroup;
    delete animationsGroup;
    delete sounds;
    delete deck;
    delete engine;
    delete background;
    delete arrowLabel;
    delete arrowMovie;
    delete scene;
}

void MainWindow::aboutToQuit()
{
  config->set_width(size().width());
  config->set_height(size().height());
  config->set_posX(pos().x());
  config->set_posY(pos().y());

  // Give Pulse audio time to clean stuff to avoid crashes on exit()
  sounds->stopAllSounds();
  QTimer::singleShot(200, qApp, &QApplication::quit);
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

void MainWindow::disableDeck(int deckId) {
  // It's safe to call with invalid ID. eg: TIGULLIO_MODERN_DECK (8) unsupported. Last (7)
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
// set the current tab to settings
    ui->tabWidget->setTabEnabled(0, false);
    ui->tabWidget->setCurrentIndex(4);
    forced_new_deck = true;
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

  ui->pushButton_reveal->setChecked(config->is_cheat_mode());

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

  ui->opt_anim_deal_cards->setChecked(config->is_anim_deal_cards());
  ui->opt_anim_play_card->setChecked(config->is_anim_play_card());
  ui->opt_anim_collect_tricks->setChecked(config->is_anim_collect_tricks());
  ui->opt_anim_pass_cards->setChecked(config->is_anim_pass_cards());
  ui->opt_anim_arrow->setChecked(config->is_animated_arrow());
  ui->opt_anim_triangle->setChecked(config->is_anim_turn_indicator());

// TODO: apply all languages changes
  switch (config->get_language()) {
    case LANG_ENGLISH: ui->opt_english->setChecked(true); break;
    case LANG_FRENCH: ui->opt_french->setChecked(true); break;
    case LANG_RUSSIAN: ui->opt_russian->setChecked(true); break;  
  }

  QSize savedSize(config->width(), config->height());
  QPoint savedPos(config->posX(), config->posY());

  // If there is no background path. We load the legacy background save by index.
  if (config->Path().isEmpty()) {
    // TODO: add credit.
    background->setBackground(config->get_background_index());
  } else {
    background->setBackgroundPixmap(config->Path());
  }

  resize(savedSize);

// With all the Linux window managers THAT doesn't works.
// move(savedPos);

  loadBackgroundPreview(background->FullPath());
}



// ************************************************************************************************
// Section 1- [ EVENTS OVERRIDE ]
//            resizeEvent(QResizeEvent *event)
//            eventFilter(QObject *obj, QEvent *event)
//            showEvent(QShowEvent *event)
//
// Section 2- [ Private Slots ]
//            onDeckChanged(int deckId)
//            onBackgroundPreviewClicked()
//            onCardClicked(QGraphicsItem *item)
//            onArrowClicked
//
// Section 3- [ Create Functions ]
//            createCreditsLabel()
//            create_arrows()
//            createButtonsGroups()
//            createPlayerNames()
//            createScoreDisplay()
//
// Section 4- [ Update Functions ]
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
// Section 5- [ Animate Functions ]
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
// Section 6- [ Mathematic Functions ]
//            cardX(int cardId, int player, int handIndex)
//            cardY(int cardId, int player, int handIndex)
//            getTakenPileCenter(int player, const QSize& viewSize) const
//            anim_rotation_value(int player)
//            calc_scale()
//            cardZ(int player, int cardId)
//            calculatePlayerNamePos(int player)
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
    if (ui->tabWidget->currentIndex() == 0) {  // 0 = Board tab
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
            engine->Start();
            updateSceneRect();
            repositionAllCardsAndElements();
        });
    }
}
// ************************************************************************************************



// ************************************[ 2- Private Slots ] ***************************************
void MainWindow::onDeckChanged(int deckId)
{
  qDebug() << "New deck: " << deckId;
  qDebug() << "Scene size:" << scene->items().size();
  QGraphicsItem *item;

  for (int i = 0; i < selectedCards.size(); i++) {
    item = deck->get_card_item(selectedCards.at(i), true);
    item->setData(1, true);
  }
}

//TODO: fix the default path
void MainWindow::onBackgroundPreviewClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Choose Background Image"),
        QDir::homePath() + FOLDER + QString("/backgrounds"),
        tr("Images (*.png *.jpg *.jpeg *.bmp *.gif)"));

    if (fileName.isEmpty()) return;

    QPixmap pix(fileName);
    if (pix.isNull()) {
        qDebug() << "Failed to load background:" << fileName;
        return;
    }

    // Update the actual scene background
    background->setBackgroundPixmap(fileName);

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
        if (engine->isPlaying() && (engine->Turn() == PLAYER_SOUTH)) {
          engine->Play(cardId);
          sounds->play(SOUND_DEALING_CARD);
          yourTurnIndicator->hide();
qDebug() << "We clicked to play a card";
          removeInvalidCardsEffect();
          playCard(cardId, PLAYER_SOUTH);
  //        engine->Step();
        }
      }
    if (player == PLAYER_NOBODY) {
      qDebug() << "Warning: Clicked card" << cardId << "not found in any player's hand!";
      return;
    }
}

void MainWindow::onArrowClicked()
{
qDebug() << "Clicked";
  if (selectedCards.size() != 3) {
    sounds->play(SOUND_ERROR);
    message(tr("You must select 3 cards to pass!"));
  } else {

qDebug() << "before: " << selectedCards.size();
    arrowLabel->hide();

    engine->set_passedFromSouth(selectedCards);
    selectedCards.clear();
    engine->Step();
  //  deck->reset_selections();
  //  update_cards_pos();
  }
}
// ************************************************************************************************



// ************************************[ 3- Update Functions **************************************
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
qDebug() << "Trick size: " << currentTrick.size();

    if (currentTrick.isEmpty()) return;

    QSize viewSize = ui->graphicsView->size();

    int count = currentTrick.size();

qDebug() << "Tricks: " << currentTrick;

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
        "<table width='%1' cellspacing='3' cellpadding='2' style='color:white; font-size:11px; margin-bottom: -14px; marging: 0; padding: 0;'>"
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
    x = qBound(10.0, x, viewSize.width() - m_scoreGroup->boundingRect().width() - 10);
    y = qBound(20.0, y, viewSize.height() - m_scoreGroup->boundingRect().height() - 10);

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

    x = qBound(10.0, x, viewSize.width() - m_scoreGroup->boundingRect().width() - 10);
    y = qBound(120.0, y, viewSize.height() - m_scoreGroup->boundingRect().height() - 10);

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



// ************************************[ 4- Create Functions ] ************************************
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

    arrowMovie = new QMovie(":/icons/arrow-11610_128.gif");
    arrowMovie->setSpeed(150);

    if (arrowMovie->isValid()) {
       arrowLabel->setMovie(arrowMovie);

    arrowLabel->setAlignment(Qt::AlignCenter);
    arrowMovie->start();
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

    connect(deckGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, [this](int id) {
      if (deck->Style() == id)
         return;

      saveCurrentTrickState();
      disableAllDecks();
      if (deck->set_deck(id)) {
        config->set_deck_style(id);
        import_allCards_to_scene();

        if (forced_new_deck) {
          ui->tabWidget->setTabEnabled(0, true);
          ui->tabWidget->setCurrentIndex(0);
          forced_new_deck = false;
          if (start_engine_dalayed) {
            start_engine_dalayed = false;
qDebug() << "Forces deck";
            engine->Start();
          //  engine->Step();
          } 
        }
        enableAllDecks();
      } else {
          enableAllDecksButOne(id);
        }
      restoreTrickCards();
      scene->update();
    });

// Game Variants buttons
    variantsGroup = new QButtonGroup(this);

    variantsGroup->addButton(ui->opt_queen_spade, 0);
    variantsGroup->addButton(ui->opt_perfect_100, 1);
    variantsGroup->addButton(ui->opt_omnibus, 2);
    variantsGroup->addButton(ui->opt_no_tricks, 3);
    variantsGroup->addButton(ui->opt_new_moon, 4);
    variantsGroup->addButton(ui->opt_no_draw, 5);

    variantsGroup->setExclusive(false);

    connect(variantsGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, [this](int id) {
       QAbstractButton *button = variantsGroup->button(id);
       bool checked = button->isChecked();

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
    });

// Language buttons
   languageGroup = new QButtonGroup(this);

   languageGroup->addButton(ui->opt_english, 0);
   languageGroup->addButton(ui->opt_french, 1);
   languageGroup->addButton(ui->opt_russian, 2);
   languageGroup->setExclusive(true);

    connect(languageGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, [this](int id) {
      config->set_language(id);
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

   connect(animationsGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, [this](int id) {
       QAbstractButton *button = animationsGroup->button(id);
       bool checked = button->isChecked();

       switch (id) {
          case 0: config->set_config_file(CONFIG_ANIM_DEAL_CARDS, checked); break;
          case 1: config->set_config_file(CONFIG_ANIM_PLAY_CARD, checked); break;
          case 2: config->set_config_file(CONFIG_ANIM_COLLECT_TRICKS, checked); break;
          case 3: config->set_config_file(CONFIG_ANIM_PASS_CARDS, checked); break;
          case 4: config->set_config_file(CONFIG_ANIMATED_ARROW, checked); break;
          case 5: config->set_config_file(CONFIG_ANIM_TURN_INDICATOR, checked); break;
       }
    });
}

/*
void MainWindow::createScoreDisplay()
{
    if (m_scoreGroup) return;

    m_scoreGroup = new QGraphicsItemGroup();
    m_scoreGroup->setZValue(Z_SCORE);

    // Fond et texte (comme avant)
    m_scoreBackground = new QGraphicsRectItem(0, 0, 320, 140);
    m_scoreBackground->setBrush(QColor(30, 30, 50, 180));
    m_scoreBackground->setPen(QPen(QColor(180, 180, 220, 200), 2));
    m_scoreBackground->setOpacity(0.88);
    m_scoreGroup->addToGroup(m_scoreBackground);

    m_scoreText = new QGraphicsTextItem();
    m_scoreText->setDefaultTextColor(Qt::white);
    m_scoreText->setFont(QFont("Arial", 11));
    m_scoreText->setPos(12, 12);
    m_scoreGroup->addToGroup(m_scoreText);

    scene->addItem(m_scoreGroup);

    // Position initiale FORCÉE en bas à gauche (marge 20 px gauche, 80 px bas)
    m_scoreGroup->setPos(20, 0);  // temporaire y=0 pour ajouter

    // Appel immédiat pour repositionner correctement
    updateScorePosition();
}
*/

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

// ************************************************************************************************

void MainWindow::message(QString mesg)
{
  ui->channel->append(mesg);
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

void MainWindow::setAnimationLock()
{
    m_animationLockCount++;

    if (m_animationLockCount == 1) {  // First lock (was 0 → 1)
        // Remember current size before fixing
        m_fixedSizeDuringLock = size();

        // Lock window size
        setFixedSize(m_fixedSizeDuringLock);

        // Disable deck switch tab
        ui->tabWidget->setTabEnabled(4, false);

        // Disable cards reveal
        ui->pushButton_reveal->setDisabled(true);

        // Disable new game
        ui->pushButton_new->setDisabled(true);

        // Disable undo
        ui->pushButton_undo->setDisabled(true);

qDebug() << "Visual cute";
        // Visual feedback
      //  ui->graphicsView->setCursor(Qt::WaitCursor);
       QApplication::setOverrideCursor(Qt::WaitCursor);


qDebug() << "Cursor: " << ui->graphicsView->cursor();
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
 //       qDebug() << "unlocked";
        ui->tabWidget->setTabEnabled(4, true);

        // TODO don't enable reveal and undo, if we're playing ONLINE
        ui->pushButton_reveal->setDisabled(false);
        ui->pushButton_undo->setDisabled(false);
        ui->pushButton_new->setDisabled(false);

      //  ui->graphicsView->unsetCursor();
        QApplication::restoreOverrideCursor();


        // Restore full resizability
        setMinimumSize(QSize(MIN_APPL_WIDTH, MIN_APPL_HEIGHT));
        setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
    }
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
    arrowMovie->start();

    updateTurnArrow();
}

// In your updateTurnArrow() or similar function
void MainWindow::showTurnIndicator()
{
    sounds->play(SOUND_YOUR_TURN);

    if (ui->opt_anim_triangle->isChecked()) {
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


// ************************************[ 5- Animate Functions ] ***********************************
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
//                scene->removeItem(card);
//                delete card;  // Safe if not parented elsewhere

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

        // Reset to center with shuffle for animation start
    //    item->setPos(deckCenter);
     //   item->setRotation(QRandomGenerator::global()->bounded(-8, 9));

        // NOW calculate final target position and rotation

        // Calculate final position
        set_card_pos(cardId, player, handIndex, revealed);
        QPointF targetPos = item->pos();

        // Define correct final rotation (independent of set_card_pos)
        qreal targetRotation = anim_rotation_value(player);

        // Reset for animation start
        item->setPos(deckCenter);
        item->setRotation(QRandomGenerator::global()->bounded(-8, 9));

        // Reset again for animation start (only pos and random rotation)
     //   item->setPos(deckCenter);
   //     item->setRotation(QRandomGenerator::global()->bounded(-8, 9));

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

        // Option #1
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
    /*
      // Option #2
      int player = 0;
    for (int i=0; i<52; i++) {
      QGraphicsItem *item = deck->get_card_item(i);

      if ((player == PLAYER_SOUTH) || (player == PLAYER_WEST))
        item->setZValue(114 - i);
      else
       item->setZValue(101 + i);
      if (++player > 3) player = 0;
    }
    */
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

    group->start(QAbstractAnimation::DeleteWhenStopped);

    QTimer::singleShot(700, this, [this]() {
      setAnimationUnlock();
    });
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


// ************************************[ 6- MATHEMATIC FUNCTIONS ] ********************************
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

//  if (deck->Style() == TIGULLIO_MODERN_DECK)
//    REFERENCE_WIDTH = 900;

  qreal baseScale = 1.0;

  // Calculate scale: clamp between 0.5 and 2.0
  qreal scale = static_cast<qreal>(viewWidth) / REFERENCE_WIDTH;
  scale = qBound(0.5, scale, 2.0);  // Clamps to 0.5–2.0 range

//  qDebug() << "scale: " << scale;

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


// **********************[ UNSORTED FUNCTIONS *****************************************************
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

        // Optionnel : on peut aussi utiliser legalSuit si tu veux garder une pré-filtration rapide
        // mais la vérification moteur est plus fiable et complète
        // if (legalSuit != SUIT_ALL && legalSuit != SUIT_NONE) {
        //     isLegal = isLegal && ((cardId / 13) == legalSuit);
        // }

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

            // Optionnel : afficher le motif de l'erreur pour debug
            // qDebug() << "Carte" << cardId << "invalide :" << engine->errorMessage(err);
        }
    }

    // Optionnel : rafraîchir la scène pour que les effets s'appliquent immédiatement
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
    for (int index : currentTrick) {  // ta liste actuelle des items du pli
        QGraphicsSvgItem *item = deck->get_card_item(index, true);
        TrickCardInfo info;
        info.cardId = item->data(0).toInt();  // ton cardId
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
qDebug() << "Nouvelle carte" << info.cardId << "reçue Z =" << newItem->zValue();
   //         scene->addItem(newItem);
            currentTrick.append(info.cardId);
        }
    }
}

void MainWindow::clearTricks()
{
  qDebug() << "clearTricks() CALLED " << currentTrick;;

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
     for (QAbstractButton *btn : deckGroup->buttons()) {
       btn->setEnabled(true);
   }});
}

void MainWindow::enableAllDecksButOne(int id)
{
   QTimer::singleShot(500, this, [this, id]() {
        for (QAbstractButton *btn : deckGroup->buttons()) {
          btn->setEnabled(true);
        }
        disableDeck(id);
   });
}

// Clear button
void MainWindow::on_pushButton_clicked()
{
  scene->clear();
}

void MainWindow::on_pushButton_2_clicked()
{
  static int s = 0;
  qDebug() << "Sounds: " << s;

  sounds->play(s);
  if (++s == 15) s = 0;
}

void MainWindow::on_pushButton_3_clicked()
{
  refresh_deck();
}

void MainWindow::on_pushButton_4_clicked()
{
  animateDeal();
//  sortPlayerHand();
//  update_cards_pos(false);
}


void MainWindow::on_pushButton_5_clicked()
{
  static int p = 0;

  showTurnArrow(p);
  if (++p > 4) p = 0;
}

void MainWindow::on_pushButton_6_clicked()
{
  static int trick = 0;
  animateCollectTrick(trick, true);

  if (++trick > 3) trick = 0;
}

void MainWindow::on_pushButton_8_clicked()
{
  showTurnIndicator();
}

void MainWindow::on_pushButton_9_clicked()
{
  static int pass = 0;

  static QList<int> passedCards[4];

qDebug() << "Direction: " << pass;

  passedCards[PLAYER_SOUTH].append(engine->Hand(PLAYER_SOUTH).at(0));
  passedCards[PLAYER_SOUTH].append(engine->Hand(PLAYER_SOUTH).at(1));
  passedCards[PLAYER_SOUTH].append(engine->Hand(PLAYER_SOUTH).at(2));

  passedCards[PLAYER_WEST].append(engine->Hand(PLAYER_WEST).at(0));
  passedCards[PLAYER_WEST].append(engine->Hand(PLAYER_WEST).at(1));
  passedCards[PLAYER_WEST].append(engine->Hand(PLAYER_WEST).at(2));

  passedCards[PLAYER_NORTH].append(engine->Hand(PLAYER_NORTH).at(0));
  passedCards[PLAYER_NORTH].append(engine->Hand(PLAYER_NORTH).at(1));
  passedCards[PLAYER_NORTH].append(engine->Hand(PLAYER_NORTH).at(2));

  passedCards[PLAYER_EAST].append(engine->Hand(PLAYER_EAST).at(0));
  passedCards[PLAYER_EAST].append(engine->Hand(PLAYER_EAST).at(1));
  passedCards[PLAYER_EAST].append(engine->Hand(PLAYER_EAST).at(2));

qDebug() << "Hand South: " << engine->Hand(PLAYER_SOUTH);
qDebug() << "Hand West: " << engine->Hand(PLAYER_WEST);
qDebug() << "Hand North: " << engine->Hand(PLAYER_NORTH);
qDebug() << "Hand East: " << engine->Hand(PLAYER_EAST);

qDebug() << "South passing: " << passedCards[PLAYER_SOUTH];
qDebug() << "West passing: " << passedCards[PLAYER_WEST];
qDebug() << "North passing: " << passedCards[PLAYER_NORTH];
qDebug() << "East passing: " << passedCards[PLAYER_EAST];


  animatePassCards(passedCards, pass);

  for (int p=0; p<4; p++)
    passedCards[p].clear();

  //completePassCards(passedCards, pass);

  if (++pass > 2) pass = 0;
}

void MainWindow::on_pushButton_10_clicked()
{
  static int ledSuit = 0;

  // Example calls
  //setCurrentSuit(ledSuit);  // Light icy blue
 //(ledSuit, QColor(255, 100, 50, 180));  // Warm orange-red
// disableIllegalCards(ledSuit, QColor(100, 150, 100, 180));  // Muted green
// disableIllegalCards(ledSuit, QColor(128, 128, 128, 180));  // Classic gray

 if (++ledSuit > 3)
   ledSuit = 0;
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

void MainWindow::on_opt_animations_clicked()
{
  bool enabled = ui->opt_animations->isChecked();

  config->set_config_file(CONFIG_ANIMATED_PLAY, enabled);
  setAnimationButtons(enabled);
}

void MainWindow::on_pushButton_score_clicked()
{
  bool checked = ui->pushButton_score->isChecked();
  m_scoreGroup->setVisible(checked);
}

