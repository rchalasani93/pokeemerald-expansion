#include "global.h"
#include "event_data.h"
#include "list_menu.h"
#include "malloc.h"
#include "pokemon_storage_system.h"
#include "roguelite.h"
#include "script_menu.h"
#include "test/overworld_script.h"
#include "test/test.h"

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

TEST("(Roguelite) Starter draft spends points and fills the party")
{
    ZeroPlayerPartyMons();
    VarSet(VAR_ROGUELITE_BUDGET, ROGUELITE_STARTER_BUDGET);

    RUN_OVERWORLD_SCRIPT(special Roguelite_BuildStarterMenu;);
    EXPECT_EQ(MultichoiceDynamic_StackSize(), ROGUELITE_STARTER_CHOICES + 1); // + reroll (no "start" with an empty party)
    FreeDynamicMenu();

    gSpecialVar_Result = 0;
    RUN_OVERWORLD_SCRIPT(special Roguelite_PickStarter;);
    EXPECT_EQ(CalculatePlayerPartyCount(), 1);
    EXPECT_LT(VarGet(VAR_ROGUELITE_BUDGET), ROGUELITE_STARTER_BUDGET);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_LEVEL), ROGUELITE_STARTER_LEVEL);

    RUN_OVERWORLD_SCRIPT(special Roguelite_BuildStarterMenu;);
    EXPECT_EQ(MultichoiceDynamic_StackSize(), ROGUELITE_STARTER_CHOICES + 2); // + start + reroll
    FreeDynamicMenu();

    gSpecialVar_Result = ROGUELITE_MENU_START_RUN;
    RUN_OVERWORLD_SCRIPT(special Roguelite_PickStarter;);
    EXPECT_EQ(gSpecialVar_Result, ROGUELITE_PICK_DONE);
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
