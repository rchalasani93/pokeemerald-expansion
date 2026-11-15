#!/usr/bin/env python3
"""Regenerate src/data/pokemon/innates.h from Elite Redux's config data.

Usage:
    git clone --depth 1 https://github.com/Elite-Redux/er-config /tmp/er-config
    python3 dev_scripts/elite_redux/gen_innates.py /tmp/er-config/SpeciesList.textproto

Only innates whose ability exists in pokeemerald-expansion are kept; Elite
Redux-exclusive abilities are dropped and reported. Species aliases (two names
for the same species id) are written once.
"""
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
OUT = os.path.join(ROOT, 'src', 'data', 'pokemon', 'innates.h')


def enum_members(path, prefix):
    src = open(path, encoding='utf-8').read()
    return dict(re.findall(r'^\s+(' + prefix + r'\w+)\s*=\s*([^,\n]+),', src, re.M)), \
        set(re.findall(r'^\s+(' + prefix + r'\w+)\s*[,=]', src, re.M))


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    er = open(sys.argv[1], encoding='utf-8').read()
    species_vals, species = enum_members(os.path.join(ROOT, 'include/constants/species.h'), 'SPECIES_')
    _, abilities = enum_members(os.path.join(ROOT, 'include/constants/abilities.h'), 'ABILITY_')

    innates, missing, seen = {}, {}, set()
    for block in er.split('\nspecies {')[1:]:
        m = re.search(r'^  id: (SPECIES_\w+)', block, re.M)
        if not m or m.group(1) not in species:
            continue
        name = m.group(1)
        value = species_vals.get(name, name).strip()
        while value.startswith('SPECIES_') and value in species_vals and species_vals[value].strip() != value:
            value = species_vals[value].strip()  # resolve aliases to the species id they point at
        if value in seen:
            continue  # alias of a species that is already covered
        kept = []
        for ability in re.findall(r'^  innate: (ABILITY_\w+)', block, re.M):
            if ability in abilities and ability != 'ABILITY_NONE':
                kept.append(ability)
            elif ability != 'ABILITY_NONE':
                missing[ability] = missing.get(ability, 0) + 1
        if kept:
            seen.add(value)
            innates[name] = kept

    lines = [
        '// Generated from Elite Redux (github.com/Elite-Redux/er-config, SpeciesList.textproto).',
        '// Only innates whose ability exists in pokeemerald-expansion are kept;',
        '// Elite Redux-exclusive abilities are dropped. Regenerate with dev_scripts/elite_redux/gen_innates.py.',
        '',
        'const u16 gSpeciesInnatesTable[NUM_SPECIES][MAX_MON_INNATES] =',
        '{',
    ]
    for name in sorted(innates):
        slots = (innates[name] + ['ABILITY_NONE'] * 3)[:3]
        lines.append('    [%s] = {%s},' % (name, ', '.join(slots)))
    lines.append('};')
    open(OUT, 'w', encoding='utf-8').write('\n'.join(lines) + '\n')

    print('%d species with innates written to %s' % (len(innates), os.path.relpath(OUT, ROOT)))
    print('Most common Elite Redux-only innates that were dropped:')
    for ability, count in sorted(missing.items(), key=lambda x: -x[1])[:15]:
        print('  %-28s %d' % (ability, count))


if __name__ == '__main__':
    main()
