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
#include "party_menu.h"
#include "pokedex.h"
#include "random.h"
#include "randomizer.h"
#include "script_menu.h"
#include "script_pokemon_util.h"
#include "string_util.h"
#include "constants/battle.h"
#include "constants/items.h"
#include "constants/species.h"
#include "constants/moves.h"
#include "constants/opponents.h"
#include "move.h"

// ---------------------------------------------------------------------------
// Data
// ---------------------------------------------------------------------------

struct RogueStarter
{
    u16 species;
    u8 cost;
};

// The 27 classic starters (Gen 1-9) are always draftable. Like PokeRogue, they
// cost 3 points, or 4 for the nine lines whose final form is strongest
// (base stat total plus best attacking stat and Speed).
static const struct RogueStarter sStarterPool[] =
{
    {SPECIES_BULBASAUR,  3}, {SPECIES_CHARMANDER, 4}, {SPECIES_SQUIRTLE,   3},
    {SPECIES_CHIKORITA,  3}, {SPECIES_CYNDAQUIL,  4}, {SPECIES_TOTODILE,   3},
    {SPECIES_TREECKO,    4}, {SPECIES_TORCHIC,    3}, {SPECIES_MUDKIP,     3},
    {SPECIES_TURTWIG,    3}, {SPECIES_CHIMCHAR,   4}, {SPECIES_PIPLUP,     3},
    {SPECIES_SNIVY,      3}, {SPECIES_TEPIG,      3}, {SPECIES_OSHAWOTT,   3},
    {SPECIES_CHESPIN,    3}, {SPECIES_FENNEKIN,   4}, {SPECIES_FROAKIE,    4},
    {SPECIES_ROWLET,     3}, {SPECIES_LITTEN,     3}, {SPECIES_POPPLIO,    3},
    {SPECIES_GROOKEY,    3}, {SPECIES_SCORBUNNY,  4}, {SPECIES_SOBBLE,     4},
    {SPECIES_SPRIGATITO, 4}, {SPECIES_FUECOCO,    3}, {SPECIES_QUAXLY,     3},
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
static const u8 sText_NextPage[] = _("Next page");
static const u8 sText_PrevPage[] = _("Previous page");
static const u8 sText_TierTags[TIER_COUNT][8] = {
    [TIER_COMMON] = _(""),
    [TIER_GREAT]  = _("{STAR}"),
    [TIER_ULTRA]  = _("{STAR}{STAR}"),
    [TIER_ROGUE]  = _("{STAR}{STAR}{STAR}"),
};

// ---------------------------------------------------------------------------
// Volatile state (only lives between a menu being built and being answered)
// ---------------------------------------------------------------------------

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

    if (Randomizer_IsEnabled(RANDOMIZE_RUN_POKEMON))
        return Randomizer_GetRunSpecies(count);
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

static u32 GetDraftBudget(void)
{
    return ROGUELITE_STARTER_BUDGET + VarGet(VAR_ROGUE_HUB_DRAFT_LEVEL) * ROGUELITE_DRAFT_OFFICE_BONUS;
}

// Supply Depot (hub area): extra starting items per level.
static void GiveSupplyDepotItems(void)
{
    u32 level = VarGet(VAR_ROGUE_HUB_SUPPLY_LEVEL);
    if (level >= 1)
    {
        AddBagItem(ITEM_POKE_BALL, 5);
        AddBagItem(ITEM_SUPER_POTION, 3);
    }
    if (level >= 2)
    {
        AddBagItem(ITEM_GREAT_BALL, 5);
        AddBagItem(ITEM_REVIVE, 2);
    }
    if (level >= 3)
    {
        AddBagItem(ITEM_RARE_CANDY, 3);
        AddBagItem(ITEM_FULL_RESTORE, 1);
    }
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
    VarSet(VAR_ROGUELITE_BUDGET, GetDraftBudget());
    VarSet(VAR_ROGUELITE_DRAFT_PAGE, 0);
    FlagSet(FLAG_ROGUELITE_ACTIVE);
    AddBagItem(ITEM_POKE_BALL, 5);
    AddBagItem(ITEM_POTION, 3);
    GiveSupplyDepotItems();
    gSpecialVar_Result = TRUE;
}

// Ends the run: run party is released, parked party comes back.
// VAR_0x8004 = ROGUELITE_END_LOST / _GAVE_UP / _WON.
// gStringVar1 = wave reached, gStringVar2 = Rogue Coins earned, VAR_RESULT = TRUE if it's a new best.
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

    // Rogue Coins for the hub: 5 per cleared wave, 50 per boss beaten, 500 for a win.
    // VAR_0x8004 says how the run ended (ROGUELITE_END_*).
    {
        u32 cleared = (gSpecialVar_0x8004 == ROGUELITE_END_LOST && wave > 0) ? wave - 1 : wave;
        u32 earned = cleared * 5 + (cleared / ROGUELITE_WAVES_PER_BIOME) * 50
                   + (gSpecialVar_0x8004 == ROGUELITE_END_WON ? 500 : 0);
        VarSet(VAR_ROGUE_COINS, min(VarGet(VAR_ROGUE_COINS) + earned, ROGUELITE_MAX_COINS));
        ConvertIntToDecimalStringN(gStringVar2, earned, STR_CONV_MODE_LEFT_ALIGN, 4);
    }

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

// Draft cost of a caught (unlocked) line, from its final form's base stat
// total, roughly like PokeRogue: weak lines are cheap, legends cost most.
static u32 GetFinalEvolution(u32 species)
{
    u32 stage;
    for (stage = 0; stage < 4; stage++)
    {
        const struct Evolution *evos = GetSpeciesEvolutions(species);
        if (evos == NULL || evos[0].method == EVOLUTIONS_END || !IsSpeciesEnabled(evos[0].targetSpecies))
            break;
        species = evos[0].targetSpecies;
    }
    return species;
}

static u32 GetUnlockCost(u32 species)
{
    u32 bst = GetSpeciesBaseStatTotal(GetFinalEvolution(species));

    if (gSpeciesInfo[species].isRestrictedLegendary || gSpeciesInfo[species].isMythical)
        return 8;
    if (gSpeciesInfo[species].isSubLegendary || gSpeciesInfo[species].isUltraBeast || gSpeciesInfo[species].isParadox)
        return 7;
    if (bst <= 360)
        return 1;
    if (bst <= 430)
        return 2;
    if (bst <= 500)
        return 3;
    if (bst <= 540)
        return 4;
    if (bst <= 580)
        return 5;
    return 6;
}

static bool32 IsClassicStarter(u32 species)
{
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sStarterPool); i++)
    {
        if (sStarterPool[i].species == species)
            return TRUE;
    }
    return FALSE;
}

// Fills list with every draftable species: the classic starters, then the
// first stage of every family the player has caught (Pokédex), like
// PokeRogue's unlocks. Returns the count.
#define DRAFT_HAS_PRE_EVO   (1 << 0)
#define DRAFT_FAMILY_CAUGHT (1 << 1)

// The 27 classic starters, then the first stage of every family with a caught
// member, in National Dex order. One pass over the species table (the generic
// pre-evolution and dex-number lookups each scan every species).
static u32 BuildDraftList(struct RogueStarter *list, u32 maxCount)
{
    u8 *marks = AllocZeroed(NUM_SPECIES);
    u32 count, species, pass, i;

    for (count = 0; count < ARRAY_COUNT(sStarterPool) && count < maxCount; count++)
        list[count] = sStarterPool[count];
    if (marks == NULL)
        return count;

    for (species = 1; species < NUM_SPECIES; species++)
    {
        const struct Evolution *evos;

        if (!IsSpeciesEnabled(species))
            continue;
        if (GetSetPokedexFlag(gSpeciesInfo[species].natDexNum, FLAG_GET_CAUGHT))
            marks[species] |= DRAFT_FAMILY_CAUGHT;
        evos = GetSpeciesEvolutions(species);
        for (i = 0; evos != NULL && evos[i].method != EVOLUTIONS_END; i++)
        {
            if (IsSpeciesEnabled(evos[i].targetSpecies))
                marks[SanitizeSpeciesId(evos[i].targetSpecies)] |= DRAFT_HAS_PRE_EVO;
        }
    }
    // A caught evolution unlocks its pre-evolutions (families are at most 3 stages)
    for (pass = 0; pass < 3; pass++)
    {
        for (species = 1; species < NUM_SPECIES; species++)
        {
            const struct Evolution *evos;
            if (marks[species] & DRAFT_FAMILY_CAUGHT || !IsSpeciesEnabled(species))
                continue;
            evos = GetSpeciesEvolutions(species);
            for (i = 0; evos != NULL && evos[i].method != EVOLUTIONS_END; i++)
            {
                if (IsSpeciesEnabled(evos[i].targetSpecies)
                 && marks[SanitizeSpeciesId(evos[i].targetSpecies)] & DRAFT_FAMILY_CAUGHT)
                {
                    marks[species] |= DRAFT_FAMILY_CAUGHT;
                    break;
                }
            }
        }
    }

    for (species = 1; species < NUM_SPECIES && count < maxCount; species++)
    {
        if (marks[species] != DRAFT_FAMILY_CAUGHT // caught family, no pre-evolution
         || GET_BASE_SPECIES_ID(species) != species
         || gSpeciesInfo[species].natDexNum == 0
         || IsClassicStarter(species))
            continue;
        list[count].species = species;
        list[count].cost = GetUnlockCost(species);
        count++;
    }
    Free(marks);
    return count;
}

#define MAX_DRAFT_LIST (ARRAY_COUNT(sStarterPool) + NATIONAL_DEX_COUNT)

// Pushes one page of the draft list for dynmultistack.
// gStringVar1 = points left, gStringVar2 = page, gStringVar3 = page count.
// VAR_0x8004 = number of unlocked (caught) lines on top of the classic starters.
void Roguelite_BuildStarterMenu(void)
{
    struct RogueStarter *list = Alloc(sizeof(struct RogueStarter) * MAX_DRAFT_LIST);
    u32 i, count, pages, page, start;

    if (list == NULL)
        return;
    count = BuildDraftList(list, MAX_DRAFT_LIST);
    pages = (count + ROGUELITE_DRAFT_PAGE_SIZE - 1) / ROGUELITE_DRAFT_PAGE_SIZE;
    page = min(VarGet(VAR_ROGUELITE_DRAFT_PAGE), pages - 1);
    VarSet(VAR_ROGUELITE_DRAFT_PAGE, page);
    start = page * ROGUELITE_DRAFT_PAGE_SIZE;

    for (i = start; i < count && i < start + ROGUELITE_DRAFT_PAGE_SIZE; i++)
    {
        u8 *str = AllocMenuString();
        u8 *end = StringCopy(str, GetSpeciesName(list[i].species));
        *end++ = CHAR_SPACE;
        *end++ = CHAR_LEFT_PAREN;
        end = ConvertIntToDecimalStringN(end, list[i].cost, STR_CONV_MODE_LEFT_ALIGN, 2);
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
    if (page + 1 < pages)
    {
        u8 *str = AllocMenuString();
        StringCopy(str, sText_NextPage);
        PushMenuItem(str, ROGUELITE_MENU_NEXT_PAGE);
    }
    if (page > 0)
    {
        u8 *str = AllocMenuString();
        StringCopy(str, sText_PrevPage);
        PushMenuItem(str, ROGUELITE_MENU_PREV_PAGE);
    }

    ConvertIntToDecimalStringN(gStringVar1, VarGet(VAR_ROGUELITE_BUDGET), STR_CONV_MODE_LEFT_ALIGN, 2);
    ConvertIntToDecimalStringN(gStringVar2, page + 1, STR_CONV_MODE_LEFT_ALIGN, 3);
    ConvertIntToDecimalStringN(gStringVar3, pages, STR_CONV_MODE_LEFT_ALIGN, 3);
    gSpecialVar_0x8004 = count - ARRAY_COUNT(sStarterPool);
    Free(list);
}

// Reads the menu choice from VAR_RESULT and applies it. gStringVar2 = species name.
void Roguelite_PickStarter(void)
{
    u32 choice = gSpecialVar_Result;
    u32 budget = VarGet(VAR_ROGUELITE_BUDGET);
    struct RogueStarter *list, starter;
    u32 count;

    if (choice == ROGUELITE_MENU_START_RUN)
    {
        gSpecialVar_Result = CalculatePlayerPartyCount() > 0 ? ROGUELITE_PICK_DONE : ROGUELITE_PICK_NEED_ONE;
        return;
    }
    if (choice == ROGUELITE_MENU_NEXT_PAGE || choice == ROGUELITE_MENU_PREV_PAGE)
    {
        u32 page = VarGet(VAR_ROGUELITE_DRAFT_PAGE);
        VarSet(VAR_ROGUELITE_DRAFT_PAGE, choice == ROGUELITE_MENU_NEXT_PAGE ? page + 1 : (page > 0 ? page - 1 : 0));
        gSpecialVar_Result = ROGUELITE_PICK_PAGE;
        return;
    }

    list = Alloc(sizeof(struct RogueStarter) * MAX_DRAFT_LIST);
    if (list == NULL)
    {
        gSpecialVar_Result = ROGUELITE_PICK_PAGE;
        return;
    }
    count = BuildDraftList(list, MAX_DRAFT_LIST);
    if (choice >= count) // B pressed (MULTI_B_PRESSED) or a stale id
    {
        Free(list);
        gSpecialVar_Result = ROGUELITE_PICK_PAGE;
        return;
    }
    starter = list[choice];
    Free(list);

    StringCopy(gStringVar2, GetSpeciesName(starter.species));
    if (IsSpeciesInParty(starter.species))
    {
        gSpecialVar_Result = ROGUELITE_PICK_ALREADY;
        return;
    }
    if (starter.cost > budget)
    {
        gSpecialVar_Result = ROGUELITE_PICK_TOO_EXPENSIVE;
        return;
    }

    ScriptGiveMon(starter.species, ROGUELITE_STARTER_LEVEL, ITEM_NONE);
    budget -= starter.cost;
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
        gSpecialVar_Result = ROGUELITE_WAVE_KIND_FINAL;
        return;
    }

    if (IsBossWave(wave))
    {
        species = EvolveForLevel(PickFromPool(biome->bosses, biome->bossCount), level + 3);
        CreateScriptedWildMon(species, min(level + 3, MAX_LEVEL), ITEM_NONE);
        MakeBoss(&gParties[B_TRAINER_OPPONENT_A][0]);
        gSpecialVar_Result = ROGUELITE_WAVE_KIND_BOSS;
        return;
    }

    if (VarGet(VAR_ROGUELITE_ROUTE) == ROGUELITE_ROUTE_TRAINER)
    {
        // Party is generated when the battle starts (CreateNPCTrainerParty -> Roguelite_CreateTrainerParty)
        gSpecialVar_Result = ROGUELITE_WAVE_KIND_TRAINER;
        return;
    }
    if (VarGet(VAR_ROGUELITE_ROUTE) == ROGUELITE_ROUTE_REST)
    {
        HealPlayerParty();
        gSpecialVar_Result = ROGUELITE_WAVE_KIND_REST;
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
    gSpecialVar_Result = ROGUELITE_WAVE_KIND_WILD;
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
    if (VarGet(VAR_ROGUELITE_ROUTE) == ROGUELITE_ROUTE_REST && !IsBossWave(wave))
    {
        gSpecialVar_Result = ROGUELITE_WAVE_CLEARED;
        return;
    }
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
            choice = RollRewardOfTier(RollRewardTier(VarGet(VAR_ROGUELITE_ROUTE) == ROGUELITE_ROUTE_TRAINER ? wave + 10 : wave));
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

// ---------------------------------------------------------------------------
// Routes (Emerald Rogue-style path choice)
// ---------------------------------------------------------------------------

static const u8 sText_RouteWild[] = _("Wild route");
static const u8 sText_RouteTrainer[] = _("Trainer route {STAR}");
static const u8 sText_RouteRest[] = _("Rest stop (heal)");
static const u8 sText_RouteBoss[] = _("Face the boss!");

bool32 Roguelite_IsBossWave(void)
{
    return Roguelite_IsRunActive() && IsBossWave(GetWave());
}

// Pushes the route options for the upcoming wave. A boss wave has no choice.
void Roguelite_BuildRouteMenu(void)
{
    u32 next = GetWave() + 1;
    u8 *str;

    if (IsBossWave(next) || next >= ROGUELITE_FINAL_WAVE)
    {
        str = AllocMenuString();
        StringCopy(str, sText_RouteBoss);
        PushMenuItem(str, ROGUELITE_ROUTE_WILD);
        return;
    }
    str = AllocMenuString();
    StringCopy(str, sText_RouteWild);
    PushMenuItem(str, ROGUELITE_ROUTE_WILD);
    str = AllocMenuString();
    StringCopy(str, sText_RouteTrainer);
    PushMenuItem(str, ROGUELITE_ROUTE_TRAINER);
    // A rest stop shows up on roughly every third wave
    if (next % 3 == 0)
    {
        str = AllocMenuString();
        StringCopy(str, sText_RouteRest);
        PushMenuItem(str, ROGUELITE_ROUTE_REST);
    }
}

// Stores the route picked in VAR_RESULT.
void Roguelite_SetRoute(void)
{
    u32 route = gSpecialVar_Result;
    if (route > ROGUELITE_ROUTE_REST || IsBossWave(GetWave() + 1))
        route = ROGUELITE_ROUTE_WILD;
    VarSet(VAR_ROGUELITE_ROUTE, route);
}

// ---------------------------------------------------------------------------
// Generated competitive trainer teams (Elite Redux-style difficulty)
// ---------------------------------------------------------------------------

static const u16 sTrainerHeldItems[] =
{
    ITEM_LEFTOVERS, ITEM_LIFE_ORB, ITEM_CHOICE_BAND, ITEM_CHOICE_SPECS, ITEM_CHOICE_SCARF,
    ITEM_FOCUS_SASH, ITEM_SITRUS_BERRY, ITEM_EXPERT_BELT, ITEM_ASSAULT_VEST, ITEM_ROCKY_HELMET,
    ITEM_LUM_BERRY, ITEM_WEAKNESS_POLICY, ITEM_SHELL_BELL, ITEM_SCOPE_LENS,
};

static const u16 sGoodStatusMoves[] =
{
    MOVE_SWORDS_DANCE, MOVE_DRAGON_DANCE, MOVE_NASTY_PLOT, MOVE_CALM_MIND, MOVE_QUIVER_DANCE,
    MOVE_BULK_UP, MOVE_SHELL_SMASH, MOVE_RECOVER, MOVE_ROOST, MOVE_SLACK_OFF, MOVE_SPORE,
    MOVE_WILL_O_WISP, MOVE_THUNDER_WAVE, MOVE_TOXIC, MOVE_STEALTH_ROCK, MOVE_LEECH_SEED,
    MOVE_PROTECT, MOVE_AGILITY,
};

static bool32 CanSpeciesLearnMove(u32 species, u32 move, u32 level)
{
    const struct LevelUpMove *levelUp = GetSpeciesLevelUpLearnset(species);
    const u16 *teachable = GetSpeciesTeachableLearnset(species);
    u32 i;

    for (i = 0; levelUp != NULL && levelUp[i].move != LEVEL_UP_MOVE_END; i++)
    {
        if (levelUp[i].move == move && levelUp[i].level <= level)
            return TRUE;
    }
    // TMs/tutors are fair game once the run is under way
    if (level >= 15)
    {
        for (i = 0; teachable != NULL && teachable[i] != MOVE_UNAVAILABLE; i++)
        {
            if (teachable[i] == move)
                return TRUE;
        }
    }
    return FALSE;
}

static bool32 IsUsableAttack(u32 move)
{
    switch (GetMoveEffect(move))
    {
    case EFFECT_OHKO:
    case EFFECT_TWO_TURNS_ATTACK:
    case EFFECT_SOLAR_BEAM:
    case EFFECT_SEMI_INVULNERABLE:
    case EFFECT_FOCUS_PUNCH:
    case EFFECT_FUTURE_SIGHT:
    case EFFECT_DREAM_EATER:
    case EFFECT_SNORE:
    case EFFECT_LAST_RESORT:
    case EFFECT_BELCH:
    case EFFECT_FLING:
        return FALSE;
    default:
        break;
    }
    if (IsExplosionMove(move) || MoveHasAdditionalEffectSelf(move, MOVE_EFFECT_RECHARGE))
        return FALSE;
    return GetMovePower(move) >= 40 && GetMoveCategory(move) != DAMAGE_CATEGORY_STATUS;
}

static u32 ScoreAttack(u32 species, u32 move, bool32 physicalAttacker)
{
    u32 score = GetMovePower(move) * max(1, GetMoveStrikeCount(move));
    u32 accuracy = GetMoveAccuracy(move);

    if (accuracy != 0)
        score = score * accuracy / 100;
    if (GetMoveType(move) == GetSpeciesType(species, 0) || GetMoveType(move) == GetSpeciesType(species, 1))
        score = score * 3 / 2;
    if ((GetMoveCategory(move) == DAMAGE_CATEGORY_PHYSICAL) != physicalAttacker)
        score = score * 3 / 5;
    return score;
}

static void CollectLearnableMoves(u32 species, u32 level, u16 *out, u32 *count, u32 maxCount)
{
    const struct LevelUpMove *levelUp = GetSpeciesLevelUpLearnset(species);
    const u16 *teachable = GetSpeciesTeachableLearnset(species);
    u32 i;

    *count = 0;
    for (i = 0; levelUp != NULL && levelUp[i].move != LEVEL_UP_MOVE_END && *count < maxCount; i++)
    {
        if (levelUp[i].level <= level)
            out[(*count)++] = levelUp[i].move;
    }
    if (level >= 15)
    {
        for (i = 0; teachable != NULL && teachable[i] != MOVE_UNAVAILABLE && *count < maxCount; i++)
            out[(*count)++] = teachable[i];
    }
}

// Picks up to three strong attacks of different types, then a setup/utility move.
static void GiveCompetitiveMoveset(struct Pokemon *mon, u32 species, u32 level, bool32 physicalAttacker)
{
    u16 *learnable = Alloc(sizeof(u16) * 256);
    u16 chosen[MAX_MON_MOVES] = {MOVE_NONE};
    u32 chosenTypes[MAX_MON_MOVES] = {0};
    u32 learnableCount, chosenCount = 0;
    u32 i, j, slot;

    if (learnable == NULL)
        return;
    CollectLearnableMoves(species, level, learnable, &learnableCount, 256);

    // Up to three attacks, each of a type not yet covered
    while (chosenCount < 3)
    {
        u32 best = MOVE_NONE, bestScore = 0;
        for (i = 0; i < learnableCount; i++)
        {
            u32 move = learnable[i];
            bool32 dupe = FALSE;
            if (!IsUsableAttack(move))
                continue;
            for (j = 0; j < chosenCount; j++)
            {
                if (chosen[j] == move || chosenTypes[j] == GetMoveType(move))
                    dupe = TRUE;
            }
            if (!dupe && ScoreAttack(species, move, physicalAttacker) > bestScore)
            {
                best = move;
                bestScore = ScoreAttack(species, move, physicalAttacker);
            }
        }
        if (best == MOVE_NONE)
            break;
        chosenTypes[chosenCount] = GetMoveType(best);
        chosen[chosenCount++] = best;
    }

    // Last slot: a setup or utility move if the species has one
    for (i = 0; i < ARRAY_COUNT(sGoodStatusMoves) && chosenCount < MAX_MON_MOVES; i++)
    {
        u32 move = sGoodStatusMoves[(i + Random()) % ARRAY_COUNT(sGoodStatusMoves)];
        if (CanSpeciesLearnMove(species, move, level))
        {
            chosen[chosenCount++] = move;
            break;
        }
    }
    Free(learnable);

    if (chosenCount == 0)
        return; // keep the default level-up moveset
    for (slot = 0; slot < MAX_MON_MOVES; slot++)
        SetMonMoveSlot(mon, chosen[slot], slot);
}

static u32 GetTrainerPartySize(u32 wave)
{
    return min(2 + (wave - 1) / ROGUELITE_WAVES_PER_BIOME, PARTY_SIZE);
}

static u32 PickTrainerSpecies(u32 wave)
{
    // Mostly the current biome, sometimes any biome reached so far
    u32 biomeId = GetBiomeIdForWave(wave);
    if (biomeId > 0 && (Random() % 3) == 0)
        biomeId = Random() % (biomeId + 1);
    if ((Random() % 4) == 0)
        return PickFromPool(sBiomes[biomeId].bosses, sBiomes[biomeId].bossCount);
    return PickFromPool(sBiomes[biomeId].wild, sBiomes[biomeId].wildCount);
}

void Roguelite_CreateTrainerParty(struct Pokemon *party)
{
    u32 wave = max(GetWave(), 1);
    u32 level = min(GetWaveLevel(wave) + 2, MAX_LEVEL);
    u32 count = GetTrainerPartySize(wave);
    u16 usedItems[PARTY_SIZE] = {ITEM_NONE};
    u32 i, j;

    ZeroPartyMons(party);
    for (i = 0; i < count; i++)
    {
        struct Pokemon *mon = &party[i];
        u32 species = SPECIES_NONE;
        u32 tries, item = ITEM_NONE, nature, iv = MAX_PER_STAT_IVS, ev252 = MAX_PER_STAT_EVS, ev4 = 4, zero = 0;
        bool32 physical;

        // Species clause
        for (tries = 0; tries < 10; tries++)
        {
            bool32 dupe = FALSE;
            species = EvolveForLevel(PickTrainerSpecies(wave), level);
            for (j = 0; j < i; j++)
            {
                if (GetMonData(&party[j], MON_DATA_SPECIES) == species)
                    dupe = TRUE;
            }
            if (!dupe)
                break;
        }

        CreateMonWithIVs(mon, species, level, Random32(), OTID_STRUCT_RANDOM_NO_SHINY, MAX_PER_STAT_IVS);
        GiveMonInitialMoveset(mon);

        physical = GetSpeciesBaseStat(species, STAT_ATK) >= GetSpeciesBaseStat(species, STAT_SPATK);
        nature = physical ? ((Random() & 1) ? NATURE_JOLLY : NATURE_ADAMANT)
                          : ((Random() & 1) ? NATURE_TIMID : NATURE_MODEST);
        SetMonData(mon, MON_DATA_HIDDEN_NATURE, &nature);
        SetMonData(mon, MON_DATA_HP_IV, &iv);
        SetMonData(mon, MON_DATA_HP_EV, &ev4);
        SetMonData(mon, MON_DATA_ATK_EV, physical ? &ev252 : &zero);
        SetMonData(mon, MON_DATA_SPATK_EV, physical ? &zero : &ev252);
        SetMonData(mon, MON_DATA_SPEED_EV, &ev252);

        // Item clause
        for (tries = 0; tries < 10; tries++)
        {
            bool32 dupe = FALSE;
            item = sTrainerHeldItems[Random() % ARRAY_COUNT(sTrainerHeldItems)];
            if (item == ITEM_CHOICE_BAND && !physical)
                item = ITEM_CHOICE_SPECS;
            else if (item == ITEM_CHOICE_SPECS && physical)
                item = ITEM_CHOICE_BAND;
            for (j = 0; j < i; j++)
            {
                if (usedItems[j] == item)
                    dupe = TRUE;
            }
            if (!dupe)
                break;
        }
        usedItems[i] = item;
        SetMonData(mon, MON_DATA_HELD_ITEM, &item);

        GiveCompetitiveMoveset(mon, species, level, physical);
        CalculateMonStats(mon);
    }
    gPartiesCount[B_TRAINER_OPPONENT_A] = count;
}

// ---------------------------------------------------------------------------
// Hub: areas built with Rogue Coins (Emerald Rogue-style)
// ---------------------------------------------------------------------------

struct HubArea
{
    const u8 *name;
    u16 flag;       // built flag for one-off areas, 0 for levelled ones
    u16 levelVar;   // level var for levelled areas, 0 for one-off ones
    u16 prices[ROGUELITE_HUB_MAX_LEVEL];
};

static const struct HubArea sHubAreas[ROGUELITE_HUB_AREA_COUNT] =
{
    [ROGUELITE_HUB_AREA_MART]        = { COMPOUND_STRING("Poké Mart"),     FLAG_ROGUE_HUB_MART,        0, {300} },
    [ROGUELITE_HUB_AREA_HEALER]      = { COMPOUND_STRING("Healer"),        FLAG_ROGUE_HUB_HEALER,      0, {600} },
    [ROGUELITE_HUB_AREA_TUTOR]       = { COMPOUND_STRING("Move Tutor"),    FLAG_ROGUE_HUB_TUTOR,       0, {400} },
    [ROGUELITE_HUB_AREA_ABILITY_LAB] = { COMPOUND_STRING("Ability Lab"),   FLAG_ROGUE_HUB_ABILITY_LAB, 0, {800} },
    [ROGUELITE_HUB_AREA_SUPPLY]      = { COMPOUND_STRING("Supply Depot"),  0, VAR_ROGUE_HUB_SUPPLY_LEVEL, {300, 700, 1200} },
    [ROGUELITE_HUB_AREA_DRAFT]       = { COMPOUND_STRING("Draft Office"),  0, VAR_ROGUE_HUB_DRAFT_LEVEL,  {400, 900, 1600} },
};

static const u8 sText_Built[] = _(" (built)");
static const u8 sText_Max[] = _(" (max)");
static const u8 sText_Level[] = _(" Lv");
static const u8 sText_Done[] = _("Done");

static u32 GetAreaLevel(u32 area)
{
    if (sHubAreas[area].flag)
        return FlagGet(sHubAreas[area].flag) ? 1 : 0;
    return VarGet(sHubAreas[area].levelVar);
}

static u32 GetAreaMaxLevel(u32 area)
{
    u32 level = 0;
    while (level < ROGUELITE_HUB_MAX_LEVEL && sHubAreas[area].prices[level] != 0)
        level++;
    return level;
}

// Pushes the Builder's list for dynmultistack. gStringVar1 = coins.
void Roguelite_BuildHubAreaMenu(void)
{
    u32 area;

    for (area = 0; area < ROGUELITE_HUB_AREA_COUNT; area++)
    {
        u32 level = GetAreaLevel(area), maxLevel = GetAreaMaxLevel(area);
        u8 *str = AllocMenuString();
        u8 *end = StringCopy(str, sHubAreas[area].name);

        if (maxLevel > 1 && level > 0)
        {
            end = StringCopy(end, sText_Level);
            end = ConvertIntToDecimalStringN(end, level, STR_CONV_MODE_LEFT_ALIGN, 1);
        }
        if (level >= maxLevel)
        {
            StringCopy(end, maxLevel > 1 ? sText_Max : sText_Built);
        }
        else
        {
            *end++ = CHAR_SPACE;
            *end++ = CHAR_HYPHEN;
            *end++ = CHAR_SPACE;
            ConvertIntToDecimalStringN(end, sHubAreas[area].prices[level], STR_CONV_MODE_LEFT_ALIGN, 4);
        }
        PushMenuItem(str, area);
    }
    {
        u8 *str = AllocMenuString();
        StringCopy(str, sText_Done);
        PushMenuItem(str, ROGUELITE_HUB_AREA_COUNT);
    }
    ConvertIntToDecimalStringN(gStringVar1, VarGet(VAR_ROGUE_COINS), STR_CONV_MODE_LEFT_ALIGN, 4);
}

// Builds or upgrades the area picked in VAR_RESULT.
// VAR_RESULT = ROGUELITE_BUILD_*; gStringVar2 = area name; VAR_0x8004 = area id.
void Roguelite_BuyHubArea(void)
{
    u32 area = gSpecialVar_Result, level, price;

    gSpecialVar_0x8004 = area;
    if (area >= ROGUELITE_HUB_AREA_COUNT)
    {
        gSpecialVar_Result = ROGUELITE_BUILD_CANCEL;
        return;
    }
    StringCopy(gStringVar2, sHubAreas[area].name);
    level = GetAreaLevel(area);
    if (level >= GetAreaMaxLevel(area))
    {
        gSpecialVar_Result = ROGUELITE_BUILD_MAXED;
        return;
    }
    price = sHubAreas[area].prices[level];
    if (VarGet(VAR_ROGUE_COINS) < price)
    {
        gSpecialVar_Result = ROGUELITE_BUILD_TOO_POOR;
        return;
    }
    VarSet(VAR_ROGUE_COINS, VarGet(VAR_ROGUE_COINS) - price);
    if (sHubAreas[area].flag)
        FlagSet(sHubAreas[area].flag);
    else
        VarSet(sHubAreas[area].levelVar, level + 1);
    ConvertIntToDecimalStringN(gStringVar1, VarGet(VAR_ROGUE_COINS), STR_CONV_MODE_LEFT_ALIGN, 4);
    gSpecialVar_Result = ROGUELITE_BUILD_DONE;
}

// gStringVar1 = Rogue Coins.
void Roguelite_BufferCoins(void)
{
    ConvertIntToDecimalStringN(gStringVar1, VarGet(VAR_ROGUE_COINS), STR_CONV_MODE_LEFT_ALIGN, 4);
}

// Ability Lab: VAR_0x8004 = party slot. Pushes the species' regular abilities
// (ids are ability slots) for dynmultistack. gStringVar1 = nickname.
void Roguelite_BuildAbilityMenu(void)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];
    u32 species = GetMonData(mon, MON_DATA_SPECIES), slot, j;
    u32 current = GetMonData(mon, MON_DATA_ABILITY_NUM);

    GetMonNickname(mon, gStringVar1);
    for (slot = 0; slot < NUM_ABILITY_SLOTS; slot++)
    {
        enum Ability ability = GetSpeciesAbility(species, slot);
        bool32 dupe = FALSE;
        u8 *str, *end;

        if (ability == ABILITY_NONE)
            continue;
        for (j = 0; j < slot; j++)
        {
            if (GetSpeciesAbility(species, j) == ability)
                dupe = TRUE;
        }
        if (dupe)
            continue;
        str = AllocMenuString();
        end = StringCopy(str, gAbilitiesInfo[ability].name);
        if (GetMonAbility(mon) == ability && (current == slot || current >= NUM_ABILITY_SLOTS))
            StringCopy(end, COMPOUND_STRING(" (now)"));
        PushMenuItem(str, slot);
    }
}

// Sets the ability slot picked in VAR_RESULT. gStringVar2 = new ability name.
void Roguelite_SetMonAbility(void)
{
    struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][gSpecialVar_0x8004];
    u32 slot = gSpecialVar_Result;

    if (slot >= NUM_ABILITY_SLOTS)
    {
        gSpecialVar_Result = FALSE;
        return;
    }
    SetMonData(mon, MON_DATA_ABILITY_NUM, &slot);
    StringCopy(gStringVar2, gAbilitiesInfo[GetMonAbility(mon)].name);
    gSpecialVar_Result = TRUE;
}
