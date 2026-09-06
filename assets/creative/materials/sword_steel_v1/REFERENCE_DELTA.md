# Sword Steel Reference Delta

## Active capability

Clean multi-band sword polish on the WPN-001 diagnostic blade before hamon,
hada, pattern welding, engraving, stylization, oxidation, or damage.

## Professional benchmark: Serpentine's Embrace

Source: <https://gamesartist.co.uk/serpentines-embrace/>

| Evidence | Current v1 | Active delta | Acceptance effect |
| --- | --- | --- | --- |
| straight, orientation-locked blade UV | implemented through `IGGY_BladeUV` | retain | required |
| 16-bit normal bake to avoid stepping | analytic fixture has no high-poly bake | actual sword bake remains unavailable | blocks actual-target acceptance, not diagnostic polish |
| plain metal with linear brushing after rejecting cluttered damask | implemented | retain quiet substrate | required |
| several layers of finish irregularity | only one finite medium track field | add broad macro polish; keep microstructure in anisotropy | blocks revised diagnostic candidate until proved |
| hamon constructed from gradient, subtractive paint, outline, warp, cleanup | absent | deliberately deferred to a Japanese-blade fork | does not block clean homogeneous steel |

## Professional benchmark: weapon-art surface workflow

Source: <https://80.lv/articles/weapon-art-tips-for-design-texturing-and-presentation>

| Evidence | Current v1 | Active delta | Acceptance effect |
| --- | --- | --- | --- |
| inspect bump, smoothness, striation, roughness and wear separately | substrate and medium finish are separated | add explicit macro/medium/micro ownership | required |
| preserve empty space between imperfections | medium tracks contain interruptions | broad field must remain quiet and non-symbolic | required |
| differentiate planes with facing and specular response | region roughness and anisotropy exist | add region-specific macro and grind strength | required |
| use custom, masked imperfections rather than uniform defaults | no generic shader noise exists | retain | required |

## Engine benchmark: Baldur's Gate 3 weapon contract

Source: <https://docs.baldursgate3.game/Creating_Weapons>

The official weapon documentation uses Base, Normal, and a Physical Map with
metalness, roughness, and AO channels, plus optional gradient masks. The
current Blender diagnostic does not yet export that adapter. The active polish
slice preserves independently inspectable sources so later channel packing is
possible, but BG3 or Unreal parity remains unverified.

## Remaining visible differences

1. No actual Box sword is present in the local package.
2. The analytic fixture does not prove production topology, normal baking,
   both-face UV layout, ricasso, or actual tip construction.
3. The current v1 finish has no separately inspectable macro polish field.
4. The current v1 applies the same medium-track strength to every blade region.
5. Microstructure is not named explicitly as an anisotropic BSDF responsibility.
6. No light-rotation proof exists; neutral and grazing views are proxies.
7. No engine-packed texture or parity proof exists.

Items 3 through 5 are the active implementation delta. Items 1, 2, 6, and 7
remain explicit limitations after this slice.
