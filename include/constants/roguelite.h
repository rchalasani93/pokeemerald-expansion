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
#define ROGUELITE_REWARD_CHOICES    3
#define ROGUELITE_DRAFT_PAGE_SIZE   8
#define ROGUELITE_DRAFT_OFFICE_BONUS 2   // extra draft points per Draft Office level

// New games start in the closed roguelite hub instead of the truck intro
#define ROGUELITE_START_IN_HUB      TRUE

// Menu ids returned by the starter draft menu (besides list indexes)
#define ROGUELITE_MENU_START_RUN    60000
#define ROGUELITE_MENU_NEXT_PAGE    60001
#define ROGUELITE_MENU_PREV_PAGE    60002

// Results of Roguelite_PickStarter
#define ROGUELITE_PICK_CONTINUE     0
#define ROGUELITE_PICK_DONE         1
#define ROGUELITE_PICK_TOO_EXPENSIVE 2
#define ROGUELITE_PICK_PAGE         3
#define ROGUELITE_PICK_NEED_ONE     4
#define ROGUELITE_PICK_ALREADY      5

// How a run ended (VAR_0x8004 for Roguelite_EndRun)
#define ROGUELITE_END_LOST          0
#define ROGUELITE_END_GAVE_UP       1
#define ROGUELITE_END_WON           2

#define ROGUELITE_MAX_COINS         9999

// Hub areas built with Rogue Coins
#define ROGUELITE_HUB_AREA_MART         0
#define ROGUELITE_HUB_AREA_HEALER       1
#define ROGUELITE_HUB_AREA_TUTOR        2
#define ROGUELITE_HUB_AREA_ABILITY_LAB  3
#define ROGUELITE_HUB_AREA_SUPPLY       4
#define ROGUELITE_HUB_AREA_DRAFT        5
#define ROGUELITE_HUB_AREA_COUNT        6
#define ROGUELITE_HUB_MAX_LEVEL         3

// Results of Roguelite_BuyHubArea
#define ROGUELITE_BUILD_DONE        0
#define ROGUELITE_BUILD_TOO_POOR    1
#define ROGUELITE_BUILD_MAXED       2
#define ROGUELITE_BUILD_CANCEL      3

// Results of Roguelite_AfterWave
#define ROGUELITE_WAVE_LOST         0
#define ROGUELITE_WAVE_CLEARED      1
#define ROGUELITE_WAVE_RUN_WON      2
#define ROGUELITE_WAVE_BOSS_FLED    3

// Routes (Emerald Rogue-style path choice between waves)
#define ROGUELITE_ROUTE_WILD        0
#define ROGUELITE_ROUTE_TRAINER     1
#define ROGUELITE_ROUTE_REST        2

// Results of Roguelite_PrepareWave
#define ROGUELITE_WAVE_KIND_WILD    0
#define ROGUELITE_WAVE_KIND_BOSS    1
#define ROGUELITE_WAVE_KIND_FINAL   2
#define ROGUELITE_WAVE_KIND_TRAINER 3
#define ROGUELITE_WAVE_KIND_REST    4

#endif // GUARD_CONSTANTS_ROGUELITE_H
