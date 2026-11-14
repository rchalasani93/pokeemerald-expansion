#include "global.h"
#include "roguelite.h"
#include "battle.h"
#include "battle_setup.h"
#include "event_data.h"
#include "item.h"
#include "list_menu.h"
#include "malloc.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "script_menu.h"
#include "script_pokemon_util.h"
#include "string_util.h"
#include "constants/battle.h"
#include "constants/items.h"
#include "constants/species.h"

// ---------------------------------------------------------------------------
// Data
// ---------------------------------------------------------------------------

struct RogueStarter
{
    u16 species;
    u8 cost;
};

// Draft pool. Cost reflects how strong the fully evolved line ends up,
// so a team is a trade-off between a few strong lines or many weaker ones.
static const struct RogueStarter sStarterPool[] =
{
    {SPECIES_BULBASAUR,  3}, {SPECIES_CHARMANDER, 3}, {SPECIES_SQUIRTLE,  3},
    {SPECIES_CHIKORITA,  3}, {SPECIES_CYNDAQUIL,  3}, {SPECIES_TOTODILE,  3},
    {SPECIES_TREECKO,    3}, {SPECIES_TORCHIC,    3}, {SPECIES_MUDKIP,    3},
    {SPECIES_PIKACHU,    2}, {SPECIES_EEVEE,      3}, {SPECIES_MAGIKARP,  3},
    {SPECIES_ABRA,       3}, {SPECIES_GASTLY,     3}, {SPECIES_MACHOP,    3},
    {SPECIES_GEODUDE,    2}, {SPECIES_ZUBAT,      2}, {SPECIES_PIDGEY,    1},
    {SPECIES_RATTATA,    1}, {SPECIES_CATERPIE,   1}, {SPECIES_WEEDLE,    1},
    {SPECIES_ODDISH,     2}, {SPECIES_POLIWAG,    2}, {SPECIES_GROWLITHE, 3},
    {SPECIES_PONYTA,     2}, {SPECIES_ZIGZAGOON,  1}, {SPECIES_POOCHYENA, 1},
    {SPECIES_WURMPLE,    1}, {SPECIES_LOTAD,      2}, {SPECIES_SEEDOT,    2},
    {SPECIES_TAILLOW,    2}, {SPECIES_WINGULL,    1}, {SPECIES_RALTS,     4},
    {SPECIES_SHROOMISH,  3}, {SPECIES_ARON,       4}, {SPECIES_ELECTRIKE, 2},
    {SPECIES_TRAPINCH,   4}, {SPECIES_SWABLU,     3}, {SPECIES_SNORUNT,   2},
    {SPECIES_SPHEAL,     3}, {SPECIES_HOUNDOUR,   3}, {SPECIES_SNEASEL,   3},
    {SPECIES_SCYTHER,    5}, {SPECIES_LAPRAS,     5}, {SPECIES_SNORLAX,   6},
    {SPECIES_DRATINI,    6}, {SPECIES_LARVITAR,   6}, {SPECIES_BAGON,     6},
    {SPECIES_BELDUM,     6}, {SPECIES_ABSOL,      4},
};

struct RogueBiome
{
    const u8 *name;
    enum BattleEnvironments environment;
    const u16 *wild;
    u8 wildCount;
    const u16 *bosses;
    u8 bossCount;
};

static const u16 sGrasslandWild[] = {
    SPECIES_ZIGZAGOON, SPECIES_POOCHYENA, SPECIES_WURMPLE, SPECIES_TAILLOW, SPECIES_SHROOMISH,
    SPECIES_SEEDOT, SPECIES_LOTAD, SPECIES_PIDGEY, SPECIES_RATTATA, SPECIES_ODDISH,
    SPECIES_BELLSPROUT, SPECIES_HOPPIP, SPECIES_SENTRET, SPECIES_NINCADA, SPECIES_SKITTY,
    SPECIES_ELECTRIKE, SPECIES_PLUSLE, SPECIES_MINUN, SPECIES_RALTS, SPECIES_SURSKIT,
};
static const u16 sGrasslandBoss[] = {
    SPECIES_SHROOMISH, SPECIES_ELECTRIKE, SPECIES_TAILLOW, SPECIES_POOCHYENA, SPECIES_SCYTHER,
};

static const u16 sCaveWild[] = {
    SPECIES_ZUBAT, SPECIES_GEODUDE, SPECIES_MAKUHITA, SPECIES_ARON, SPECIES_SABLEYE,
    SPECIES_MAWILE, SPECIES_NOSEPASS, SPECIES_ONIX, SPECIES_MACHOP, SPECIES_WHISMUR,
    SPECIES_DIGLETT, SPECIES_PARAS, SPECIES_CUBONE, SPECIES_SANDSHREW, SPECIES_TRAPINCH,
};
static const u16 sCaveBoss[] = {
    SPECIES_ARON, SPECIES_ONIX, SPECIES_MACHOP, SPECIES_LARVITAR, SPECIES_TRAPINCH,
};

static const u16 sSeaWild[] = {
    SPECIES_TENTACOOL, SPECIES_WINGULL, SPECIES_MARILL, SPECIES_CORPHISH, SPECIES_CARVANHA,
    SPECIES_WAILMER, SPECIES_STARYU, SPECIES_HORSEA, SPECIES_SHELLDER, SPECIES_KRABBY,
    SPECIES_PSYDUCK, SPECIES_GOLDEEN, SPECIES_MAGIKARP, SPECIES_CHINCHOU, SPECIES_SPHEAL,
};
static const u16 sSeaBoss[] = {
    SPECIES_WAILMER, SPECIES_CARVANHA, SPECIES_LAPRAS, SPECIES_MAGIKARP, SPECIES_SPHEAL,
};

static const u16 sVolcanoWild[] = {
    SPECIES_NUMEL, SPECIES_SLUGMA, SPECIES_TORKOAL, SPECIES_VULPIX, SPECIES_GROWLITHE,
    SPECIES_PONYTA, SPECIES_MAGBY, SPECIES_HOUNDOUR, SPECIES_KOFFING, SPECIES_GRIMER,
    SPECIES_SPOINK, SPECIES_GEODUDE, SPECIES_CHARMANDER, SPECIES_TORCHIC,
};
static const u16 sVolcanoBoss[] = {
    SPECIES_HOUNDOUR, SPECIES_GROWLITHE, SPECIES_NUMEL, SPECIES_TORKOAL, SPECIES_MAGBY,
};

static const u16 sSkyWild[] = {
    SPECIES_SWABLU, SPECIES_BAGON, SPECIES_ABSOL, SPECIES_DRATINI, SPECIES_BELDUM,
    SPECIES_SHUPPET, SPECIES_DUSKULL, SPECIES_CHIMECHO, SPECIES_NATU, SPECIES_SNEASEL,
    SPECIES_MISDREAVUS, SPECIES_LARVITAR, SPECIES_GASTLY,
};
static const u16 sSkyBoss[] = {
    SPECIES_BAGON, SPECIES_BELDUM, SPECIES_DRATINI, SPECIES_ABSOL, SPECIES_SWABLU,
};

#define BIOME(n, env, w, b) { .name = COMPOUND_STRING(n), .environment = env, .wild = w, .wildCount = ARRAY_COUNT(w), .bosses = b, .bossCount = ARRAY_COUNT(b) }

static const struct RogueBiome sBiomes[ROGUELITE_BIOME_COUNT] =
{
    BIOME("Grassland",  BATTLE_ENVIRONMENT_GRASS,    sGrasslandWild, sGrasslandBoss),
    BIOME("Cave",       BATTLE_ENVIRONMENT_CAVE,     sCaveWild,      sCaveBoss),
    BIOME("Open Sea",   BATTLE_ENVIRONMENT_WATER,    sSeaWild,       sSeaBoss),
    BIOME("Volcano",    BATTLE_ENVIRONMENT_MOUNTAIN, sVolcanoWild,   sVolcanoBoss),
    BIOME("Sky Pillar", BATTLE_ENVIRONMENT_RAYQUAZA, sSkyWild,       sSkyBoss),
};

#define ROGUELITE_FINAL_BOSS SPECIES_RAYQUAZA

enum RewardTier
{
    TIER_COMMON,
    TIER_GREAT,
    TIER_ULTRA,
    TIER_ROGUE,
    TIER_COUNT,
};

#define REWARD_HEAL_PARTY ITEM_NONE

struct RogueReward
{
    u16 item;   // REWARD_HEAL_PARTY = full party heal instead of an item
    u8 quantity;
    u8 tier;
};

static const struct RogueReward sRewards[] =
{
    {REWARD_HEAL_PARTY,    1, TIER_COMMON},
    {ITEM_POKE_BALL,       5, TIER_COMMON},
    {ITEM_POTION,          3, TIER_COMMON},
    {ITEM_SUPER_POTION,    2, TIER_COMMON},
    {ITEM_FULL_HEAL,       2, TIER_COMMON},
    {ITEM_RARE_CANDY,      1, TIER_COMMON},
    {ITEM_SITRUS_BERRY,    1, TIER_COMMON},
    {ITEM_ETHER,           2, TIER_COMMON},

    {ITEM_GREAT_BALL,      5, TIER_GREAT},
    {ITEM_HYPER_POTION,    2, TIER_GREAT},
    {ITEM_REVIVE,          2, TIER_GREAT},
    {ITEM_RARE_CANDY,      3, TIER_GREAT},
    {ITEM_FIRE_STONE,      1, TIER_GREAT},
    {ITEM_WATER_STONE,     1, TIER_GREAT},
    {ITEM_THUNDER_STONE,   1, TIER_GREAT},
    {ITEM_LEAF_STONE,      1, TIER_GREAT},
    {ITEM_MOON_STONE,      1, TIER_GREAT},
    {ITEM_LINKING_CORD,    1, TIER_GREAT},
    {ITEM_SCOPE_LENS,      1, TIER_GREAT},
    {ITEM_SHELL_BELL,      1, TIER_GREAT},

    {ITEM_ULTRA_BALL,      5, TIER_ULTRA},
    {ITEM_FULL_RESTORE,    2, TIER_ULTRA},
    {ITEM_MAX_REVIVE,      1, TIER_ULTRA},
    {ITEM_RARE_CANDY,      5, TIER_ULTRA},
    {ITEM_LEFTOVERS,       1, TIER_ULTRA},
    {ITEM_LIFE_ORB,        1, TIER_ULTRA},
    {ITEM_CHOICE_BAND,     1, TIER_ULTRA},
    {ITEM_CHOICE_SPECS,    1, TIER_ULTRA},
    {ITEM_CHOICE_SCARF,    1, TIER_ULTRA},
    {ITEM_FOCUS_SASH,      1, TIER_ULTRA},
    {ITEM_LUCKY_EGG,       1, TIER_ULTRA},
    {ITEM_EVIOLITE,        1, TIER_ULTRA},
    {ITEM_PP_UP,           1, TIER_ULTRA},

    {ITEM_MASTER_BALL,     1, TIER_ROGUE},
    {ITEM_ABILITY_CAPSULE, 1, TIER_ROGUE},
    {ITEM_MAX_ELIXIR,      3, TIER_ROGUE},
};

static const u8 sText_HealParty[] = _("Full party heal");
static const u8 sText_StartRun[] = _("Start the run!");
static const u8 sText_Reroll[] = _("Reroll choices");
static const u8 sText_TierTags[TIER_COUNT][8] = {
    [TIER_COMMON] = _(""),
    [TIER_GREAT]  = _("{STAR}"),
    [TIER_ULTRA]  = _("{STAR}{STAR}"),
    [TIER_ROGUE]  = _("{STAR}{STAR}{STAR}"),
};

// ---------------------------------------------------------------------------
// Volatile state (only lives between a menu being built and being answered)
// ---------------------------------------------------------------------------

EWRAM_DATA static u8 sStarterChoices[ROGUELITE_STARTER_CHOICES] = {0};
EWRAM_DATA static u8 sRewardChoices[ROGUELITE_REWARD_CHOICES] = {0};
EWRAM_DATA static bool8 sWaveIsDouble = FALSE;

static const u16 sStashVars[PARTY_SIZE] =
{
    VAR_ROGUELITE_STASH_0, VAR_ROGUELITE_STASH_1, VAR_ROGUELITE_STASH_2,
    VAR_ROGUELITE_STASH_3, VAR_ROGUELITE_STASH_4, VAR_ROGUELITE_STASH_5,
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

bool32 Roguelite_IsRunActive(void)
{
    return FlagGet(FLAG_ROGUELITE_ACTIVE);
}

static u32 GetWave(void)
{
    return VarGet(VAR_ROGUELITE_WAVE);
}

static u32 GetBiomeIdForWave(u32 wave)
{
    u32 biome = (wave > 0 ? wave - 1 : 0) / ROGUELITE_WAVES_PER_BIOME;
    return min(biome, ROGUELITE_BIOME_COUNT - 1);
}

static bool32 IsBossWave(u32 wave)
{
    return (wave % ROGUELITE_WAVES_PER_BIOME) == 0;
}

static u32 GetWaveLevel(u32 wave)
{
    u32 level = 3 + (wave * 11) / 10;
    return min(level, MAX_LEVEL);
}

enum BattleEnvironments Roguelite_GetBattleEnvironment(void)
{
    return sBiomes[GetBiomeIdForWave(GetWave())].environment;
}

static u32 CountUsablePartyMons(void)
{
    u32 i, count = 0;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        if (GetMonData(mon, MON_DATA_SPECIES) != SPECIES_NONE
         && !GetMonData(mon, MON_DATA_IS_EGG)
         && GetMonData(mon, MON_DATA_HP) != 0)
            count++;
    }
    return count;
}

static bool32 IsSpeciesInParty(u32 species)
{
    u32 i;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES) == species)
            return TRUE;
    }
    return FALSE;
}

// Walks a species up its evolution line so wild mons match their level.
// Non-level evolutions (stones, trade, friendship) kick in at higher waves.
static u32 EvolveForLevel(u32 species, u32 level)
{
    u32 stage;
    for (stage = 0; stage < 3; stage++)
    {
        const struct Evolution *evos = GetSpeciesEvolutions(species);
        u32 next = SPECIES_NONE;
        u32 i;

        if (evos == NULL)
            break;

        for (i = 0; evos[i].method != EVOLUTIONS_END; i++)
        {
            u32 target = evos[i].targetSpecies;
            if (!IsSpeciesEnabled(target))
                continue;
            if (evos[i].method == EVO_LEVEL && evos[i].param != 0)
            {
                if (level >= evos[i].param)
                {
                    next = target;
                    break;
                }
            }
            else if (evos[i].method == EVO_LEVEL || evos[i].method == EVO_ITEM || evos[i].method == EVO_TRADE)
            {
                if (level >= 30 + stage * 8)
                {
                    next = target;
                    break;
                }
            }
        }

        if (next == SPECIES_NONE)
            break;
        species = next;
    }
    return species;
}

static u32 PickFromPool(const u16 *pool, u32 count)
{
    u32 tries;
    for (tries = 0; tries < 8; tries++)
    {
        u32 species = pool[Random() % count];
        if (IsSpeciesEnabled(species))
            return species;
    }
    return SPECIES_ZIGZAGOON;
}

static void MakeBoss(struct Pokemon *mon)
{
    u32 iv = MAX_PER_STAT_IVS;
    u32 item = ITEM_SITRUS_BERRY;
    SetMonData(mon, MON_DATA_HP_IV, &iv);
    SetMonData(mon, MON_DATA_ATK_IV, &iv);
    SetMonData(mon, MON_DATA_DEF_IV, &iv);
    SetMonData(mon, MON_DATA_SPEED_IV, &iv);
    SetMonData(mon, MON_DATA_SPATK_IV, &iv);
    SetMonData(mon, MON_DATA_SPDEF_IV, &iv);
    SetMonData(mon, MON_DATA_HELD_ITEM, &item);
    CalculateMonStats(mon);
}

static u8 *AllocMenuString(void)
{
    return Alloc(100);
}

static void PushMenuItem(u8 *name, u32 id)
{
    struct ListMenuItem item;
    item.name = name;
    item.id = id;
    MultichoiceDynamic_PushElement(item);
}

// ---------------------------------------------------------------------------
// Specials: run lifecycle
// ---------------------------------------------------------------------------

// VAR_RESULT = wave number if a run is in progress, else 0.
void Roguelite_GetRunState(void)
{
    gSpecialVar_Result = Roguelite_IsRunActive() ? max(GetWave(), 1) : 0;
}

// Parks the current party in the PC and starts a fresh run.
// VAR_RESULT = FALSE if the PC had no room.
void Roguelite_StartRun(void)
{
    u32 i, freeSlots = 0, box, pos;

    for (box = 0; box < TOTAL_BOXES_COUNT; box++)
    {
        for (pos = 0; pos < IN_BOX_COUNT; pos++)
        {
            if (GetBoxMonDataAt(box, pos, MON_DATA_SPECIES) == SPECIES_NONE)
                freeSlots++;
        }
    }
    if (freeSlots < CalculatePlayerPartyCount())
    {
        gSpecialVar_Result = FALSE;
        return;
    }

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        VarSet(sStashVars[i], 0);
        if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE)
            continue;
        if (CopyMonToPC(mon) == MON_GIVEN_TO_PC)
            VarSet(sStashVars[i], gSpecialVar_MonBoxId * IN_BOX_COUNT + gSpecialVar_MonBoxPos + 1);
    }
    ZeroPlayerPartyMons();
    CalculatePlayerPartyCount();

    VarSet(VAR_ROGUELITE_WAVE, 0);
    VarSet(VAR_ROGUELITE_BUDGET, ROGUELITE_STARTER_BUDGET);
    FlagSet(FLAG_ROGUELITE_ACTIVE);
    AddBagItem(ITEM_POKE_BALL, 5);
    AddBagItem(ITEM_POTION, 3);
    gSpecialVar_Result = TRUE;
}

// Ends the run: run party is released, parked party comes back.
// gStringVar1 = wave reached, VAR_RESULT = TRUE if it's a new best.
void Roguelite_EndRun(void)
{
    u32 i, wave = GetWave();

    gSpecialVar_Result = FALSE;
    if (wave > VarGet(VAR_ROGUELITE_BEST_WAVE))
    {
        VarSet(VAR_ROGUELITE_BEST_WAVE, wave);
        gSpecialVar_Result = TRUE;
    }
    ConvertIntToDecimalStringN(gStringVar1, wave, STR_CONV_MODE_LEFT_ALIGN, 3);

    ZeroPlayerPartyMons();
    for (i = 0; i < PARTY_SIZE; i++)
    {
        u32 slot = VarGet(sStashVars[i]);
        VarSet(sStashVars[i], 0);
        if (slot == 0)
            continue;
        slot--;
        if (GetBoxMonDataAt(slot / IN_BOX_COUNT, slot % IN_BOX_COUNT, MON_DATA_SPECIES) == SPECIES_NONE)
            continue;
        BoxMonToMon(GetBoxedMonPtr(slot / IN_BOX_COUNT, slot % IN_BOX_COUNT), &gParties[B_TRAINER_PLAYER][i]);
        ZeroBoxMonAt(slot / IN_BOX_COUNT, slot % IN_BOX_COUNT);
    }
    CompactPartySlots();
    CalculatePlayerPartyCount();
    HealPlayerParty();

    VarSet(VAR_ROGUELITE_WAVE, 0);
    FlagClear(FLAG_ROGUELITE_ACTIVE);
}

// ---------------------------------------------------------------------------
// Specials: starter draft
// ---------------------------------------------------------------------------

// Rolls ROGUELITE_STARTER_CHOICES options and pushes them for dynmultistack.
// gStringVar1 = points left.
void Roguelite_BuildStarterMenu(void)
{
    u32 i, j;
    u32 budget = VarGet(VAR_ROGUELITE_BUDGET);

    for (i = 0; i < ROGUELITE_STARTER_CHOICES; i++)
    {
        u32 tries, choice = 0;
        for (tries = 0; tries < 20; tries++)
        {
            bool32 dupe = FALSE;
            choice = Random() % ARRAY_COUNT(sStarterPool);
            for (j = 0; j < i; j++)
                if (sStarterChoices[j] == choice)
                    dupe = TRUE;
            if (!dupe && IsSpeciesEnabled(sStarterPool[choice].species) && !IsSpeciesInParty(sStarterPool[choice].species))
                break;
        }
        sStarterChoices[i] = choice;
    }

    for (i = 0; i < ROGUELITE_STARTER_CHOICES; i++)
    {
        const struct RogueStarter *starter = &sStarterPool[sStarterChoices[i]];
        u8 *str = AllocMenuString();
        u8 *end = StringCopy(str, GetSpeciesName(starter->species));
        *end++ = CHAR_SPACE;
        *end++ = CHAR_LEFT_PAREN;
        end = ConvertIntToDecimalStringN(end, starter->cost, STR_CONV_MODE_LEFT_ALIGN, 2);
        *end++ = CHAR_RIGHT_PAREN;
        *end = EOS;
        PushMenuItem(str, i);
    }

    if (CalculatePlayerPartyCount() > 0)
    {
        u8 *str = AllocMenuString();
        StringCopy(str, sText_StartRun);
        PushMenuItem(str, ROGUELITE_MENU_START_RUN);
    }
    {
        u8 *str = AllocMenuString();
        StringCopy(str, sText_Reroll);
        PushMenuItem(str, ROGUELITE_MENU_REROLL);
    }

    ConvertIntToDecimalStringN(gStringVar1, budget, STR_CONV_MODE_LEFT_ALIGN, 2);
}

// Reads the menu choice from VAR_RESULT and applies it. gStringVar2 = species name.
void Roguelite_PickStarter(void)
{
    u32 choice = gSpecialVar_Result;
    u32 budget = VarGet(VAR_ROGUELITE_BUDGET);
    const struct RogueStarter *starter;

    if (choice == ROGUELITE_MENU_START_RUN)
    {
        gSpecialVar_Result = CalculatePlayerPartyCount() > 0 ? ROGUELITE_PICK_DONE : ROGUELITE_PICK_NEED_ONE;
        return;
    }
    if (choice == ROGUELITE_MENU_REROLL || choice >= ROGUELITE_STARTER_CHOICES)
    {
        gSpecialVar_Result = ROGUELITE_PICK_REROLL;
        return;
    }

    starter = &sStarterPool[sStarterChoices[choice]];
    StringCopy(gStringVar2, GetSpeciesName(starter->species));
    if (starter->cost > budget)
    {
        gSpecialVar_Result = ROGUELITE_PICK_TOO_EXPENSIVE;
        return;
    }

    ScriptGiveMon(starter->species, ROGUELITE_STARTER_LEVEL, ITEM_NONE);
    budget -= starter->cost;
    VarSet(VAR_ROGUELITE_BUDGET, budget);

    if (budget == 0 || CalculatePlayerPartyCount() >= PARTY_SIZE)
        gSpecialVar_Result = ROGUELITE_PICK_DONE;
    else
        gSpecialVar_Result = ROGUELITE_PICK_CONTINUE;
}

// ---------------------------------------------------------------------------
// Specials: waves
// ---------------------------------------------------------------------------

// Advances to the next wave and builds its enemy party.
// gStringVar1 = wave, gStringVar2 = biome name. VAR_RESULT: 0 normal, 1 boss, 2 final boss.
// VAR_0x8004 = TRUE if this wave enters a new biome.
void Roguelite_PrepareWave(void)
{
    u32 wave = GetWave() + 1;
    const struct RogueBiome *biome = &sBiomes[GetBiomeIdForWave(wave)];
    u32 level = GetWaveLevel(wave);
    u32 species;

    VarSet(VAR_ROGUELITE_WAVE, wave);
    ConvertIntToDecimalStringN(gStringVar1, wave, STR_CONV_MODE_LEFT_ALIGN, 3);
    StringCopy(gStringVar2, biome->name);
    gSpecialVar_0x8004 = (wave % ROGUELITE_WAVES_PER_BIOME) == 1;
    sWaveIsDouble = FALSE;

    if (wave >= ROGUELITE_FINAL_WAVE)
    {
        CreateScriptedWildMon(ROGUELITE_FINAL_BOSS, min(level + 5, MAX_LEVEL), ITEM_NONE);
        MakeBoss(&gParties[B_TRAINER_OPPONENT_A][0]);
        gSpecialVar_Result = 2;
        return;
    }

    if (IsBossWave(wave))
    {
        species = EvolveForLevel(PickFromPool(biome->bosses, biome->bossCount), level + 3);
        CreateScriptedWildMon(species, min(level + 3, MAX_LEVEL), ITEM_NONE);
        MakeBoss(&gParties[B_TRAINER_OPPONENT_A][0]);
        gSpecialVar_Result = 1;
        return;
    }

    species = EvolveForLevel(PickFromPool(biome->wild, biome->wildCount), level);
    if (CountUsablePartyMons() >= 2 && (Random() % 100) < 15)
    {
        u32 species2 = EvolveForLevel(PickFromPool(biome->wild, biome->wildCount), level - 1);
        CreateScriptedDoubleWildMon(species, level, ITEM_NONE, species2, level - 1, ITEM_NONE);
        sWaveIsDouble = TRUE;
    }
    else
    {
        CreateScriptedWildMon(species, level, ITEM_NONE);
    }
    gSpecialVar_Result = 0;
}

// Starts the battle built by Roguelite_PrepareWave. Follow with waitstate.
void Roguelite_StartWaveBattle(void)
{
    if (sWaveIsDouble)
        BattleSetup_StartScriptedDoubleWildBattle();
    else
        BattleSetup_StartScriptedWildBattle();
}

// Evaluates the wave that just finished. VAR_RESULT = ROGUELITE_WAVE_*.
// VAR_0x8004 = TRUE if it was a boss wave. Boss waves fully heal the party when cleared.
void Roguelite_AfterWave(void)
{
    u32 wave = GetWave();

    gSpecialVar_0x8004 = IsBossWave(wave);
    switch (gBattleOutcome)
    {
    case B_OUTCOME_LOST:
    case B_OUTCOME_DREW:
    case B_OUTCOME_FORFEITED:
        gSpecialVar_Result = ROGUELITE_WAVE_LOST;
        return;
    case B_OUTCOME_RAN:
    case B_OUTCOME_PLAYER_TELEPORTED:
        if (IsBossWave(wave))
        {
            // You can't run from a boss: retry the wave.
            VarSet(VAR_ROGUELITE_WAVE, wave - 1);
            gSpecialVar_Result = ROGUELITE_WAVE_BOSS_FLED;
            return;
        }
        break;
    default:
        break;
    }

    if (wave >= ROGUELITE_FINAL_WAVE)
    {
        gSpecialVar_Result = ROGUELITE_WAVE_RUN_WON;
        return;
    }
    if (IsBossWave(wave))
        HealPlayerParty();
    gSpecialVar_Result = ROGUELITE_WAVE_CLEARED;
}

// ---------------------------------------------------------------------------
// Specials: rewards
// ---------------------------------------------------------------------------

static u32 RollRewardTier(u32 wave)
{
    u32 roll = Random() % 100;
    u32 rogue = 1 + wave / 10;
    u32 ultra = 5 + wave / 2;
    u32 great = 25 + wave / 2;

    if (roll < rogue)
        return TIER_ROGUE;
    if (roll < rogue + ultra)
        return TIER_ULTRA;
    if (roll < rogue + ultra + great)
        return TIER_GREAT;
    return TIER_COMMON;
}

static u32 RollRewardOfTier(u32 tier)
{
    u32 i, count = 0, pick;
    for (i = 0; i < ARRAY_COUNT(sRewards); i++)
        if (sRewards[i].tier == tier)
            count++;
    pick = Random() % count;
    for (i = 0; i < ARRAY_COUNT(sRewards); i++)
    {
        if (sRewards[i].tier != tier)
            continue;
        if (pick-- == 0)
            return i;
    }
    return 0;
}

static void FormatReward(u8 *dst, const struct RogueReward *reward)
{
    dst = StringCopy(dst, sText_TierTags[reward->tier]);
    if (reward->item == REWARD_HEAL_PARTY)
    {
        StringCopy(dst, sText_HealParty);
        return;
    }
    if (reward->quantity > 1)
    {
        dst = ConvertIntToDecimalStringN(dst, reward->quantity, STR_CONV_MODE_LEFT_ALIGN, 2);
        *dst++ = CHAR_x;
        *dst++ = CHAR_SPACE;
    }
    CopyItemNameHandlePlural(reward->item, dst, reward->quantity);
}

// Rolls ROGUELITE_REWARD_CHOICES distinct rewards and pushes them for dynmultistack.
void Roguelite_BuildRewardMenu(void)
{
    u32 i, j, wave = GetWave();

    for (i = 0; i < ROGUELITE_REWARD_CHOICES; i++)
    {
        u32 tries, choice = 0;
        for (tries = 0; tries < 20; tries++)
        {
            bool32 dupe = FALSE;
            choice = RollRewardOfTier(RollRewardTier(wave));
            for (j = 0; j < i; j++)
                if (sRewardChoices[j] == choice)
                    dupe = TRUE;
            if (!dupe)
                break;
        }
        sRewardChoices[i] = choice;
    }

    for (i = 0; i < ROGUELITE_REWARD_CHOICES; i++)
    {
        u8 *str = AllocMenuString();
        FormatReward(str, &sRewards[sRewardChoices[i]]);
        PushMenuItem(str, i);
    }
}

// Gives the reward picked in VAR_RESULT. gStringVar1 = reward text.
// VAR_RESULT = FALSE if the bag had no room.
void Roguelite_GiveReward(void)
{
    u32 choice = gSpecialVar_Result;
    const struct RogueReward *reward;

    if (choice >= ROGUELITE_REWARD_CHOICES)
        choice = 0;
    reward = &sRewards[sRewardChoices[choice]];
    FormatReward(gStringVar1, reward);

    if (reward->item == REWARD_HEAL_PARTY)
    {
        HealPlayerParty();
        gSpecialVar_Result = TRUE;
        return;
    }
    gSpecialVar_Result = AddBagItem(reward->item, reward->quantity);
}
