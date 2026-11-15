#ifndef GUARD_INNATES_H
#define GUARD_INNATES_H

// Elite Redux-style innates: up to MAX_MON_INNATES extra abilities per species
// that are always active alongside the Pokémon's regular ability.
//
// Engine model:
//  - A battler's "ability slots" are slot 0 (its regular ability) followed by
//    its active innates. Innates are suppressed by the same things that
//    suppress abilities (Gastro Acid, Neutralizing Gas, Mold Breaker for
//    breakable abilities; Ability Shield protects them).
//  - Checks like "does X have Levitate" use HasAbilityOrInnate/BattlerHasAbility.
//  - Ability switch blocks iterate all slots with FOREACH_ABILITY_SLOT.
//  - Event-style triggers (switch-in, end turn, ...) run one slot per call
//    via AbilityBattleEffectsAllSlots, so several abilities can fire in turn.

#include "battle.h"
#include "battle_util.h"

#define MAX_MON_INNATES 3

enum Ability GetSpeciesInnate(enum Species species, u32 slot);
enum Ability GetBattlerInnate(enum BattlerId battler, u32 slot);
enum Ability GetBattlerAbilityInSlot(enum BattlerId battler, enum Ability mainAbility, u32 slot);
bool32 BattlerHasInnate(enum BattlerId battler, enum Ability ability);
enum Ability GetAbilityForPopUp(enum BattlerId battler);
bool32 MonHasAbility(struct Pokemon *mon, enum Ability ability);
bool32 SpeciesHasInnate(enum Species species, enum Ability ability);
enum Ability GetFirstAbilitySlotInList(enum BattlerId battler, enum Ability mainAbility, const u16 *list, u32 count);
u32 AbilityBattleEffectsAllSlotsFiltered(enum AbilityEffect caseID, enum BattlerId battler, enum Ability mainAbility, enum Move move, bool32 shouldAbilityTrigger, bool32 (*filter)(enum Ability), bool32 *moreSlotsPending);
u32 AbilityBattleEffectsFirstSlot(enum AbilityEffect caseID, enum BattlerId battler, enum Ability mainAbility, enum Move move, bool32 shouldAbilityTrigger, bool32 (*filter)(enum Ability));
u32 AbilityBattleEffectsAllSlots(enum AbilityEffect caseID, enum BattlerId battler, enum Ability mainAbility, enum Move move, bool32 shouldAbilityTrigger, bool32 *moreSlotsPending);

// ---- Inline fast paths -------------------------------------------------------
// These run in the AI's hottest loops, so the common case (the species has no
// matching innate) costs one table read and a few compares.

extern const u16 gNoInnates[MAX_MON_INNATES];
#if TESTING
extern u8 gTestInnatesCount;
const u16 *Test_GetSpeciesInnates(enum Species species);
#elif B_INNATES == TRUE
extern const u16 gSpeciesInnatesTable[NUM_SPECIES][MAX_MON_INNATES];
extern const u16 gSpeciesAbilitiesTable[NUM_SPECIES][NUM_ABILITY_SLOTS];
#endif
const u16 *Randomizer_GetInnates(enum Species species);
bool32 Randomizer_InnatesOn(void);

// Cheap global gate: FALSE means no innates can exist, so every check is a single compare.
#if TESTING
#define INNATES_ACTIVE() (gTestInnatesCount != 0)
#elif B_INNATES == TRUE
#define INNATES_ACTIVE() TRUE
#else
#define INNATES_ACTIVE() FALSE
#endif

static inline const u16 *GetSpeciesInnates(enum Species species)
{
#if TESTING
    if (gTestInnatesCount == 0)
        return gNoInnates;
    return Test_GetSpeciesInnates(species);
#elif B_INNATES == TRUE
    if (species >= NUM_SPECIES)
        return gNoInnates;
    if (Randomizer_InnatesOn())
        return Randomizer_GetInnates(species);
    return gSpeciesInnatesTable[species];
#else
    (void)species;
    return gNoInnates;
#endif
}

// Number of innate slots to look at (0 for species without innates).
static inline u32 GetInnateSlotCount(enum BattlerId battler)
{
    const u16 *innates;

    if (!INNATES_ACTIVE())
        return 0;
    innates = GetSpeciesInnates(gBattleMons[battler].species);
    return innates[2] ? 3 : (innates[1] ? 2 : (innates[0] ? 1 : 0));
}

// mainAbility == ability, or the battler has ability as an active innate.
static inline bool32 HasAbilityOrInnate(enum BattlerId battler, enum Ability mainAbility, enum Ability ability)
{
    const u16 *innates;

    if (mainAbility == ability)
        return TRUE;
    if (!INNATES_ACTIVE() || ability == ABILITY_NONE)
        return FALSE;
    innates = GetSpeciesInnates(gBattleMons[battler].species);
    if (innates[0] != ability && innates[1] != ability && innates[2] != ability)
        return FALSE;
    return BattlerHasInnate(battler, ability);
}

static inline bool32 BattlerHasAbility(enum BattlerId battler, enum Ability ability)
{
    return HasAbilityOrInnate(battler, GetBattlerAbility(battler), ability);
}

// Iterates slot 0 (mainAbility) and every active innate of battler.
// Use directly before a statement, typically `switch (var)`. `break` inside the
// switch keeps working as before. Species without innates loop exactly once.
#define FOREACH_ABILITY_SLOT(var, battler, mainAbility)                                                 \
    for (u32 var##Slot = 0, var##SlotCount = GetInnateSlotCount(battler); var##Slot <= var##SlotCount; var##Slot++) \
        for (enum Ability var = (var##Slot == 0 ? (mainAbility) : GetBattlerAbilityInSlot(battler, mainAbility, var##Slot)), \
                          var##Once = TRUE; var##Once; var##Once = FALSE)

#if TESTING
void Test_SetSpeciesInnates(enum Species species, enum Ability innate1, enum Ability innate2, enum Ability innate3);
void Test_ClearSpeciesInnates(void);
#endif

#endif // GUARD_INNATES_H
