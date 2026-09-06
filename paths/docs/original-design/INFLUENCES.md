# First Move — the influences

Game design research · 6 September 2026

**Recommendation: let Number Munchers supply the mathematical interaction, Aimlabs supply the practice structure, and Tetris supply the pressure and release.** The proposed game is called First Move for now.

Read the [game design](FIRST_MOVE_GAME_DESIGN.md), [construction extension](CONSTRUCTION.md), or [interactive rules sketch](FIRST_MOVE.html).

## 1. Number Munchers: recognize a property and act on it

A contemporary educational software guide describes Number Munchers' Factors activity as movement through a grid, selecting numbers that satisfy the stated factor rule while avoiding pursuing creatures. Its important design contribution here is that a mathematical classification determines which action is correct. [Contemporary software guide, Number Munchers entry, indexed excerpt](https://files.eric.ed.gov/fulltext/ED312159.pdf).

**What we take:** a readable rule, many candidate objects, quick selection, and satisfying consumption or clearing.

**Our mathematical extension:** the objects can be equations, function mappings, matrices, statements, or short proof steps. The rule can ask for linearity in specified unknowns, satisfaction of a hypothesis, compatibility of dimensions, or a valid implication.

The rule is precise about domains and roles. “Linear in a and b; x is known” trains something different from “linear in x.” Learning to notice that difference is valuable.

The initial interaction uses a small animated cursor that moves between cells. Pursuing enemies are a later optional arcade variation; the first version gets pressure from the board itself.

## 2. Aimlabs: break performance into trainable components

Aimlabs' own regimen guide organizes practice around component skills such as flicking, tracking, and switching, with tasks and playlists chosen for particular needs. It also emphasizes reflecting on results and adjusting the routine. These are product design practices, not evidence that an analogous math game improves advanced mathematics. [Aimlabs regimen guide](https://www.aimlabs.com/articles/aimlabs/how-to-develop-your-own-aim-training-regimen/).

**What we take:** short scenarios, immediate retries, clear task definitions, stable conditions for personal comparisons, and feedback on a specific skill.

**Our extension:** the task library measures reading a symbol, identifying an unknown, noticing a missing condition, selecting an applicable method, recognizing a setup, and checking a local step separately.

Fast mouse movement is not a mathematical skill measure. The game supports direct keyboard selection and records the control scheme. The familiar training-app satisfaction should come from making the same mathematical distinction more reliably and fluently.

## 3. Tetris: incoming work, decisions, and release

Official Tetris descriptions and a licensed product manual describe a field where completed rows clear, incomplete material accumulates, and reaching the top ends the run. The manual also describes an upcoming-piece preview and rewards for consecutive or combined clears. [Official Tetris overview](https://play.tetris.com/about), [Basic Fun Tetris manual, page 2](https://www.basicfun.com/wp-content/uploads/2025/06/09686_1L_IM.pdf).

**What we take:** a visible capacity limit, incoming material, preview, the risk of holding a nearly completed structure, and a large release when several rows clear.

**Our extension:** each row is a small mathematical classification problem. Correctly classifying a row makes it ready. The player can clear it immediately for room or keep it on the board while preparing another row for a larger clear.

This borrows the pacing and planning relationship. The board's objects, cursor, visual identity, sounds, and rules are original to this design.

## 4. The fit between them

| Influence | The player's thought | Our corresponding action |
|---|---|---|
| Number Munchers | “Which of these satisfy the rule?” | Mark the qualifying mathematical objects. |
| Aimlabs | “Which specific skill am I training?” | Choose a short, defined scenario and compare equivalent runs. |
| Tetris | “Can I prepare another row before I need the space?” | Bank a ready row or clear it now. |

The recurring loop becomes:

**Read the rule → scan the objects → commit a classification → bank or clear → handle the next arrival.**

Learning the rule, playing it fluently, and assessing independent use are distinct activities. The surrounding study app already provides the longer explanations and full problems.

## 5. A relevant finding from educational game research

Habgood and Ainsworth studied versions of Zombie Division, a mathematics game for children aged seven to eleven. In their studies, integrating the mathematical decisions into combat produced stronger learning in a fixed-time comparison and more voluntary play than a version that placed the learning questions outside combat. The study included instruction and reflection; it does not validate First Move or establish transfer to adult advanced mathematics. [Habgood and Ainsworth, 2011, accepted manuscript](https://shura.shu.ac.uk/3556/1/Habgood_Ainsworth_final.pdf).

**Design inference:** mathematical decisions should directly control useful game actions. The classification changes the board, opens space, and enables a planned clear. We should still test whether those decisions become useful outside the game.

The proposed row mechanic is an untested design. Its fun, readability, and educational value are separate questions requiring observation.

## 6. Where to be deliberate

**Recognition and construction:** selecting valid options establishes recognition under the displayed conditions. Producing an equation or proof step without options is a different task. The lab and study desk must include the latter.

**Pressure and new material:** new notation first appears in a calm lesson. Timed play is available for introduced material, with pace chosen by the player. Timing and mathematical difficulty are separate settings.

**Visual search and mathematics:** objects remain readable and stationary while being judged. The first prototype uses keyboard or direct selection so an aiming skill does not dominate the mathematical task.

**Arcade score and learning:** rows cleared and combo points describe a run. Learning history preserves initial accuracy, help, exposure, and later unassisted performance.

**Errors and rhythm:** after an error, the run preserves the initial attempt. Review provides the reason and a chance to repair. Repair can recover board space, but does not create a fresh independent success.

## 7. What to prototype first

Use a four-cell row and a single precise rule:

> Select the equations that are linear in a and b. The real number x is known.

Mix an ordinary linear equation, an equation with a squared unknown, a product of unknowns, and a familiar-looking polynomial in the known input.

The first playable question is whether marking, committing, banking, and clearing feels good even with no timer. The next is whether adding arrivals creates a useful decision about board space. Only then should the design add more mathematical families and pressure.

## Source-access note

The Number Munchers description above is supported by the indexed excerpt of a contemporary guide. The full scan exceeded the browsing tool's document size limit, and the separate MECC manual archive link returned an access error. The design does not rely on uninspected details of its scoring or difficulty schedule.
