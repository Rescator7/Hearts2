#include "engine.h"
#include <QDebug>
#include <algorithm>  // for std::shuffle
#include <random>
#include <QRandomGenerator>
#include <QTimer>
#include <QMessageBox>
#include <QDir>

#include "define.h"
#include "cpus.h"
#include "deck.h"
#include "qpushbutton.h"

std::random_device rd;
std::mt19937 gen(rd());

Engine::Engine(QWidget* mainWin, QObject* parent)
    : QObject(parent), mainWindow(mainWin)
{
}

void Engine::Start()
{
  init_variables();

  QFile file(QDir::homePath() + SAVEDGAME_FILENAME);
  if (file.exists()) {
    if (load_game()) {
      emit sig_message(tr("Previous saved game has been loaded!"), MESSAGE_SYSTEM);
      file.remove();
    } else {
      // Remove any previous backup of saved game file.
      // Or file.rename() will fail, and we'll always start a game
      // with a corrupted saved game.
      QFile file_corrupted(QDir::homePath() + SAVEDGAME_CORRUPTED);
      if (file_corrupted.exists())
        file_corrupted.remove();

      // Make a backup of the current corrupted game. (for analyze)
      file.rename(QDir::homePath() + SAVEDGAME_CORRUPTED);
      emit sig_message(errorMessage(ERRCORRUPTED), MESSAGE_ERROR);
      start_new_game();
    }
  } else {
      start_new_game();
  }
}

int Engine::getRandomNameIndex() {
  return std::uniform_int_distribution<int>(1, MAX_PLAYERS_NAME - 1)(gen);
}

void Engine::set_variant(int variant, bool enabled)
{
  switch (variant) {
    case VARIANT_NEW_MOON:    variant_new_moon = enabled; break;
    case VARIANT_NO_DRAW:     variant_no_draw = enabled; break;
    case VARIANT_NO_TRICKS:   variant_no_tricks = enabled; break;
    case VARIANT_PERFECT_100: variant_perfect_100 = enabled; break;
    case VARIANT_OMNIBUS:     variant_omnibus = enabled; break;
    case VARIANT_QUEEN_SPADE: variant_queen_spade = enabled; break;
  }
//  qDebug() << "Set variant: " << variant << "enabled: " << enabled;
}

void Engine::generate_players_name()
{
  bool name_taken[MAX_PLAYERS_NAME] = {};

  name_taken[0] = true; // reserved for player south as "You"
  playersName[PLAYER_SOUTH] = tr("You");
  playersIndex[PLAYER_SOUTH] = 0;

  for (int p = 1; p < 4; p++) {
    int index = getRandomNameIndex();
    while (name_taken[index])
      index = getRandomNameIndex();
    name_taken[index] = true;
    playersName[p] = names[index];
    playersIndex[p] = index;
  }
}

void Engine::init_variables()
{
// clear the QList
  for (int p = 0; p < 4; p++) {
    playerHandsById[p].clear();
    cardsSelected[p].clear();
    passedCards[p].clear();
    hand_score[p] = 0;
}

// clear cards played
  for (int i = 0; i < DECK_SIZE; i++) {
    cardPlayed[i] = false;
  }

// clear the tricks pile
   currentTrick.clear();

// Set the leading suit
  currentSuit = CLUBS;

  cpt_played = 0;

  cards_left = DECK_SIZE;

  hearts_broken = false;

  jack_diamond_in_trick = false;
  queen_spade_in_trick = false;

  jack_diamond_owner = PLAYER_NOBODY;
  previous_winner = PLAYER_SOUTH;

  best_hand = 0;

  trick_value = 0;

  undoStack.available = false;
}

GAME_ERROR Engine::start_new_game()
{
  if (isBusy()) {
    return ERRLOCKED;
  }

  direction = PASS_LEFT;
  game_status = NEW_ROUND;   // Init vars will be called in Loop() for the NEW_ROUND
  game_score = 0;

  for (int p = 0; p < 4; p++) {
    hand_score[p] = 0;
    total_score[p] = 0;
  }

  generate_players_name();
  emit sig_message(tr("Starting a new game."), MESSAGE_SYSTEM);
  emit sig_new_players();
  emit sig_update_stat(0, STATS_GAME_STARTED);
  emit sig_update_scores_board(playersName, hand_score, total_score);
  LockedLoop();

  return NOERROR;
}

bool Engine::Undo()
{
  if (isBusy()) {
    emit sig_message(errorMessage(ERRLOCKED), MESSAGE_ERROR);
    return false;
  }

  if (undoStack.available) {
    popUndo();
    emit sig_message(tr("The cancellation was successful!"), MESSAGE_INFO);

    return true;
  }

  emit sig_message(tr("There is no undo available!"), MESSAGE_ERROR);

  return false;
}

void Engine::sort_players_hand()
{
  for (int player = 0; player < 4; player++) {
    // Step 1: Create indices 0 to 12
    QList<int> indices;
    for (int i = 0; i < playerHandsById[player].size(); i++) {
       indices.append(i);
    }

    // Step 2: Sort indices by card ID order
    if ((player == PLAYER_SOUTH) || (player == PLAYER_WEST)) {
      std::sort(indices.begin(), indices.end(), [this, player](int a, int b) {
      return playerHandsById[player][a] > playerHandsById[player][b]; });
    }
    else {
      std::sort(indices.begin(), indices.end(), [this, player](int a, int b) {
      return playerHandsById[player][a] < playerHandsById[player][b]; });
     }

    // Step 3: Rebuild BOTH hands in sorted order
    QList<int> sortedIds;

    for (int sortedIndex : indices) {
      sortedIds.append(playerHandsById[player][sortedIndex]);
    }

    // Step 4: Replace BOTH lists
    playerHandsById[player] = sortedIds;
  }
}

void Engine::shuffle_deck()
{
    QList<int> shuffled_cards;

    int player = 0;

    shuffled_cards.reserve(DECK_SIZE);

    // Fill with 0 to 51
    for (int i = 0; i < DECK_SIZE; ++i) {
      shuffled_cards.append(i);
    }

    // Shuffle using Qt's secure random generator
    std::shuffle(shuffled_cards.begin(), shuffled_cards.end(), *QRandomGenerator::global());

    for (int i = 0; i < DECK_SIZE; i++) {
      playerHandsById[player].append(shuffled_cards.at(i));
      if (++player > 3)
        player = 0;
    }

// We don't emit shuffle sound when booting the game either from a saved game or "new game".
    if (!firstTime) {
      emit sig_play_sound(SOUND_SHUFFLING_CARDS);
    }
}

void Engine::completePassCards()
{
    if (direction == PASS_HOLD) return;

    // Define who receives from whom (same mapping as animation)
    int receiver[4];
    switch (direction) {
        case PASS_LEFT:
            receiver[PLAYER_SOUTH] = PLAYER_WEST;
            receiver[PLAYER_WEST]  = PLAYER_NORTH;
            receiver[PLAYER_NORTH] = PLAYER_EAST;
            receiver[PLAYER_EAST]  = PLAYER_SOUTH;
            break;
        case PASS_RIGHT:
            receiver[PLAYER_SOUTH] = PLAYER_EAST;
            receiver[PLAYER_WEST]  = PLAYER_SOUTH;
            receiver[PLAYER_NORTH] = PLAYER_WEST;
            receiver[PLAYER_EAST]  = PLAYER_NORTH;
            break;
        case PASS_ACROSS:
            receiver[PLAYER_SOUTH] = PLAYER_NORTH;
            receiver[PLAYER_WEST]  = PLAYER_EAST;
            receiver[PLAYER_NORTH] = PLAYER_SOUTH;
            receiver[PLAYER_EAST]  = PLAYER_WEST;
            break;
        default:
            return;
    }

   int card1, card2, card3;
   for (int giver = 0; giver < 4; giver++) {
     card1 = passedCards[giver].at(0);
     card2 = passedCards[giver].at(1);
     card3 = passedCards[giver].at(2);
     playerHandsById[giver].removeOne(card1);
     playerHandsById[giver].removeOne(card2);
     playerHandsById[giver].removeOne(card3);
     playerHandsById[receiver[giver]].append(card1);
     playerHandsById[receiver[giver]].append(card2);
     playerHandsById[receiver[giver]].append(card3);
   }

     qDebug() << "Player S: " << playerHandsById[PLAYER_SOUTH].size();
     qDebug() << "Player N: " << playerHandsById[PLAYER_NORTH].size();
     qDebug() << "Player W: " << playerHandsById[PLAYER_WEST].size();
     qDebug() << "Player E: " << playerHandsById[PLAYER_EAST].size();

   sort_players_hand();
}

void Engine::completePassCards(const QList<int> Cards[4], DIRECTION d)
{
  for (int p = 0; p < 4; p++)
    passedCards[p] = Cards[p];

  direction = d;
  completePassCards();
}

void Engine::Loop()
{
  bool tram;
  PLAYER owner;
  int cardId;

  switch (game_status) {
     case NEW_ROUND:       qDebug() << "Engine: NEW ROUND";
                           locked = true;
                           init_variables();
                           emit sig_refresh_cards_played();
                           shuffle_deck();
                           emit sig_enableAllCards();
                           emit sig_clear_deck();
                           if (firstTime) {
                             firstTime = false;
                             game_status = SELECT_CARDS;
                           } else {
                             game_status = DEAL_CARDS;
                           }
                           LockedLoop();
                           break;
    case DEAL_CARDS:       qDebug() << "Engine: DEAL_CARDS";
                           emit sig_deal_cards();
                           break;
    case SELECT_CARDS:     qDebug() << "Engine: SELECT_CARDS direction " << direction;
                           sort_players_hand();          // sort after animated deal
                           emit sig_refresh_deck();

                           if (direction == PASS_HOLD) {
                             game_status = PLAY_TWO_CLUBS;
                             LockedLoop();
                           } else
                               emit sig_pass_to(direction);

                           busy = false;
                           emit sig_busy(busy);
                           locked = false;
                           break;
    case TRADE_CARDS:      qDebug() << "Engine: TRADE CARDS";
                           locked = true;
                           cpus_select_cards();
                           completePassCards();
                           emit sig_passed();
                           break;
    case PLAY_TWO_CLUBS :  qDebug() << "Engine: PLAY_TWO_CLUBS";
                           sort_players_hand();          // need to sort again after trading cards
                           emit sig_refresh_deck();
                           game_status = PLAY_A_CARD_1;
                           cpt_played = 1;
                           owner = Owner(CLUBS_TWO);
                           turn = owner;
                           best_hand = CLUBS_TWO;
                           best_hand_owner = PLAYER_SOUTH;
                           currentSuit = CLUBS;
                           if ((owner != PLAYER_SOUTH) && (owner != PLAYER_NOBODY)) {
                             playerHandsById[owner].removeAll(CLUBS_TWO);
                             cards_left--;
                             currentTrick.append(CLUBS_TWO);
                             emit sig_play_card(CLUBS_TWO, owner);
                             emit sig_card_played(CLUBS_TWO);
                           }
                           if (owner == PLAYER_SOUTH) {
                             locked = false;
                             busy = false;
                             emit sig_busy(busy);
                             emit sig_your_turn();
                           }
                           break;
     case PLAY_A_CARD_1 :
     case PLAY_A_CARD_2 :
     case PLAY_A_CARD_3 :
     case PLAY_A_CARD_4 : qDebug() << "Engine: PLAY_A_CARD_: " << game_status;
                          locked = true;
                          qDebug() << "Engine: currentSuit: " << currentSuit;
                          filterValidMoves(turn);
                          if ((turn != PLAYER_SOUTH) && (turn != PLAYER_NOBODY)) {
                          //   cardId = validHandsById[turn].at(0);
                             cardId = AI_get_cpu_move();
                             qDebug() << "Engine: Player: " << turn << "ValidHand: " << validHandsById[turn] << "cardId: " << cardId << "Full hand: " << playerHandsById[turn];
                             Play(cardId, turn);
                             qDebug() << "ENgine: Player: " << turn << "CardId removed from hand: " << cardId;
                           } else
                              if (turn == PLAYER_SOUTH) {
                                qDebug() << "Your turn, cards left: " << cards_left;
                                locked = false;
                                busy = false;
                                emit sig_busy(busy);
                                emit sig_your_turn();
                              }
                           qDebug() << "Engine: Card Left: " << cards_left;
                           break;
     case END_TURN:        qDebug() << "Engine: END_TURN";
                           currentSuit = SUIT_ALL;
                           previous_winner = turn;
                           turn = best_hand_owner;
                           tram = is_it_TRAM(turn);
                           if (tram) {
                             trick_value += calculate_tricks_from_tram();
                           };
                           if (trick_value) {
                             hand_score[best_hand_owner] += trick_value;
                             trick_value = 0;
                             emit sig_update_scores_board(playersName, hand_score, total_score);
                           }
                           if (jack_diamond_in_trick) {
                             jack_diamond_owner = best_hand_owner;
                             jack_diamond_in_trick = false;
                           }
                           if (queen_spade_in_trick) {
                             queen_spade_in_trick = false;
                             emit sig_update_stat(playersIndex[best_hand_owner], STATS_QUEEN_SPADE);
                           }
                           emit sig_collect_tricks(turn, tram);
                           currentTrick.clear();

                           break;
     case LOOP_TURN:   qDebug() << "Engine: LOOP_TURN";
                       qDebug() << "Status: " << game_status << "turn : " << turn << "cards_left: " << cards_left << "cpt_played: " << cpt_played;
                       qDebug() << "South: " << playerHandsById[PLAYER_SOUTH] << "sise:" << playerHandsById[PLAYER_SOUTH].size();
                       qDebug() << "Westh: " << playerHandsById[PLAYER_WEST] << "sise:" << playerHandsById[PLAYER_WEST].size();
                       qDebug() << "North: " << playerHandsById[PLAYER_NORTH] << "sise:" << playerHandsById[PLAYER_NORTH].size();
                       qDebug() << "East: " << playerHandsById[PLAYER_EAST] << "sise:" << playerHandsById[PLAYER_EAST].size();
  //                     qDebug() << "Pile: " << currentTrick;

                        cpt_played = 1;
                        if (cards_left > 0) {
                          game_status = PLAY_A_CARD_1;                       
                         } else {
                             game_status = END_ROUND;
                         }

                         LockedLoop();
                         break;
     case END_ROUND:     update_total_scores();
                         qDebug() << "Engine: END_ROUND " << game_score;
                         if (!is_game_over()) {
                           game_status = NEW_ROUND;
                           advance_direction();
                          } else {
                              game_status = GAME_OVER;
                         }
                         LockedLoop();
                         break;
     case GAME_OVER:     qDebug() << "Engine: GAME_OVER";
                         undoStack.available = false;
                         busy = false;
                         locked = false;
                         emit sig_busy(busy);
                         emit sig_play_sound(SOUND_GAME_OVER);
                         emit sig_update_stat(0, STATS_GAME_FINISHED);
                         sendGameResult();
                         for (int p = 0; p < 4; p++) {
                           // while new game will call init_variables, and this list will be cleared.
                           // This clear() prevent using the reveal button to show cards that weren't "played" before a TRAM.
                           playerHandsById[p].clear();
                         }
                         emit sig_clear_deck();
                         break;
  }
}

void Engine::LockedLoop()
{
  QTimer::singleShot(0, this, [this]() {
                            locked = true;   // lock on to avoid race condition
                            Loop();
                     });
}

void Engine::Step()
{
  if (game_status == GAME_OVER)
    return;

  if (isPlaying())
    advance_turn();

  game_status = static_cast<GAME_STATUS>(static_cast<int>(game_status) + 1);

  LockedLoop();
}

void Engine::advance_turn()
{
  switch(turn) {
    case PLAYER_SOUTH : turn = PLAYER_WEST; break;
    case PLAYER_WEST :  turn = PLAYER_NORTH; break;
    case PLAYER_NORTH : turn = PLAYER_EAST; break;
    case PLAYER_EAST :  turn = PLAYER_SOUTH; break;
    case PLAYER_COUNT :
    case PLAYER_NOBODY : qCritical() << "advance_turn invalid turn !";
  }

  qDebug() << "Engine:: advance_turn = " << turn;

  busy = true;
  emit sig_busy(busy);

  cpt_played++;
}

void Engine::advance_direction()
{
  QString mesg;

  switch(direction) {
    case PASS_LEFT:   direction = PASS_RIGHT;
                      mesg = tr("We now pass the cards to the person on the right.");
                      break;
    case PASS_RIGHT:  direction = PASS_ACROSS;
                      mesg = tr("We now pass the cards to the player opposite.");
                      break;
    case PASS_ACROSS: direction = PASS_HOLD;
                      mesg = tr("No card exchange this round.");
                      break;
    case PASS_HOLD:   direction = PASS_LEFT;
                      mesg = tr("We now pass the cards to the person on the left.");
                      break;
  }

  emit sig_message(mesg, MESSAGE_INFO);
}

void Engine::check_for_best_hand(PLAYER player, int cardId)
{
  SUIT cardSuit = (SUIT)(cardId / 13);
  if (currentSuit == SUIT_ALL) {
    best_hand = cardId;
    best_hand_owner = player;
    currentSuit = cardSuit;
  } else {
      if ((cardSuit == currentSuit) && (cardId > best_hand)) {
        best_hand = cardId;
        best_hand_owner = player;
      }
  }
}

void Engine::Play(int cardId, PLAYER player)
{
  // TODO: Maybe i could check if the move is valid for PLAYER_SOUTH ?
  // This would be required if "disableInvalidMove" is made as a settings option.

  busy = true;
  emit sig_busy(busy);

  if (player == PLAYER_SOUTH) {
    pushUndo();
  }

  playerHandsById[player].removeAll(cardId);
  cards_left--;

  cardPlayed[cardId] = true;
  emit sig_card_played(cardId);

  currentTrick.append(cardId);

  check_for_best_hand(player, cardId);

  // MainWindow already play / animate the move for PLAYER_SOUTH. (removing disabled cards effect too)
  if (player != PLAYER_SOUTH) {
    emit sig_play_card(cardId, turn);
  }

  if ((cardId / 13) == HEARTS) {
    trick_value++;
    if (!hearts_broken) {
      hearts_broken = true;
      emit sig_message(tr("Hearts has been broken!"), MESSAGE_INFO);
      emit sig_play_sound(SOUND_BREAKING_HEARTS);
    }
  } else
  if (cardId == SPADES_QUEEN) {
    trick_value += 13;
    queen_spade_in_trick = true;
    if (variant_queen_spade) {
      hearts_broken = true;
      emit sig_message(tr("Hearts has been broken!"), MESSAGE_INFO);
      emit sig_play_sound(SOUND_BREAKING_HEARTS);
    }
    emit sig_play_sound(SOUND_QUEEN_SPADE);
  } if (cardId == DIAMONDS_JACK) {
      jack_diamond_in_trick = true;
  }

  emit sig_refresh_deck();
}

int Engine::calculate_tricks_from_tram()
{
  int tricks = 0;
  for (int p = 0; p < 4; p++) {
    const QList<int> &hand = playerHandsById[p];
    for (int cardId : hand) {
       if (cardId == SPADES_QUEEN) {
         tricks += 13;
         queen_spade_in_trick = true;
       } else {
           if (cardId == DIAMONDS_JACK) {
             jack_diamond_in_trick = true;
           }  else {
                if ((cardId / 13) == HEARTS) {
                  tricks++;
                }
           }
       }
    }
  }
  return tricks;
}

bool Engine::getNewMoonChoice()
{
    QMessageBox msgBox(mainWindow);

    msgBox.setIcon(QMessageBox::Question);
    msgBox.setWindowTitle("🌕 Shoot the Moon ! 🌕");

    msgBox.setText("<font size='+2' color='#FFD700'><b>Congratulations!</b></font><br>"
                   "<font color='#FF69B4'>You have taken all the hearts!</font>");

    msgBox.setInformativeText(tr("What would you like to do?"));

    QPushButton *addButton = msgBox.addButton(tr("➕ 26 to opponents"), QMessageBox::AcceptRole);
    QPushButton *subButton = msgBox.addButton(tr("➖ 26 to myself"), QMessageBox::RejectRole);

    addButton->setStyleSheet("QPushButton { background-color: #FF4444; color: white; padding: 12px 18px; border-radius: 8px; font-weight: bold; }");
    subButton->setStyleSheet("QPushButton { background-color: #44AA44; color: white; padding: 12px 18px; border-radius: 8px; font-weight: bold; }");

    msgBox.exec();

    return (msgBox.clickedButton() == addButton);
}

void Engine::shoot_moon(int player)
{
  QString mesg;
  int yesNo = true, bonus;

  if ((player == PLAYER_SOUTH) && variant_new_moon && (total_score[player] >= 26)) {
    yesNo = getNewMoonChoice();
  }

  if (yesNo == false) {
     bonus = total_score[player] >= 26 ? 26 : total_score[player];
     mesg = (player == PLAYER_SOUTH ? tr("You") : playersName[player]) + tr(" substracted ") + QString::number(bonus) + tr(" pts to ") +
                             (player == PLAYER_SOUTH ? tr("your") : tr("his/her")) + tr(" score!");
    total_score[player] -= bonus;
  } else {
      mesg = (player == PLAYER_SOUTH ? tr("You") : playersName[player]) + tr(" added 26 pts to everyone's scores!");

      for (int p = 0; p < 4; p++) {
        if (p == player) continue;
        total_score[p] += 26;
      }
  }

  emit sig_message(mesg, MESSAGE_INFO);
}

void Engine::update_total_scores()
{
  QString mesg;

  int bonus;
  bool moon = false;

  for (int p = 0; p < 4; p++) {
    if ((hand_score[p] == 26) && ((jack_diamond_owner == p) || !variant_omnibus)) {
      if (p == PLAYER_SOUTH) {
        mesg = tr("You shoot the moon!");
      } else {
        mesg = tr("Player '") + playersName[p] + tr("' shoot the moon!");
      }
      emit sig_play_sound(SOUND_SHOOT_MOON);
      emit sig_message(mesg, MESSAGE_INFO);

      shoot_moon(p);
      moon = true;
      emit sig_update_stat(playersIndex[p], STATS_SHOOT_MOON);
    }
  }

  for (int p = 0; p < 4; p++) {
    // if someone moon we disable the bonus, except the PERFECT_100
    if (!moon) {
      total_score[p] += hand_score[p];

      if (variant_no_tricks && (hand_score[p] == 0)) {
        bonus = total_score[p] >= NO_TRICK_VALUE ? NO_TRICK_VALUE : total_score[p];

        if (bonus) {
          total_score[p] -= bonus;

          if (p == PLAYER_SOUTH) {
            mesg = tr("You receive the bonus: no tricks bonus ") + QString::number(bonus) + tr(" point(s)");
          }
          else {
            mesg = tr("Player '") + playersName[p] + tr("' receive the bonus: no tricks bonus ") + QString::number(bonus) + tr(" point(s)");
          }
          emit sig_update_stat(playersIndex[p], STATS_NO_TRICKS);
          emit sig_message(mesg, MESSAGE_INFO);
        }
      }

      if (variant_omnibus && (jack_diamond_owner == p)) {
        bonus = total_score[p] >= OMNIBUS_VALUE ? OMNIBUS_VALUE : total_score[p];

        if (bonus) {
          total_score[p] -= bonus;
          if (p == PLAYER_SOUTH) {
            mesg = tr("You receive the bonus: omnibus ") + QString::number(bonus) + tr(" point(s)");
          }
          else {
            mesg = tr("Player '") + playersName[p] + tr("' receive the bonus: omnibus ") + QString::number(bonus) + tr(" point(s)");
          }
          emit sig_update_stat(playersIndex[p], STATS_OMNIBUS);
          emit sig_message(mesg, MESSAGE_INFO);
        }
      }
    }

    if (variant_perfect_100 && (total_score[p] == GAME_OVER_SCORE)) {
      total_score[p] = PERFECT_100_VALUE;

      if (p == PLAYER_SOUTH) {
        mesg = tr("You got the perfect 100!\n[Info]: Your score has been set to 50.");
      } else {
          mesg = tr("Player '") + playersName[p] + tr("' got the perfect 100!\n[Info]: Player '") +
                                  playersName[p] + tr("' score has been set to 50.");
        }

      emit sig_message(mesg, MESSAGE_INFO);
      emit sig_update_stat(playersIndex[p], STATS_PERFECT_100);
      emit sig_play_sound(SOUND_PERFECT_100);
    }

    // check for a new higher game_score
    if (total_score[p] > game_score) {
      game_score = total_score[p];
    }
  }

  mesg = tr("New scores: '") + playersName[PLAYER_SOUTH] + ": " +
          QString::number(total_score[PLAYER_SOUTH]) + " (" + QString::number(hand_score[PLAYER_SOUTH]) + ")', '" +
                                   playersName[PLAYER_WEST] + ": " +
          QString::number(total_score[PLAYER_WEST]) + " (" + QString::number(hand_score[PLAYER_WEST]) + ")', '" +
                                   playersName[PLAYER_NORTH] + ": " +
          QString::number(total_score[PLAYER_NORTH]) + " (" + QString::number(hand_score[PLAYER_NORTH]) + ")', '" +
                                   playersName[PLAYER_EAST] + ": " +
          QString::number(total_score[PLAYER_EAST]) + " (" + QString::number(hand_score[PLAYER_EAST]) + ")'";

  emit sig_message(mesg, MESSAGE_INFO);

  for (int p = 0; p < 4; p++) {
    hand_score[p] = 0;
  }

  emit sig_update_stat(0, STATS_HANDS_PLAYED);
  emit sig_update_scores_board(playersName, hand_score, total_score);
}

PLAYER Engine::Owner(int cardId) const
{
  for (int p = 0; p < 4; ++p) {
    if (playerHandsById[p].contains(cardId)) {
      return static_cast<PLAYER>(p);
    }
  }
  return PLAYER_NOBODY;
}

GAME_ERROR Engine::validate_move(PLAYER player, int cardId)
{
  int suit = cardId / 13;
  int countPerSuit[4];

  if (!playerHandsById[player].contains(cardId)) {
    return ERRINVALID;
  }

  if ((cards_left == DECK_SIZE) && (cardId != CLUBS_TWO)) {
    return ERR2CLUBS;
  }

  if ((suit == HEARTS) && !hearts_broken && !can_break_hearts(player))
    return ERRHEARTS;

  if ((cardId == SPADES_QUEEN) && (cards_left > DECK_SIZE - 4) && !can_play_qs_first_hand(player))
    return ERRQUEEN;

  countSuits(player, countPerSuit[CLUBS], countPerSuit[SPADES], countPerSuit[DIAMONDS], countPerSuit[HEARTS]);

  if ((currentSuit != SUIT_ALL) && (suit != currentSuit) && countPerSuit[currentSuit])
    return ERRSUIT;

  return NOERROR;
}

bool Engine::can_break_hearts(PLAYER player)
{
  int clubs, spades, diamonds, hearts;

  if (player < PLAYER_SOUTH || player > PLAYER_EAST) {
     return false;
  }

  if (currentSuit == HEARTS)
    return true;

  countSuits(player, clubs, spades, diamonds, hearts);

  if (!clubs && !spades && !diamonds)
    return true;

  if (currentSuit == SUIT_ALL)
    return false;

  if (cards_left > DECK_SIZE - 4)
    return false;

  if (!clubs && (currentSuit == CLUBS))
    return true;

  if (!spades && (currentSuit == SPADES))
    return true;

  if (!diamonds && (currentSuit == DIAMONDS))
    return true;

  return false;
}

bool Engine::can_play_qs_first_hand(PLAYER player)
{
   int clubs, spades, diamonds, hearts;

   if (player < PLAYER_SOUTH || player > PLAYER_EAST) {
     return false;
   }

   countSuits(player, clubs, spades, diamonds, hearts);
   if (clubs) return false;
   if (diamonds) return false;
   if (spades > 1) return false;

   return true;
}

void Engine::countSuits(PLAYER player, int &clubs, int &spades, int &diamonds, int &hearts) const
{
    clubs = spades = diamonds = hearts = 0;

    if (player < PLAYER_SOUTH || player > PLAYER_EAST) {
      return;
    }

    const QList<int> &hand = playerHandsById[player];

    for (int cardId : hand) {
        int suit = cardId / 13;  // 0 = club, 1 = spade, 2 = diamond, 3 = heart

        switch (suit) {
            case CLUBS: ++clubs;       break;
            case SPADES: ++spades;     break;
            case DIAMONDS: ++diamonds; break;
            case HEARTS: ++hearts;     break;
        }
    }
}

void Engine::filterValidMoves(PLAYER player)
{
  const QList<int> &hand = playerHandsById[player];

  validHandsById[player].clear();
  for (int cardId : hand) {
    if (validate_move(player, cardId) == NOERROR)
      validHandsById[player].append(cardId);
  }
}

bool Engine::is_it_TRAM(PLAYER player)
{
  if (!detect_tram || (cards_left < 5)) {
    return false;
  }

  // tram if there ain't any penality or (bonus jack diamond / omnibus) left.
  if (!Owner(SPADES_QUEEN) && !leftInSuit(HEARTS)) {
    if (!variant_omnibus || !Owner(DIAMONDS_JACK)) {
      return true;
    }
  }

  int cardId, best_card[4] = {-1, -1, -1, -1};

  for (int p = 0; p < 4; p++) {
    if (p != player) {

      cardId = highestCardInSuitForPlayer((PLAYER)p, CLUBS);
      if ((cardId != INVALID_CARD) && (cardId > best_card[CLUBS])) {
        best_card[CLUBS] = cardId;
      }

      cardId = highestCardInSuitForPlayer((PLAYER)p, SPADES);
      if ((cardId != INVALID_CARD) && (cardId > best_card[SPADES])) {
        best_card[SPADES] = cardId;
      }

      cardId = highestCardInSuitForPlayer((PLAYER)p, DIAMONDS);
      if ((cardId != INVALID_CARD) && (cardId > best_card[DIAMONDS])) {
        best_card[DIAMONDS] = cardId;
      }

      cardId = highestCardInSuitForPlayer((PLAYER)p, HEARTS);
      if ((cardId != INVALID_CARD) && (cardId > best_card[HEARTS])) {
        best_card[HEARTS] = cardId;
      }
    }
  }

  if (countCardsInSuit(player, CLUBS) && (best_card[CLUBS] > lowestCardInSuitForPlayer(player, CLUBS))) return false;
  if (countCardsInSuit(player, SPADES) && (best_card[SPADES] > lowestCardInSuitForPlayer(player, SPADES))) return false;
  if (countCardsInSuit(player, DIAMONDS) && (best_card[DIAMONDS] > lowestCardInSuitForPlayer(player, DIAMONDS))) return false;
  if (countCardsInSuit(player, HEARTS) && (best_card[HEARTS] > lowestCardInSuitForPlayer(player, HEARTS))) return false;

  cards_left = 0;

  QString mesg;
  if (player == PLAYER_SOUTH)
    mesg = tr("You takes the rest!");
  else
    mesg = tr("Player '") + playersName[player] + tr("' takes the rest!");

  emit sig_message(mesg, MESSAGE_INFO);
  emit sig_play_sound(SOUND_TRAM);

  return true;
}

bool Engine::is_game_over()
{
  if (game_score < GAME_OVER_SCORE) {
    return false;
  }

  if (variant_no_draw && is_it_draw()) {
    return false;
  }

  return true;
}

bool Engine::isCardPlayed(int cardId)
{
  if ((cardId < 0) || (cardId >= DECK_SIZE)) {
    return false;
  }

  return cardPlayed[cardId];
}

bool Engine::is_it_draw()
{
  int lowest = total_score[PLAYER_SOUTH];

  int cpt = 0;
  for (int i = 1; i < 4; i++) {
     if (total_score[i] < lowest) {
       lowest = total_score[i];
       cpt = 0;
     }
     else {
       if (total_score[i] == lowest)
         cpt++;
     }
  }

  return cpt;
}

void Engine::sendGameResult()
{
  QString mesg, result;

  int lowest = 0;

  result = is_it_draw() ? tr("Drew !") : tr("Won !");

  for (int i =0 ; i < 4; i++) {
    int cpt = 0;
    for (int i2 = 0; i2 < 4; i2++)
      if (total_score[i] > total_score[i2])
        cpt++;

    switch(cpt) {
      case 0 : emit sig_update_stat(playersIndex[i], STATS_FIRST_PLACE);
               lowest = total_score[i];
               break;
      case 1 : emit sig_update_stat(playersIndex[i], STATS_SECOND_PLACE);
               break;
      case 2 : emit sig_update_stat(playersIndex[i], STATS_THIRD_PLACE);
               break;
      case 3 : emit sig_update_stat(playersIndex[i], STATS_FOURTH_PLACE);
               break;
    }
    emit sig_update_stat_score(playersIndex[i], total_score[i]);
  }

  mesg = tr("GAME OVER!\n[Info]: Player '")  + tr("You") + "': " +
         QString::number(total_score[PLAYER_SOUTH]) + tr(" point(s) ") + (total_score[PLAYER_SOUTH] == lowest ? result : "") +
        tr("\n[Info]: Player '")                    + playersName[PLAYER_WEST] + "': " +
         QString::number(total_score[PLAYER_WEST]) + tr(" point(s) ") + (total_score[PLAYER_WEST] == lowest ? result : "") +
        tr("\n[Info]: Player '")                    + playersName[PLAYER_NORTH] + "': " +
         QString::number(total_score[PLAYER_NORTH]) + tr(" point(s) ") + (total_score[PLAYER_NORTH] == lowest ? result : "") +
        tr("\n[Info]: Player '")                    + playersName[PLAYER_EAST] + "': " +
         QString::number(total_score[PLAYER_EAST]) + tr(" point(s) ") + (total_score[PLAYER_EAST] == lowest ? result : "");

  emit sig_message(mesg, MESSAGE_INFO);

  QString formatedMesg = "<h2 style='color:#f38ba8; text-align:center;'>GAME OVER!</h2><br>";

  for (int i = 0; i < 4; i++) {
    bool isWinner = (total_score[i] == lowest);
    if (isWinner) {
        formatedMesg += "<span style='color:#a6e3a1; font-weight:bold;'> ";
    }
    formatedMesg += tr("Player '");
    formatedMesg += (i == 0) ? tr("You") : QString(playersName[i]);
    formatedMesg += QString("': ") + QString::number(total_score[i]) + tr(" point(s)");
    if (isWinner) {
        formatedMesg += " " + result + " 🏆</span>";
    }
    if (i < 3) {
        formatedMesg += "<br>";
    }
  }

  QMessageBox msgBox(mainWindow);  // ou mainWindow si dans une autre classe
  msgBox.setWindowTitle(tr("Game result"));
  msgBox.setTextFormat(Qt::RichText);
  msgBox.setText(formatedMesg);
  msgBox.setIcon(QMessageBox::NoIcon);
  msgBox.setStyleSheet(QStringLiteral(
    "QMessageBox {"
    "   background-color: #1e1e2e;"
    "   color: #cdd6f4;"
    "   font-family: 'Segoe UI', Arial;"
    "   font-size: 14px;"
    "}"
    "QLabel {"
    "   color: #cdd6f4;"
    "   min-width: 380px;"
    "   text-align: center;"
    "}"
    "QPushButton {"
    "   background-color: #89b4fa;"  // bleu clair pour OK
    "   color: #1e1e2e;"
    "   border: 1px solid #89b4fa;"
    "   padding: 8px 24px;"
    "   border-radius: 6px;"
    "   font-size: 13px;"
    "   font-weight: bold;"
    "}"
    "QPushButton:hover {"
    "   background-color: #74c7ec;"
    "}" ));

  msgBox.setStandardButtons(QMessageBox::Ok);
  msgBox.exec();
}

int Engine::lowestCardInSuitForPlayer(PLAYER player, SUIT suit) const
{
  if (!isValid(player) || !isValid(suit))
    return INVALID_CARD;

  int lowest = 52;

  const QList<int> &hand = playerHandsById[player];

  for (int cardId : hand) {
    int cardSuit = cardId / 13;
    if ((cardSuit == suit) && cardId < lowest) {
      lowest = cardId;
    }
  }

  if (lowest == 52) {
    return INVALID_CARD;
  }

  return lowest;
}

int Engine::leftInSuit(SUIT suit) const
{
  if (!isValid(suit)) {
    return 0;
  }

  int count = 0;
  for (int p = 0; p < 4; p++) {
    count += countCardsInSuit((PLAYER)p, suit);
  }

  return count;
}

int Engine::highestCardInSuitForPlayer(PLAYER player, SUIT suit) const
{
  if (!isValid(player) || !isValid(suit))
    return INVALID_CARD;

  int highest = -1;

  const QList<int> &hand = playerHandsById[player];

  for (int cardId : hand) {
    int cardSuit = cardId / 13;
    if ((cardSuit == suit) && cardId > highest) {
      highest = cardId;
    }
  }

  if (highest == -1) {
    return INVALID_CARD;
  }

  return highest;
}

void Engine::set_passedFromSouth(QList<int> &cards)
{
  locked = true;
  passedCards[PLAYER_SOUTH] = cards;

  qDebug() << "selection size: " << passedCards[PLAYER_SOUTH].size();
}

int Engine::get_player_card(PLAYER player, int handIndex)
{
  if (!isValid(player))
    return INVALID_CARD;

  if ((handIndex < 0) || (handIndex >= playerHandsById[player].size()))
    return INVALID_CARD;

  return playerHandsById[player].at(handIndex);
}

int Engine::handSize(PLAYER player)
{
  if (!isValid(player))
    return 0;

  return playerHandsById[player].size();
}

int Engine::countCardsInSuit(PLAYER player, SUIT suit) const
{
  if (!isValid(player) || !isValid(suit))
    return 0;

  const QList<int> &hand = playerHandsById[player];
  int count = 0;

  for (int cardId : hand) {
    int cardSuit = cardId / 13;  // 0=club, 1=spade, 2=diamond, 3=heart
    if (cardSuit == suit) {
      count++;
    }
  }

  return count;
}

bool Engine::save_game()
{
  if (game_status == GAME_OVER) {
    return false;
  }

  if ((game_score == 0) && (cards_left == DECK_SIZE)) {
    return false;
  }

  QFile file(QDir::homePath() + SAVEDGAME_FILENAME);

  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
     return false;
  }

  QTextStream out(&file);

  out << playersIndex[PLAYER_SOUTH] << " " << playersIndex[PLAYER_WEST] << " "
      << playersIndex[PLAYER_NORTH] << " " << playersIndex[PLAYER_EAST] << "\n";

  out << turn            << " " << QString::number(currentTrick.size()) << " "
      << best_hand_owner << " " << jack_diamond_owner << " "
      << direction       << " " << currentSuit << " " << "\n";

  out << hearts_broken  << " " << trick_value << " "
      << best_hand      << " " << (isPlaying() ? "1" : "0") << "\n";

  out << hand_score[PLAYER_SOUTH] << " "
      << hand_score[PLAYER_WEST]  << " "
      << hand_score[PLAYER_NORTH] << " "
      << hand_score[PLAYER_EAST]  << "\n";

  out << total_score[PLAYER_SOUTH] << " "
      << total_score[PLAYER_WEST]  << " "
      << total_score[PLAYER_NORTH] << " "
      << total_score[PLAYER_EAST]  << "\n";

  for (int cardId : currentTrick) {
    out << QString::number(cardId) << " ";
  }

  int fill;
  fill = 4 - currentTrick.size();
  for (int i = 0; i < fill; i++) {
    out << QString::number(INVALID_CARD) << " ";
  }
  out << "\n";

  for (int p = 0; p < 4; p++) {
     QList<int> hand = playerHandsById[p];

     for (int cardId : hand) {
       out << QString::number(cardId) << " ";
     }

     fill = 13 - hand.size();
     for (int i = 0; i < fill; i++) {
       out << QString::number(INVALID_CARD) << " ";
     }

     out << "\n";
  }

  file.close();

  return true;
}

bool Engine::load_game()
{
  QFile file(QDir::homePath() + SAVEDGAME_FILENAME);

  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return false;
  }

  int cpt = 0;
  int trickPileSize = 0;
  bool card_found[DECK_SIZE] = {};

  game_score = 0;
  for (int i = 0; i < DECK_SIZE; i++) {
    cardPlayed[i] = true;
  }

  while (!file.atEnd()) {
    int value;
    QString line = file.readLine();
    cpt++;

    switch (cpt) {
       // Line #1: Data = 4 x players index (0 = You)
       case 1: for (int p = 0; p < 4; p++) {
                 value = line.section(' ', p, p).toInt();
                 if ((value < 0) || (value >= MAX_PLAYERS_NAME)) {
                   return false;
                 }
                 qDebug() << "Engine: load_game -> case 1: " << value;
                 playersIndex[p] = value;
                 playersName[p] = names[value];
               }
               // check for duplicate.
               for (int i = 0; i < 4; ++i)
                 for (int j = i + 1; j < 4; ++j)
                   if (playersIndex[i] == playersIndex[j])
                     return false;
               break;

       // Line #2: Data = Turn, Trick Pile size, Best Hand owner, Jack of diamond owner, Direction, current suit
       case 2: for (int d = 0; d < 6; d++) {
                 value = line.section(' ', d, d).toInt();
                   qDebug() << "Engine: load_game -> case 2: " << value << " d: " << d;
                   switch (d) {
                      case 0 : if ((value < 0) || (value > 3)) return false;
                               turn = (PLAYER)value; break;
                      case 1 : if ((value < 0) || (value > 3)) return false;
                               trickPileSize = value; break;
                      case 2 : if ((value < -1) || (value > 3)) return false;
                               best_hand_owner = (PLAYER)value; break;
                      case 3 : if ((value < -1) || (value > 3)) return false;
                               jack_diamond_owner = (PLAYER)value; break;
                      case 4 : if ((value < 0) || (value > 3)) return false;
                               direction = (DIRECTION)value; break;
                      case 5 : if ((value < 0) || (value == SUIT_COUNT) || (value > SUIT_ALL)) return false;
                               currentSuit = (SUIT)value; break;
                   }
               }
               break;

        // Line #3: Data = Hearts broken, trick_value, best_hand, is playing
        case 3: for (int i = 0; i < 4; i++) {
                   value = line.section(' ', i, i).toInt();

                   qDebug() << "Engine: load_game -> case 3: " << value;
                   switch (i) {
                      case 0 : if ((value < 0) || (value > 1))
                                 return false;
                               hearts_broken = value;
                               break;
                      case 1 : if ((value < 0) || (value > 26))
                                 return false;
                               trick_value = value;
                               break;
                      case 2 : if ((value < CLUBS_TWO) || (value >= DECK_SIZE))
                                 return false;
                               best_hand = value;
                               break;
                      case 3 : if ((value < 0) || (value > 1))
                                 return false;
                               if (value == 0) {
                                 game_status = SELECT_CARDS;
                               } else {
                                     switch (trickPileSize) {
                                       case 1: game_status = PLAY_A_CARD_2;
                                               break;
                                       case 2: game_status = PLAY_A_CARD_3;
                                               break;
                                       case 3: game_status = PLAY_A_CARD_4;
                                               break;
                                       default:game_status = PLAY_A_CARD_1;
                                               break;
                                     }
                                     // We'll only know later if PLAYER_SOUTH hold the two of clubs, then game_status should be play_two_clubs
                                 }
                               break;
                    }
                }
               break;

        // Line #4: Data = Players hand scores
        case 4: for (int p = 0; p < 4; p++) {
                  value = line.section(' ', p, p).toInt();
                  qDebug() << "Engine: load_game -> case 4: " << value;

                   if ((value < 0) || (value > 26))
                     return false;
                   hand_score[p] = value;
                }
               break;

        // Line #5: Data = Players total scores
        case 5: for (int p = 0; p < 4; p++) {
                  value = line.section(' ', p, p).toInt();
                  qDebug() << "Engine: load_game -> case 5: " << value;

                  if (value < 0) {
                     return false;
                  }
                  if ((value >= GAME_OVER_SCORE) && !variant_no_draw)
                    return false;
                  total_score[p] = value;
                  if (value > game_score) {
                    game_score = value;
                  }
                 }
                break;

        // Line #6: Data = Trick Pile
        case 6: for (int i = 0; i < 4; i++) {
                  value = line.section(' ', i, i).toInt();
                  qDebug() << "Engine: load_game -> case 6: " << value;

                  if (value == INVALID_CARD)
                    break;

                  if (card_found[value]) return false;

                  if (value == DIAMONDS_JACK)
                    jack_diamond_in_trick = true;

                  card_found[value] = true;

                  if (value == INVALID_CARD) break;
                  if ((value < CLUBS_TWO) || (value >= DECK_SIZE)) {
                    return false;
                  }
                  currentTrick.append(value);
                }
                break;

        // Line #7 - #10: Data = 4 x Players hand.
        case 7:
        case 8:
        case 9:
        case 10: for (int i = 0; i < 13; i++) {
                  value = line.section(' ', i, i).toInt();
                  qDebug() << "Engine: load_game -> case " << cpt << ": " << value;

                  if (value == INVALID_CARD)
                    break;

                  if (card_found[value]) {
                    return false;
                  }

                  // Now we can be more precise about is_playing is true.
                  if ((value == CLUBS_TWO) && (game_status != SELECT_CARDS)) {
                    game_status = PLAY_TWO_CLUBS;
                  }

                  if ((value < CLUBS_TWO) || (value >= DECK_SIZE))
                    return false;

                  card_found[value] = true;
                  cardPlayed[value] = false;
                  playerHandsById[cpt - 7].append(value);
                }
                 break;

        default: break;
    }
  }

  if (trickPileSize) {}; // avoid compiler warning

  if (cpt != 10) return false;

  cards_left = 0;
  for (int p = 0; p < 4; p++) {
    cards_left += playerHandsById[p].size();
  }

qDebug() << "Engine: load_game -> Cards Left: " << cards_left;
qDebug() << "Engine: load_game -> SOUTH" << playerHandsById[PLAYER_SOUTH];
qDebug() << "Engine: load_game -> WEST" << playerHandsById[PLAYER_WEST];
qDebug() << "Engine: load_game -> NORTH" << playerHandsById[PLAYER_NORTH];
qDebug() << "Engine: load_game -> EAST" << playerHandsById[PLAYER_EAST];
qDebug() << "Engine: load_game -> trickPile" << trickPileSize;

  if ((game_status == SELECT_CARDS) && (cards_left != DECK_SIZE))
    return false;

qDebug() << "Engine: load_game -> Step select";

  if (isPlaying() && (cards_left < 1))
    return false;

  if (playersIndex[PLAYER_SOUTH] != 0)
    return false;

  int s = playerHandsById[PLAYER_SOUTH].size();
  for (int p = 1; p < 4; p++) {
    int diff = s - playerHandsById[p].size();
    if ((diff < -1) || (diff > 1))
      return false;
  }

  firstTime = false;

  emit sig_refresh_cards_played();
  emit sig_setTrickPile(currentTrick);

qDebug() << "Engine: load_game -> trickPile: " << currentTrick;

  emit sig_refresh_deck();

  emit sig_new_players();
  emit sig_update_scores_board(playersName, hand_score, total_score);

qDebug() << "Engine: load_game -> game_status = " << game_status;

  LockedLoop();

  return true;
}

void Engine::pushUndo()
{
  undoStack.available = true;
  for (int i = 0; i < 4; i++) {
    undoStack.playerHandsById[i] = playerHandsById[i];
    undoStack.hand_score[i] = hand_score[i];
    undoStack.total_score[i] = total_score[i];
  }
  for (int i = 0; i < DECK_SIZE; i++) {
    undoStack.cardPlayed[i] = cardPlayed[i];
  }
  undoStack.currentTrick = currentTrick;
  undoStack.hearts_broken = hearts_broken;
  undoStack.jack_diamond_in_trick = jack_diamond_in_trick;
  undoStack.queen_spade_in_trick = queen_spade_in_trick;
  undoStack.cpt_played = cpt_played;
  undoStack.cards_left = cards_left;
  undoStack.game_score = game_score;
  undoStack.best_hand = best_hand;
  undoStack.trick_value = trick_value;
  undoStack.direction = direction;
  undoStack.best_hand_owner = best_hand_owner;
  undoStack.jack_diamond_owner = jack_diamond_owner;
  undoStack.turn = turn;
  undoStack.previous_winner = previous_winner;
  undoStack.game_status = game_status;
  undoStack.currentSuit = currentSuit;
  undoStack.busy = busy;
}

void Engine::popUndo()
{
  undoStack.available = false;
  for (int i = 0; i < 4; i++) {
    playerHandsById[i] = undoStack.playerHandsById[i];
    hand_score[i] = undoStack.hand_score[i];
    total_score[i] = undoStack.total_score[i];
  }
  for (int i = 0; i < DECK_SIZE; i++) {
    cardPlayed[i] = undoStack.cardPlayed[i];
  }
  currentTrick = undoStack.currentTrick;
  hearts_broken = undoStack.hearts_broken;
  jack_diamond_in_trick = undoStack.jack_diamond_in_trick;
  queen_spade_in_trick = undoStack.queen_spade_in_trick;
  cpt_played = undoStack.cpt_played;
  cards_left = undoStack.cards_left;
  game_score = undoStack.game_score;
  best_hand = undoStack.best_hand;
  trick_value = undoStack.trick_value;
  direction = undoStack.direction;
  best_hand_owner = undoStack.best_hand_owner;
  jack_diamond_owner = undoStack.jack_diamond_owner;
  turn = undoStack.turn;
  previous_winner = undoStack.previous_winner;
  game_status = undoStack.game_status;
  currentSuit = undoStack.currentSuit;
  busy = undoStack.busy;

  emit sig_busy(busy);
  emit sig_refresh_cards_played();
  emit sig_setTrickPile(currentTrick);
  emit sig_refresh_deck();
  emit sig_update_scores_board(playersName, hand_score, total_score);

  LockedLoop();
}

QString Engine::errorMessage(GAME_ERROR err)
{
    switch (err) {
        case NOERROR:      return "";
        case ERR2CLUBS:    return tr("You must play the 2 of Clubs on the first trick!");
        case ERRHEARTS:    return tr("Hearts are not broken yet!");
        case ERRQUEEN:     return tr("The Queen of Spades is not allowed on the first trick!");
        case ERRSUIT:      return tr("You must follow the suit led!");
        case ERRINVALID:   return tr("Invalid card or not in your hand.");
        case ERRLOCKED:    return tr("The game engine is busy, please try again when it's your turn to play.");
        case ERRCORRUPTED: return tr("The saved game file is corrupted! Deleted!");
        default:           return tr("Unknown error.");
    }
}

// *****************************************************************************************************************************************************
// **************************************************************** [ AI Section ] *********************************************************************
// *****************************************************************************************************************************************************
bool Engine::is_moon_an_option()
{
  if (!(AI_CPU_flags[turn] & AI_flags_try_moon))
    return false;

  if ((turn != PLAYER_SOUTH) && (hand_score[PLAYER_SOUTH] || (variant_omnibus && (jack_diamond_owner == PLAYER_SOUTH)))) return false;
  if ((turn != PLAYER_WEST) && (hand_score[PLAYER_WEST] || (variant_omnibus && (jack_diamond_owner == PLAYER_WEST)))) return false;
  if ((turn != PLAYER_NORTH) && (hand_score[PLAYER_NORTH] || (variant_omnibus && (jack_diamond_owner == PLAYER_NORTH)))) return false;
  if ((turn != PLAYER_EAST) && (hand_score[PLAYER_EAST] || (variant_omnibus && (jack_diamond_owner == PLAYER_EAST)))) return false;

  if (countCardsInSuit(turn, HEARTS) && (Owner(HEARTS_ACE) != turn) && !cardPlayed[HEARTS_ACE])
    return false;

  return true;
}

int Engine::eval_card_strength(PLAYER player, DECK_INDEX cardId)
{
  SUIT suit = (SUIT)(cardId / 13);

  if (!countCardsInSuit(player, suit))
   return 0;

  DECK_INDEX first, last;
  switch (suit) {
    case CLUBS:    first = CLUBS_TWO;
                   last = CLUBS_ACE;
                   break;
    case SPADES:   first = SPADES_TWO;
                   last = SPADES_ACE;
                   break;
    case DIAMONDS: first = DIAMONDS_TWO;
                   last = DIAMONDS_ACE;
                   break;
    case HEARTS:   first = HEARTS_TWO;
                   last = HEARTS_ACE;
                   break;
    case SUIT_ALL:
    case SUIT_COUNT: return 0;
                     break;
  }

  int cpt = 0;
  for (int i = first; i <= last; i++) {
     if (!cardPlayed[i] && (Owner(i) != player)) {
       if (cardId > i)
         cpt++;
     }
  }

 int value = (cpt / std::max(1, leftInSuit(suit))) * 100;

 return value;
}

int Engine::AI_eval_lead_hearts(DECK_INDEX cardId)
{
  if (is_moon_an_option()) {
    if ((cardId == SPADES_QUEEN) || (variant_omnibus && (cardId == DIAMONDS_JACK)))
      return -50;
    else {
       if (cardId > best_hand)
         return -cardId + 30;
       else
         return -30;
    }
  }
  else {
     if (cardId < best_hand)
       return cardId - best_hand + 30;
     else
       return -30;
  }
}

int Engine::AI_eval_lead_spade(DECK_INDEX cardId)
{
  bool spades_queen_table = currentTrick.indexOf(SPADES_QUEEN) != -1;
  bool spades_king_table  = currentTrick.indexOf(SPADES_KING) != -1;
  bool spades_ace_table   = currentTrick.indexOf(SPADES_ACE) != -1;

 // the lead is spade and the ace/king is in the trick? play the queen otherwise don't.
 if (cardId == SPADES_QUEEN) {
   if ((spades_ace_table || spades_king_table) && !is_moon_an_option())
     return 104;
   else
     return -104;
 }

 // avoid giving the jack of diamond
  if (variant_omnibus && (cardId == DIAMONDS_JACK))
    return -62;

  // last to talk, the queen is not in the trick.. throw your ace/king.
   if (!is_moon_an_option()) {
     if ((game_status == PLAY_A_CARD_4) && !spades_queen_table && ((cardId == SPADES_ACE) || (cardId == SPADES_KING)))
       return 40;
   }

  // if the ace is in the trick throw away our king
  if ((cardId == SPADES_KING) && spades_ace_table && !is_moon_an_option())
    return 75;

  if ((cardId == SPADES_ACE) || (cardId == SPADES_KING)) {
    if (game_status == PLAY_A_CARD_4) {
      return 75 + cardId;
    }
    if (!cardPlayed[SPADES_QUEEN] && (Owner(SPADES_QUEEN) != turn) && !is_moon_an_option()) {
      return - 140;
    }
  }

  return -cardId;
}

int Engine::AI_eval_lead_diamond(DECK_INDEX cardId)
{
  bool spades_queen_table = currentTrick.indexOf(SPADES_QUEEN) != -1;
  bool diamonds_jack_table = currentTrick.indexOf(DIAMONDS_JACK) != -1;
  bool diamonds_queen_table = currentTrick.indexOf(DIAMONDS_QUEEN) != -1;
  bool diamonds_king_table = currentTrick.indexOf(DIAMONDS_KING) != -1;
  bool diamonds_ace_table = currentTrick.indexOf(DIAMONDS_ACE) != -1;

  if (variant_omnibus) {
  // If the jack of diamond is on the table, try to grab it with the ace/king/queen. (if it's safe).
    if (((cardId == DIAMONDS_ACE) || (cardId == DIAMONDS_KING) || (cardId == DIAMONDS_QUEEN)) && diamonds_jack_table) {
      if (spades_queen_table)
        return -65;
      else
      if (cardPlayed[SPADES_QUEEN] || (Owner(SPADES_QUEEN) == turn) ||
         (game_status == PLAY_A_CARD_4) || is_moon_an_option())
        return 25 + (cardId % 13);
     }

  // will we win this hand?
     if (cardId == DIAMONDS_JACK) {
       if (((cardPlayed[SPADES_QUEEN] && !spades_queen_table) || (Owner(SPADES_QUEEN) == turn)) &&
           ((cardPlayed[DIAMONDS_ACE] && !diamonds_ace_table) || (Owner(DIAMONDS_ACE) == turn)) &&
           ((cardPlayed[DIAMONDS_KING] && !diamonds_king_table) || (Owner(DIAMONDS_KING) == turn)) &&
           ((cardPlayed[DIAMONDS_QUEEN] && !diamonds_queen_table) || (Owner(DIAMONDS_QUEEN) == turn)))
         return  62;

  // You are the last one to play in the trick, try to use your Jack of diamond:
  // if it the strongest card of the trick.
       if ((game_status == PLAY_A_CARD_4) && !spades_queen_table &&
                                         !diamonds_ace_table &&
                                         !diamonds_king_table &&
                                         !diamonds_queen_table)
         return 30;

       // don't play the jack of diamond
       return -65;
     }
  }

  return 0;
}

int Engine::AI_eval_lead_freesuit(DECK_INDEX cardId)
{
  SUIT suit = (SUIT)(cardId / 13);

  // try to catch someone with the queen of spade.
  if (cardId == SPADES_QUEEN) {
    int diff_spade = leftInSuit(SPADES) - countCardsInSuit(turn, SPADES);

    if ((AI_CPU_flags[turn] & AI_flags_count_spade) && !is_moon_an_option()) {
      if ((diff_spade == 1) && (Owner(SPADES_ACE) != turn) && !cardPlayed[SPADES_ACE])
        return 100;

      if ((diff_spade == 1) && (Owner(SPADES_KING) != turn) && !cardPlayed[SPADES_KING])
        return 101;

      if ((diff_spade == 2) && (Owner(SPADES_ACE) != turn) && (Owner(SPADES_KING) != turn) &&
                               !cardPlayed[SPADES_ACE] && !cardPlayed[SPADES_KING])
        return 102;
    }

    // don't lead the queen of spade
    if (!is_moon_an_option())
      return -100;
  }

  // don't lead in a suit where there is no cards left beside ours.
  if (!is_moon_an_option()) {
    if (leftInSuit(suit) == countCardsInSuit(turn, suit))
      return -80;
  }

  // avoid leading the ace/king of spade, if the queen hasn't been played and we don't own it
  // otherwise, let's try to elimitate some spade.
  if (((cardId == SPADES_ACE) || (cardId == SPADES_KING)) && !cardPlayed[SPADES_QUEEN]) {
    if ((Owner(SPADES_QUEEN) == turn) && ((countCardsInSuit(turn, SPADES) > 4) || is_moon_an_option()))
      return 70;
    else
      return -90;
  }

 // avoid leading in spade if the queen of spade is not played yet and the ace/king/queen spade is in your hand,
  // and you got less than 5 spades... otherwise lead in spade.
  if (suit == SPADES) {
    if (!cardPlayed[SPADES_QUEEN]) {
      if (((Owner(SPADES_QUEEN) == turn) || (Owner(SPADES_ACE) == turn) || (Owner(SPADES_KING) == turn)) &&
          (countCardsInSuit(turn, SPADES) < 4))
        return -70;
      else
        return 60;
    }
  }

  // play the jack of diamond (omnibus) only if your sure to wins the trick
  // but, not the queen of spade.
  if (variant_omnibus && (cardId == DIAMONDS_JACK)) {
    if ((cardPlayed[DIAMONDS_ACE] || (Owner(DIAMONDS_ACE) == turn)) &&
        (cardPlayed[DIAMONDS_KING] || (Owner(DIAMONDS_KING) == turn)) &&
        (cardPlayed[DIAMONDS_QUEEN] || (Owner(DIAMONDS_QUEEN) == turn))
     && (cardPlayed[SPADES_QUEEN] || (Owner(SPADES_QUEEN) == turn)))
      return 64;
    else
      return -64;
  }

  // try to catch the jack of diamond with a/k/q if the queen of spade has been played.
  if (variant_omnibus && (cardPlayed[SPADES_QUEEN] || (Owner(SPADES_QUEEN) == turn)) &&
                 !cardPlayed[DIAMONDS_JACK] && (Owner(DIAMONDS_JACK) != turn) &&
                ((cardId == DIAMONDS_ACE) || (cardId == DIAMONDS_KING) || (cardId == DIAMONDS_QUEEN)))
    return 63;

  // the stronger that card is, the worst it is to play.
  if (is_moon_an_option())
    return eval_card_strength(turn, cardId);
  else
    return -eval_card_strength(turn, cardId);
}

int Engine::AI_get_cpu_move()
{
  int eval[13] = {-32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768};

  for (int i = 0; i < validHandsById[turn].size(); i++) {
     DECK_INDEX cardId = (DECK_INDEX)validHandsById[turn].at(i);

     switch (currentSuit) {
       case SUIT_ALL:   eval[i] = AI_eval_lead_freesuit(cardId);
       qDebug() << "Free card: " << cardId << " eval: " << eval[i];
                        break;

       case SPADES:     eval[i] = AI_eval_lead_spade(cardId);
       qDebug() << "Spade card: " << cardId << " eval: " << eval[i];

                        break;

       case DIAMONDS:   eval[i] = AI_eval_lead_diamond(cardId);
                        break;

       case HEARTS:     eval[i] = AI_eval_lead_hearts(cardId);
                        break;
       case CLUBS:
       case SUIT_COUNT: break;
      }

      if ((currentSuit != SUIT_ALL) && (currentSuit != SPADES)) {
        // give away the queen of spade
        if (!is_moon_an_option()) {
          if (cardId == SPADES_QUEEN)
            eval[i] = 105;
          else
          // the queen of spade has not been played yet, let's throw our ace/king spade away.
          if (((cardId == SPADES_ACE) || (cardId == SPADES_KING)) && !cardPlayed[SPADES_QUEEN])
            eval[i] = 61;
        }
      }

      // if we didn't hit a specific evaluation in previous section, let's try those evaluations.
      if (eval[i] == -32768) {
        bool spades_queen_table = currentTrick.indexOf(SPADES_QUEEN) != -1;

        // don't give away the jack of diamond.
        if ((currentSuit != DIAMONDS) && variant_omnibus && (cardId == DIAMONDS_JACK))
         eval[i] = -63;
        else
        // we are the last one of the trick: play our bigger cards, but not in hearts, and not if queen spade is in
         if ((game_status == PLAY_A_CARD_4) && (currentSuit != HEARTS) && !spades_queen_table)
           eval[i] = cardId % 13 + 2;
        else
        if (!is_moon_an_option() && (currentSuit != SUIT_ALL)) {
           // throw away our big hearts cards
           if ((currentSuit != HEARTS) && (cardId / 13 == HEARTS))
             eval[i] = (cardId % 13) + 30;
           else
           // not in the suit, throw the bigest cards
           if (currentSuit != cardId / 13)
             eval[i] = (cardId % 13) + 15;
           else {
           // in the suit, throw our bigest cards under the best cards on the table
              if (cardId < best_hand)
                eval[i] = -(cardId - best_hand);
           }
        }
        else
        // play the lowest card.
          eval[i] = -(cardId % 13 + 2);
      }
  }

 // search for the best move.
 int best_eval = -32768, best_cardId = INVALID_CARD;

 for (int i = 0; i < validHandsById[turn].size(); i++)
   if (eval[i] > best_eval) {
     best_eval = eval[i];
     best_cardId = validHandsById[turn].at(i);
    }

 if (best_cardId == INVALID_CARD) {
   return validHandsById[turn].at(0);
   qDebug() << "CPU choose had to choose a default card to play!";
 } else
    return best_cardId;
}

bool Engine::AI_select_Randoms(PLAYER player)
{
    QList<int> &hand = playerHandsById[player];
    QList<int> &passed = passedCards[player];

    if (hand.isEmpty() || passed.size() >= 3) {
        return passed.size() == 3;
    }

    // Crée une copie temporaire des cartes disponibles
    QList<int> available = hand;

    // Retire celles déjà passées
    for (int c : passed) {
        available.removeAll(c);
    }

    if (available.isEmpty()) {
        return false;
    }

    // Tire une carte aléatoire parmi les restantes
    int r = std::uniform_int_distribution<int>(0, available.size() - 1)(gen);
    int cardId = available.at(r);

    passed.append(cardId);

    return passed.size() == 3;
}

bool Engine::is_prepass_to_moon(PLAYER player)
{
  const int aces[4] = {CLUBS_ACE, SPADES_ACE, DIAMONDS_ACE, HEARTS_ACE};

  int bonus = 0;
  int discard = 0;

  for (int i = 0; i < 4; i++) {
     int cpt_suit = countCardsInSuit(player, (SUIT)i);

     if (cpt_suit) {
       if (cpt_suit > 8)
         bonus = 2;
       else
       if (cpt_suit > 6)
         bonus = 1;


       if ((Owner(aces[i]) != player) && ((i == HEARTS) || ((cpt_suit >= 3) && (cpt_suit <= 6))))
         discard += cpt_suit;
     }

     if (discard > 3)
       return false;
  }

  int card_value;
  int low1 = 65535;
  int low2 = 65535;
  int low3 = 65535;

  double total = 0;
  for (int i = 0; i < playerHandsById[player].size(); i++) {
    card_value = (playerHandsById[player].at(i) % 13) + 2;

    if (card_value < low1) {
      if (low1 < low2) {
        if (low2 < low3)
          low3 = low2;
        low2 = low1;
      }
      low1 = card_value;
    }
    else
    if (card_value < low2) {
      if (low2 < low3)
        low3 = low2;
      low2 = card_value;
    }
    else
    if (card_value < low3)
      low3 = card_value;

    total += card_value;
  }

  total = (total - low1 - low2 - low3) / 10 + bonus;

  if (total < 10)                                     // although, some hands with a total of 7 are promising and
    return false;                                     // some with total of 11 aren't, it's usually better to
                                                      // have total above 10.
  return true;
}

bool Engine::AI_select_To_Moon(PLAYER player)
{
  if (Owner(HEARTS_ACE) != player)
    return false;

  int heart_count = countCardsInSuit(player, HEARTS);

  // if we're having heart, we needs to eliminate heart absolutely.
  if (heart_count && (heart_count < 9)) {
    int high_cards = 0;
    if (Owner(HEARTS_KING) == player) high_cards++;
    if (Owner(HEARTS_QUEEN) == player) high_cards++;
    if (Owner(HEARTS_JACK) == player) high_cards++;

    for (int cardId : playerHandsById[player]) {
      int suit = cardId / 13;  // 0 = club, 1 = spade, 2 = diamond, 3 = heart
      if (suit != HEARTS) continue;
      if (cardId < HEARTS_JACK - high_cards) {
        if (trySelectCardId(player, cardId) && AI_Ready(player)) {
          return true;
        }
      }
    }
  }
  return false;
}

bool Engine::AI_select_Spades(PLAYER player)
{
  int spade_count = countCardsInSuit(player, SPADES);

  if (!spade_count)
    return false;

  bool qs_passed = false;

  // if AI_flags_safe_keep_qs is not set, or it's not safe to keep -> pass the queen of spade.
  if (!(AI_CPU_flags[player] & AI_flags_safe_keep_qs) || (spade_count < 5)) {
    if (Owner(SPADES_QUEEN) == player) {
      if (trySelectCardId(player, SPADES_QUEEN) && AI_Ready(player)) {
        return true;
      }
      qs_passed = true;
     }
  }

  // if we kept the queen of spade then don't pass either ace or king of spade, otherwise try to pass them.
  if ((Owner(SPADES_QUEEN) != player) || qs_passed) {
    if (trySelectCardId(player, SPADES_ACE) && AI_Ready(player)) {
      return true;
    }
    if (trySelectCardId(player, SPADES_KING) && AI_Ready(player)) {
      return true;
    }
  }

  return false;
}

bool Engine::AI_select_Hearts(PLAYER player)
{
  int heart_count = countCardsInSuit(player, HEARTS);

  if (!heart_count)
    return false;

  // if AI_flags_pass_hearts_zero is set, let's roll the dice so 70% of the time we don't pass.
  int chance = std::uniform_int_distribution<int>(0, 100)(gen);
  if ((AI_CPU_flags[player] & AI_flags_pass_hearts_zero) && (chance < 70))
    return false;

  bool passed_hearts = false;
  for (int cardId : passedCards[player]) {
    if ((cardId / 13) == HEARTS) {
      passed_hearts = true;
      break;
    }
  }

  bool select_low = (AI_CPU_flags[player] & AI_flags_pass_hearts_low) || (chance >= 70);
  bool select_high = AI_CPU_flags[player] & AI_flags_pass_hearts_high;

  int low_position = heart_count > 1 ? 1 : 0;
  int high_position = heart_count > 2 ? heart_count - 2 : heart_count - 1;

  int cpt = 0;

  if (select_low | select_high) {
    for (int cardId : playerHandsById[player]) {
      int suit = cardId / 13;
       if (suit == HEARTS) {
         if (passedCards[player].indexOf(cardId) != -1) continue;

         if ((cpt == low_position) && select_low && !passed_hearts && (cardId != HEARTS_ACE)) {
           if (trySelectCardId(player, cardId) && AI_Ready(player)) {
             return true;
           }
           cpt++;
           continue;
         }

         if ((cpt == high_position) && select_high && (cardId >= HEARTS_JACK)) {
           if (trySelectCardId(player, cardId) && AI_Ready(player)) {
             return true;
           }
         }
         cpt++;
       }
    }
  }

  return false;
}

bool Engine::AI_pass_friendly(PLAYER player)
{
  if (!variant_omnibus) {
    return false;
  }

  if (!(AI_CPU_flags[player] & AI_flags_friendly)) {
    return false;
  }

  int cpt = 0;
  for (int i = 0; i < 4; i++) {
    if (player == i) continue;
    if (total_score[player] > total_score[i]) {
      cpt++;
    }
  }

  if ((cpt == 0) || (cpt == 3)) { // If it has the best or the worst score, don't pass friendly.
    return false;
  }

  PLAYER receiver = PLAYER_SOUTH; // just a default initialisation
  switch (direction) {
    case PASS_LEFT: if (player == PLAYER_WEST) {
                      receiver = PLAYER_NORTH;
                      break;
                    }
                    if (player == PLAYER_NORTH) {
                      receiver = PLAYER_EAST;
                      break;
                    }
                    if (player == PLAYER_EAST) {
                      receiver = PLAYER_SOUTH;
                      break;
                    }
                    break;
    case PASS_RIGHT: if (player == PLAYER_WEST) {
                       receiver = PLAYER_SOUTH;
                       break;
                     }
                     if (player == PLAYER_NORTH) {
                       receiver = PLAYER_WEST;
                       break;
                     }
                     if (player == PLAYER_EAST) {
                       receiver = PLAYER_NORTH;
                       break;
                     }
                     break;
    case PASS_ACROSS: if (player == PLAYER_WEST) {
                        receiver = PLAYER_EAST;
                        break;
                      }
                      if (player == PLAYER_NORTH) {
                        receiver = PLAYER_SOUTH;
                        break;
                      }
                      if (player == PLAYER_EAST) {
                        receiver = PLAYER_WEST;
                      }
                      break;
    case PASS_HOLD: break;
  }

  if ((total_score[receiver] - OMNIBUS_VALUE) < total_score[player]) {
    return false;
  }

  return true;
}

bool Engine::AI_elim_suit(PLAYER player, SUIT suit)
{
  int count_suit = countCardsInSuit(player, suit);

  if (!count_suit) {
    return false;
  }

  // if AI_flags_elim_suit is set, try to eliminate our clubs.
  if (AI_CPU_flags[player] & AI_flags_pass_elim_suit) {
    if (count_suit <= 3) {
      for (auto cardId = playerHandsById[player].crbegin(); cardId != playerHandsById[player].crend(); ++cardId) {
         if ((*cardId / 13) != suit) continue;
         if (trySelectCardId(player, *cardId) && AI_Ready(player)) {
           return true;
         }
      }
    }
  }

  return false;
}

bool Engine::trySelectCardId(PLAYER player, int cardId)
{
  if (Owner(cardId) != player) {
    return false;
  } // not the owner

  if (passedCards[player].indexOf(cardId) != -1) {
    return false;
  } // already selected

  if (passedCards[player].size() >= 3) {
    return false;
  } // 3 cards already selected

  passedCards[player].append(cardId);
  return true;
}

bool Engine::AI_select_Clubs(PLAYER player)
{
  int count_clubs = countCardsInSuit(player, CLUBS);

  if (!count_clubs) {
    return false;
  }

  if (AI_elim_suit(player, CLUBS)) {
    return true;
  }

  int cardId;
  // if AI_flags_try_moon is set, pass our lowest cards, otherwise pass the highest.
  if ((AI_CPU_flags[player] & AI_flags_try_moon) && is_prepass_to_moon(player)) {
    cardId = lowestCardInSuitForPlayer(player, CLUBS);
    if (trySelectCardId(player, cardId) && AI_Ready(player)) {
      return true;
    }
  } else {
      cardId = highestCardInSuitForPlayer(player, CLUBS);
      if (trySelectCardId(player, cardId) && AI_Ready(player)) {
        return true;
      }
    }

  return false;
}

bool Engine::AI_select_Diamonds(PLAYER player)
{
  int count_diamonds = countCardsInSuit(player, DIAMONDS);

  if (!count_diamonds) {
    return false;
  }

  // check for omnibus friendly pass. only 1 friendly pass.
  // trySelectCardId check the Owner of the card, but this way we'll pass only 1 top card not 3 of them
  if (variant_omnibus && AI_pass_friendly(player)) {
    if (Owner(DIAMONDS_JACK) == player) {
      if (trySelectCardId(player, DIAMONDS_JACK) && AI_Ready(player)) {
        return true;
      }
    }
    else
    if (Owner(DIAMONDS_ACE) == player) {
      if (trySelectCardId(player, DIAMONDS_ACE) && AI_Ready(player)) {
        return true;
      }
    }
    else
    if (Owner(DIAMONDS_KING) == player) {
      if (trySelectCardId(player, DIAMONDS_KING) && AI_Ready(player)) {
        return true;
      }
    }
  }

  // if AI_flags_elim_suit is set, try to eliminate our diamond.
  if (AI_elim_suit(player, DIAMONDS)) {
    return true;
  }

  // if not omnibus pass highest diamond, but not in try to moon
  if (!variant_omnibus && (!(AI_CPU_flags[player] & AI_flags_try_moon) || !is_prepass_to_moon(player))) {
    int highest = highestCardInSuitForPlayer(player, DIAMONDS);
    if (trySelectCardId(player, highest) && AI_Ready(player)) {
      return true;
    }
  }

  return false;
}

void Engine::cpus_select_cards()
{
  for (int p = 1; p < 4; p++) {
    for (int c = 0; c < 3; c++) {
      if ((AI_CPU_flags[p] & AI_flags_try_moon) && is_prepass_to_moon((PLAYER)p)) {
        if (AI_select_To_Moon((PLAYER)p)) break;
      }
      if (AI_select_Spades((PLAYER)p)) break;
      if (AI_select_Hearts((PLAYER)p)) break;
      if (AI_select_Clubs((PLAYER)p)) break;
      if (AI_select_Diamonds((PLAYER)p)) break;
      if (AI_select_Randoms((PLAYER)p)) break;
    }
  }
}
