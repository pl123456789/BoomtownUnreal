# Boomtown Build Bible v2.0 — Unreal Engine 5.8

**Supersedes:** `Boomtown_Build_Bible_v1.0.pdf` (2026-07-21, external reference folder).
**Reconciled against:** repository `main` @ `3da785d` (M0014-R5D), 2026-07-29.

---

## 0. Authority

**Git is truth.**

The repository is the record of what exists. This document plans what happens next. Where the two disagree, git wins and this document is wrong and must be corrected.

Consequences of that rule:

- A milestone is complete when a commit proves it, not when a plan lists it.
- Reference PDFs, chat history, and prior notes describe *intent*. Only the repository describes *state*.
- This file lives in the repo so plan and proof are diffable in the same history. Update it in the same commit as the work it describes wherever practical.

### Ownership and approval

Who decides what. These boundaries exist so that scope, sequencing, and history each have exactly one owner.

| Party | Role | Owns |
|---|---|---|
| **Philippe** | Product Owner | Final approval. Authorizes every commit. Approves feel, historical direction, and scope |
| **Codex** | Bible Steward / Technical Director | Audits the past, specifies the current milestone, proposes future sequencing. Reviews implementation and Bible updates before Philippe authorizes a commit |
| **Claude** | Implementation Engineer | Executes approved milestones. Reports conflicts and proposes amendments. **Does not independently redefine the roadmap** |
| **Git** | Record | Authoritative record of the past. Not a party, but it outranks every party on questions of what already happened |

Consequences worth stating plainly:

- Claude may **propose** an amendment to this document — including to the sequencing — but adopting it requires Codex review and Philippe's approval. A proposal is not an adoption, and executing an unapproved amendment is out of scope regardless of how obviously correct it looks.
- Codex specifies; Claude implements. Where a specification is ambiguous or unbounded, Claude raises it rather than resolving it silently during execution.
- Philippe authorizes commits. No commit is created on Claude's own judgement that a milestone looks finished.
- Where any party's recollection conflicts with git, git is correct.

### Verification legend

Every status claim in this document carries one of these. Nothing is marked complete on the strength of a plan.

| Mark | Meaning |
|---|---|
| **COMMIT** | A commit exists with this milestone ID. The commit is the evidence. |
| **SOURCE** | Code for this exists in `Source/`, but no milestone commit claims it and its behaviour is unverified here. |
| **PLAYED** | Philippe confirmed the behaviour in a play test. |
| **NONE** | No commit, no source. Not built. |

---

## 1. Engine migration status

Boomtown began as a Unity prototype. The current production branch is **Unreal Engine 5.8**, and the Unity work did not transfer — it was re-implemented, not ported.

**The Unity era is reference material only.** The Master Design Document (`Boomtown_Bible_v3 Claude.pdf`) records Phases 0–6 as "Completed". Those phases were completed *in Unity*. They prove nothing about this branch and must never be read as Unreal progress:

| Unity phase (Master Doc) | Status on the Unreal branch |
|---|---|
| Phase 0 — Vision | Carried forward as design intent (still authoritative) |
| Phase 1 — Technical Foundation | Rebuilt for Unreal |
| Phase 2 — RTS Camera System | Rebuilt — `Prospector/RTSCameraPawn` |
| Phase 3 — Bill & Ted Character Framework | Partially rebuilt — Bill only; Ted **NONE** |
| Phase 4 — Procedural Terrain, Rivers, Forests, Navigation | Partially rebuilt — terrain and hydrology in progress; forests **NONE** |
| Phase 5 — First Playable Prospecting Loop | Components exist (**SOURCE**); end-to-end loop unproven |
| Phase 6 — Historical Art Direction & Modular Characters | **NONE** on this branch — reference art only |

What remains fully valid from the Unity era: the Constitution's eight laws, the design chain, the profession roster, the five-species forest data, and the historical research. Those are design inputs, not implementation.

---

## 2. Build contract

Sent to Claude at the start of every coding session. Carried unchanged from v1.0.

1. Do only the active milestone.
2. Do not redesign the game or broaden the milestone.
3. Preserve working systems unless the milestone requires a change.
4. Keep the project compiling and playable at every checkpoint.
5. Make multiplayer authority and replication explicit from the beginning.
6. Do not add plugins or marketplace dependencies without approval.
7. Do not perform large refactors merely for cleanliness.
8. Change only milestone-related files whenever practical.
9. Report exactly what changed, which files changed, and how to test it.
10. Create one focused Git commit after the user confirms the test passes.

> **Contract debt on rule 5.** The as-built code does not honour this rule: no gameplay state is server-authoritative anywhere in `Source/`, and `ISelectableUnit` documents itself as deliberately non-networked.
>
> The debt is specifically **server authority over commands, extraction, sediment, gold, and inventory** — issuing a job must be validated against ownership of the controlled unit on the server, and `DigSphereAtWorldLocation`, `FSedimentPacket` transfer, panning outcomes, and `TotalGoldGrams` must be server-resolved. Local selection highlighting is presentation and need not replicate.
>
> No commit records a decision to defer this, so it is not dated here. See §9, forward milestone **M0031**. Treat it as a known exception rather than a silent one.

### Session rules

- One milestone per session whenever possible.
- Start with repository inspection, not broad design discussion.
- Read specific files before editing rather than re-explaining the project.
- Reject suggestions not required by the milestone; record them in backlog notes.
- Describe defects with exact observed behaviour, reproduction steps, and expected result.
- Prefer targeted patches over rewrites.
- End the session after the acceptance test and commit report.

### Definition of done

- Editor opens the project without new compile errors.
- The written acceptance test passes from a clean Play-In-Editor start.
- No known duplication, stuck-state, or authority defect.
- No unrelated feature or refactor added.
- Changed files listed and understandable.
- New values are data-driven where they are likely to change.
- Host/client smoke test passes for replicated gameplay state — **currently not achievable; see contract debt above.**
- The exact Git commit exists and the working tree is clean.

> **Exception — verification-only operational milestones.** Some milestones verify state rather than change it: environment transfers, audits, regression passes. Their correct result is an **unchanged repository**, so the commit clause does not apply and there is nothing to stage. M0015 is the worked example. Such a milestone is done when its acceptance evidence is recorded here and the working tree is provably unchanged. Do not manufacture a commit to satisfy the checklist.

---

## 3. Control standard (CoH3-inspired), as built

| Input | Intended action | Status |
|---|---|---|
| W / A / S / D | Pan RTS camera | **COMMIT** M0006 · `RTSCameraPawn` |
| Mouse wheel | Zoom | **COMMIT** M0009A · zoom keeps follow target centred |
| Middle mouse drag | Orbit / rotate with pitch limits | **COMMIT** M0010 |
| Left mouse | Select unit | **COMMIT** M0005 · `ISelectableUnit` |
| Right mouse | Smart command (move / interact) | **COMMIT** M0008 · route commands to selected unit |
| Space | Centre camera on selected unit | **COMMIT** M0009 |
| C | Toggle follow selected unit | **COMMIT** M0009 |
| Tab | Cycle controllable workers | **COMMIT** M0007 |
| Escape | Cancel command, **then** deselect | **SOURCE — partial** · `DeselectAction` mapped to `EKeys::Escape` calls `DeselectCurrentUnit()`. It deselects, but does not cancel the active command as a first stage |
| Shift + right mouse | Queue waypoint | **SOURCE** · `EnqueueMoveJob` / `EnqueueDigJob` / `EnqueuePanJob`; unmodified right-click calls `ClearJobQueue()` first. Broader than v1.0 specified — the queue carries dig and pan jobs, not just waypoints |
| Left mouse drag | Box-select | **NONE** |
| Shift + left mouse | Add/remove from selection | **NONE** |
| Double left click | Select nearby same type | **NONE** |

**Control rule.** Right click is always contextual. Cursor, hover feedback, and command acknowledgement must make the resolved action obvious before and after the click — no hover or acknowledgement feedback exists yet (see M0018, M0019, M0021).

Camera architecture note, worth preserving: the player possesses a free-floating `ARTSCameraPawn`. `AProspectorCharacter` is **commanded**, never possessed. Any future control work must not quietly convert this into direct possession.

---

## 4. Production workflow

One active milestone. One test. One commit. Then stop.

1. **Read** — open the next unchecked milestone and nothing beyond it.
2. **Protect** — confirm a clean Git checkpoint.
3. **Build** — paste the milestone card.
4. **Inspect** — review the changed-file list before testing.
5. **Compile** — resolve milestone-related compile errors only.
6. **Play-test** — run the written acceptance test in under five minutes.
7. **Network-test** — when the milestone touches gameplay state, test host/client authority.
8. **Commit** — use the exact milestone commit message after the test passes.
9. **Record** — mark complete here, note defects and follow-up work.
10. **Stop** — do not begin the next milestone in the same session.

**Defect rule.** A failed acceptance test does not create a new milestone. Keep the same milestone active, record the exact defect, issue a targeted repair, rerun the same test, and commit only when it passes.

**Amendment, 2026-07-29 — late-discovered defects.** The rule above assumes the defect surfaces while its milestone is still the active one. It does not cover a defect found after that milestone's commits landed and after later milestones closed. For that case only:

- The defect may be repaired under its **own numbered milestone**, so the repair is traceable in git rather than buried in a reopened older ID.
- The originating milestone stays marked **open in §6** until the repair lands. It does not get to look finished because a different number is doing its work.
- The repair milestone must name the defect ID and the originating milestone in its commit message.

This amendment exists because DEF-001 was found on 2026-07-29, three days after `afdc57a`, after M0015 had closed and **while M0016 was still being prepared** — M0016 was uncommitted at the time of discovery and at the time this amendment was written. The alternative — continuing under M0014's R-series and shifting the entire forward queue down one — was considered and rejected as more churn than the traceability is worth. Reverse this amendment if that judgement proves wrong; it is deliberately narrow and applies to no other case.

**Build rule.** Live Coding for safe function-body changes only. A full closed-editor `Build.bat` rebuild whenever reflected fields, class layout, interfaces, inheritance, or headers may have changed.

---

## 5. Ten-age roadmap

Unchanged from v1.0. The ages protect the long vision; only the active milestone controls daily work.

| Age | Range | Outcome |
|---|---|---|
| I — The Prospector | M0001–M0050 | CoH3 controls, digging, panning, raw gold, selling, first tool upgrades |
| II — Partnership | M0051–M0060 | Bill and Ted as a reusable, multiplayer-safe work crew |
| III — The Claim | M0061–M0070 | Ownership, sampling, finite reserves, depletion |
| IV — The Camp | M0071–M0090 | Shelter, storage, arrivals, merchant demand, opportunity-driven growth |
| V — The Boom Camp | M0091–M0200 | Living employees, construction, blacksmith, cook, doctor, lumber, stable |
| VI — Boomtown | M0201–M0350 | Families, services, crime and law, schools, newspaper, institutions |
| VII — Industry | M0351–M0450 | Hard-rock mining, sawmill, foundry, assay, steam, production chains |
| VIII — Transportation | M0451–M0600 | Pack animals, wagons, roads, bridges, ferries, warehouses, freight |
| IX — Civilization | M0601–M0700 | Property, companies, banks, politics, multi-town economics |
| X — Legacy | M0701–M0800 | Depletion, decline, migration, new towns, retained wealth, reputation |

**Roadmap rule.** The roadmap is not permission to build ahead. Ages VI–X remain protected backlog until the single-claim, single-camp simulation is trustworthy.

---

## 6. As-built record — M0001 to M0016

Taken directly from `git log`. This table is the authoritative answer to "what is done?"

| ID | Title | Commit | Date | Status |
|---|---|---|---|---|
| M0001 | Project audit and clean baseline | — | — | **NONE** — no commit exists; work was done but never committed |
| M0002 | Git safety checkpoint | `82ad1c9` | 2026-07-22 | **COMMIT** · tag `checkpoint-m0002` (local only) |
| M0003 | Project folder structure | `260d021` | 2026-07-22 | **COMMIT** |
| M0004 | Enhanced Input and movement foundation | `7f0f0f9` | 2026-07-22 | **COMMIT** |
| M0005 | CoH3 unit selection | `fa7345d` | 2026-07-22 | **COMMIT** |
| M0006 | Hybrid WASD character control | `bbf9094` | 2026-07-22 | **COMMIT** |
| M0007 | Tab cycle units | `5c6d25b` | 2026-07-22 | **COMMIT** |
| M0008 | Route commands to selected unit | `efbf5c6` | 2026-07-22 | **COMMIT** |
| M0009 | Centre and follow selected unit | `a1cb4b9` | 2026-07-22 | **COMMIT** |
| M0009A | Keep follow target centred during zoom | `8020df5` | 2026-07-22 | **COMMIT** · sub-milestone, no v1.0 equivalent |
| M0010 | Middle mouse camera orbit | `bfd47bf` | 2026-07-23 | **COMMIT** |
| M0011 | Windows Alpha 0.1 packaging | `f98d6a7` | 2026-07-23 | **COMMIT** · not a v1.0 milestone |
| M0012 | — | — | — | **NONE** — number never used |
| M0013 | Landscape runtime integration and navigation validation | `b603ce7` | 2026-07-25 | **COMMIT** · WIP checkpoint `94e0037` precedes it |
| M0014 | Primary reach hydrology | `cec8ee2`, `afdc57a`, `589bf9b`, `3da785d` | 2026-07-26/27 | **IN PROGRESS** — R4A, R5C, R5D landed; not closed out; **blocked by DEF-001** |
| M0015 | Transfer vacation work to home PC | — | 2026-07-29 | **PASSED, no commit** — an ops milestone whose correct result is an unchanged repo |
| M0016 | Build Bible reconciliation (this document) | — | 2026-07-29 | in progress |

**M0014 is the open milestone.** It has four commits and an R-series naming scheme that implies further steps. Nothing in the repo marks it closed, so it is not closed.

### Numbers that are spent

M0001–M0016 are consumed as recorded above, including the M0001 and M0012 gaps. **Do not reuse them.** The forward queue starts at M0017.

### Known defects

Open defects against as-built work. A defect is not a milestone — under the defect rule it keeps its originating milestone active until repaired.

#### DEF-001 — Standalone crashes in Landscape material initialization

| Field | Detail |
|---|---|
| **Status** | OPEN — blocks M0014 |
| **Discovered** | 2026-07-29, Play → Standalone Game |
| **Introduced by** | `afdc57a` (M0014-R4A), 2026-07-26 |
| **Severity** | Game is unrunnable outside the editor |

**Symptom.** Selected Viewport PIE passes. Play → **Standalone Game reproducibly crashes during Landscape material initialization** on startup:

```
Assertion failed: !Parent || GIsEditor || IsRunningCommandlet()
MaterialInstanceConstant.cpp:81
SetParentEditorOnly() may only be used to initialize (not change) the parent
outside of the editor.  GIsEditor=0, IsRunningCommandlet()=0
```

Callstack is dominated by `UnrealEditor_Landscape` frames. Timeline: world brought up for play at `07:171`, assert at `07:418` — during gameplay start, not during load.

#### Proven evidence

Stated separately from interpretation, because the two must not be conflated.

- The assert text, file, line, and flag values above, from `Saved/Crashes/UECC-Windows-EFDC909C4D1FCC2256034A9BFB5108B0_0000`.
- The callstack is dominated by `UnrealEditor_Landscape` frames.
- Timing: world up for play at `07:171`, assert at `07:418` — gameplay start, not load.
- `MI_LandscapeReadability_M0014` is referenced as `LandscapeMaterial` in **10** landscape component/proxy actor files under `Content/__ExternalActors__/Levels/AIDestruction/`, and both diagnostic material assets were added in `afdc57a`.
- `Source/` contains **zero** material-instance calls. This is asset state, not C++.
- **Reported by Philippe from the editor's Landscape rebuild banner: 55 Landscape proxies carry stale data across multiple categories.** Not independently verified here.

#### Suspected cause

The 55-proxy stale-data report is the stronger signal, and it is broader than a single material assignment: stale landscape data spans multiple categories, not just material instances. The diagnostic material may be one contributing factor rather than the whole cause.

Working hypothesis: Landscape parents a per-component `ULandscapeMaterialInstanceConstant` beneath the landscape material, and because per-component data is not baked to disk in a state matching current sources, Landscape rebuilds it at load — which calls `SetParentEditorOnly()`. Whether the diagnostic material assignment *causes* the staleness or merely *travels with* it is **not established**.

**Why PIE does not surface it.** The assertion permits changing the parent of an already-parented material instance when `GIsEditor=1`; the same operation asserts with `GIsEditor=0` unless running as a commandlet. This explains how PIE could pass while Standalone crashes, without proving that both modes execute an identical path.

What is **not** proven: that PIE executes the identical code path, and that the defect has been latent since 2026-07-26. `afdc57a` is when the diagnostic material was introduced, which is the earliest plausible date — not a demonstrated one. No Standalone run between 2026-07-26 and 2026-07-29 is on record either way, so the defect's true start date is unestablished.

**Impact beyond standalone.** Standalone Game is proven to crash. A **cooked** build is a different case and its outcome is **unknown**: cooking may rebuild the stale data, and Shipping configuration handles assertions differently from Development. Treat packaging as **blocked or at risk until a cooked-build test passes** — do not assume either a crash or a pass. The M0011 Windows Alpha predates `afdc57a` and is unaffected. See §12.

#### Repair actions (undecided, nothing executed)

1. *Rebuild stale landscape data* — in UE 5.8 the editor's rebuild banner invokes **Landscape Build All**, which covers modified packages, grass maps, physical materials, and Nanite. It is **not** merely "Update Material Instances"; naming it that understates what is stale and what the rebuild touches. Then save the affected proxies.
2. *Remove the diagnostic* — reassign the real landscape material before rebuilding, if R4A's readability material was temporary scaffolding, which its name and `Content/Materials/Diagnostic/` location both suggest.
3. *Verify usage flags* — confirm `M_LandscapeReadability_M0014` has **Used with Landscape** enabled, if it is to remain.

Sequencing matters: rebuilding while the diagnostic is still assigned bakes the diagnostic in. Decide the material question first, then rebuild.

**Unverified.** The material's usage flags; the Landscape Material slot value as the editor reports it; whether Landscape Build All alone clears the defect; and cooked-build behaviour. Binary asset inspection proves the reference exists in ten component files — it does not prove intent or causation.

**Relationship to M0015.** None. The M0015 transfer passed on its own terms — build, UE 5.8 launch, `AIDestruction` load, MCP endpoint, and an unchanged repository were all verified. DEF-001 is a pre-existing asset-state defect dating to 2026-07-26 that the transfer neither caused nor was required to detect. **It remains unfixed.**

---

## 7. As-built systems inventory

What actually exists in `Source/`, independent of what any commit message claims. Several of these have no milestone ID at all — the repo is ahead of the plan in places and behind it in others.

### Prospector

| File | What it is | Status |
|---|---|---|
| `RTSCameraPawn` | Free-floating possessed camera: WASD pan, wheel zoom, middle-mouse orbit with pitch clamps, follow-pivot correction | **COMMIT** M0006/M0009A/M0010 |
| `SelectableUnit` | Minimal selection interface — `SetSelected` / `IsSelected`. Explicitly non-networked | **COMMIT** M0005 |
| `ProspectorCharacter` | Bill. Commanded, not possessed. Holds the **job queue** (`EnqueueMoveJob` / `EnqueueDigJob` / `EnqueuePanJob` / `ClearJobQueue`), the carried `FSedimentPacket`, the tailings pile, and `TotalGoldGrams` | **COMMIT** M0004/M0008 for movement; **SOURCE** for the job queue, sediment carry, and gold total |
| `ProspectorPlayerController` | Selection cursor, right-click command routing, Escape deselect, shift-modified queueing | **COMMIT** M0005/M0008; Escape and shift-queueing are **SOURCE** |
| `PanningMinigameComponent` | **Full gold-panning mini-game.** Continuous tilt input against a drifting flow direction; gold tracked by real grain size, so fine gold washes out first when overtilted and nuggets hang on longest. Tunable wash rates, overtilt threshold, per-grain loss | **SOURCE** — no milestone ID, behaviour unverified |
| `GoldPanWidget` | Panning UI | **SOURCE** — no milestone ID |

### Voxel / geology

| File | What it is | Status |
|---|---|---|
| `GravelBarSite` | **The diggable pocket.** Smooth-voxel volume with layered geology (bedrock → gravel → sand → topsoil). Gold budget is *mass-conserved*: a fixed total reserve distributed by hydraulic trap score, so a rich spot is only rich at a poor spot's expense. Snaps to traced ground via `FTerrainSurfaceQuery`, so it blends into either terrain implementation without a seam | **SOURCE** — no milestone ID |
| `DensityChunkComponent` | Voxel density field | **SOURCE** |
| `GeoTypes.h` | Geology/sediment types incl. `FSedimentPacket` | **SOURCE** |

### Terrain

| File | What it is | Status |
|---|---|---|
| `OverworldHeightfield` | Rollback procedural terrain | **SOURCE** |
| `HeightmapSampler` | Heightmap sampling (`Hope00`, Hope TRIM 4033 UInt16) | **SOURCE** |
| `TerrainSurfaceQuery` | Terrain-agnostic ground trace used by gravel bar and hydrology | **SOURCE** |

### Hydrology

| File | What it is | Status |
|---|---|---|
| `PrimaryReachHydrology` | Primary reach spline, actor placement, water profile metadata | **COMMIT** M0014, in progress |
| `ReachHydrologyTypes.h` | Reach/water profile types | **COMMIT** M0014 |

### Content

Levels `AIDestruction` (World Partition, 268 actors) and `Test/TerrainValidation`; heightmaps; landscape readability diagnostic materials; the `Content/Boomtown/*` folder skeleton from M0003, still all `.gitkeep` — no gameplay assets have been authored into it.

> **The significant finding.** `GravelBarSite`, `PanningMinigameComponent`, and the `AProspectorCharacter` job queue together cover a substantial portion of what v1.0 scheduled as Batch 3 (dig spots) and Batch 4 (panning loop) — and the dig → carry → pan chain is wired end to end in source: `DigSphereAtWorldLocation` produces an `FSedimentPacket`, `CarriedBucket` holds it, `StartMinigame` consumes it, and `HandlePanningFinished` banks gold into `TotalGoldGrams` while tracking tailings.
>
> This is **tracked** code — git has held it since M0002. The gap is not version control. It is that this implementation carries **no milestone ID, no written acceptance test, and no commit claiming it**: an unmilestoned, unverified tracked implementation.
>
> Coverage of planned intent is substantial, but the **end-to-end loop remains unproven** — no test has demonstrated it completes without duplication, stuck state, or resource loss. Batch C exists to establish that proof, not to rewrite working code.

---

## 8. Divergence from v1.0

Recorded so nobody reads the old PDF as a status report. From M0005 onward the numbers mean different things.

| ID | v1.0 planned | Actually shipped |
|---|---|---|
| M0002 | Git safety checkpoint | same |
| M0003 | Production folder structure | same |
| M0004 | Enhanced Input foundation | same, plus movement |
| M0005 | RTS camera pan | CoH3 unit selection |
| M0006 | RTS camera zoom | Hybrid WASD character control |
| M0007 | RTS camera rotate | Tab cycle units *(v1.0's M0015)* |
| M0008 | Left-click selection | Route commands to selected unit |
| M0009 | Right-click movement | Centre and follow *(v1.0's M0010)* |
| M0010 | Focus and follow camera | Middle mouse camera orbit *(v1.0's M0007)* |
| M0011 | Selection ring | Windows Alpha 0.1 packaging |
| M0012 | Hover feedback | nothing |
| M0013 | Drag-box selection | Landscape runtime integration |
| M0014 | Shift add/remove selection | Primary reach hydrology |
| M0015 | Tab cycle workers | Transfer vacation work to home PC |

Two structural departures beyond renumbering:

1. **Terrain and hydrology were pulled forward.** v1.0's M0001–M0100 contains no landscape or hydrology milestone; §9 of v1.0 explicitly parks geography as protected backlog. Repo M0013 and M0014 are that work, built early. Given Constitution law 4 — geography is gameplay — this is defensible, but it happened outside the plan and the plan never absorbed it.
2. **Packaging arrived early.** v1.0 places packaging only at the M0050 and M0100 checkpoints. Repo M0011 shipped a Windows Alpha well before the vertical slice existed.

---

## 9. Forward queue — M0017 onward

v1.0's unbuilt milestones, renumbered into a single forward sequence, with additions where the repo created needs v1.0 never anticipated. Age I still ends at M0050.

### Batch A — close out what is open (M0017)

| ID | Milestone | Deliverable | Acceptance test |
|---|---|---|---|
| M0017 | Primary reach hydrology close-out **and DEF-001 repair** | The remaining M0014 R-steps — **to be enumerated before approval, see below** — plus the DEF-001 repair | One observable condition per R-step; **and** Standalone Game reaches `AIDestruction` without the `SetParentEditorOnly` assert |

> **Numbering basis.** M0017 carries M0014's remaining work under a separate number by the late-discovered-defect amendment in §4. **M0014 stays open in §6 until M0017 lands** — it does not get to look finished because M0017 is doing its work. M0017's commit message must name both DEF-001 and M0014.
>
> **M0017 is not approvable in its current form.** "Finish the R-series" would let the milestone invent its own scope during execution, which is exactly what the build contract forbids. The R-step list must be written down first, each with an observable acceptance condition.
>
> **What the repository establishes.** `APrimaryReachHydrology` is an authoring record for the approved ~1.887 km Primary reach. It holds the authored centreline (compiled-in, not rediscovered at runtime), an evidence-derived flow direction, and one index-aligned sample per point carrying geometry and downstream chainage. It exposes `RebuildFromApprovedCenterline()` (CallInEditor), `GetReachLength()`, and a nearest-sample lookup. It **deliberately draws nothing** — rendering visible water is explicitly delegated to a future consumer that reads from it.
>
> **What the repository does not establish.** The R-series itself. Git contains R4A, R5C, and R5D; R5A and R5B appear nowhere, so they were either skipped, renamed, or done without commits. The series therefore cannot be reconstructed from the repository, and nothing in it defines the terminal step.
>
> **Required before approval:** Philippe and the Technical Director supply the remaining R-step list. Each entry needs an observable condition — a value that can be read, a query that returns a known answer, or a visible result — not "looks right". Candidate conditions the code already supports: `GetReachLength()` returns the approved 1.887 km within tolerance; the nearest-sample query returns monotonically increasing chainage along the centreline; `RebuildFromApprovedCenterline()` is idempotent across repeated runs.

### Batch B — control polish and command language (M0018–M0026)

Carried from v1.0 Batch 2. Most are unbuilt; **M0020 and M0025 are not** — Escape deselect and shift-modified job queueing already exist in source, so those two are completion and acceptance work rather than new implementation. See §3.

| ID | Milestone | Deliverable | Acceptance test |
|---|---|---|---|
| M0018 | Selection ring | Reusable ground ring shown only for selected units | Ring follows terrain, disappears instantly on deselect |
| M0019 | Hover feedback | Highlight selectable under cursor without selecting | Only the hovered selectable highlights; clears reliably |
| M0020 | Escape cancel stage — **completion** | Deselect already works (`DoDeselectCommand` → `DeselectCurrentUnit`). Add the missing first stage: Escape cancels the active command or job queue, and only deselects when there is nothing left to cancel | Two-stage behaviour is predictable, never strands a unit mid-order, and never deselects while a cancellable order is running |
| M0021 | Command acknowledgement | Visual and audio feedback for valid and invalid commands | Fires once per command; invalid orders are distinguishable |
| M0022 | Drag-box selection | Left-mouse drag selects units in a screen rectangle | UI box matches the resolved selection area |
| M0023 | Shift add/remove selection | Shift-click adds or removes from the active selection | Membership changes without clearing unrelated units |
| M0024 | Double-click same type | Double-click selects nearby units of the same class | Only matching units within the configured radius |
| M0025 | Job queue — **acceptance** | The queue already exists and carries move, dig, and pan jobs. Write its acceptance test rather than reimplementing it; cover mixed job types, not just waypoints | Jobs complete in issued order; a mixed move/dig/pan queue resolves correctly; unmodified right-click clears the queue; cancellation leaves no partial job state |
| M0026 | Path preview | Destination marker and optional path line for the issued move | Accurate, clears on arrival, does not affect navigation |

### Batch C — reconciliation and acceptance (M0027–M0030)

**No new gameplay systems.** These milestones write acceptance tests for tracked implementation that already exists, and give it commits and milestone IDs. If one of them turns out to require new code, it has failed its own definition and must be re-scoped as an implementation milestone rather than quietly widened.

| ID | Milestone | Deliverable | Acceptance test |
|---|---|---|---|
| M0027 | Gravel bar site acceptance | Written test for existing `AGravelBarSite`: layered geology, mass-conserved gold budget, terrain-agnostic placement | Same seed gives reproducible geology; total extracted gold never exceeds the site reserve |
| M0028 | Panning minigame acceptance | Written test for existing `UPanningMinigameComponent`: tilt-versus-flow, per-grain-size loss, cancel behaviour | Overtilting measurably costs fine gold before nuggets; cancel banks nothing; no state leaks between runs |
| M0029 | Dig → carry → pan end-to-end acceptance | Written test for the **existing** chain: `DigSphereAtWorldLocation` → `CarriedBucket` → `StartMinigame` → `HandlePanningFinished` → `TotalGoldGrams`, including the job-queue path | A tester completes dig → carry → pan → gold without developer instructions; no sediment or gold duplicated or lost across an interrupted run |
| M0030 | Control regression map | Test-only map exercising every control in §3, including Escape and shift-queueing | All controls pass a written five-minute checklist |

### Batch D — implementation: authority and inventory (M0031–M0032)

Genuinely new code, unlike Batch C.

| ID | Milestone | Deliverable | Acceptance test |
|---|---|---|---|
| M0031 | **Server authority retrofit** | Pay down the rule-5 contract debt per §2: server validation of controlled-unit ownership before a job is accepted, and server resolution of extraction, `FSedimentPacket` transfer, panning outcome, and gold total. Selection highlighting stays local presentation | Two-client smoke test: a client cannot command a unit it does not own; no duplicated or lost sediment or gold; both clients agree on gold totals |
| M0032 | Raw gold and currency as separate state | Establish a spendable currency balance as state distinct from `TotalGoldGrams` — currency does not exist yet. **State only: no conversion mechanism, no assay.** Result readout matches the authoritative gold gain | Raw gold and currency are independently readable and cannot be confused for one another; gold persists across selection changes; currency starts at its configured value and is unaffected by panning |

> M0031 is deliberately placed before the economy work. Every milestone after it moves resources or money, and retrofitting authority beneath a live economy is materially harder than beneath a single loop. It is also the point where the Definition of Done's host/client clause becomes satisfiable again.

### Batch E — first money and tool upgrades (M0033–M0042)

Carried from v1.0 Batch 5, renumbered: merchant tent actor, merchant interaction, assay and sell gold, wallet transactions and readable transaction events, store list UI, better shovel purchase, better pan purchase, upgrade comparison panel, transaction feedback, and a first-gold demo checkpoint.

> **Boundary with M0032.** M0032 establishes raw gold and currency as separate *state* and nothing more. v1.0's "currency wallet" milestone is therefore reduced here to wallet **transactions** — atomic movement, non-negative balance, readable events — since the balance itself already exists by then. **Gold → currency conversion is owned solely by the assay milestone**, which is where the conversion rate and its acceptance test belong. No milestone in this batch may re-establish currency state.

### Batch F — Bill & Ted work crew (M0043–M0050)

Carried from v1.0 Batch 6, renumbered and compressed to fit Age I: Ted character data, Ted selection and movement, reusable task order, Ted work orders, task reservations, worker status panel, shared company storage, two-worker production test.

Ages II–IV (claims, camp, arrivals — v1.0 Batches 7–10) resume at M0051 and are **not** re-planned here. Re-plan them when Age I closes, not before; the same drift this document corrects will otherwise recur.

---

## 10. Protected backlog — M0101 to M0800

Unchanged from v1.0. These ranges preserve the full-game path without authorising premature implementation.

| Range | System |
|---|---|
| M0101–M0125 | Living employees depth |
| M0126–M0150 | Camp services |
| M0151–M0200 | Boom camp businesses |
| M0201–M0250 | Families and housing |
| M0251–M0300 | Crime and law |
| M0301–M0350 | Town institutions |
| M0351–M0400 | Industrial mining |
| M0401–M0450 | Lumber and steam industry |
| M0451–M0500 | Pack transport |
| M0501–M0550 | Roads and bridges |
| M0551–M0600 | Regional freight |
| M0601–M0650 | Property and companies |
| M0651–M0700 | Multi-town civilization |
| M0701–M0750 | Boom and bust |
| M0751–M0800 | Legacy loop |

### Housekeeping backlog

Recorded, not scheduled. None of these are milestones.

- `.vsconfig` is a machine-specific Visual Studio component manifest under version control. It is rewritten by every `UnrealBuildTool -projectfiles` run on a PC with a different toolchain (14.44 on the travel machine, 14.50 here). Either gitignore it or standardise VS components across machines.
- `.claude/scheduled_tasks.lock` is not covered by `.gitignore` — only `settings.local.json` is — which is why `.claude/` shows as untracked at all.
- Tag `checkpoint-m0002` exists locally but was never pushed; the remote has no tags.
- `Content/Boomtown/*` is still an empty `.gitkeep` skeleton from M0003.

---

## 11. Reference library

Guides production; not permission to skip milestone verification. Located at `D:\UnityProjects\Boomtown inspiration\`.

| Reference | Production use |
|---|---|
| Boomtown Bible v3.0 | Master vision, the eight-law Constitution, design chain, long-term system roadmap. **Its Phases 0–6 are Unity-era; see §1.** |
| Asset Production Bible | Character, environment, prop, naming, LOD, modelling standards. **Image-only PDF — no extractable text; needs OCR to be searchable.** |
| The Cariboo Trail chronicles | Historical narrative: travel, settlement, mining, frontier conditions (164 pp, text) |
| Gold Rush Trail Guide | Geographic corridor reference, New Westminster to Barkerville (48 pp, text) |
| Barkerville and GUI imagery | Visual target for town density, street character, interface direction |
| `Boomtown_Build_Bible_v1.0.pdf` | **Superseded by this document.** Retain for provenance only — its M0005+ numbering is misleading |

### The Constitution

Every feature, at any stage, is tested against these eight laws before it is added. They survive the engine migration unchanged.

1. Nothing exists without a reason.
2. Buildings are consequences.
3. Citizens solve problems.
4. Geography is gameplay.
5. Transportation creates wealth.
6. Scarcity creates stories.
7. History constrains design.
8. The simulation never lies to the player.

**Constitutional test:** why does it exist, which law does it reinforce, what new stories can it generate. A feature that cannot clear all three is reconsidered rather than added.

---

## 12. Release gate — Demo Alpha

Demo Alpha is complete only when:

- M0017–M0050 are verified in the Unreal branch, each with a commit;
- the 30-minute First Gold to First Camp session works from a clean build;
- save/load restores the vertical slice;
- a two-client smoke test completes without duplicated resources, stuck workers, or authority errors.

The two-client clause is currently unsatisfiable — see the contract debt in §2 and milestone M0031. Demo Alpha cannot be declared before that milestone lands, regardless of how complete the single-player loop looks.

The clean-build clause is also **at risk**: **DEF-001** crashes Standalone Game at startup. Whether a cooked package fails the same way is unknown — cooking may rebuild the stale landscape data, and Shipping handles assertions differently from Development. Packaging is therefore **blocked or at risk until a cooked-build test passes**. Do not declare Demo Alpha on the strength of a PIE session, and do not assume a package is fine merely because it is cooked.
