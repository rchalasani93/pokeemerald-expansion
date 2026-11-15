#include "global.h"
#include "test/battle.h"

SINGLE_BATTLE_TEST("Aura Guard halves damage from contact moves", s16 damage)
{
    enum Ability ability;
    PARAMETRIZE { ability = ABILITY_STEADFAST; }
    PARAMETRIZE { ability = ABILITY_AURA_GUARD; }
    GIVEN {
        ASSUME(MoveMakesContact(MOVE_SCRATCH));
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_LUCARIO) { Ability(ability); }
    } WHEN {
        TURN { MOVE(player, MOVE_SCRATCH); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(0.5), results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Aura Guard doesn't reduce damage from non-contact moves", s16 damage)
{
    enum Ability ability;
    PARAMETRIZE { ability = ABILITY_STEADFAST; }
    PARAMETRIZE { ability = ABILITY_AURA_GUARD; }
    GIVEN {
        ASSUME(!MoveMakesContact(MOVE_SWIFT));
        PLAYER(SPECIES_WOBBUFFET);
        OPPONENT(SPECIES_LUCARIO) { Ability(ability); }
    } WHEN {
        TURN { MOVE(player, MOVE_SWIFT); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_EQ(results[0].damage, results[1].damage);
    }
}

SINGLE_BATTLE_TEST("Aura Guard isn't bypassed by Mold Breaker", s16 damage)
{
    enum Ability ability;
    PARAMETRIZE { ability = ABILITY_STEADFAST; }
    PARAMETRIZE { ability = ABILITY_AURA_GUARD; }
    GIVEN {
        ASSUME(MoveMakesContact(MOVE_SCRATCH));
        PLAYER(SPECIES_PINSIR) { Ability(ABILITY_MOLD_BREAKER); }
        OPPONENT(SPECIES_LUCARIO) { Ability(ability); }
    } WHEN {
        TURN { MOVE(player, MOVE_SCRATCH); }
    } SCENE {
        HP_BAR(opponent, captureDamage: &results[i].damage);
    } FINALLY {
        EXPECT_MUL_EQ(results[0].damage, Q_4_12(0.5), results[1].damage);
    }
}
