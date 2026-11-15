# Roguelite Mode (PokeRogue-style)

New games start in the **Rogue Hub**, a closed-off town (no exits, so no "you can't leave without a Pokémon" block). Talk to the scientist in the south of the hub to start a run. See `docs/rogue_hub.md` for the hub, buildings and randomizer.

## How a run works

1. **Park your party.** Your current Pokémon are moved to the PC for the length of the run and come back when it ends.
2. **Draft a team.** You have **10 points** (more with the Draft Office upgrade). The list starts with all **27 classic starters**: Charmander, Cyndaquil, Treecko, Chimchar, Fennekin, Froakie, Scorbunny, Sobble and Sprigatito cost 4, the rest cost 3. After them comes every Pokémon you've caught, as the first stage of its family: catch a Pidgeot and Pidgey is draftable in the next run. Unlock costs follow the final evolution's base stat total (1–6), sub-legendaries cost 7 and box legendaries/mythicals 8. The list shows 8 at a time with Next/Previous page. Pick up to 6 (no duplicates), then start once you have at least one. Starters are level 5.
3. **Clear waves.** Each wave is a wild battle that scales with the wave number (`level = 3 + wave × 1.1`). Wild Pokémon are evolved to match their level. Some waves are double battles.
4. **Bosses.** Every 10th wave is a boss: +3 levels, perfect IVs and a Sitrus Berry. You can't run from a boss. Beating one fully heals your team and moves you to the next biome.
5. **Rewards.** After every cleared wave, pick 1 of 3 rewards. Better tiers (★ / ★★ / ★★★) show up more often in later waves.
6. **Between waves** you can go to the next wave, take a break (walk around, save, use the bag, then talk to the scientist to continue), or end the run.
7. **Winning or losing.** If your whole team faints, the run ends. There's no whiteout. Clear **wave 50**, a level-63 Rayquaza, to win. Your best wave is saved.
8. **Rogue Coins.** Every run pays out: 5 per cleared wave, 50 per boss beaten, 500 for a win (cap 9,999). Spend them at the hub's Builder.

| Waves | Biome      | Battle background |
|-------|------------|-------------------|
| 1–10  | Grassland  | Grass             |
| 11–20 | Cave       | Cave              |
| 21–30 | Open Sea   | Water             |
| 31–40 | Volcano    | Mountain          |
| 41–50 | Sky Pillar | Rayquaza          |

## Where things live

| What | File |
|------|------|
| Run logic, draft pool, biomes, reward table | `src/roguelite.c` |
| Constants (wave counts, budget, menu ids) | `include/constants/roguelite.h` |
| Event script and text | `data/scripts/roguelite.inc` |
| Specials | `data/specials.inc` (bottom) |
| No-whiteout and biome background hooks | `src/battle_setup.c` |
| Hub map, buildings, Builder | `data/maps/RogueHub/` |
| Tests | `test/roguelite.c` (`make check TESTS="(Roguelite)"`) |

Saved state reuses unused slots: `FLAG_ROGUELITE_ACTIVE` (0x20) and `VAR_ROGUELITE_*` (wave, best wave, draft budget, and 6 slots that remember where your party was parked in the PC).

## Tuning

- **Run length:** change `ROGUELITE_WAVES_PER_BIOME` / `ROGUELITE_BIOME_COUNT`, then add entries to `sBiomes`.
- **Difficulty curve:** `GetWaveLevel()` in `src/roguelite.c`.
- **Draft:** edit `sStarterPool` (species and cost), `ROGUELITE_STARTER_BUDGET` and `GetUnlockCost()`.
- **Rewards:** edit `sRewards` and the tier odds in `RollRewardTier()`.

## Known limits and ideas for later

- Items you win go into your normal Bag and stay there after the run.
- During a break you can heal for free at the hub Healer once it's built.
- No trainer waves yet. Random trainer parties would be the next big feature.
- Ideas: a shop every few waves, run modifiers, egg moves and hidden abilities as unlockable draft upgrades.
