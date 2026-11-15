#include "global.h"
#include "battle.h"
#include "battle_util.h"
#include "innates.h"
#include "test_runner.h"
#include "constants/abilities.h"
#include "constants/hold_effects.h"

const u16 gNoInnates[MAX_MON_INNATES] = {ABILITY_NONE, ABILITY_NONE, ABILITY_NONE};

#if B_INNATES == TRUE && !TESTING
#include "data/pokemon/innates.h"
#endif

#if TESTING
#define TEST_INNATE_ENTRIES 8
struct TestInnates
{
    u16 species;
    u16 innates[MAX_MON_INNATES];
};
EWRAM_DATA static struct TestInnates sTestInnates[TEST_INNATE_ENTRIES] = {0};
IWRAM_DATA u8 gTestInnatesCount = 0; // IWRAM: read on every ability check

void Test_SetSpeciesInnates(enum Species species, enum Ability innate1, enum Ability innate2, enum Ability innate3)
{
    u32 i;
    for (i = 0; i < TEST_INNATE_ENTRIES; i++)
    {
        if (sTestInnates[i].species == species || sTestInnates[i].species == SPECIES_NONE)
        {
            if (sTestInnates[i].species == SPECIES_NONE)
                gTestInnatesCount++;
            sTestInnates[i].species = species;
            sTestInnates[i].innates[0] = innate1;
            sTestInnates[i].innates[1] = innate2;
            sTestInnates[i].innates[2] = innate3;
            return;
        }
    }
}

void Test_ClearSpeciesInnates(void)
{
    memset(sTestInnates, 0, sizeof(sTestInnates));
    gTestInnatesCount = 0;
}
#endif

enum Ability GetSpeciesInnate(enum Species species, u32 slot)
{
    if (slot >= MAX_MON_INNATES || species == SPECIES_NONE)
        return ABILITY_NONE;
    return GetSpeciesInnates(species)[slot];
}

#if TESTING
const u16 *Test_GetSpeciesInnates(enum Species species)
{
    u32 i;
    for (i = 0; i < gTestInnatesCount; i++)
    {
        if (sTestInnates[i].species == species)
            return sTestInnates[i].innates;
    }
    return gNoInnates;
}
#endif

// Mirrors the suppression rules in GetBattlerAbilityInternal.
static bool32 IsInnateSuppressed(enum BattlerId battler, enum Ability innate)
{
    bool32 hasAbilityShield = GetBattlerHoldEffectIgnoreAbility(battler) == HOLD_EFFECT_ABILITY_SHIELD;

    if (gBattleStruct->battlerState[battler].notOnField || gSpecialStatuses[battler].attackerInParty)
        return TRUE;

    if (gAbilitiesInfo[innate].cantBeSuppressed)
        goto MOLD_BREAKER_CHECK;

    if (gBattleMons[battler].volatiles.gastroAcid)
        return TRUE;

    if (!hasAbilityShield && IsNeutralizingGasOnField() && innate != ABILITY_NEUTRALIZING_GAS)
        return TRUE;

MOLD_BREAKER_CHECK:
    if (!hasAbilityShield
     && battler != gBattlerAttacker
     && gBattleStruct->moldBreakerActive
     && gAbilitiesInfo[innate].breakable)
        return TRUE;

    return FALSE;
}

enum Ability GetBattlerInnate(enum BattlerId battler, u32 slot)
{
    enum Ability innate;

    if (battler >= gBattlersCount)
        return ABILITY_NONE;

    innate = GetSpeciesInnate(gBattleMons[battler].species, slot);
    if (innate == ABILITY_NONE || IsInnateSuppressed(battler, innate))
        return ABILITY_NONE;
    return innate;
}

// Slot 0 is mainAbility as the caller already resolved it; slots 1..MAX_MON_INNATES
// are innates. Duplicates of earlier slots come back as ABILITY_NONE so an ability
// never applies twice.
enum Ability GetBattlerAbilityInSlot(enum BattlerId battler, enum Ability mainAbility, u32 slot)
{
    enum Ability innate;
    u32 i;

    if (slot == 0)
        return mainAbility;

    if (battler >= gBattlersCount)
        return ABILITY_NONE;
    innate = GetSpeciesInnate(gBattleMons[battler].species, slot - 1);
    if (innate == ABILITY_NONE || innate == mainAbility || innate == gBattleMons[battler].ability)
        return ABILITY_NONE;
    for (i = 0; i < slot - 1; i++)
    {
        if (GetSpeciesInnate(gBattleMons[battler].species, i) == innate)
            return ABILITY_NONE;
    }
    if (IsInnateSuppressed(battler, innate))
        return ABILITY_NONE;
    return innate;
}

bool32 BattlerHasInnate(enum BattlerId battler, enum Ability ability)
{
    u32 i;
    enum Species species;

    if (ability == ABILITY_NONE || battler >= gBattlersCount)
        return FALSE;
    // Hot path (AI evaluates this constantly): compare raw species data first and
    // only run the suppression checks when an innate actually matches.
    species = gBattleMons[battler].species;
    for (i = 0; i < MAX_MON_INNATES; i++)
    {
        if (GetSpeciesInnate(species, i) == ability)
            return !IsInnateSuppressed(battler, ability);
    }
    return FALSE;
}

// First ability slot (regular ability, then innates) for which the event triggers.
u32 AbilityBattleEffectsFirstSlot(enum AbilityEffect caseID, enum BattlerId battler, enum Ability mainAbility, enum Move move, bool32 shouldAbilityTrigger, bool32 (*filter)(enum Ability))
{
    u32 slot;
    for (slot = 0; slot <= MAX_MON_INNATES; slot++)
    {
        enum Ability ability = GetBattlerAbilityInSlot(battler, mainAbility, slot);
        u32 effect;

        if (slot != 0 && ability == ABILITY_NONE)
            continue;
        if (filter != NULL && !filter(ability))
            continue;
        effect = AbilityBattleEffects(caseID, battler, ability, move, shouldAbilityTrigger);
        if (effect)
            return effect;
    }
    return 0;
}

// Ability popups show the battler's regular ability. When the ability that just
// triggered is one of its innates instead, show that.
enum Ability GetAbilityForPopUp(enum BattlerId battler)
{
    if (gLastUsedAbility != ABILITY_NONE
     && gLastUsedAbility != gBattleMons[battler].ability
     && BattlerHasInnate(battler, gLastUsedAbility))
        return gLastUsedAbility;
    return gBattleMons[battler].ability;
}


// Runs an ability event for every ability slot of a battler, one slot per call.
// Event loops call this repeatedly and only advance once *moreSlotsPending is FALSE.
EWRAM_DATA static u8 sSlotCursor[MAX_BATTLERS_COUNT] = {0};
EWRAM_DATA static u8 sSlotCursorCase[MAX_BATTLERS_COUNT] = {0};

u32 AbilityBattleEffectsAllSlots(enum AbilityEffect caseID, enum BattlerId battler, enum Ability mainAbility, enum Move move, bool32 shouldAbilityTrigger, bool32 *moreSlotsPending)
{
    return AbilityBattleEffectsAllSlotsFiltered(caseID, battler, mainAbility, move, shouldAbilityTrigger, NULL, moreSlotsPending);
}

// Same, but only slots whose ability passes filter are tried (NULL = all).
u32 AbilityBattleEffectsAllSlotsFiltered(enum AbilityEffect caseID, enum BattlerId battler, enum Ability mainAbility, enum Move move, bool32 shouldAbilityTrigger, bool32 (*filter)(enum Ability), bool32 *moreSlotsPending)
{
    u32 slot;

    *moreSlotsPending = FALSE;
    if (sSlotCursorCase[battler] != caseID)
    {
        sSlotCursorCase[battler] = caseID;
        sSlotCursor[battler] = 0;
    }

    for (slot = sSlotCursor[battler]; slot <= MAX_MON_INNATES; slot++)
    {
        enum Ability ability = GetBattlerAbilityInSlot(battler, mainAbility, slot);
        u32 effect;

        if (slot != 0 && ability == ABILITY_NONE)
            continue;
        if (filter != NULL && !filter(ability))
            continue;
        effect = AbilityBattleEffects(caseID, battler, ability, move, shouldAbilityTrigger);
        if (effect)
        {
            if (slot < MAX_MON_INNATES)
            {
                sSlotCursor[battler] = slot + 1;
                *moreSlotsPending = TRUE;
            }
            else
            {
                sSlotCursor[battler] = 0;
            }
            return effect;
        }
    }

    sSlotCursor[battler] = 0;
    return 0;
}

// "First match wins": the first ability slot whose ability is in list, or mainAbility if none is.
// Used by switch blocks that decide something and return, instead of stacking effects.
enum Ability GetFirstAbilitySlotInList(enum BattlerId battler, enum Ability mainAbility, const u16 *list, u32 count)
{
    u32 slot, i, slotCount = GetInnateSlotCount(battler);
    for (slot = 0; slot <= slotCount; slot++)
    {
        enum Ability ability = (slot == 0) ? mainAbility : GetBattlerAbilityInSlot(battler, mainAbility, slot);
        if (ability == ABILITY_NONE)
            continue;
        for (i = 0; i < count; i++)
        {
            if (list[i] == ability)
                return ability;
        }
    }
    return mainAbility;
}

bool32 SpeciesHasInnate(enum Species species, enum Ability ability)
{
    u32 i;

    if (ability == ABILITY_NONE)
        return FALSE;
    for (i = 0; i < MAX_MON_INNATES; i++)
    {
        if (GetSpeciesInnate(species, i) == ability)
            return TRUE;
    }
    return FALSE;
}

// Out of battle (field effects, Pickup, hatching...): the Pokémon's regular
// ability or any of its innates. Nothing suppresses abilities out of battle.
bool32 MonHasAbility(struct Pokemon *mon, enum Ability ability)
{
    return GetMonAbility(mon) == ability || SpeciesHasInnate(GetMonData(mon, MON_DATA_SPECIES), ability);
}
