#include "config.h"

#include <QFile>
#include <QDir>

Config::Config() {
 QFile file(QDir::homePath() + CONFIG_FILENAME);

 if (!file.exists())
   save_config_file();      // create the file by saving the default values.
 else
   load_config_file();
}

bool Config::load_config_file() {
  QFile file(QDir::homePath() + CONFIG_FILENAME);

  int cpt = 0;

  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return false;
  }

  while (!file.atEnd()) {
      cpt++;
      QString line = file.readLine();
      QString param = line.section(" ", 0, 0);
      QString value = line.section(" ", -1, -1).simplified();

      if (param == "Language") {
        if (value == "english")
          language = LANG_ENGLISH;
        else
        if (value == "french")
          language = LANG_FRENCH;
        else
        if (value == "russian")
          language = LANG_RUSSIAN;

        continue;
      }

      if (param == "Deck_Style") {
        if (value == "standard")
          deck_style = STANDARD_DECK;
        else
        if (value == "english")
          deck_style = ENGLISH_DECK;
        else
        if (value == "russian")
          deck_style = RUSSIAN_DECK;
        else
        if (value == "nicu_white")
          deck_style = NICU_WHITE_DECK;
        else
        if (value == "tigullio_modern")
          deck_style = TIGULLIO_MODERN_DECK;
        else
        if (value == "mittelalter")
          deck_style = MITTELALTER_DECK;
        else
        if (value == "neoclassical")
          deck_style = NEOCLASSICAL_DECK;
        else
        if (value == "tigullio_international")
          deck_style = TIGULLIO_INTERNATIONAL_DECK;
        else
        if (value == "kaiser_jubilaum")
          deck_style = KAISER_JUBILAUM;

        continue;
      }

      if (param == "Hearts_Style") {
        if (value == "text")
          hearts_style = HEARTS_TEXT_ONLY;
        else
        if (value == "pink")
          hearts_style = HEARTS_ICONS_PINK;
        else
        if (value == "grey")
          hearts_style = HEARTS_ICONS_GREY;
        else
        if (value == "suit")
          hearts_style = HEARTS_ICONS_SUIT;
        else
        if (value == "cpu")
          hearts_style = HEARTS_ICONS_CPU;

        continue;
      }

      if (param == "Speed") {
        if (value == "slow")
          speed = SPEED_SLOW;
        else
        if (value == "normal")
          speed = SPEED_NORMAL;
        else
        if (value == "fast")
          speed = SPEED_FAST;
        else
        if (value == "expert")
          speed = SPEED_EXPERT;

        continue;
      }

      if (param == "Speed_play_card") {
        expert_speeds[SPEED_PLAY_CARD] = value.toInt();

        continue;
      }

      if (param == "Speed_play_two_clubs") {
        expert_speeds[SPEED_PLAY_TWO_CLUBS] = value.toInt();

        continue;
      }

      if (param == "Speed_shuffle") {
        expert_speeds[SPEED_SHUFFLE] = value.toInt();

        continue;
      }

      if (param == "Speed_clear_table") {
        expert_speeds[SPEED_CLEAR_TABLE] = value.toInt();

        continue;
      }

      if (param == "Speed_your_turn") {
        expert_speeds[SPEED_YOUR_TURN] = value.toInt();

        continue;
      }

      if (param == "Speed_pass_cards") {
        expert_speeds[SPEED_PASS_CARDS] = value.toInt();

        continue;
      }

      if (param == "Speed_animate_pass_cards") {
        expert_speeds[SPEED_ANIMATE_PASS_CARDS] = value.toInt();

        continue;
      }

      if (param == "Speed_animate_play_card") {
        expert_speeds[SPEED_ANIMATE_PLAY_CARD] = value.toInt();

        continue;
      }

      if (param == "Window_Width") {
        appWidth = value.toInt();

        continue;
      }

      if (param == "Window_Height") {
        appHeight = value.toInt();

        continue;
      }

      if (param == "Window_PosX") {
        appPosX = value.toInt();

        continue;
      }

      if (param == "Window_PosY") {
        appPosY = value.toInt();

        continue;
      }

      if (param == "Background") {
        if (value == "None")
          background_index = BACKGROUND_NONE;
        else
        if (value == "Green")
          background_index = BACKGROUND_GREEN;
        else
        if (value == "Universe")
          background_index = BACKGROUND_UNIVERSE;
        else
        if (value == "Ocean")
          background_index = BACKGROUND_OCEAN;
        else
        if (value == "Mt_Fuji")
          background_index = BACKGROUND_MT_FUJI;
        else
        if (value == "Everest")
          background_index = BACKGROUND_EVEREST;
        else
        if (value == "Desert")
          background_index = BACKGROUND_DESERT;
        else
        if (value == "Wooden_1")
          background_index = BACKGROUND_WOODEN_1;
        else
        if (value == "Wooden_2")
          background_index = BACKGROUND_WOODEN_2;
        else
        if (value == "Wooden_3")
          background_index = BACKGROUND_WOODEN_3;
        else
        if (value == "Wooden_4")
          background_index = BACKGROUND_WOODEN_4;
        else
        if (value == "Leaves")
          background_index = BACKGROUND_LEAVES;
        else
        if (value == "Marble")
          background_index = BACKGROUND_MARBLE;

        continue;
      }

      if (param == "Username") {
        username  = value;
        continue;
      }

      if (param == "Password") {
        password = value;
        continue;
      }

      if (param == "Background_Path") {
        background_path = value;
        continue;
      }

      bool enable;

      if (value == "true")
        enable = true;
      else
        enable = false;

      if (param == "Animated_Play")
        animated_play = enable;
      else
      if (param == "Auto_Centering")
        auto_centering = enable;
      else
      if (param == "Cheat_Mode")
        cheat_mode = enable;
      else
      if (param == "Info_Channel")
        info_channel = enable;
      else
      if (param == "Show_Direction")
        show_direction = enable;
      else
      if (param == "Sounds")
        sounds = enable;
      else
      if (param == "Detect_Tram")
        detect_tram = enable;
      else
      if (param == "Easy_Card_Selection")
        easy_card_selection = enable;
      else
      if (param == "Card_Display")
        card_display = enable;
      else
      if (param == "Empty_Slot")
        empty_slot_opaque = enable;
      else
      if (param == "Perfect_100")
        perfect_100 = enable;
      else
      if (param == "Omnibus")
        omnibus = enable;
      else
      if (param == "Queen_Spade_Break_Heart")
        queen_spade_break_heart = enable;
      else
      if (param == "No_Trick_Bonus")
        no_trick_bonus = enable;
      else
      if (param == "New_Moon")
         new_moon = enable;
      else
      if (param == "No_Draw")
        no_draw = enable;
      else
      if (param == "Save_Game")
        save_game = enable;
      else
      if (param == "Warning")
        warning = enable;
      else
      if (param == "Auto_Start")
        auto_start = enable;
      else
      if (param == "Anim_Deal_Cards")
        anim_deal_cards = enable;
      else
      if (param == "Anim_Play_Card")
        anim_play_card = enable;
      else
      if (param == "Anim_Collect_Tricks")
        anim_collect_tricks = enable;
      else
      if (param == "Anim_Pass_Cards")
        anim_pass_cards = enable;
      else
      if (param == "Animated_Arrow")
        animated_arrow = enable;
      else
      if (param == "Anim_Turn_Indicator")
        anim_turn_indication = enable;
      else {
          // unknown param
      }
      if (cpt > 45) break; // too many lines ?? corrupted file ??
  }
  file.close();

  return true;
}

bool Config::set_language(int lang) {
  language = lang;

  return save_config_file();
}

bool Config::set_deck_style(int style) {
  deck_style = style;

  return save_config_file();
}

bool Config::set_hearts_style(int style) {
  hearts_style = style;

  return save_config_file();
}

bool Config::set_speed(int s) {
  speed = s;

  return save_config_file();
}

bool Config::set_expert_speeds(QList<int> values)
{
  if (values.size() != 8)
    return false;

  speed = SPEED_EXPERT;

  expert_speeds[SPEED_PLAY_CARD] = values.at(0);
  expert_speeds[SPEED_PLAY_TWO_CLUBS] = values.at(1);
  expert_speeds[SPEED_SHUFFLE] = values.at(2);
  expert_speeds[SPEED_CLEAR_TABLE] = values.at(3);
  expert_speeds[SPEED_YOUR_TURN] = values.at(4);
  expert_speeds[SPEED_PASS_CARDS] = values.at(5);
  expert_speeds[SPEED_ANIMATE_PASS_CARDS] = values.at(6);
  expert_speeds[SPEED_ANIMATE_PLAY_CARD] = values.at(7);

  return save_config_file();
}

bool Config::set_config_file(int param, bool enable)
{
  switch (param) {
    case CONFIG_AUTO_CENTERING :          auto_centering = enable; break;
    case CONFIG_CHEAT_MODE :              cheat_mode = enable; break;
    case CONFIG_INFO_CHANNEL :            info_channel = enable; break;
    case CONFIG_SHOW_DIRECTION :          show_direction = enable; break;
    case CONFIG_SOUNDS :                  sounds = enable; break;
    case CONFIG_DETECT_TRAM :             detect_tram = enable; break;
    case CONFIG_PERFECT_100 :             perfect_100 = enable; break;
    case CONFIG_OMNIBUS :                 omnibus = enable; break;
    case CONFIG_QUEEN_SPADE :             queen_spade_break_heart = enable; break;
    case CONFIG_NO_TRICK :                no_trick_bonus = enable; break;
    case CONFIG_NEW_MOON :                new_moon = enable; break;
    case CONFIG_NO_DRAW :                 no_draw = enable; break;
    case CONFIG_SAVE_GAME :               save_game = enable; break;
    case CONFIG_EASY_CARD_SELECTION :     easy_card_selection = enable; break;
    case CONFIG_CARD_DISPLAY :            card_display = enable; break;
    case CONFIG_EMPTY_SLOT :              empty_slot_opaque = enable; break;
    case CONFIG_WARNING :                 warning = enable; break;
    case CONFIG_AUTO_START :              auto_start = enable; break;
    case CONFIG_ANIMATED_PLAY :           animated_play = enable; break;
    case CONFIG_ANIM_DEAL_CARDS :         anim_deal_cards = enable; break;
    case CONFIG_ANIM_PLAY_CARD :          anim_play_card = enable; break;
    case CONFIG_ANIM_COLLECT_TRICKS :     anim_collect_tricks = enable; break;
    case CONFIG_ANIM_PASS_CARDS :         anim_pass_cards = enable; break;
    case CONFIG_ANIMATED_ARROW :          animated_arrow = enable; break;
    case CONFIG_ANIM_TURN_INDICATOR :     anim_turn_indication = enable; break;
  }

  return save_config_file();
}

bool Config::save_config_file()
{
  QFile file(QDir::homePath() + CONFIG_FILENAME);

  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
     return false;
  }

  QTextStream out(&file);

  out << "Window_Width = " << appWidth << "\n";
  out << "Window_Height = " << appHeight << "\n";
  out << "Window_PosX = " << appPosX << "\n";
  out << "Window_PosY = " << appPosY << "\n";
  out << "Background_Path = " << background_path << "\n";

  if (username.size() && password.size()) {
    out << "Username = " << username << "\n";
    out << "Password = " << password << "\n";
    out << "Warning = " << (warning ? "true" : "false") << "\n";
  }

  switch (language) {
    case LANG_ENGLISH: out << "Language = english" << "\n"; break;
    case LANG_FRENCH:  out << "Language = french"  << "\n"; break;
    case LANG_RUSSIAN: out << "Language = russian" << "\n"; break;
  }

  switch (deck_style) {
    case STANDARD_DECK:               out << "Deck_Style = standard" << "\n"; break;
    case ENGLISH_DECK:                out << "Deck_Style = english" << "\n"; break;
    case RUSSIAN_DECK:                out << "Deck_Style = russian" << "\n"; break;
    case NICU_WHITE_DECK:             out << "Deck_Style = nicu_white" << "\n"; break;
    case TIGULLIO_MODERN_DECK:        out << "Deck_Style = tigullio_modern" << "\n"; break;
    case MITTELALTER_DECK:            out << "Deck_Style = mittelalter" << "\n"; break;
    case NEOCLASSICAL_DECK:           out << "Deck_Style = neoclassical" << "\n"; break;
    case TIGULLIO_INTERNATIONAL_DECK: out << "Deck_Style = tigullio_international" << "\n"; break;
    case KAISER_JUBILAUM:             out << "Deck_Style = kaiser_jubilaum" << "\n"; break;
  }

  switch (hearts_style) {
    case HEARTS_TEXT_ONLY:  out << "Hearts_Style = text" << "\n"; break;
    case HEARTS_ICONS_PINK: out << "Hearts_Style = pink" << "\n"; break;
    case HEARTS_ICONS_GREY: out << "Hearts_Style = grey" << "\n"; break;
    case HEARTS_ICONS_SUIT: out << "Hearts_Style = suit" << "\n"; break;
    case HEARTS_ICONS_CPU:  out << "Hearts_Style = cpu" << "\n"; break;
  }

  switch (background_index) {
    case BACKGROUND_NONE:     out << "Background = None" << "\n"; break;
    case BACKGROUND_GREEN:    out << "Background = Green" << "\n"; break;
    case BACKGROUND_UNIVERSE: out << "Background = Universe" << "\n"; break;
    case BACKGROUND_OCEAN:    out << "Background = Ocean" << "\n"; break;
    case BACKGROUND_EVEREST:  out << "Background = Everest" << "\n"; break;
    case BACKGROUND_MT_FUJI:  out << "Background = Mt_Fuji" << "\n"; break;
    case BACKGROUND_DESERT:   out << "Background = Desert" << "\n"; break;
    case BACKGROUND_WOODEN_1: out << "Background = Wooden_1" << "\n"; break;
    case BACKGROUND_WOODEN_2: out << "Background = Wooden_2" << "\n"; break;
    case BACKGROUND_WOODEN_3: out << "Background = Wooden_3" << "\n"; break;
    case BACKGROUND_WOODEN_4: out << "Background = Wooden_4" << "\n"; break;
    case BACKGROUND_LEAVES:   out << "Background = Leaves" << "\n"; break;
    case BACKGROUND_MARBLE:   out << "Background = Marble" << "\n"; break;
  }

  out << "Animated_Play = " << (animated_play ? "true" : "false") << "\n";
  out << "Auto_Centering = " << (auto_centering ? "true" : "false") << "\n";
  out << "Cheat_Mode = " << (cheat_mode ? "true" : "false") << "\n";
  out << "Info_Channel = " << (info_channel ? "true" : "false") << "\n";
  out << "Show_Direction = " << (show_direction ? "true" : "false") << "\n";
  out << "Sounds = " << (sounds ? "true" : "false") << "\n";
  out << "Detect_Tram = " << (detect_tram ? "true" : "false") << "\n";
  out << "Easy_Card_Selection = " << (easy_card_selection ? "true" : "false") << "\n";
  out << "Card_Display = " << (card_display ? "true" : "false") << "\n";
  out << "Empty_Slot = " << (empty_slot_opaque ? "true" : "false") << "\n";
  out << "Auto_Start = " << (auto_start ? "true" : "false") << "\n";
  out << "Anim_Deal_Cards = " << (anim_deal_cards ? "true" : "false") << "\n";
  out << "Anim_Play_Card = " << (anim_play_card ? "true" : "false") << "\n";
  out << "Anim_Collect_Tricks = " << (anim_collect_tricks ? "true" : "false") << "\n";
  out << "Anim_Pass_Cards = " << (anim_pass_cards ? "true" : "false") << "\n";
  out << "Animated_Arrow = " << (animated_arrow ? "true" : "false") << "\n";
  out << "Anim_Turn_Indicator = " << (anim_turn_indication ? "true" : "false") << "\n";

  switch (speed) {
    case SPEED_SLOW : out << "Speed = slow" << "\n"; break;
    case SPEED_FAST : out << "Speed = fast" << "\n"; break;
    case SPEED_EXPERT : out << "Speed = expert" << "\n";

                        out << "Speed_play_card = " << expert_speeds[SPEED_PLAY_CARD] << "\n";
                        out << "Speed_play_two_clubs = " << expert_speeds[SPEED_PLAY_TWO_CLUBS] << "\n";
                        out << "Speed_shuffle = " << expert_speeds[SPEED_SHUFFLE] << "\n";
                        out << "Speed_clear_table = " << expert_speeds[SPEED_CLEAR_TABLE] << "\n";
                        out << "Speed_your_turn = " << expert_speeds[SPEED_YOUR_TURN] << "\n";
                        out << "Speed_pass_cards = " << expert_speeds[SPEED_PASS_CARDS] << "\n";
                        out << "Speed_animate_pass_cards = " << expert_speeds[SPEED_ANIMATE_PASS_CARDS] << "\n";
                        out << "Speed_animate_play_card = " << expert_speeds[SPEED_ANIMATE_PLAY_CARD] << "\n";

                        break;
    default :           out << "Speed = normal" << "\n";
  }

  out << "Perfect_100 = " << (perfect_100 ? "true" : "false") << "\n";
  out << "Omnibus = " << (omnibus ? "true" : "false") << "\n";
  out << "Queen_Spade_Break_Heart = " << (queen_spade_break_heart ? "true" : "false") << "\n";
  out << "No_Trick_Bonus = " << (no_trick_bonus ? "true" : "false") << "\n";
  out << "New_Moon = " << (new_moon ? "true" : "false") << "\n";
  out << "No_Draw = " << (no_draw ? "true" : "false") << "\n";
  out << "Save_Game = " << (save_game ? "true" : "false") << "\n";

  file.close();
  return true;
}

void Config::set_online(QString u, QString p)
{
  username = u;
  password = p;

  save_config_file();
}

int Config::get_speed(int type) {
  if (speed == SPEED_EXPERT)
    return expert_speeds[type];

  switch (type) {
    case SPEED_CLEAR_TABLE: return SPEED_CLEAR_TABLE_VALUES[speed]; break;
    case SPEED_SHUFFLE:     return SPEED_SHUFFLE_VALUES[speed]; break;
    case SPEED_PASS_CARDS:  return SPEED_PASS_CARDS_VALUES[speed]; break;
    case SPEED_PLAY_CARD:   return SPEED_PLAY_CARD_VALUES[speed]; break;
    case SPEED_PLAY_TWO_CLUBS: return SPEED_PLAY_TWO_CLUBS_VALUES[speed]; break;
    case SPEED_YOUR_TURN:   return SPEED_YOUR_TURN_VALUES[speed]; break;
    case SPEED_ANIMATE_PASS_CARDS: return SPEED_ANIMATE_PASS_CARDS_VALUES[speed]; break;
    case SPEED_ANIMATE_PLAY_CARD: return SPEED_ANIMATE_PLAY_CARD_VALUES[speed]; break;
  }
  return 0;
}

bool Config::set_background_path(const QString &path)
{
  background_path = path;

  return save_config_file();
}

bool Config::set_width(int w)
{
  appWidth = w;

  return save_config_file();
}

bool Config::set_height(int h)
{
  appHeight = h;

  return save_config_file();
}

bool Config::set_posX(int x)
{
  appPosX = x;

  return save_config_file();
}

bool Config::set_posY(int y)
{
  appPosY = y;

  return save_config_file();
}
