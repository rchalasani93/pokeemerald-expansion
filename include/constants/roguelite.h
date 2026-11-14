#ifndef GUARD_CONSTANTS_ROGUELITE_H
#define GUARD_CONSTANTS_ROGUELITE_H

// Roguelite ("PokeRogue-style") run mode.
// A run is a chain of waves: wild battles that scale with the wave number,
// a boss every ROGUELITE_WAVES_PER_BIOME waves, a pick-one-of-three reward
// after every cleared wave, and a final boss on wave ROGUELITE_FINAL_WAVE.
// The player's normal party is parked in the PC for the length of the run.

#define ROGUELITE_WAVES_PER_BIOME   10
#define ROGUELITE_BIOME_COUNT       5
#define ROGUELITE_FINAL_WAVE        (ROGUELITE_WAVES_PER_BIOME * ROGUELITE_BIOME_COUNT)
#define ROGUELITE_STARTER_BUDGET    10
#define ROGUELITE_STARTER_LEVEL     5
#define ROGUELITE_STARTER_CHOICES   5
#define ROGUELITE_REWARD_CHOICES    3

// Menu ids returned by the starter draft menu (besides 0..ROGUELITE_STARTER_CHOICES-1)
#define ROGUELITE_MENU_START_RUN    100
#define ROGUELITE_MENU_REROLL       101

// Results of Roguelite_PickStarter
#define ROGUELITE_PICK_CONTINUE     0
#define ROGUELITE_PICK_DONE         1
#define ROGUELITE_PICK_TOO_EXPENSIVE 2
#define ROGUELITE_PICK_REROLL       3
#define ROGUELITE_PICK_NEED_ONE     4

// Results of Roguelite_AfterWave
#define ROGUELITE_WAVE_LOST         0
#define ROGUELITE_WAVE_CLEARED      1
#define ROGUELITE_WAVE_RUN_WON      2
#define ROGUELITE_WAVE_BOSS_FLED    3

#endif // GUARD_CONSTANTS_ROGUELITE_H
