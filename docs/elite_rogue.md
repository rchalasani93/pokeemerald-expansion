# Elite Rogue: Elite Redux mechanics on top of the Roguelite mode

This branch (`feature/elite-rogue`) builds on `feature/roguelite-mode` and adds the main Elite Redux mechanics plus Emerald Rogue-style route choices to the run. See `docs/roguelite_mode.md` for how a run works.

## 1. Innates (up to 3 always-on abilities)

Every Pokémon keeps its regular ability. On top of that, its species can have up to **3 innates**, which are abilities that are always active, like in Elite Redux.

- **Data:** `src/data/pokemon/innates.h` is generated from Elite Redux's own config (`Elite-Redux/er-config`). 1,222 species get innates.
- **Elite Redux-only abilities are skipped.** About 38% of ER's innate slots use abilities that only exist in Elite Redux (Predator, Power Core, Hyper Aggressive, …). Those slots are left out, so some species have fewer innates than in ER.
- **Regenerating:** `python3 dev_scripts/elite_redux/gen_innates.py <path>/er-config/SpeciesList.textproto`
- **Seeing them:** the summary screen lists a Pokémon's innates under its ability. When an innate triggers in battle, its own name shows in the ability popup and in battle messages.
- **Rules:** innates are suppressed by the same things as abilities: Gastro Acid, Neutralizing Gas, and Mold Breaker for breakable abilities. Ability Shield protects them. Skill Swap, Entrainment and similar moves only change the regular ability. An innate that matches the regular ability doesn't apply twice.
- **Turning it off:** `B_INNATES` in `include/config/battle.h`.

### What innates cover

| Works with innates | How |
|---|---|
| Switch-in abilities (Intimidate, Drizzle, …). Several can fire one after another. | `AbilityBattleEffectsAllSlots` (one slot per event step) |
| Status-immunity abilities, end-of-turn abilities (Speed Boost, Rain Dish, …), on-hit abilities (Rough Skin, Static, …) and attacker effects (Poison Touch, …) | same mechanism |
| Absorbing abilities (Volt Absorb, Flash Fire, Sap Sipper, …) | `CanAbilityAbsorbMove` |
| Damage modifiers: base power, attack, defense, final modifiers, partner abilities. Stacking innates multiply together. | `FOREACH_ABILITY_SLOT` around the ability switches |
| Accuracy and evasion, -ate type changes, STAB (Adaptability), crits, speed, priority, grounding (Levitate), contact and Klutz checks, AI awareness | checks rewritten to `HasAbilityOrInnate` / `BattlerHasAbility` |

**Not covered yet:** abilities whose logic still compares a single local `ability` variable in code that doesn't know which battler it belongs to. These are mostly party-wide and out-of-battle effects, like Pickup or field effects of the lead Pokémon. Form-change abilities (Zen Mode, Power Construct, …) only work as the regular ability.

## 2. Harder trainers (Elite Redux-style AI)

- `B_ELITE_TRAINER_AI` (default `TRUE`): every regular trainer battles with **Smart Trainer AI plus move and switch prediction**. Frontier, Safari, the first battle and recorded battles are left alone.
- During a run, wild Pokémon also use smart AI, and bosses know your whole team (Omniscient + HP Aware).

## 3. Run upgrades (Emerald Rogue-style)

- **Route choice between waves:** *Wild route*, *Trainer route ★* (rewards roll one tier better), and roughly every third wave a *Rest stop* that fully heals with no fight. Boss waves give no choice.
- **Rogue trainers:** `TRAINER_ROGUELITE` gets a team built when the battle starts:
  - 2 Pokémon in waves 1–10, rising to 6 by the end
  - level = wave level + 2
  - perfect IVs, 252/252 EVs, Jolly/Adamant or Timid/Modest natures
  - held items with an item clause, and a species clause
  - three strong attacks of different types plus a setup or utility move, chosen from level-up and TM/tutor learnsets
- Losing to a run trainer ends the run. You don't white out.

## 4. Battle UI

- `B_SHOW_DAMAGE_PREVIEW` (default `TRUE`): the move menu shows the estimated damage range against the opposing Pokémon in place of "TYPE/". For example, **Fire 34-41%**. It's calculated with the real damage formula (abilities, innates, items, weather, terrain) from the lowest to the highest damage roll.
- The move info panel (`B_SHOW_MOVE_DESCRIPTION`) still shows power, accuracy and category.

## Tests

- `test/battle/innates.c`: innate behaviour. Tests run with no ER data, and each test declares its own innates with `Test_SetSpeciesInnates`. Run with `make check TESTS="Innates"`.
- `test/roguelite.c`: run logic, trainer team generation and route menus. Run with `make check TESTS="(Roguelite)"`.
- The rest of the expansion test suite runs with no innates, which shows that the ability refactor doesn't change normal battles.

## Credits

Innate data comes from **Elite Redux** (darkyy92 and the ER team). They made it open source so other hacks could use it. The route and run design is inspired by **Emerald Rogue** (Pokabbie). Neither project's code was copied in; this is a re-implementation on pokeemerald-expansion.
