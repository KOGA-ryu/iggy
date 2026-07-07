# Ownership Trace & Kill Method

How we turn a scattered god-struct field into a **kill plan** — not just "how we find references," but how we
decide *what each reference is*, how we migrate or delete it, and how we prove it **stayed dead**. This is the
method behind every decomposition slice (`docs/god_struct_decomposition_target_map.md`,
`docs/ownership_deficit_audit.md`).

---

## Three layers: grep finds, read verifies, compiler finalizes

Grep is the **net**, not the truth. It surfaces candidate sites fast but is blind to context (is the owner in
scope here? read or write? the window or a nested copy?). The real trace is three layers:

1. **grep** → candidate sites (fast, imperfect).
2. **read each site** → verify context.
3. **the compiler** → *proves* completeness: delete the field, and every straggler grep missed becomes a
   compile error.

Never claim the grep list is complete. The acceptance gate is `grep field → zero` **after** the compiler confirms.

## The grep primitives (annotated)

```sh
# FIELD: what/where on the god-struct
grep -nE 'ProductActiveRoomState activeRoom;' ProductAppWindowState.hpp

# READERS: dot-anchor the member access, exclude tests
grep -rnE '\.activeRoom\b' --include='*.cpp' src | grep -v test
#          ^^ \. and \b — matches window.activeRoom, NOT the type name / a comment / a substring

# WRITERS: assignments only, strip comparisons
grep -rnE 'activeRoom =' src | grep -vE '==|!='

# OWNER DISAMBIGUATION: window-owned vs nested copies (this is how the duplicate is found)
grep -rnE 'window\.activeRoom\b'                # the truth
grep -rnE '(roomEditing|state)\.activeRoom\b'   # the mirror copies → the deficit
```

Flags: `-rn` line hits · `-rl` file list · `| wc -l` / `| sort | uniq -c` inventory. Gotchas: **quote globs in
zsh** (`--include='*.cpp'` — unquoted trips `nomatch`); **multi-line calls** (`foo(\n receipt,`) are invisible
to line-based grep, so goldens use a Python regex where `\s` crosses newlines; watch **`.cpp`/`.hpp` partner
pairing** when re-pointing includes.

## The recursion (the part that matters)

A field read *inside a function* means that function's **callers** are transitive readers. Don't stop at the
direct reads — recurse:

```
field → reader-functions → their callers → their callers → … until the leaves
```

Grep the reader-function's *name* as a call, then read each caller to see whether the owner is in scope (adopt
the identity/source predicate directly) or must be threaded (extend a request/context struct). This transitive
layer is what turns a flat "S delete" into an L slice — it is the real cost, and it hides one call away.

## Classify every hit before touching code

After the grep/read pass, every hit gets classified before editing:

- **Truth read**: legitimate read from the owning state.
- **Mirror read**: read from a copied/derived state that should not own truth.
- **Writer**: mutation site that must either move to the owner or become a command/event.
- **Projection**: derived output that should be rebuilt from truth, not stored as truth.
- **Bridge/threading site**: place where the true owner is not in scope yet.
- **Dead/stale site**: leftover path that should be deleted, not migrated.

This prevents the dumb version of refactoring: moving every reference blindly and preserving the same ownership
bug in nicer clothes.

## The final gate is not just "build passes"

The delete-field gate has three checks:

```sh
# 1. No remaining direct refs
grep -rnE '\.activeRoom\b' src apps tests

# 2. No remaining type-level mirrors unless explicitly allowed
grep -rnE 'ProductActiveRoomState' src apps tests

# 3. No helper/predicate wrappers hiding the old dependency
grep -rnE 'activeRoom|ActiveRoom|roomEditing.*room|editing.*room' src apps tests
```

The first grep catches obvious survivors. The second catches copied ownership **by type**. The third catches
semantic wrappers that no longer mention the exact old field shape (it is intentionally high-recall — eyeball
the false positives). A build pass proves syntax and linkage. **These greps prove the architectural corpse is
not still twitching under a new blanket.**

## Flat hit-count lies

Line count is not effort. A field with 62 direct reads may be easier than one predicate with 16 transitive
callers. The real cost is:

```text
effort = ownership ambiguity + transitive fan-out + mutation risk + test coverage weakness
```

So the trace does not just count hits — it identifies **where ownership crosses a boundary.** That is where the
bugs breed, and it is why the flat audit's S/M/L ratings are directional only: each slice runs this trace
before it is called anything (deficit #2 was rated "cheapest S" and proved L on exactly this transitive
fan-out). Two passes rarely nail a deep fan-out in one go — the adversarial critique re-runs the greps from a
skeptic's angle to catch stragglers, and the compiler-guarded delete-last absorbs whatever all passes still miss.

## The one-line discipline

**grep to scope it, read to verify it, compile to finish it — and classify every hit before you touch it, so
you kill the ownership bug instead of re-housing it. Then negative-grep to confirm the corpse stayed dead.**
