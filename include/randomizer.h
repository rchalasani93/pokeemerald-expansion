#ifndef GUARD_RANDOMIZER_H
#define GUARD_RANDOMIZER_H

// Elite Redux-style randomizer, toggled per save on page 2 of the Options menu.
//
// Every result is a pure function of (save seed, species, slot), so a species
// always gets the same randomized abilities/types/moves within one save and
// the game needs no extra storage. Turning an option off restores the real
// data immediately. Tests see the real data unless they turn an option on.

#include "constants/flags.h"

enum RandomizerOption
{
    RANDOMIZE_ABILITIES,
    RANDOMIZE_INNATES,
    RANDOMIZE_TYPES,
    RANDOMIZE_MOVES,
    RANDOMIZE_STATS,
    RANDOMIZE_RUN_POKEMON,
    RANDOMIZER_OPTION_COUNT,
};

extern const u16 gRandomizerOptionFlags[RANDOMIZER_OPTION_COUNT];

// Cheap check for the hot paths (reads the save flag byte directly).
#if TESTING
// Tests toggle options through this global (reset before every test)
extern u8 gTestRandomizerOptions;
#define RANDOMIZER_ON(option) ((gTestRandomizerOptions >> (option)) & 1)
#else
#define RANDOMIZER_ON(option) ((gSaveBlock1Ptr->flags[gRandomizerOptionFlags[option] / 8] >> (gRandomizerOptionFlags[option] % 8)) & 1)
#endif

bool32 Randomizer_IsEnabled(enum RandomizerOption option);
void Randomizer_SetEnabled(enum RandomizerOption option, bool32 enabled);
void Randomizer_EnsureSeed(void);

enum Ability Randomizer_GetAbility(enum Species species, u32 slot, enum Ability original);
const u16 *Randomizer_GetInnates(enum Species species);
enum Type Randomizer_GetType(enum Species species, u32 slot);
u32 Randomizer_GetBaseStat(enum Species species, u32 statIndex);
const struct LevelUpMove *Randomizer_GetLevelUpLearnset(enum Species species, const struct LevelUpMove *original);
enum Species Randomizer_GetRunSpecies(u32 salt);

#endif // GUARD_RANDOMIZER_H
