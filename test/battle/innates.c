#include "global.h"
#include "innates.h"
#include "test/battle.h"

// Innates (Elite Redux-style): extra always-on abilities per species.
// Tests run without the generated Elite Redux table; each test declares its own innates.

SINGLE_BATTLE_TEST("Innates: a switch-in innate activates alongside the regular ability")
{
    GIVEN {
        Test_SetSpeciesInnates(SPECIES_ARBOK, ABILITY_INTIMIDATE, ABILITY_NONE, ABILITY_NONE);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_ARBOK) { Ability(ABILITY_SHED_SKIN); }
    } WHEN {
        TURN { }
    } SCENE {
        ABILITY_POPUP(opponent, ABILITY_INTIMIDATE);
        ANIMATION(ANIM_TYPE_GENERAL, B_ANIM_STATS_CHANGE, player);
        MESSAGE("Wobbuffet's Attack fell!");
    } THEN {
        EXPECT_EQ(player->statStages[STAT_ATK], DEFAULT_STAT_STAGE - 1);
        EXPECT_EQ(opponent->ability, ABILITY_SHED_SKIN);
    }
}

SINGLE_BATTLE_TEST("Innates: the regular ability and several innates all trigger on switch-in")
{
    GIVEN {
        Test_SetSpeciesInnates(SPECIES_ARBOK, ABILITY_INTIMIDATE, ABILITY_DRIZZLE, ABILITY_NONE);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_ARBOK) { Ability(ABILITY_SHED_SKIN); }
    } WHEN {
        TURN { }
    } SCENE {
        ABILITY_POPUP(opponent, ABILITY_INTIMIDATE);
        MESSAGE("Wobbuffet's Attack fell!");
        ABILITY_POPUP(opponent, ABILITY_DRIZZLE);
        MESSAGE("It started to rain!");
    } THEN {
        EXPECT(gBattleWeather & B_WEATHER_RAIN);
    }
}

SINGLE_BATTLE_TEST("Innates: damage-boosting innates apply", s16 damage)
{
    bool32 hasInnate;
    PARAMETRIZE { hasInnate = FALSE; }
    PARAMETRIZE { hasInnate = TRUE; }
    GIVEN {
        ASSUME(GetMoveCategory(MOVE_SCRATCH) == DAMAGE_CATEGORY_PHYSICAL);
        if (hasInnate)
            Test_SetSpeciesInnates(SPECIES_WOBBUFFET, ABILITY_HUGE_POWER, ABILITY_NONE, ABILITY_NONE);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(player, MOVE_SCRATCH); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(2.0), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Innates: an absorbing innate absorbs the move and shows its own popup")
{
    GIVEN {
        ASSUME(GetMoveType(MOVE_THUNDER_SHOCK) == TYPE_ELECTRIC);
        Test_SetSpeciesInnates(SPECIES_WOBBUFFET, ABILITY_NONE, ABILITY_VOLT_ABSORB, ABILITY_NONE);
        PLAYER(SPECIES_WOBBUFFET) { HP(1); MaxHP(100); }
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(opponent, MOVE_THUNDER_SHOCK); }
    } SCENE {
        ABILITY_POPUP(player, ABILITY_VOLT_ABSORB);
        HP_BAR(player, damage: -25);
        MESSAGE("Wobbuffet had its HP restored.");
    }
}

SINGLE_BATTLE_TEST("Innates: Levitate as an innate grants Ground immunity")
{
    GIVEN {
        ASSUME(GetMoveType(MOVE_MUD_SLAP) == TYPE_GROUND);
        Test_SetSpeciesInnates(SPECIES_WOBBUFFET, ABILITY_LEVITATE, ABILITY_NONE, ABILITY_NONE);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(opponent, MOVE_MUD_SLAP); }
    } SCENE {
        NONE_OF { HP_BAR(player); }
    }
}

SINGLE_BATTLE_TEST("Innates: contact innates punish the attacker")
{
    GIVEN {
        ASSUME(MoveMakesContact(MOVE_SCRATCH));
        Test_SetSpeciesInnates(SPECIES_WYNAUT, ABILITY_IRON_BARBS, ABILITY_NONE, ABILITY_NONE);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(player, MOVE_SCRATCH); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_SCRATCH, player);
        ABILITY_POPUP(opponent, ABILITY_IRON_BARBS);
        HP_BAR(player);
    }
}

SINGLE_BATTLE_TEST("Innates: end-of-turn innates trigger")
{
    GIVEN {
        Test_SetSpeciesInnates(SPECIES_WOBBUFFET, ABILITY_SPEED_BOOST, ABILITY_NONE, ABILITY_NONE);
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(player, MOVE_CELEBRATE); MOVE(opponent, MOVE_CELEBRATE); }
    } SCENE {
        ABILITY_POPUP(player, ABILITY_SPEED_BOOST);
        MESSAGE("Wobbuffet's Speed rose!");
    } THEN {
        EXPECT_EQ(player->statStages[STAT_SPEED], DEFAULT_STAT_STAGE + 1);
    }
}

SINGLE_BATTLE_TEST("Innates: Gastro Acid suppresses innates too")
{
    GIVEN {
        ASSUME(GetMoveType(MOVE_MUD_SLAP) == TYPE_GROUND);
        Test_SetSpeciesInnates(SPECIES_WYNAUT, ABILITY_LEVITATE, ABILITY_NONE, ABILITY_NONE);
        PLAYER(SPECIES_WOBBUFFET) { Speed(100); }
        OPPONENT(SPECIES_WYNAUT) { Speed(1); }
    } WHEN {
        TURN { MOVE(player, MOVE_GASTRO_ACID); }
        TURN { MOVE(player, MOVE_MUD_SLAP); }
    } SCENE {
        ANIMATION(ANIM_TYPE_MOVE, MOVE_GASTRO_ACID, player);
        ANIMATION(ANIM_TYPE_MOVE, MOVE_MUD_SLAP, player);
        HP_BAR(opponent);
    }
}

SINGLE_BATTLE_TEST("Innates: Pokémon without innates behave exactly as before")
{
    GIVEN {
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_WYNAUT);
    } WHEN {
        TURN { MOVE(player, MOVE_SCRATCH); }
    } SCENE {
        NONE_OF { ABILITY_POPUP(opponent); ABILITY_POPUP(player); }
    } THEN {
        EXPECT_EQ(GetSpeciesInnate(SPECIES_WOBBUFFET, 0), ABILITY_NONE);
    }
}
