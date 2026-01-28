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
      emit sig_message(tr("Previous saved game has been loaded!"));
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
      emit sig_message(errorMessage(ERRCORRUPTED));
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
  if (locked) {
  //  emit sig_error(errorMessage(ERRLOCKED));
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
  emit sig_message(tr("Starting a new game."));
  emit sig_new_players();
  emit sig_update_stat(0, STATS_GAME_STARTED);
  emit sig_update_scores_board(playersName, hand_score, total_score);
  LockedLoop();

  return NOERROR;
}

bool Engine::undo()
{
  if (undoStack.available) {
    popUndo();
    return true;
  }

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
    qDebug() << "AI" << player << "a choisi carte" << cardId;

    return passed.size() == 3;
}

void Engine::cpus_select_cards()
{
  for (int p = 1; p < 4; p++) {
    for (int c = 0; c < 3; c++) {
      if (AI_select_Randoms((PLAYER)p)) break;
    }
  }
}

void Engine::Loop()
{
  bool tram;
  PLAYER owner;
  int cardId;

  switch (game_status) {
     case NEW_ROUND:       qDebug() << "NEW ROUND";
                           locked = true;
                           init_variables();
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
    case DEAL_CARDS:       qDebug() << "DEAL_CARDS";
                           emit sig_deal_cards();
                           break;
    case SELECT_CARDS:     qDebug() << "SELECT_CARDS direction " << direction;
                           sort_players_hand();          // sort after animated deal
                           emit sig_refresh_deck();

                           if (direction == PASS_HOLD) {
                             game_status = PLAY_TWO_CLUBS;
                             LockedLoop();
                           } else
                               emit sig_pass_to(direction);
                           emit sig_busy(false);
                           locked = false;
                           break;
    case TRADE_CARDS:      qDebug() << "TRADE CARDS";
                           locked = true;
                           cpus_select_cards();
                           completePassCards();
                           emit sig_passed();
                           break;
    case PLAY_TWO_CLUBS :  qDebug() << "PLAY_TWO_CLUBS";
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
                           }
                           if (owner == PLAYER_SOUTH) {
                             locked = false;
                             emit sig_busy(false);
                             emit sig_your_turn();
                           }
                           break;
     case PLAY_A_CARD_1 : previous_winner = turn;
                          currentTrick.clear();
     case PLAY_A_CARD_2 :
     case PLAY_A_CARD_3 :
     case PLAY_A_CARD_4 : qDebug() << "Play a card: " << game_status;
                          locked = true;
                          qDebug() << "current_suit: " << currentSuit;
                          filterValidMoves(turn);
                          if ((turn != PLAYER_SOUTH) && (turn != PLAYER_NOBODY)) {
                             cardId = validHandsById[turn].at(0);

                             qDebug() << "Player: " << turn << "ValidHand: " << validHandsById[turn] << "cardId: " << cardId << "Full hand: " << playerHandsById[turn];
                             Play(cardId, turn);
                             qDebug() << "Remove from: " << turn << "CardId: " << cardId;
                           } else
                              if (turn == PLAYER_SOUTH) {
                                qDebug() << "Your turn, cards left: " << cards_left;
                                locked = false;
                                emit sig_busy(false);
                                emit sig_your_turn();
                              }
                              qDebug() << "Refresh";
                           qDebug() << "Card LEFT: " << cards_left;
                           break;
     case END_TURN:        currentSuit = SUIT_ALL;
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
                           qDebug() << "END_TURN";
                           break;
     case LOOP_TURN:   qDebug() << "Status: " << game_status << "turn : " << turn << "cards_left: " << cards_left << "cpt_played: " << cpt_played;
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
                         qDebug() << "END_ROUND " << game_score;
                         if (!is_game_over()) {
                           game_status = NEW_ROUND;
                           advance_direction();
                          } else {
                              game_status = GAME_OVER;
                         }
                         LockedLoop();
                         break;
     case GAME_OVER:     qDebug() << "GAME_OVER";
                         emit sig_busy(false);
                         locked = false;
                         sendGameResult();
                         emit sig_play_sound(SOUND_GAME_OVER);
                         emit sig_update_stat(0, STATS_GAME_FINISHED);
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

  if (turn != PLAYER_SOUTH) {
    emit sig_busy(true);
  }

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

  emit sig_message(mesg);
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

  if (player == PLAYER_SOUTH) {
    pushUndo();
  }

  playerHandsById[player].removeAll(cardId);
  cards_left--;

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
      emit sig_message(tr("Hearts has been broken!"));
      emit sig_play_sound(SOUND_BREAKING_HEARTS);
    }
  } else
  if (cardId == SPADES_QUEEN) {
    trick_value += 13;
    queen_spade_in_trick = true;
    if (variant_queen_spade) {
      hearts_broken = true;
      emit sig_message(tr("Hearts has been broken!"));
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

    msgBox.setInformativeText("What would you like to do?");

    QPushButton *addButton = msgBox.addButton("➕ 26 to opponents", QMessageBox::AcceptRole);
    QPushButton *subButton = msgBox.addButton("➖ 26 to myself", QMessageBox::RejectRole);

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

  emit sig_message(mesg);
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
      emit sig_message(mesg);

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
          emit sig_message(mesg);
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
          emit sig_message(mesg);
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

      emit sig_message(mesg);
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

  emit sig_message(mesg);

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

  emit sig_message(mesg);
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

  emit sig_message(mesg);
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

  while (!file.atEnd()) {
    int value;
    QString line = file.readLine();
    cpt++;
    qDebug() << "Step: " << cpt;

    switch (cpt) {
       // Line #1: Data = 4 x players index (0 = You)
       case 1: for (int p = 0; p < 4; p++) {
                 value = line.section(' ', p, p).toInt();
                 if ((value < 0) || (value >= MAX_PLAYERS_NAME)) {
                   return false;
                 }
                 qDebug() << "case 1: " << value;
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
                   qDebug() << "case 2: " << value << " d: " << d;
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

                   qDebug() << "case 3: " << value;
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
                  qDebug() << "case 4: " << value;

                   if ((value < 0) || (value > 26))
                     return false;
                   hand_score[p] = value;
                }
               break;

        // Line #5: Data = Players total scores
        case 5: for (int p = 0; p < 4; p++) {
                  value = line.section(' ', p, p).toInt();
                  qDebug() << "case 5: " << value;

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
                  qDebug() << "case 6: " << value;

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
                  qDebug() << "case 7: " << value;

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

qDebug() << "Cards Left: " << cards_left;
qDebug() << "SOUTH" << playerHandsById[PLAYER_SOUTH];
qDebug() << "WEST" << playerHandsById[PLAYER_WEST];
qDebug() << "NORTH" << playerHandsById[PLAYER_NORTH];
qDebug() << "EAST" << playerHandsById[PLAYER_EAST];
qDebug() << "Pile" << trickPileSize;

  if ((game_status == SELECT_CARDS) && (cards_left != DECK_SIZE))
    return false;
qDebug() << "Step select";

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

 // emit sig_clear_deck();
 // emit sig_enableAllCards();
  firstTime = false;

  emit sig_setTrickPile(currentTrick);
qDebug() << "Pile: " << currentTrick;
  emit sig_refresh_deck();

  emit sig_new_players();
  emit sig_update_scores_board(playersName, hand_score, total_score);

qDebug() << "STATUS: " << game_status;

  LockedLoop();

qDebug() << "Step H";

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
}

void Engine::popUndo()
{
  undoStack.available = false;
  for (int i = 0; i < 4; i++) {
    playerHandsById[i] = undoStack.playerHandsById[i];
    hand_score[i] = undoStack.hand_score[i];
    total_score[i] = undoStack.total_score[i];
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
