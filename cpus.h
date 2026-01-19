#ifndef CPUS_H
#define CPUS_H

constexpr int AI_flags_pass_hearts_zero      = 0x1;
constexpr int AI_flags_pass_hearts_high      = 0x2;
constexpr int AI_flags_pass_hearts_low       = 0x4;
constexpr int AI_flags_try_moon              = 0x8;
constexpr int AI_flags_pass_elim_suit        = 0x10;
constexpr int AI_flags_safe_keep_qs          = 0x20;
constexpr int AI_flags_count_spade           = 0x40;
constexpr int AI_flags_friendly              = 0x80;

constexpr int MAX_PLAYERS_NAME = 44;

constexpr char names[MAX_PLAYERS_NAME][10] = {"You", "Aina", "Airi", "Alex", "Charles", "Christian", "Christine", "Cindy",
                                              "Danny", "David", "Denis", "Elena", "Erica", "Gabriel", "Grace", "Karine",
                                              "Karl", "Jason", "Jennifer", "John", "Linda", "Mai", "Maimi", "Marc", "Mary",
                                              "Masaki", "Michael", "Michelle", "Myriam", "Nadia", "Patricia", "Paul", "Reina",
                                              "Rick", "Riho", "Robert", "Sam", "Sandy", "Sara", "Sayuki", "Sayumi", "Sonia",
                                              "Sophie", "Tomoko"};

constexpr int AI_CPU_flags[MAX_PLAYERS_NAME] = {
    0,                                                          // You
    AI_flags_pass_hearts_low  | AI_flags_try_moon |
    AI_flags_count_spade,                                       // Aina
    AI_flags_pass_hearts_high | AI_flags_pass_elim_suit |
    AI_flags_count_spade | AI_flags_friendly,                   // Airi
    AI_flags_pass_hearts_low  | AI_flags_safe_keep_qs,          // Alex
    AI_flags_pass_hearts_zero | AI_flags_try_moon,              // Charles
    AI_flags_pass_hearts_high | AI_flags_count_spade,           // Christian
    AI_flags_pass_hearts_zero | AI_flags_try_moon |
    AI_flags_pass_elim_suit | AI_flags_friendly,                // Christine
    AI_flags_pass_hearts_low,                                   // Cindy
    AI_flags_pass_hearts_zero | AI_flags_pass_elim_suit,        // Dany
    AI_flags_pass_hearts_high | AI_flags_count_spade |
    AI_flags_friendly,                                          // David
    AI_flags_pass_hearts_low  | AI_flags_safe_keep_qs,          // Denis
    AI_flags_pass_hearts_high | AI_flags_try_moon,              // Elena
    AI_flags_pass_hearts_low | AI_flags_count_spade |
    AI_flags_friendly,                                          // Erica
    AI_flags_pass_hearts_zero | AI_flags_pass_elim_suit,        // Gabriel
    AI_flags_pass_hearts_zero | AI_flags_safe_keep_qs |
    AI_flags_friendly,                                          // Grace
    AI_flags_pass_hearts_low  | AI_flags_pass_hearts_high |
    AI_flags_try_moon | AI_flags_count_spade,                   // Karine
    AI_flags_pass_hearts_high | AI_flags_friendly,              // Karl
    AI_flags_pass_hearts_low  | AI_flags_pass_hearts_high,      // Jason
    AI_flags_pass_hearts_high | AI_flags_pass_elim_suit,        // Jennifer
    AI_flags_pass_hearts_zero | AI_flags_try_moon |
    AI_flags_count_spade | AI_flags_friendly,                   // John
    AI_flags_pass_hearts_low,                                   // Linda
    AI_flags_pass_hearts_zero,                                  // Mai
    AI_flags_pass_hearts_low | AI_flags_count_spade |
    AI_flags_friendly,                                          // Maimi
    AI_flags_pass_hearts_high | AI_flags_try_moon,              // Marc
    AI_flags_pass_hearts_low  | AI_flags_pass_elim_suit,        // Mary
    AI_flags_pass_hearts_low  | AI_flags_pass_hearts_high,      // Masaki
    AI_flags_pass_hearts_zero | AI_flags_count_spade |
    AI_flags_friendly,                                          // Michael
    AI_flags_pass_hearts_high | AI_flags_safe_keep_qs,          // Michelle
    AI_flags_pass_hearts_low  | AI_flags_try_moon |
    AI_flags_pass_elim_suit | AI_flags_friendly,                // Myriam
    AI_flags_pass_hearts_zero | AI_flags_try_moon,              // Nadia
    AI_flags_pass_hearts_high | AI_flags_count_spade,           // Patricia
    AI_flags_pass_hearts_zero,                                  // Paul
    AI_flags_pass_hearts_low  | AI_flags_try_moon,              // Reina
    AI_flags_pass_hearts_high | AI_flags_safe_keep_qs |
    AI_flags_friendly,                                          // Rick
    AI_flags_pass_hearts_low  | AI_flags_pass_hearts_high |
    AI_flags_count_spade  | AI_flags_friendly,                  // Riho
    AI_flags_pass_hearts_zero | AI_flags_pass_elim_suit,        // Robert
    AI_flags_pass_hearts_low,                                   // Sam
    AI_flags_pass_hearts_zero | AI_flags_safe_keep_qs |
    AI_flags_count_spade,                                       // Sandy
    AI_flags_pass_hearts_zero,                                  // Sara
    AI_flags_pass_hearts_high | AI_flags_try_moon |
    AI_flags_count_spade | AI_flags_friendly,                   // Sayuki
    AI_flags_pass_hearts_low  | AI_flags_pass_elim_suit |
    AI_flags_safe_keep_qs,                                      // Sayumi
    AI_flags_pass_hearts_low  | AI_flags_try_moon,              // Sonia
    AI_flags_pass_hearts_high | AI_flags_count_spade |
    AI_flags_friendly,                                          // Sophie
    AI_flags_pass_hearts_low  | AI_flags_safe_keep_qs           // Tomoko
};

#endif // CPUS_H
