#include "global.h"
#include "event_data.h"
#include "list_menu.h"
#include "malloc.h"
#include "pokemon_storage_system.h"
#include "roguelite.h"
#include "pokedex.h"
#include "randomizer.h"
#include "string_util.h"
#include "move.h"
#include "script_menu.h"
#include "test/overworld_script.h"
#include "test/test.h"
#include "constants/opponents.h"

static void FreeDynamicMenu(void)
{
    u32 i, count = MultichoiceDynamic_StackSize();
    for (i = 0; i < count; i++)
        Free((void *)MultichoiceDynamic_PeekElementAt(i)->name);
    MultichoiceDynamic_DestroyStack();
}

static u32 ExpectedWaveLevel(u32 wave)
{
    return 3 + (wave * 11) / 10;
}

TEST("(Roguelite) Starting a run parks the party and ending it brings the party back")
{
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_PIKACHU, 50;);
    FlagClear(FLAG_ROGUELITE_ACTIVE);

    RUN_OVERWORLD_SCRIPT(special Roguelite_StartRun;);
    EXPECT_EQ(gSpecialVar_Result, TRUE);
    EXPECT(FlagGet(FLAG_ROGUELITE_ACTIVE));
    EXPECT_EQ(CalculatePlayerPartyCount(), 0);
    EXPECT_EQ(VarGet(VAR_ROGUELITE_BUDGET), ROGUELITE_STARTER_BUDGET);

    RUN_OVERWORLD_SCRIPT(givemon SPECIES_BAGON, 5;);
    VarSet(VAR_ROGUELITE_WAVE, 7);
    RUN_OVERWORLD_SCRIPT(special Roguelite_EndRun;);

    EXPECT(!FlagGet(FLAG_ROGUELITE_ACTIVE));
    EXPECT_EQ(CalculatePlayerPartyCount(), 1);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_PIKACHU);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_LEVEL), 50);
    EXPECT_GE(VarGet(VAR_ROGUELITE_BEST_WAVE), 7);
}

TEST("(Roguelite) Boss waves are stronger and come with perfect IVs")
{
    struct Pokemon *boss = &gParties[B_TRAINER_OPPONENT_A][0];

    FlagSet(FLAG_ROGUELITE_ACTIVE);
    VarSet(VAR_ROGUELITE_WAVE, 9);
    RUN_OVERWORLD_SCRIPT(special Roguelite_PrepareWave;);

    EXPECT_EQ(VarGet(VAR_ROGUELITE_WAVE), 10);
    EXPECT_EQ(gSpecialVar_Result, 1);
    EXPECT_EQ(GetMonData(boss, MON_DATA_LEVEL), ExpectedWaveLevel(10) + 3);
    EXPECT_EQ(GetMonData(boss, MON_DATA_HP_IV), MAX_PER_STAT_IVS);
    EXPECT_EQ(GetMonData(boss, MON_DATA_SPEED_IV), MAX_PER_STAT_IVS);
    FlagClear(FLAG_ROGUELITE_ACTIVE);
}

TEST("(Roguelite) The final wave is Rayquaza")
{
    FlagSet(FLAG_ROGUELITE_ACTIVE);
    VarSet(VAR_ROGUELITE_WAVE, ROGUELITE_FINAL_WAVE - 1);
    RUN_OVERWORLD_SCRIPT(special Roguelite_PrepareWave;);

    EXPECT_EQ(gSpecialVar_Result, 2);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES), SPECIES_RAYQUAZA);
    FlagClear(FLAG_ROGUELITE_ACTIVE);
}

TEST("(Roguelite) Wild Pokémon are evolved to match their level")
{
    u32 i, j, wave = 0;
    PARAMETRIZE { wave = 3; }
    PARAMETRIZE { wave = 24; }
    PARAMETRIZE { wave = 46; }

    FlagSet(FLAG_ROGUELITE_ACTIVE);
    ZeroPlayerPartyMons(); // no doubles: keep it to one enemy
    for (i = 0; i < 25; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_OPPONENT_A][0];
        const struct Evolution *evos;
        u32 level;

        VarSet(VAR_ROGUELITE_WAVE, wave - 1);
        RUN_OVERWORLD_SCRIPT(special Roguelite_PrepareWave;);
        level = GetMonData(mon, MON_DATA_LEVEL);
        EXPECT_EQ(level, ExpectedWaveLevel(wave));

        evos = GetSpeciesEvolutions(GetMonData(mon, MON_DATA_SPECIES));
        if (evos == NULL)
            continue;
        for (j = 0; evos[j].method != EVOLUTIONS_END; j++)
        {
            if (evos[j].method == EVO_LEVEL && evos[j].param != 0 && IsSpeciesEnabled(evos[j].targetSpecies))
                EXPECT_LT(level, evos[j].param);
        }
    }
    FlagClear(FLAG_ROGUELITE_ACTIVE);
}

static void ClearCaughtFlag(enum Species species)
{
    u32 n = SpeciesToNationalPokedexNum(species) - 1;
    gSaveBlock1Ptr->dexCaught[n / 8] &= ~(1 << (n % 8));
}

TEST("(Roguelite) Starter draft offers the 27 classic starters, a page at a time")
{
    ZeroPlayerPartyMons();
    VarSet(VAR_ROGUELITE_BUDGET, ROGUELITE_STARTER_BUDGET);
    VarSet(VAR_ROGUELITE_DRAFT_PAGE, 0);

    RUN_OVERWORLD_SCRIPT(special Roguelite_BuildStarterMenu;);
    // A full page of starters + "Next page" (no "Start" with an empty party)
    EXPECT_EQ(MultichoiceDynamic_StackSize(), ROGUELITE_DRAFT_PAGE_SIZE + 1);
    EXPECT_EQ(MultichoiceDynamic_PeekElementAt(ROGUELITE_DRAFT_PAGE_SIZE)->id, ROGUELITE_MENU_NEXT_PAGE);
    FreeDynamicMenu();

    // Entry 1 is Charmander, one of the 4-point starters
    gSpecialVar_Result = 1;
    RUN_OVERWORLD_SCRIPT(special Roguelite_PickStarter;);
    EXPECT_EQ(gSpecialVar_Result, ROGUELITE_PICK_CONTINUE);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES), SPECIES_CHARMANDER);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_LEVEL), ROGUELITE_STARTER_LEVEL);
    EXPECT_EQ(VarGet(VAR_ROGUELITE_BUDGET), ROGUELITE_STARTER_BUDGET - 4);

    // The same Pokémon can't be drafted twice
    gSpecialVar_Result = 1;
    RUN_OVERWORLD_SCRIPT(special Roguelite_PickStarter;);
    EXPECT_EQ(gSpecialVar_Result, ROGUELITE_PICK_ALREADY);

    // Bulbasaur costs 3
    gSpecialVar_Result = 0;
    RUN_OVERWORLD_SCRIPT(special Roguelite_PickStarter;);
    EXPECT_EQ(VarGet(VAR_ROGUELITE_BUDGET), ROGUELITE_STARTER_BUDGET - 7);

    // Paging
    gSpecialVar_Result = ROGUELITE_MENU_NEXT_PAGE;
    RUN_OVERWORLD_SCRIPT(special Roguelite_PickStarter;);
    EXPECT_EQ(gSpecialVar_Result, ROGUELITE_PICK_PAGE);
    EXPECT_EQ(VarGet(VAR_ROGUELITE_DRAFT_PAGE), 1);
    RUN_OVERWORLD_SCRIPT(special Roguelite_BuildStarterMenu;);
    EXPECT_EQ(MultichoiceDynamic_PeekElementAt(0)->id, ROGUELITE_DRAFT_PAGE_SIZE);
    FreeDynamicMenu();

    gSpecialVar_Result = ROGUELITE_MENU_START_RUN;
    RUN_OVERWORLD_SCRIPT(special Roguelite_PickStarter;);
    EXPECT_EQ(gSpecialVar_Result, ROGUELITE_PICK_DONE);
    VarSet(VAR_ROGUELITE_DRAFT_PAGE, 0);
}

TEST("(Roguelite) Catching any member of a family unlocks its first stage for the draft")
{
    u32 i, before, after;
    bool32 found = FALSE;

    ZeroPlayerPartyMons();
    GetSetPokedexFlag(SpeciesToNationalPokedexNum(SPECIES_PIDGEOT), FLAG_SET_CAUGHT);
    ClearCaughtFlag(SPECIES_PIDGEY);
    ClearCaughtFlag(SPECIES_PIDGEOTTO);
    VarSet(VAR_ROGUELITE_DRAFT_PAGE, 0);
    RUN_OVERWORLD_SCRIPT(special Roguelite_BuildStarterMenu;);
    after = gSpecialVar_0x8004;
    FreeDynamicMenu();

    // Walk every page looking for Pidgey
    for (i = 0; i < 200 && !found; i++)
    {
        u32 j;
        VarSet(VAR_ROGUELITE_DRAFT_PAGE, i);
        RUN_OVERWORLD_SCRIPT(special Roguelite_BuildStarterMenu;);
        for (j = 0; j < MultichoiceDynamic_StackSize(); j++)
        {
            const u8 *name = MultichoiceDynamic_PeekElementAt(j)->name;
            if (StringCompareN(name, GetSpeciesName(SPECIES_PIDGEY), StringLength(GetSpeciesName(SPECIES_PIDGEY))) == 0)
                found = TRUE;
        }
        if (VarGet(VAR_ROGUELITE_DRAFT_PAGE) != i) // past the last page
            i = 200;
        FreeDynamicMenu();
    }
    EXPECT(found);

    ClearCaughtFlag(SPECIES_PIDGEOT);
    VarSet(VAR_ROGUELITE_DRAFT_PAGE, 0);
    RUN_OVERWORLD_SCRIPT(special Roguelite_BuildStarterMenu;);
    before = gSpecialVar_0x8004;
    FreeDynamicMenu();
    EXPECT_EQ(after, before + 1);
}

TEST("(Roguelite) Runs pay Rogue Coins for cleared waves and beaten bosses")
{
    u32 end = 0, wave = 0, expected = 0;
    PARAMETRIZE { end = ROGUELITE_END_LOST;    wave = 12; expected = 11 * 5 + 50; }
    PARAMETRIZE { end = ROGUELITE_END_GAVE_UP; wave = 10; expected = 10 * 5 + 50; }
    PARAMETRIZE { end = ROGUELITE_END_LOST;    wave = 10; expected = 9 * 5; }
    PARAMETRIZE { end = ROGUELITE_END_WON;     wave = 50; expected = 50 * 5 + 5 * 50 + 500; }

    VarSet(VAR_ROGUE_COINS, 0);
    FlagSet(FLAG_ROGUELITE_ACTIVE);
    VarSet(VAR_ROGUELITE_WAVE, wave);
    gSpecialVar_0x8004 = end;
    RUN_OVERWORLD_SCRIPT(special Roguelite_EndRun;);
    EXPECT_EQ(VarGet(VAR_ROGUE_COINS), expected);
    VarSet(VAR_ROGUE_COINS, 0);
}

TEST("(Roguelite) The Builder spends Rogue Coins on hub areas and upgrades")
{
    VarSet(VAR_ROGUE_COINS, 700);
    FlagClear(FLAG_ROGUE_HUB_MART);
    VarSet(VAR_ROGUE_HUB_DRAFT_LEVEL, 0);

    gSpecialVar_Result = ROGUELITE_HUB_AREA_MART;
    RUN_OVERWORLD_SCRIPT(special Roguelite_BuyHubArea;);
    EXPECT_EQ(gSpecialVar_Result, ROGUELITE_BUILD_DONE);
    EXPECT(FlagGet(FLAG_ROGUE_HUB_MART));
    EXPECT_EQ(VarGet(VAR_ROGUE_COINS), 400);

    gSpecialVar_Result = ROGUELITE_HUB_AREA_MART;
    RUN_OVERWORLD_SCRIPT(special Roguelite_BuyHubArea;);
    EXPECT_EQ(gSpecialVar_Result, ROGUELITE_BUILD_MAXED);

    gSpecialVar_Result = ROGUELITE_HUB_AREA_DRAFT;
    RUN_OVERWORLD_SCRIPT(special Roguelite_BuyHubArea;);
    EXPECT_EQ(gSpecialVar_Result, ROGUELITE_BUILD_DONE);
    EXPECT_EQ(VarGet(VAR_ROGUE_HUB_DRAFT_LEVEL), 1);
    EXPECT_EQ(VarGet(VAR_ROGUE_COINS), 0);

    gSpecialVar_Result = ROGUELITE_HUB_AREA_DRAFT;
    RUN_OVERWORLD_SCRIPT(special Roguelite_BuyHubArea;);
    EXPECT_EQ(gSpecialVar_Result, ROGUELITE_BUILD_TOO_POOR);

    // Draft Office level 1 adds 2 points to the next run's draft
    ZeroPlayerPartyMons();
    FlagClear(FLAG_ROGUELITE_ACTIVE);
    RUN_OVERWORLD_SCRIPT(special Roguelite_StartRun;);
    EXPECT_EQ(VarGet(VAR_ROGUELITE_BUDGET), ROGUELITE_STARTER_BUDGET + ROGUELITE_DRAFT_OFFICE_BONUS);

    FlagClear(FLAG_ROGUELITE_ACTIVE);
    FlagClear(FLAG_ROGUE_HUB_MART);
    VarSet(VAR_ROGUE_HUB_DRAFT_LEVEL, 0);
}

TEST("(Roguelite) The Ability Lab switches a Pokémon's regular ability")
{
    ZeroPlayerPartyMons();
    RUN_OVERWORLD_SCRIPT(givemon SPECIES_PIKACHU, 20;);
    gSpecialVar_0x8004 = 0;
    gSpecialVar_Result = 2; // hidden ability slot
    RUN_OVERWORLD_SCRIPT(special Roguelite_SetMonAbility;);
    EXPECT_EQ(gSpecialVar_Result, TRUE);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_ABILITY_NUM), 2);
    EXPECT_EQ(GetMonAbility(&gParties[B_TRAINER_PLAYER][0]), GetSpeciesAbility(SPECIES_PIKACHU, 2));
}

TEST("(Roguelite) Randomizer options change species data and turn back off cleanly")
{
    u32 i, changedTypes = 0, changedAbilities = 0;
    const struct LevelUpMove *learnset;

    for (i = 0; i < 40; i++)
    {
        u32 species = SPECIES_BULBASAUR + i;
        enum Type t0 = GetSpeciesType(species, 0);
        enum Ability a0 = GetSpeciesAbility(species, 0);
        u32 bst = GetSpeciesBaseStatTotal(species);

        Randomizer_SetEnabled(RANDOMIZE_TYPES, TRUE);
        Randomizer_SetEnabled(RANDOMIZE_ABILITIES, TRUE);
        Randomizer_SetEnabled(RANDOMIZE_STATS, TRUE);
        if (GetSpeciesType(species, 0) != t0)
            changedTypes++;
        if (GetSpeciesAbility(species, 0) != a0)
            changedAbilities++;
        EXPECT_EQ(GetSpeciesBaseStatTotal(species), bst); // stats are shuffled, total kept
        EXPECT_EQ(GetSpeciesType(species, 0), GetSpeciesType(species, 0)); // stable for a species

        Randomizer_SetEnabled(RANDOMIZE_TYPES, FALSE);
        Randomizer_SetEnabled(RANDOMIZE_ABILITIES, FALSE);
        Randomizer_SetEnabled(RANDOMIZE_STATS, FALSE);
        EXPECT_EQ(GetSpeciesType(species, 0), t0);
        EXPECT_EQ(GetSpeciesAbility(species, 0), a0);
    }
    EXPECT_GT(changedTypes, 20);
    EXPECT_GT(changedAbilities, 30);

    // Level-up moves keep the real levels; the first one is always an attack
    Randomizer_SetEnabled(RANDOMIZE_MOVES, TRUE);
    learnset = GetSpeciesLevelUpLearnset(SPECIES_CHARMANDER);
    EXPECT_NE(GetMoveCategory(learnset[0].move), DAMAGE_CATEGORY_STATUS);
    for (i = 0; learnset[i].move != LEVEL_UP_MOVE_END; i++)
        EXPECT_EQ(learnset[i].level, gSpeciesInfo[SPECIES_CHARMANDER].levelUpLearnset[i].level);
    Randomizer_SetEnabled(RANDOMIZE_MOVES, FALSE);
    EXPECT_EQ(GetSpeciesLevelUpLearnset(SPECIES_CHARMANDER), gSpeciesInfo[SPECIES_CHARMANDER].levelUpLearnset);
}

TEST("(Roguelite) Rewards offer three choices and grant the picked one")
{
    VarSet(VAR_ROGUELITE_WAVE, 20);
    RUN_OVERWORLD_SCRIPT(special Roguelite_BuildRewardMenu;);
    EXPECT_EQ(MultichoiceDynamic_StackSize(), ROGUELITE_REWARD_CHOICES);
    FreeDynamicMenu();

    gSpecialVar_Result = 1;
    RUN_OVERWORLD_SCRIPT(special Roguelite_GiveReward;);
    EXPECT_EQ(gSpecialVar_Result, TRUE);
}

TEST("(Roguelite) Trainer routes generate a competitive team")
{
    u32 i, j, count;
    struct Pokemon *party = gParties[B_TRAINER_OPPONENT_A];

    FlagSet(FLAG_ROGUELITE_ACTIVE);
    VarSet(VAR_ROGUELITE_ROUTE, ROGUELITE_ROUTE_TRAINER);
    VarSet(VAR_ROGUELITE_WAVE, 24);
    RUN_OVERWORLD_SCRIPT(special Roguelite_PrepareWave;);
    EXPECT_EQ(gSpecialVar_Result, ROGUELITE_WAVE_KIND_TRAINER);

    Roguelite_CreateTrainerParty(party);
    count = 0;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&party[i], MON_DATA_SPECIES) != SPECIES_NONE)
            count++;
    }
    EXPECT_EQ(count, 4); // waves 21-30 field 4 Pokémon

    for (i = 0; i < count; i++)
    {
        EXPECT_EQ(GetMonData(&party[i], MON_DATA_LEVEL), ExpectedWaveLevel(25) + 2);
        EXPECT_EQ(GetMonData(&party[i], MON_DATA_ATK_IV), MAX_PER_STAT_IVS);
        EXPECT_EQ(GetMonData(&party[i], MON_DATA_SPEED_EV), MAX_PER_STAT_EVS);
        EXPECT_NE(GetMonData(&party[i], MON_DATA_HELD_ITEM), ITEM_NONE);
        EXPECT_NE(GetMonData(&party[i], MON_DATA_MOVE1), MOVE_NONE);
        for (j = 0; j < i; j++)
        {
            EXPECT_NE(GetMonData(&party[i], MON_DATA_SPECIES), GetMonData(&party[j], MON_DATA_SPECIES)); // species clause
            EXPECT_NE(GetMonData(&party[i], MON_DATA_HELD_ITEM), GetMonData(&party[j], MON_DATA_HELD_ITEM)); // item clause
        }
    }
    VarSet(VAR_ROGUELITE_ROUTE, ROGUELITE_ROUTE_WILD);
    FlagClear(FLAG_ROGUELITE_ACTIVE);
}

TEST("(Roguelite) Boss waves leave no route to choose")
{
    VarSet(VAR_ROGUELITE_WAVE, 9);
    RUN_OVERWORLD_SCRIPT(special Roguelite_BuildRouteMenu;);
    EXPECT_EQ(MultichoiceDynamic_StackSize(), 1);
    FreeDynamicMenu();

    VarSet(VAR_ROGUELITE_WAVE, 4);
    RUN_OVERWORLD_SCRIPT(special Roguelite_BuildRouteMenu;);
    EXPECT_GE(MultichoiceDynamic_StackSize(), 2);
    FreeDynamicMenu();
}
