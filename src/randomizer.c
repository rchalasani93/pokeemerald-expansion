#include "global.h"
#include "event_data.h"
#include "innates.h"
#include "move.h"
#include "pokemon.h"
#include "random.h"
#include "randomizer.h"
#include "constants/characters.h"
#include "constants/abilities.h"
#include "constants/moves.h"
#include "constants/species.h"

const u16 gRandomizerOptionFlags[RANDOMIZER_OPTION_COUNT] =
{
    [RANDOMIZE_ABILITIES]   = FLAG_RANDOMIZE_ABILITIES,
    [RANDOMIZE_INNATES]     = FLAG_RANDOMIZE_INNATES,
    [RANDOMIZE_TYPES]       = FLAG_RANDOMIZE_TYPES,
    [RANDOMIZE_MOVES]       = FLAG_RANDOMIZE_MOVES,
    [RANDOMIZE_STATS]       = FLAG_RANDOMIZE_STATS,
    [RANDOMIZE_RUN_POKEMON] = FLAG_RANDOMIZE_RUN_POKEMON,
};

// Salts keep the different randomizations independent of each other.
enum
{
    SALT_ABILITY = 0x100,
    SALT_INNATE = 0x200,
    SALT_TYPE = 0x300,
    SALT_TYPE_COUNT = 0x310,
    SALT_STATS = 0x400,
    SALT_MOVE = 0x500,
    SALT_RUN = 0x600,
};

#if TESTING
IWRAM_DATA u8 gTestRandomizerOptions = 0;
#endif

bool32 Randomizer_IsEnabled(enum RandomizerOption option)
{
    if (option >= RANDOMIZER_OPTION_COUNT)
        return FALSE;
#if TESTING
    return RANDOMIZER_ON(option);
#else
    return FlagGet(gRandomizerOptionFlags[option]);
#endif
}

void Randomizer_SetEnabled(enum RandomizerOption option, bool32 enabled)
{
    if (option >= RANDOMIZER_OPTION_COUNT)
        return;
#if TESTING
    if (enabled)
    {
        Randomizer_EnsureSeed();
        gTestRandomizerOptions |= 1 << option;
    }
    else
    {
        gTestRandomizerOptions &= ~(1 << option);
    }
    return;
#endif
    if (enabled)
    {
        Randomizer_EnsureSeed();
        FlagSet(gRandomizerOptionFlags[option]);
    }
    else
    {
        FlagClear(gRandomizerOptionFlags[option]);
    }
}

void Randomizer_EnsureSeed(void)
{
    if (VarGet(VAR_RANDOMIZER_SEED_LO) == 0 && VarGet(VAR_RANDOMIZER_SEED_HI) == 0)
    {
        u32 seed = Random32() | 1;
        VarSet(VAR_RANDOMIZER_SEED_LO, seed & 0xFFFF);
        VarSet(VAR_RANDOMIZER_SEED_HI, seed >> 16);
    }
}

static u32 GetSeed(void)
{
    return VarGet(VAR_RANDOMIZER_SEED_LO) | (VarGet(VAR_RANDOMIZER_SEED_HI) << 16);
}

// 32-bit integer hash (lowbias32). Deterministic and well mixed.
static u32 Hash(u32 species, u32 salt)
{
    u32 x = GetSeed() ^ (species * 0x9E3779B1u) ^ (salt * 0x85EBCA77u);
    x ^= x >> 16;
    x *= 0x7FEB352Du;
    x ^= x >> 15;
    x *= 0x846CA68Bu;
    x ^= x >> 16;
    return x;
}

// ---------------------------------------------------------------------------
// Abilities and innates
// ---------------------------------------------------------------------------

static bool32 IsAbilityRandomizable(enum Ability ability)
{
    if (ability == ABILITY_NONE || ability >= ABILITIES_COUNT)
        return FALSE;
    // Form changes and other species-bound mechanics only work on their own species
    if (gAbilitiesInfo[ability].cantBeSwapped || gAbilitiesInfo[ability].cantBeSuppressed)
        return FALSE;
    switch (ability)
    {
    case ABILITY_WONDER_GUARD:  // makes a random species nearly unbeatable
    case ABILITY_TRUANT:        // and these make it useless
    case ABILITY_SLOW_START:
    case ABILITY_DEFEATIST:
    case ABILITY_KLUTZ:
    case ABILITY_PROTOSYNTHESIS:
    case ABILITY_QUARK_DRIVE:
        return FALSE;
    default:
        // Skips empty slots and placeholders such as ABILITY_317 ("-------")
        return gAbilitiesInfo[ability].name[0] != EOS && gAbilitiesInfo[ability].name[0] != CHAR_HYPHEN;
    }
}

static enum Ability RandomAbility(enum Species species, u32 salt)
{
    u32 tries;
    for (tries = 0; tries < 16; tries++)
    {
        enum Ability ability = 1 + Hash(species, salt + tries * 0x10000) % (ABILITIES_COUNT - 1);
        if (IsAbilityRandomizable(ability))
            return ability;
    }
    return ABILITY_NONE;
}

enum Ability Randomizer_GetAbility(enum Species species, u32 slot, enum Ability original)
{
    enum Ability ability;

    if (!RANDOMIZER_ON(RANDOMIZE_ABILITIES) || original == ABILITY_NONE || !IsAbilityRandomizable(original))
        return original;
    ability = RandomAbility(species, SALT_ABILITY + slot);
    return ability != ABILITY_NONE ? ability : original;
}

// Randomized innates are cached because the battle engine reads them constantly.
#define INNATE_CACHE_SIZE 16
struct InnateCacheEntry
{
    u16 species;
    u16 innates[MAX_MON_INNATES];
};
EWRAM_DATA static struct InnateCacheEntry sInnateCache[INNATE_CACHE_SIZE] = {0};
EWRAM_DATA static u32 sInnateCacheSeed = 0;

bool32 Randomizer_InnatesOn(void)
{
    return RANDOMIZER_ON(RANDOMIZE_INNATES);
}

const u16 *Randomizer_GetInnates(enum Species species)
{
    struct InnateCacheEntry *entry = &sInnateCache[species % INNATE_CACHE_SIZE];
    u32 i, j;

    if (sInnateCacheSeed != GetSeed())
    {
        memset(sInnateCache, 0, sizeof(sInnateCache));
        sInnateCacheSeed = GetSeed();
    }
    if (entry->species == species && species != SPECIES_NONE)
        return entry->innates;

    entry->species = species;
    for (i = 0; i < MAX_MON_INNATES; i++)
    {
        enum Ability ability = RandomAbility(species, SALT_INNATE + i);
        for (j = 0; j < i; j++)
        {
            if (entry->innates[j] == ability)
                ability = ABILITY_NONE;
        }
        entry->innates[i] = ability;
    }
    return entry->innates;
}

// ---------------------------------------------------------------------------
// Types and base stats
// ---------------------------------------------------------------------------

static const u8 sRandomTypes[] =
{
    TYPE_NORMAL, TYPE_FIGHTING, TYPE_FLYING, TYPE_POISON, TYPE_GROUND, TYPE_ROCK, TYPE_BUG, TYPE_GHOST, TYPE_STEEL,
    TYPE_FIRE, TYPE_WATER, TYPE_GRASS, TYPE_ELECTRIC, TYPE_PSYCHIC, TYPE_ICE, TYPE_DRAGON, TYPE_DARK, TYPE_FAIRY,
};

enum Type Randomizer_GetType(enum Species species, u32 slot)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[SanitizeSpeciesId(species)];
    enum Type first, second;
    bool32 dualType = info->types[0] != info->types[1];

    if (!RANDOMIZER_ON(RANDOMIZE_TYPES))
        return info->types[slot];

    first = sRandomTypes[Hash(species, SALT_TYPE) % ARRAY_COUNT(sRandomTypes)];
    if (slot == 0 || !dualType)
        return first;
    second = sRandomTypes[Hash(species, SALT_TYPE + 1) % ARRAY_COUNT(sRandomTypes)];
    if (second == first)
        second = sRandomTypes[(Hash(species, SALT_TYPE + 1) + 1 + Hash(species, SALT_TYPE_COUNT) % (ARRAY_COUNT(sRandomTypes) - 1)) % ARRAY_COUNT(sRandomTypes)];
    return second;
}

// Shuffles which stat gets which base value; the total stays the same.
u32 Randomizer_GetBaseStat(enum Species species, u32 statIndex)
{
    const struct SpeciesInfo *info = &gSpeciesInfo[SanitizeSpeciesId(species)];
    u8 stats[NUM_STATS];
    u32 i, h;

    stats[STAT_HP] = info->baseHP;
    stats[STAT_ATK] = info->baseAttack;
    stats[STAT_DEF] = info->baseDefense;
    stats[STAT_SPEED] = info->baseSpeed;
    stats[STAT_SPATK] = info->baseSpAttack;
    stats[STAT_SPDEF] = info->baseSpDefense;
    if (statIndex >= NUM_STATS)
        return 0;
    if (!RANDOMIZER_ON(RANDOMIZE_STATS) || species == SPECIES_SHEDINJA) // Shedinja's 1 HP is its whole design
        return stats[statIndex];

    // Fisher-Yates shuffle driven by the species hash
    h = Hash(species, SALT_STATS);
    for (i = NUM_STATS - 1; i > 0; i--)
    {
        u32 j = h % (i + 1);
        u8 tmp = stats[i];
        stats[i] = stats[j];
        stats[j] = tmp;
        h = (h >> 3) | (h << 29);
    }
    return stats[statIndex];
}

// ---------------------------------------------------------------------------
// Level-up learnsets
// ---------------------------------------------------------------------------

static bool32 IsMoveRandomizable(enum Move move)
{
    if (move == MOVE_NONE || move >= MOVES_COUNT || move == MOVE_STRUGGLE)
        return FALSE;
    return !gMovesInfo[move].metronomeBanned && gMovesInfo[move].name[0] != EOS;
}

static enum Move RandomMove(enum Species species, u32 salt, bool32 mustAttack)
{
    u32 tries;
    for (tries = 0; tries < 32; tries++)
    {
        enum Move move = 1 + Hash(species, salt + tries * 0x10000) % (MOVES_COUNT - 1);
        if (!IsMoveRandomizable(move))
            continue;
        if (mustAttack && (GetMoveCategory(move) == DAMAGE_CATEGORY_STATUS || GetMovePower(move) < 30 || GetMovePower(move) > 60))
            continue;
        return move;
    }
    return mustAttack ? MOVE_TACKLE : MOVE_NONE;
}

// A few learnsets are buffered at once because callers sometimes compare two
// species (an evolution and its pre-evolution, for example).
#define LEARNSET_CACHE_SIZE 4
#define MAX_LEARNSET_LENGTH 64
struct LearnsetCacheEntry
{
    u16 species;
    u32 seed;
    struct LevelUpMove moves[MAX_LEARNSET_LENGTH + 1];
};
EWRAM_DATA static struct LearnsetCacheEntry sLearnsetCache[LEARNSET_CACHE_SIZE] = {0};
EWRAM_DATA static u8 sLearnsetCacheNext = 0;

const struct LevelUpMove *Randomizer_GetLevelUpLearnset(enum Species species, const struct LevelUpMove *original)
{
    struct LearnsetCacheEntry *entry = NULL;
    u32 i, j;

    if (!RANDOMIZER_ON(RANDOMIZE_MOVES) || original == NULL)
        return original;

    for (i = 0; i < LEARNSET_CACHE_SIZE; i++)
    {
        if (sLearnsetCache[i].species == species && sLearnsetCache[i].seed == GetSeed() && species != SPECIES_NONE)
            return sLearnsetCache[i].moves;
    }
    entry = &sLearnsetCache[sLearnsetCacheNext];
    sLearnsetCacheNext = (sLearnsetCacheNext + 1) % LEARNSET_CACHE_SIZE;
    entry->species = species;
    entry->seed = GetSeed();

    // Same levels as the real learnset, random moves. The first move is always
    // a weak-to-medium attack so a fresh Pokémon can still fight.
    for (i = 0; i < MAX_LEARNSET_LENGTH && original[i].move != LEVEL_UP_MOVE_END; i++)
    {
        enum Move move = RandomMove(species, SALT_MOVE + i, i == 0);
        for (j = 0; j < i; j++)
        {
            if (entry->moves[j].move == move)
                move = RandomMove(species, SALT_MOVE + i + 0x80, FALSE);
        }
        entry->moves[i].move = move != MOVE_NONE ? move : original[i].move;
        entry->moves[i].level = original[i].level;
    }
    entry->moves[i].move = LEVEL_UP_MOVE_END;
    entry->moves[i].level = 0;
    return entry->moves;
}

// ---------------------------------------------------------------------------
// Roguelite runs
// ---------------------------------------------------------------------------

// Any first-stage, non-legendary species (so EvolveForLevel can grow it).
enum Species Randomizer_GetRunSpecies(u32 salt)
{
    u32 tries;
    for (tries = 0; tries < 64; tries++)
    {
        enum Species species = 1 + Random() % (NUM_SPECIES - 1);
        (void)salt;
        if (!IsSpeciesEnabled(species) || gSpeciesInfo[species].isRestrictedLegendary || gSpeciesInfo[species].isSubLegendary || gSpeciesInfo[species].isMythical
         || gSpeciesInfo[species].isUltraBeast || gSpeciesInfo[species].isParadox || gSpeciesInfo[species].isMegaEvolution
         || gSpeciesInfo[species].isPrimalReversion || gSpeciesInfo[species].isGigantamax || gSpeciesInfo[species].isTotem
         || gSpeciesInfo[species].isTeraForm || GetSpeciesPreEvolution(species) != SPECIES_NONE)
            continue;
        return species;
    }
    return SPECIES_ZIGZAGOON;
}
