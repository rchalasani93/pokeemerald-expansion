#ifndef GUARD_ROGUELITE_H
#define GUARD_ROGUELITE_H

#include "constants/battle.h"
#include "constants/roguelite.h"

bool32 Roguelite_IsRunActive(void);
enum BattleEnvironments Roguelite_GetBattleEnvironment(void);
bool32 Roguelite_IsBossWave(void);
void Roguelite_CreateTrainerParty(struct Pokemon *party);

#endif // GUARD_ROGUELITE_H
