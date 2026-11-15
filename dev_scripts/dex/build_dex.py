#!/usr/bin/env python3
"""Build a standalone offline Pokédex (dex/index.html) from this repo's data.

    python3 dev_scripts/dex/build_dex.py

Runs the project's C preprocessor (arm-none-eabi-cpp) over the game sources so
every config switch, macro and form is resolved exactly as the ROM sees it,
then reads species, learnsets, moves, abilities, innates, types, wild
encounters, trainers and the roguelite run tables. Output is one HTML file
with the data and sprites embedded; open it in any browser, no server needed.
"""
import base64
import json
import os
import re
import struct
import subprocess
import sys
import tempfile
import unicodedata
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
sys.path.insert(0, HERE)
import cparse  # noqa: E402
from cparse import Expr, Init, c_string, evaluate  # noqa: E402

OUT_DIR = os.path.join(ROOT, 'dex')
CPP = os.environ.get('CPP', 'arm-none-eabi-cpp')
CPP_FLAGS = ['-iquote', 'include', '-Wno-trigraphs', '-DMODERN=1', '-DTESTING=0', '-DEMERALD', '-std=gnu17']


def log(msg):
    print(msg, file=sys.stderr)


def preprocess(src):
    with tempfile.NamedTemporaryFile(suffix='.i', delete=False) as f:
        out = f.name
    subprocess.run([CPP, *CPP_FLAGS, src, '-o', out], cwd=ROOT, check=True)
    text = open(out, encoding='utf-8', errors='replace').read()
    os.unlink(out)
    return text


def ident(v):
    return v.ident() if isinstance(v, Expr) else None


def num(v, env, default=0):
    if isinstance(v, Expr):
        r = evaluate(v, env)
        if isinstance(r, (int, float)):
            return r
    return default


def pretty_constant(name, prefix):
    s = name[len(prefix):] if name.startswith(prefix) else name
    s = re.sub(r'(?<=[A-Z])(?=\d)', '_', s)  # ROUTE101 -> ROUTE_101
    return ' '.join(w.capitalize() for w in s.split('_') if w)


def form_label(constant, name):
    """'SPECIES_CHARIZARD_MEGA_X' + 'Charizard' -> 'Mega X'; '' for base forms."""
    plain = unicodedata.normalize('NFKD', name.replace('♀', '_F').replace('♂', '_M'))
    base = re.sub(r'[^A-Z0-9]', '', plain.encode('ascii', 'ignore').decode().upper())
    rest = constant[len('SPECIES_'):]
    squashed = ''
    for i, ch in enumerate(rest):
        if ch != '_':
            squashed += ch
        if squashed == base:
            tail = rest[i + 1:].strip('_')
            return pretty_constant(tail, '') if tail else ''
    return ''


# --------------------------------------------------------------------------
# Sprites: crop the first frame of anim_front.png and make index 0 transparent
# --------------------------------------------------------------------------

def png_chunks(data):
    pos = 8
    while pos < len(data):
        length = struct.unpack('>I', data[pos:pos + 4])[0]
        ctype = data[pos + 4:pos + 8]
        yield ctype, data[pos + 8:pos + 8 + length]
        pos += 12 + length


def make_chunk(ctype, body):
    return struct.pack('>I', len(body)) + ctype + body + struct.pack('>I', zlib.crc32(ctype + body) & 0xffffffff)


def front_sprite_data_uri(path):
    try:
        data = open(path, 'rb').read()
    except OSError:
        return None
    if data[:8] != b'\x89PNG\r\n\x1a\n':
        return None
    ihdr = plte = None
    idat = b''
    for ctype, body in png_chunks(data):
        if ctype == b'IHDR':
            ihdr = body
        elif ctype == b'PLTE':
            plte = body
        elif ctype == b'IDAT':
            idat += body
    if ihdr is None or plte is None:
        return None
    width, height, depth, ctype_ = struct.unpack('>IIBB', ihdr[:10])
    if ctype_ != 3 or ihdr[12] != 0:  # indexed, non-interlaced only
        return None
    frame_h = min(width, height)  # sheets are frames stacked vertically
    stride = 1 + (width * depth + 7) // 8
    raw = zlib.decompress(idat)[:stride * frame_h]
    new_ihdr = struct.pack('>II', width, frame_h) + ihdr[8:]
    out = (b'\x89PNG\r\n\x1a\n' + make_chunk(b'IHDR', new_ihdr) + make_chunk(b'PLTE', plte)
           + make_chunk(b'tRNS', b'\x00') + make_chunk(b'IDAT', zlib.compress(raw, 9)) + make_chunk(b'IEND', b''))
    return 'data:image/png;base64,' + base64.b64encode(out).decode()


# --------------------------------------------------------------------------
# Extraction
# --------------------------------------------------------------------------

def main():
    log('Preprocessing sources...')
    texts = {name: preprocess('src/%s.c' % name) for name in ('pokemon', 'move', 'battle_main', 'item', 'innates')}
    tokens = {name: cparse.tokenize(text) for name, text in texts.items()}

    log('Reading enums...')
    env = {}
    for toks in tokens.values():
        env.update(cparse.parse_enums(toks))

    # ---- types
    types_init = cparse.find_array(tokens['battle_main'], 'gTypesInfo')
    type_names = {}
    for idx, val in types_init.indexed:
        tid = ident(idx)
        name = c_string(val.get('name'))
        if tid and name:
            type_names[tid] = name
    chart_init = cparse.find_array(tokens['battle_main'], 'gTypeEffectivenessTable')
    type_order = sorted(type_names, key=lambda t: env.get(t, 999))
    chart = {}
    for idx, row in chart_init.indexed:
        atk = ident(idx)
        if atk not in type_names:
            continue
        vals = [num(v, env, 4096) / 4096 for v in row.items]
        chart[atk] = {t: round(vals[env[t]], 2) for t in type_order if env.get(t, 99) < len(vals)}
    hidden_types = {'TYPE_NONE', 'TYPE_MYSTERY', 'TYPE_STELLAR'}
    types = [t for t in type_order if t not in hidden_types]

    # ---- abilities
    abil_init = cparse.find_array(tokens['pokemon'], 'gAbilitiesInfo')
    abilities = {}
    for idx, val in abil_init.indexed:
        aid = ident(idx)
        name = c_string(val.get('name'))
        if aid and name and aid != 'ABILITY_NONE':
            abilities[aid] = {'name': name, 'desc': c_string(val.get('description')) or ''}

    # ---- moves
    move_init = cparse.find_array(tokens['move'], 'gMovesInfo')
    flag_names = {
        'makesContact': 'Contact', 'soundMove': 'Sound', 'punchingMove': 'Punch', 'bitingMove': 'Bite',
        'pulseMove': 'Pulse', 'ballisticMove': 'Ballistic', 'slicingMove': 'Slicing', 'windMove': 'Wind',
        'powderMove': 'Powder', 'danceMove': 'Dance', 'healingMove': 'Healing',
    }
    moves = {}
    for idx, val in move_init.indexed:
        mid = ident(idx)
        name = c_string(val.get('name'))
        if not mid or not name or mid == 'MOVE_NONE':
            continue
        # Z-Moves and Max Moves are battle gimmicks, not learnable moves
        value = env.get(mid, 0)
        if any(env.get(lo, 1) <= value <= env.get(hi, 0) for lo, hi in (('FIRST_Z_MOVE', 'LAST_Z_MOVE'), ('FIRST_MAX_MOVE', 'LAST_MAX_MOVE'))):
            continue
        cat = ident(val.get('category')) or ''
        moves[mid] = {
            'name': name,
            'type': ident(val.get('type')) or 'TYPE_NORMAL',
            'cat': {'DAMAGE_CATEGORY_PHYSICAL': 'Physical', 'DAMAGE_CATEGORY_SPECIAL': 'Special'}.get(cat, 'Status'),
            'pow': int(num(val.get('power'), env)),
            'acc': int(num(val.get('accuracy'), env)),
            'pp': int(num(val.get('pp'), env)),
            'pri': int(num(val.get('priority'), env)),
            'desc': c_string(val.get('description')) or '',
            'flags': [label for field, label in flag_names.items() if num(val.get(field), env) == 1],
        }

    # ---- learnsets
    level_sets = cparse.find_all_arrays(tokens['pokemon'], r's\w+LevelUpLearnset')
    teach_sets = cparse.find_all_arrays(tokens['pokemon'], r's\w+TeachableLearnset')
    egg_sets = cparse.find_all_arrays(tokens['pokemon'], r's\w+EggMoveLearnset')

    def level_learnset(sym):
        init = level_sets.get(sym)
        out = []
        for item in (init.items if init else []):
            if isinstance(item, Init):
                m = ident(item.get('move'))
                if m in moves:
                    out.append([int(num(item.get('level'), env)), m])
        return out

    def flat_learnset(table, sym):
        init = table.get(sym)
        return [ident(v) for v in (init.items if init else []) if ident(v) in moves]

    # ---- sprites
    pic_paths = dict(re.findall(r'(gMonFrontPic_\w+)\[\]\s*=\s*INCGFX_U32\("([^"]+)"', texts['pokemon']))

    # ---- innates
    innate_init = cparse.find_array(tokens['innates'], 'gSpeciesInnatesTable')
    innates = {}
    if innate_init:
        for idx, val in innate_init.indexed:
            sid = ident(idx)
            if sid:
                innates[sid] = [ident(v) for v in val.items if ident(v) in abilities]
    # Regular abilities in game: gSpeciesAbilitiesTable overrides the species data
    regular_init = cparse.find_array(tokens['innates'], 'gSpeciesAbilitiesTable')
    regular = {}
    if regular_init:
        for idx, val in regular_init.indexed:
            sid = ident(idx)
            if sid:
                regular[sid] = [ident(v) for v in val.items]

    # ---- species
    log('Reading species...')
    species_init = cparse.find_array(tokens['pokemon'], 'gSpeciesInfo')
    species = {}
    sprites = {}
    skip_flags = ('isTotem', 'isGigantamax', 'isTeraForm', 'isCosmetic')
    for idx, val in species_init.indexed:
        sid = ident(idx)
        if not sid or sid in ('SPECIES_NONE', 'SPECIES_EGG') or not isinstance(val, Init):
            continue
        name = c_string(val.get('speciesName'))
        hp = int(num(val.get('baseHP'), env))
        if not name or hp == 0 or any(num(val.get(f), env) == 1 for f in skip_flags):
            continue
        type_list = [ident(t) for t in val.get('types').items] if isinstance(val.get('types'), Init) else ['TYPE_NORMAL']
        type_list = [t for i, t in enumerate(type_list) if t and t not in type_list[:i]]
        abil = [ident(a) for a in val.get('abilities').items] if isinstance(val.get('abilities'), Init) else []
        if sid in regular and any(a and a != 'ABILITY_NONE' for a in regular[sid]):
            abil = regular[sid]
        evos = []
        if isinstance(val.get('evolutions'), Init):
            for e in val.get('evolutions').items:
                if not isinstance(e, Init) or len(e.items) < 3:
                    continue
                method = ident(e.items[0])
                target = ident(e.items[2])
                if method and target:
                    param = e.items[1]
                    pval = ident(param) or num(param, env)
                    evos.append([method, pval, target])
        form_note = form_label(sid, name)
        base_name = sid[len('SPECIES_'):]
        entry = {
            'name': name,
            'form': form_note,
            'dex': env.get(ident(val.get('natDexNum')) or '', 0),
            'types': type_list,
            'stats': [hp] + [int(num(val.get(f), env)) for f in ('baseAttack', 'baseDefense', 'baseSpAttack', 'baseSpDefense', 'baseSpeed')],
            'abilities': [a if a in abilities else None for a in (abil + [None, None, None])[:3]],
            'innates': innates.get(sid, []),
            'evos': evos,
            'category': c_string(val.get('categoryName')) or '',
            'desc': c_string(val.get('description')) or '',
            'height': int(num(val.get('height'), env)),
            'weight': int(num(val.get('weight'), env)),
            'catchRate': int(num(val.get('catchRate'), env)),
            'eggGroups': [pretty_constant(ident(g) or '', 'EGG_GROUP_') for g in (val.get('eggGroups').items if isinstance(val.get('eggGroups'), Init) else [])],
            'level': level_learnset(ident(val.get('levelUpLearnset'))),
            'tm': flat_learnset(teach_sets, ident(val.get('teachableLearnset'))),
            'egg': flat_learnset(egg_sets, ident(val.get('eggMoveLearnset'))),
            'const': base_name,
        }
        species[sid] = entry
        pic = pic_paths.get(ident(val.get('frontPic')) or '')
        if pic:
            uri = front_sprite_data_uri(os.path.join(ROOT, pic))
            if uri:
                sprites[sid] = uri

    # pre-evolution links
    for sid, s in species.items():
        for method, param, target in s['evos']:
            if target in species:
                species[target].setdefault('prevo', sid)

    # ---- items (names only, for trainers and run rewards)
    item_init = cparse.find_array(tokens['item'], 'gItemsInfo')
    items = {}
    for idx, val in item_init.indexed:
        iid = ident(idx)
        name = c_string(val.get('name'))
        if iid and name:
            items[iid] = name

    # ---- wild encounters
    log('Reading encounters and trainers...')
    locations = []
    merged = {}  # map name -> {(method, species): [min level, max level]}
    enc = json.load(open(os.path.join(ROOT, 'src/data/wild_encounters.json'), encoding='utf-8'))
    for group in enc.get('wild_encounter_groups', []):
        if not group.get('for_maps'):
            continue
        for e in group.get('encounters', []):
            mapname = e.get('map')
            if not mapname:
                continue
            suffix = e.get('base_label', '')
            time = ' (%s)' % suffix.split('_')[-1].capitalize() if re.search(r'_(Morning|Day|Evening|Night)$', suffix) else ''
            # Tables that share a map name (game-version variants, Altering Cave sets) are merged into one entry
            slots = merged.setdefault(pretty_constant(mapname, 'MAP_') + time, {})
            for key, label in (('land_mons', 'Grass'), ('water_mons', 'Surfing'), ('rock_smash_mons', 'Rock Smash'),
                               ('fishing_mons', 'Fishing'), ('hidden_mons', 'Hidden')):
                block = e.get(key)
                if not block:
                    continue
                for mon in block.get('mons', []):
                    sp = mon.get('species')
                    if sp in species:
                        rec = slots.setdefault((label, sp), [99, 0])
                        rec[0] = min(rec[0], mon.get('min_level', 0))
                        rec[1] = max(rec[1], mon.get('max_level', 0))
    method_order = ['Grass', 'Surfing', 'Fishing', 'Rock Smash', 'Hidden']
    for name, slots in merged.items():
        if slots:
            locations.append({
                'map': name,
                'mons': sorted([[method, sp, lo, hi] for (method, sp), (lo, hi) in slots.items()],
                               key=lambda r: (method_order.index(r[0]), r[2], r[3])),
            })
    locations.sort(key=lambda l: l['map'])

    # ---- trainers
    name_to_species = {}
    for sid, s in species.items():
        name_to_species.setdefault(s['name'].lower(), sid)
    trainers = []
    party_text = open(os.path.join(ROOT, 'src/data/trainers.party'), encoding='utf-8').read()
    parts = re.split(r'^=== (TRAINER_\w+) ===\s*$', party_text, flags=re.M)
    for tid, body in zip(parts[1::2], parts[2::2]):
        chunks = [c for c in re.split(r'\n\s*\n', body.strip()) if c.strip()]
        if not chunks:
            continue
        header = dict(re.findall(r'^(\w[\w ]*):\s*(.+)$', chunks[0], re.M))
        party = []
        for chunk in chunks[1:]:
            lines = [l.strip() for l in chunk.strip().split('\n') if l.strip() and not l.strip().startswith('#')]
            if not lines:
                continue
            first = lines[0]
            item = None
            if '@' in first:
                first, item = [x.strip() for x in first.split('@', 1)]
            m = re.match(r'^(.*?)\s*\((.+)\)$', first)
            spname = m.group(2) if m else first
            spname = re.sub(r'\s*\((M|F)\)$', '', spname).strip()
            mon = {'species': name_to_species.get(spname.lower()) or spname, 'item': item, 'level': None,
                   'ability': None, 'moves': []}
            for line in lines[1:]:
                if line.startswith('- '):
                    mon['moves'].append(line[2:].strip())
                elif ':' in line:
                    k, v = [x.strip() for x in line.split(':', 1)]
                    if k == 'Level':
                        mon['level'] = v
                    elif k == 'Ability':
                        mon['ability'] = v
            party.append(mon)
        if party:
            trainers.append({
                'id': tid,
                'name': header.get('Name', ''),
                'class': header.get('Class', ''),
                'double': header.get('Double Battle', 'No') == 'Yes',
                'party': party,
            })

    # ---- roguelite run data (src/roguelite.c)
    run = None
    rogue_path = os.path.join(ROOT, 'src/roguelite.c')
    if os.path.exists(rogue_path):
        rsrc = open(rogue_path, encoding='utf-8').read()
        pools = {name: re.findall(r'SPECIES_\w+', body)
                 for name, body in re.findall(r'static const u16 (s\w+)\[\] = \{(.*?)\};', rsrc, re.S)}
        biomes = []
        for name, envname, wild, boss in re.findall(r'BIOME\("([^"]+)",\s*(\w+),\s*(\w+),\s*(\w+)\)', rsrc):
            biomes.append({'name': name, 'env': pretty_constant(envname, 'BATTLE_ENVIRONMENT_'),
                           'wild': [s for s in pools.get(wild, []) if s in species],
                           'boss': [s for s in pools.get(boss, []) if s in species]})
        starters = [[sp, int(c)] for sp, c in re.findall(r'\{(SPECIES_\w+),\s*(\d+)\}', rsrc) if sp in species]
        tiers = {'TIER_COMMON': 'Common', 'TIER_GREAT': 'Great', 'TIER_ULTRA': 'Ultra', 'TIER_ROGUE': 'Rogue'}
        rewards = []
        for item, qty, tier in re.findall(r'\{(ITEM_\w+|REWARD_HEAL_PARTY),\s*(\d+),\s*(TIER_\w+)\}', rsrc):
            rewards.append([items.get(item, 'Full party heal' if item == 'REWARD_HEAL_PARTY' else pretty_constant(item, 'ITEM_')),
                            int(qty), tiers.get(tier, tier)])
        final = re.search(r'#define ROGUELITE_FINAL_BOSS (SPECIES_\w+)', rsrc)
        run = {'biomes': biomes, 'starters': starters, 'rewards': rewards,
               'finalBoss': final.group(1) if final else None}

    # ---- index tables to keep the file compact
    move_ids = sorted(moves, key=lambda m: moves[m]['name'])
    move_index = {m: i for i, m in enumerate(move_ids)}
    abil_ids = sorted(abilities, key=lambda a: abilities[a]['name'])
    abil_index = {a: i for i, a in enumerate(abil_ids)}
    type_index = {t: i for i, t in enumerate(types)}
    species_ids = sorted(species, key=lambda s: (species[s]['dex'] or 99999, env.get(s, 0)))
    species_index = {s: i for i, s in enumerate(species_ids)}

    def ti(t):
        return type_index.get(t, type_index.get('TYPE_NORMAL'))

    data = {
        'types': [type_names[t] for t in types],
        'chart': [[chart.get(a, {}).get(d, 1) for d in types] for a in types],
        'abilities': [[abilities[a]['name'], abilities[a]['desc']] for a in abil_ids],
        'moves': [[moves[m]['name'], ti(moves[m]['type']), moves[m]['cat'], moves[m]['pow'], moves[m]['acc'],
                   moves[m]['pp'], moves[m]['pri'], moves[m]['desc'], moves[m]['flags']] for m in move_ids],
        'species': [],
        'locations': [{'map': l['map'], 'mons': [[m, species_index[s], lo, hi] for m, s, lo, hi in l['mons']]} for l in locations],
        'trainers': [],
        'run': None,
    }
    evo_text = {
        'EVO_LEVEL': 'Lv. {p}', 'EVO_ITEM': '{p}', 'EVO_TRADE': 'Trade', 'EVO_FRIENDSHIP': 'Friendship',
    }
    for sid in species_ids:
        s = species[sid]
        evo_list = []
        for method, param, target in s['evos']:
            if target not in species_index:
                continue
            if method == 'EVO_LEVEL' and isinstance(param, (int, float)) and param > 0:
                how = 'Lv. %d' % param
            elif isinstance(param, str) and param.startswith('ITEM_'):
                how = items.get(param, pretty_constant(param, 'ITEM_'))
            else:
                how = evo_text.get(method, pretty_constant(method, 'EVO_')).replace('{p}', str(param))
                if method == 'EVO_LEVEL':
                    how = 'Level up (special)'
            evo_list.append([species_index[target], how])
        data['species'].append({
            'n': s['name'], 'f': s['form'], 'd': s['dex'],
            't': [ti(t) for t in s['types']],
            's': s['stats'],
            'a': [abil_index.get(a, -1) if a else -1 for a in s['abilities']],
            'i': [abil_index[a] for a in s['innates'] if a in abil_index],
            'e': evo_list,
            'p': species_index.get(s.get('prevo'), -1),
            'c': s['category'], 'x': s['desc'], 'h': s['height'], 'w': s['weight'],
            'g': s['eggGroups'], 'r': s['catchRate'],
            'lv': [[lvl, move_index[m]] for lvl, m in s['level'] if m in move_index],
            'tm': sorted(move_index[m] for m in s['tm'] if m in move_index),
            'eg': sorted(move_index[m] for m in s['egg'] if m in move_index),
            'k': s['const'],
        })
    for t in trainers:
        data['trainers'].append({
            'id': t['id'], 'n': t['name'], 'c': t['class'], 'db': t['double'],
            'p': [[species_index.get(m['species'], -1) if isinstance(m['species'], str) else -1,
                   m['species'] if m['species'] not in species_index else '',
                   m['level'], m['item'], m['ability'], m['moves']] for m in t['party']],
        })
    if run:
        data['run'] = {
            'biomes': [{'name': b['name'], 'env': b['env'],
                        'wild': [species_index[s] for s in b['wild']],
                        'boss': [species_index[s] for s in b['boss']]} for b in run['biomes']],
            'starters': [[species_index[s], c] for s, c in run['starters']],
            'rewards': run['rewards'],
            'finalBoss': species_index.get(run['finalBoss'], -1),
        }
    sprite_list = [sprites.get(sid, '') for sid in species_ids]

    # ---- write
    template = open(os.path.join(HERE, 'template.html'), encoding='utf-8').read()
    payload = json.dumps(data, separators=(',', ':'), ensure_ascii=False)
    sprite_payload = json.dumps(sprite_list, separators=(',', ':'))
    # JSON is embedded in <script type="application/json">; escape "</" so it can't close the tag
    html = (template.replace('__DEX_DATA__', payload.replace('</', '<\\/'))
                    .replace('__DEX_SPRITES__', sprite_payload.replace('</', '<\\/')))
    os.makedirs(OUT_DIR, exist_ok=True)
    out = os.path.join(OUT_DIR, 'index.html')
    open(out, 'w', encoding='utf-8').write(html)
    log('Wrote %s: %d species (%d entries with forms), %d moves, %d abilities, %d locations, %d trainers, %d sprites (%.1f MB)' % (
        os.path.relpath(out, ROOT), len({s['d'] for s in data['species']}), len(data['species']), len(data['moves']), len(data['abilities']),
        len(data['locations']), len(data['trainers']), sum(1 for s in sprite_list if s), len(html) / 1e6))


if __name__ == '__main__':
    main()
