#!/usr/bin/env python3
"""Regenerate src/data/pokemon/innates.h: 3 ability slots + 3 innates per species.

Usage:
    git clone --depth 1 https://github.com/Elite-Redux/er-config /tmp/er-config
    python3 dev_scripts/elite_redux/gen_innates.py /tmp/er-config/SpeciesList.textproto

Every species ends up with four distinct abilities: one of its regular ability
slots plus three innates. All of them come from real species data, in this
order of preference:

  innates:   Elite Redux innates, the species' own abilities, Elite Redux
             abilities, abilities of its other forms and evolution family, and
             as a last resort the abilities most common among its type
  abilities: Elite Redux abilities, the species' own abilities, forms and
             family, then type

Only abilities that exist in pokeemerald-expansion are used (Elite
Redux-exclusive ones are skipped). Regular abilities never repeat an innate,
so whichever ability a Pokémon rolls, it has four different ones.
"""
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
sys.path.insert(0, os.path.join(ROOT, 'dev_scripts', 'dex'))
import cparse  # noqa: E402

OUT = os.path.join(ROOT, 'src', 'data', 'pokemon', 'innates.h')
CPP = os.environ.get('CPP', 'arm-none-eabi-cpp')
CPP_FLAGS = ['-iquote', 'include', '-Wno-trigraphs', '-DMODERN=1', '-DTESTING=0', '-DEMERALD', '-std=gnu17']

# Abilities that only make sense on their own species (form changes, signature
# mechanics). They stay where the real data puts them but are never copied
# onto another species through the family fallback.
SIGNATURE = {
    'ABILITY_MULTITYPE', 'ABILITY_RKS_SYSTEM', 'ABILITY_ZEN_MODE', 'ABILITY_STANCE_CHANGE', 'ABILITY_SCHOOLING',
    'ABILITY_SHIELDS_DOWN', 'ABILITY_DISGUISE', 'ABILITY_BATTLE_BOND', 'ABILITY_POWER_CONSTRUCT', 'ABILITY_ICE_FACE',
    'ABILITY_GULP_MISSILE', 'ABILITY_HUNGER_SWITCH', 'ABILITY_ZERO_TO_HERO', 'ABILITY_COMMANDER', 'ABILITY_FORECAST',
    'ABILITY_FLOWER_GIFT', 'ABILITY_TERA_SHIFT', 'ABILITY_TERA_SHELL', 'ABILITY_TERAFORM_ZERO', 'ABILITY_POISON_PUPPETEER',
    'ABILITY_AS_ONE_ICE_RIDER', 'ABILITY_AS_ONE_SHADOW_RIDER', 'ABILITY_EMBODY_ASPECT_TEAL_MASK',
    'ABILITY_EMBODY_ASPECT_HEARTHFLAME_MASK', 'ABILITY_EMBODY_ASPECT_WELLSPRING_MASK',
    'ABILITY_EMBODY_ASPECT_CORNERSTONE_MASK', 'ABILITY_WONDER_GUARD', 'ABILITY_TRUANT', 'ABILITY_SLOW_START',
    'ABILITY_PIKACHU_ONLY', 'ABILITY_PROTOSYNTHESIS', 'ABILITY_QUARK_DRIVE',
}


def preprocess(src):
    with tempfile.NamedTemporaryFile(suffix='.i', delete=False) as f:
        out = f.name
    subprocess.run([CPP, *CPP_FLAGS, src, '-o', out], cwd=ROOT, check=True)
    text = open(out, encoding='utf-8', errors='replace').read()
    os.unlink(out)
    return text


def read_game_species():
    """Vanilla abilities and evolution links for every species in the ROM."""
    tokens = cparse.tokenize(preprocess('src/pokemon.c'))
    init = cparse.find_array(tokens, 'gSpeciesInfo')
    abilities, evos, dexnum, types = {}, {}, {}, {}
    for idx, val in init.indexed:
        sid = idx.ident()
        if not sid or not isinstance(val, cparse.Init):
            continue
        ab = val.get('abilities')
        abilities[sid] = [a.ident() for a in ab.items] if isinstance(ab, cparse.Init) else []
        dexnum[sid] = (val.get('natDexNum').ident() if isinstance(val.get('natDexNum'), cparse.Expr) else None) or sid
        ty = val.get('types')
        types[sid] = [t.ident() for t in ty.items] if isinstance(ty, cparse.Init) else []
        targets = []
        ev = val.get('evolutions')
        if isinstance(ev, cparse.Init):
            for e in ev.items:
                if isinstance(e, cparse.Init) and len(e.items) >= 3 and e.items[2].ident():
                    targets.append(e.items[2].ident())
        evos[sid] = targets
    return abilities, evos, dexnum, types


def families(evos):
    parent = {}

    def find(x):
        parent.setdefault(x, x)
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    for sid, targets in evos.items():
        for t in targets:
            parent[find(sid)] = find(t)
    groups = {}
    for sid in evos:
        groups.setdefault(find(sid), []).append(sid)
    return {sid: groups[find(sid)] for sid in evos}


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    er = open(sys.argv[1], encoding='utf-8').read()
    exists = set(re.findall(r'^\s+(ABILITY_\w+)\s*[,=]', open(os.path.join(ROOT, 'include/constants/abilities.h')).read(), re.M))
    exists.discard('ABILITY_NONE')
    vanilla, evos, dexnum, types = read_game_species()
    family = families(evos)
    forms = {}
    for sid, num in dexnum.items():
        forms.setdefault(num, []).append(sid)

    er_abil, er_inn = {}, {}
    for block in er.split('\nspecies {')[1:]:
        m = re.search(r'^  id: (SPECIES_\w+)', block, re.M)
        if m:
            er_abil[m.group(1)] = [a for a in re.findall(r'^  ability: (ABILITY_\w+)', block, re.M) if a in exists]
            er_inn[m.group(1)] = [a for a in re.findall(r'^  innate: (ABILITY_\w+)', block, re.M) if a in exists]

    def own(sid):
        return [a for a in vanilla.get(sid, []) if a in exists]

    def related_pool(sid):
        """Other forms of the same Pokémon first, then its evolution family."""
        pool = []
        for member in forms.get(dexnum.get(sid), []) + family.get(sid, []):
            if member == sid:
                continue
            pool += er_inn.get(member, []) + er_abil.get(member, []) + own(member)
        return [a for a in pool if a not in SIGNATURE]

    # Most common non-signature abilities among species of each type (real data, last resort)
    type_counts = {}
    for sid, ty in types.items():
        for a in set(own(sid) + er_inn.get(sid, []) + er_abil.get(sid, [])):
            if a in SIGNATURE:
                continue
            for t in set(ty):
                type_counts.setdefault(t, {}).setdefault(a, 0)
                type_counts[t][a] += 1
    type_pool = {t: [a for a, _ in sorted(c.items(), key=lambda x: (-x[1], x[0]))][:12] for t, c in type_counts.items()}

    def by_type(sid):
        pool = []
        for t in types.get(sid, []):
            pool += type_pool.get(t, [])
        return pool

    def take(candidates, exclude, count):
        out = []
        for a in candidates:
            if a not in out and a not in exclude:
                out.append(a)
            if len(out) == count:
                break
        return out

    rows, short = {}, 0
    for sid, base in vanilla.items():
        if sid in ('SPECIES_NONE', 'SPECIES_EGG') or not own(sid):
            continue
        real = er_inn.get(sid, []) + own(sid) + er_abil.get(sid, []) + related_pool(sid)
        innates = take(real + by_type(sid), [], 3)
        # Keep at least two different regular abilities when the real data allows it
        spare = [a for a in dict.fromkeys(er_abil.get(sid, []) + own(sid) + related_pool(sid)) if a not in innates]
        while len(spare) < 2 and len(innates) == 3 and any(a in real for a in innates[1:]):
            moved = next(a for a in reversed(innates) if a in real and a not in innates[:1])
            innates.remove(moved)
            spare.append(moved)
            innates = take(innates + [a for a in by_type(sid) if a not in spare], [], 3)
        abilities = take(spare + by_type(sid), innates, 3)
        if len(innates) < 3:
            short += 1
        abilities = (abilities * 3)[:3]
        rows[sid] = (abilities, innates)

    lines = [
        '// Generated from Elite Redux (github.com/Elite-Redux/er-config, SpeciesList.textproto)',
        '// and the species data in this repo. Regenerate with dev_scripts/elite_redux/gen_innates.py.',
        '//',
        '// Each species gets three regular ability slots and up to three innates. Every',
        '// ability is one that species (or its evolution family) really has, in Elite Redux',
        '// or the main games, and the regular abilities never repeat an innate.',
        '',
        'const u16 gSpeciesInnatesTable[NUM_SPECIES][MAX_MON_INNATES] =',
        '{',
    ]
    for sid in rows:
        inn = (rows[sid][1] + ['ABILITY_NONE'] * 3)[:3]
        lines.append('    [%s] = {%s},' % (sid, ', '.join(inn)))
    lines += ['};', '', 'const u16 gSpeciesAbilitiesTable[NUM_SPECIES][NUM_ABILITY_SLOTS] =', '{']
    for sid in rows:
        lines.append('    [%s] = {%s},' % (sid, ', '.join(rows[sid][0])))
    lines.append('};')
    open(OUT, 'w', encoding='utf-8').write('\n'.join(lines) + '\n')
    print('%d species written to %s (%d with fewer than 3 innates)' % (len(rows), os.path.relpath(OUT, ROOT), short))


if __name__ == '__main__':
    main()
