# Asset Reference Requests — what to create so I can build higher-fidelity assets

My pipeline: Blender headless generation of box/frustum/octagon geometry, textured
with **seamless palette tiles** (currently 12 procedural numpy textures) via
cube-projection UVs, exported to glTF with a metadata contract (collision,
sockets, category, walkable). Two levers raise fidelity: **(1) better textures**
(drop-in, huge leverage, no geometry change) and **(2) concept art / orthographic
model sheets** (a silhouette/proportion target I build detailed geometry to).

Ranked by leverage. You do not need to make all of it — Tier 1 alone upgrades
every existing kit.

---

## TIER 1 — TEXTURE TILES  (highest leverage, drop-in, upgrades all 18 kits)

A library of **seamless, tileable** textures. **Spec for every tile:**
- Seamless / tileable (wraps on all 4 edges)
- 512×512 or 1024×1024 PNG, sRGB
- **Flat albedo** — even lighting, NO baked shadows/highlights/AO (the engine lights them)
- Note the intended **meters-per-tile** (e.g. "planks ≈ 1.0 m", "cobble ≈ 0.5 m")
- Drop into `assets/creative/materials/incoming/` and I wire them into the palette + retexture the kits that need them

**Metals** (fixes the #1 issue — loot & weapons currently render near-black):
`gold_brass`, `steel_bright` (polished), `silver_tarnished`, `copper`, `bronze`,
`iron_blued`, `iron_rusted`

**Wood:** `oak_dark`, `walnut`, `pine_plank`, `driftwood_weathered`,
`wood_painted_red/green/blue`, `wood_lacquered`

**Stone:** `marble_white`, `granite`, `sandstone`, `slate`, `brick_red`,
`flagstone`, `stone_mossy`, `cave_rock`

**Cloth / leather:** `wool_red/blue/green`, `linen_canvas`, `burlap_sack`,
`leather_brown`, `leather_black`, `fur`, `tapestry_heraldic`, `velvet`

**Ground:** `grass`, `dirt_path`, `sand`, `snow`, `forest_floor`,
**`water_blue`** (recurring gap — marsh/riverbank water is faked as dark iron today)

**Special / organic:** `glass_clear`, `glass_stained`, `parchment`, `wax`,
`ceramic_glaze`, `gem_ruby/emerald/sapphire/amethyst`, `plaster_painted` (color set),
`skin_tone` (2–3 for NPCs), `hair`, `leaf_foliage`, `thatch_reed`

**Emissive** (for the lighting kit — mark or supply as glow maps):
`flame_fire`, `ember_glow`, `candle_glow`, `rune_glow` — plus a note of the
**light color temperature** each implies (warm torch ~2000K, cold moon ~7000K, magic = your call)

---

## TIER 2 — CHARACTER TURNAROUNDS  (for on-model NPC / creature geometry)

**Front + side (ideally + 3/4) orthographic turnarounds**, consistent scale,
clean background, T-pose or neutral stance. Purpose: I retune my parameterized
humanoid-blockout system to your proportions so figures read as *your* characters,
not generic blockouts. Even rough sketches work.

- **The thief (player)** + **the duo partner** — the heroes, seen most
- **Guard archetypes** — patrol, alert, captain/officer
- **Core townsfolk** — merchant, noble, commoner (m/f), child, priest, beggar
- **Animals** (if you want the guard-dog/animal kit) — dog, horse, rat, cat

## TIER 3 — HERO-PROP & ARCHITECTURE CONCEPTS  (orthographic where precision matters)

- **Signature props** — the notebook (the interface!), the reliquary, the vault/
  strongroom door, a hero weapon set, key objective items. Front/side elevations.
- **Architecture style sheet** — the world's building vocabulary: roof style,
  window & door shapes, ornament, wall construction, materials. This is what makes
  all my building kits read as ONE world instead of a grab-bag.
- **Lighting fixtures** — candelabra, chandelier, wall sconce, standing lantern,
  brazier, fireplace: silhouette + intended glow color.

## TIER 4 — ART DIRECTION / STYLE ONE-PAGER  (cheap to make, aligns everything)

- **The look target** — stylized-realistic? painterly low-poly? gritty? This tells
  me how hard to push geometric detail vs. lean on texture.
- **Color script / mood boards** per world or biome
- **Light guidance** — color temps + intensity for stealth (warm interior vs. cold
  night vs. magic), since light/shadow is the core stealth mechanic
- **Scale bible** — canonical dimensions (door 2.1 m, ceiling 3 m, weapon lengths,
  furniture heights). I have a 1.8 m human gauge; a dimensions table locks the rest.
- **Poly-budget guidance** per class (hero vs. background vs. blockout)

---

## CREATE-FIRST PRIORITY  (for maximum immediate leverage)

1. **Metal + `water_blue` + a few cloth tiles** (Tier 1 subset) — instantly upgrades
   heist loot, armory weapons/armor, and every water/biome scene. Unblocks the
   palette polish with *real* textures instead of my procedural ones.
2. **Art-direction one-pager + color palette** (Tier 4) — cheap, aligns all future work.
3. **Thief + duo + guard turnarounds** (Tier 2) — the heroes.
4. **Architecture style sheet** (Tier 3) — world coherence.

## FORMAT NOTES

- Textures: flat/seamless/no-baked-light PNG, meters-per-tile noted → `materials/incoming/`.
- Concepts: any resolution, clean background. **Orthographic front/side** most useful
  for to-spec builds; loose silhouette/mood sketches fine for style. Rough is fine —
  I do not need polished art, just a clear target.
