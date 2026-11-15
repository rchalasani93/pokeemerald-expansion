# Elite Rogue Dex

An offline Pokédex for this hack, in the style of the Elite Redux dex. It is one
HTML file with the data and sprites inside it: `dex/index.html`. Double-click it
to open it in a browser. No server, no internet, no install.

## What it shows

| Tab | Contents |
|---|---|
| Species | Sprite, types, base stats, abilities, **innates**, type matchups, evolution line, level-up / TM / egg moves, wild locations, and where it appears in a run |
| Moves | Type, category, power, accuracy, PP, priority, flags, and every Pokémon that learns it |
| Abilities | Effect text, and who has it as an innate or as a regular ability |
| Locations | Wild encounters per map, by method and level |
| Trainers | Every trainer's team: level, item, ability, moves |
| Run | Roguelite biomes, wild and boss pools, draft costs, reward tiers |

Species filters: name or dex number, up to two types, ability or innate, a move
it learns, and sorting by any base stat or total. Links are shareable within the
file, for example `index.html#species/5`.

## Rebuilding after you change the game

    python3 dev_scripts/dex/build_dex.py

It takes about 10 seconds and needs the same toolchain as the ROM build
(`arm-none-eabi-cpp`) plus Python 3. Rebuild whenever you change species,
moves, abilities, innates, encounters, trainers or `src/roguelite.c`.

## How it works

`build_dex.py` runs the project's C preprocessor over the game sources, so
config switches, macros and forms resolve exactly as they do in the ROM. Values
such as `.power = B_UPDATED_MOVE_DATA >= GEN_6 ? 90 : 95` come out as the number
the game uses. `cparse.py` is a small C initializer parser and constant
evaluator. `template.html` is the page; the script fills in the data.

Sprites are the first frame of each `anim_front.png`, cropped and given a
transparent background.

The page never loads anything from the network, and game text is inserted as
plain text, never as HTML.
