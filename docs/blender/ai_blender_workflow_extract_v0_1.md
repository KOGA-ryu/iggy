# AI-in-Blender workflow — video extract for the factory lanes

2026-07-28 · source: "The SMART Way to Use AI in Blender" (~8 min, creator's first upload; transcript supplied by Ace — no URL/channel captured, no frames inspected; transcript-only extract). Second entry in the Blender-workflow research series, after [butterfly_knife_manual_modeling_extract_v0_1.md](butterfly_knife_manual_modeling_extract_v0_1.md). Where the knife video was a *craft* study (manual topology), this is a *pipeline placement* study: where AI tools sit inside a human-controlled scene workflow. The author's thesis matches our doctrine exactly: AI for leverage at the edges, human control at the center — "not pressing a button and getting a finished render."

## 1. Workflow as performed (condensed, timestamped)

- **0:19 — Reference → locked camera.** A Pinterest reference image (itself AI-generated — used for layout/mood, not material truth) goes into **fSpy** (free camera-matching tool): align a few axes, export a solved camera into Blender, model the scene over the locked view.
- **0:35 — Foundation-first modeling.** Largest layout-defining object first (the desk); every smaller prop positions relative to it. Hero object (computer) hand-modeled.
- **0:57 — The keyboard mistake (the video's best lesson).** Modeled keycaps to a random reference, then couldn't find a matching keycap texture — forced to generate a custom texture and align every cap by hand. His fix, stated as a rule: **find the texture first, then model to it** for text-bearing or repeated elements.
- **1:34 — AI 3D generation, scoped.** Unnamed image-to-3D service (image prompts beat text prompts). Self-stated limitations: fails on intricate models, fails on text, needs good reference images, and above all **bad topology → uneditable output**; quad-topo generation "not up to par." Sanctioned uses: scene blockouts to be replaced later, background/low-detail props, stylized/Eevee scenes, objects whose bad side can face away (the film camera's melted text front is simply pointed away from camera). Delivered: a lamp, film viewer, film camera — all background dressing.
- **3:16 — Cloth-sim curtains** (sardonic "three easy steps" = ~10): plane → subdivide → vertex group → shape keys + proportional-edit keyframes → cloth modifier (silk preset) → collision object drags the curtain → subdivision + solidify.
- **3:47 — ChatGPT-written micro-add-ons.** Two examples: a texturing helper (save node presets, copy/paste node trees, duplicate/delete materials, purge unused in one click) and a one-click object importer from a library .blend (replacing manual append). Process: prompt → paste to .py → install; iterate prompts until right.
- **4:34 — Texturing sources.** Simple PBR for most props; **Internet Archive** for period print (80s magazine covers, book jackets) harvested in bulk; ChatGPT image generation as the fallback for the missing keycap texture.
- **5:16 — Lighting (the "make or break" step).** HDRI base is flat and floods the interior; fix via **Light Path node**: gate the HDRI's contribution by *Is Diffuse Ray* (multiply node) so bounced outdoor light stays strong while direct transmission through the window is controlled — works whenever a transparent barrier separates inside/outside; must be mixed with a second low-strength sky. *Camera Ray* node brightens the sky for the camera only. Sun lamp: strength 5, 3000 K, angle tuned until shadows match the reference. Background gap filled with layered tree PNG cards (borders hidden from camera, shadow visibility off) plus a **separate gobo image** casting the tree-shadow pattern.
- **7:03 — Revision as a step with a name.** Build an explicit weakness list (dull textures, flat lighting, unreal spots) — his own eyes "plus a few extra things from ChatGPT" — then push past "done." "Revision is what takes a render from good enough to something you're proud of."

## 2. Technique inventory → where each fits for us

| Technique | Verdict for the factory |
|---|---|
| fSpy camera-match from a reference photo | **Adopt — highest-leverage item in the video** (see §3) |
| Foundation-first modeling order | Already doctrine (blockout-first); no change |
| Texture-first for text/repeated elements | Adopt as stated rule — it is the general form of the audit's letterset finding: author the 2D asset (letterset, motif strokes, keycap sheet) *before* geometry commits to it |
| AI image-to-3D for background/blockout | Adopt **as policy boundary** for the diffusion-5090 lane (see §4) |
| AI image-to-3D for hero/donor work | Reject — his own reasons (uneditable topology, text failure) are our reasons; our all-quad deterministic donors are the counter-position |
| ChatGPT micro-add-ons for UI grind | No new capability for the box crew (their bpy tooling is beyond this), but validates the pattern; possible future use for one-click gallery/plate operators |
| Internet Archive print harvesting | Adopt for period paper/print in interior kits — with per-item license checks (IA is not uniformly CC0; our Met/Cleveland discipline stays the bar) |
| Is-Diffuse-Ray HDRI gating, Camera Ray sky | Plate-standard toolbox: interior/window scenes and presentation renders |
| PNG cards + separate gobo for backdrop shadow interest | Plate-standard toolbox: cheap raking-shadow drama for context plates |
| Cloth-sim curtain recipe | File under soft-goods for homestead interiors; not weapons-relevant |
| AI-generated reference image as target | Caution: fine for layout/mood (his use); **never material truth** — the Asset Library audit exists because the bar must be real objects (museum CC0), not synthetic pictures of objects |
| Revision list + AI critic | Already exceeded (adversarial plates, visgate, the audit itself) — but his "beginners stop at done" framing is a good one-line justification for reject-plates in STUDY_NOTES |

## 3. The new idea this video unlocks: same-camera A/B plates

fSpy solves a camera from a single photo. Our reference library is *made of single photos with known subjects*. Combine them: solve the camera of a museum photograph (e.g., Met 32.75.225's catalog shot), place our arming sword in that exact camera, and render. The result is a **register-perfect A/B plate — our asset in the reference's own framing** — which converts the Asset Library audit's side-by-side judgment from "compare two differently-lit, differently-framed images" into a pixel-honest overlay. This directly upgrades:

- the audit loop (findings become undeniable or dissolve),
- Ace's redline loop (one image, flip between real and ours),
- the plate standard (add an `ab_reference` plate type citing the accession whose camera it borrows).

Cost is small (fSpy is free; one solve per reference photo, reusable forever). Recommend the box planner add `ab_met_32_75_225` as the first solved camera and gate the arming sword's next finish pass against it.

## 4. Lane policy the video crystallizes (for the three-lane doctrine)

- **Equation/donor lane (hero):** no generated meshes, ever — the video's "uneditable topology" complaint is the formal justification written from the other side of the fence.
- **Diffusion-5090 lane (set dressing):** sanctioned exactly per the video's list — blockouts-to-replace, background props, low-detail objects, hidden-side objects — plus our additions: never load-bearing for silhouettes at inspection distance, never text-bearing, always quarantined from the donor library, provenance-tagged as generated.
- **Ace's-hands lane:** untouched; references and redlines remain the human channel.

This gives the harvest-first/provenance doctrine a clean written boundary for *when generation is allowed at all* — worth a paragraph in STUDY_NOTES or the factory battle plan.

## 5. Caveats

Transcript-only: no frames inspected, the AI 3D service is unnamed in the transcript, and the creator/URL weren't captured — add them to this doc if the thread continues. Claims about tool quality (e.g., "quad topo not up to par") are the author's assessment as of his recording, consistent with our own diffusion-lane experience but not independently verified today.
