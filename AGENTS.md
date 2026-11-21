# AGENTS.md

## Mission
Keep the project buildable, testable, and easy to extend. Prefer small, verifiable changes over broad rewrites.

## Working rules
1. Reproduce the issue before fixing it.
   - Run the smallest relevant command first.
   - For build issues, use:
     - `cmake -S . -B build`
     - `cmake --build build`
   - For runtime issues, isolate to the relevant target or scene.

2. Root cause before patch.
   - Identify the specific file and symbol causing the problem.
   - Do not stack speculative fixes.

3. Keep edits surgical.
   - One bug, one root cause, one patch.
   - Avoid unrelated refactors in the same pass.

4. Verify before claiming success.
   - Re-run the same build/test command after the fix.
   - If build was not run, do not say it is fixed.

5. Prefer stable repo conventions.
   - Keep project-level CMake in one place.
   - Do not duplicate `project(...)` or `cmake_minimum_required(...)` blocks.
   - Match existing class/interface names and ownership patterns.

## Repo-specific guidance
- The main build is configured from [CMakeLists.txt](CMakeLists.txt).
- The backend factory contract is in [include/RenderBackend.h](include/RenderBackend.h) and [include/RenderBackendFactory.h](include/RenderBackendFactory.h).
- Rendering and scene work should stay behind the backend abstraction rather than using direct concrete casts.
- The vendored GLAD config in [dependencies/glad/CMakeLists.txt](dependencies/glad/CMakeLists.txt) should remain compatible with current CMake versions.

## Roles
- **Orchestrator** — the first agent in a session. Owns work decomposition, the task board, merge order, and cumulative verification. Exactly one orchestrator per session.
- **Worker** — executes exactly one lane it was dispatched to. A worker must never re-decompose work, edit files outside its declared ownership, or touch another lane's files.

## Default auto-dispatch policy
- **By default**, any request that generates code or content — features, multi-file changes, refactors, tests, documentation — must be decomposed into 2–4 independent workstreams and dispatched to concurrent agents. This is the default behavior, not an opt-in.
- **Single-agent fallback** applies only when ALL of the following hold:
  - the change is confined to one file,
  - the diff is under ~50 lines,
  - and it is not part of a larger dispatched effort.
- If any two workstreams would need to edit the same file, they must be merged into a single workstream (serialization). Parallel dispatch requires disjoint file sets by construction.

## Concurrent workstream model
Fixed lane identifiers — always reference lanes by these ids:

| Lane | Scope |
|------|-------|
| L1 | **Build & config** — CMake, dependencies, install/setup, presets. Never combine with rendering logic in the same task. |
| L2 | **Rendering backend** — OpenGL backend, Vulkan probing, factory selection, GL state management. |
| L3 | **Scene / asset pipeline** — OBJ loading, cache, scene graph, culling, material binding. |
| L4 | **Tests & validation** — unit tests, smoke builds, targeted runtime checks. |
| L5 | **Documentation & process** — README, backlog updates, agent workflow notes. |

Rules:
- Each agent must be named `agent-<lane>-<seq>` (e.g. `agent-L2-1`, `agent-L4-1`) and state its lane in its first message.
- Lane = a disjoint set of files. Before its first edit, an agent must declare the exact file paths it owns on the task board.
- Two agents must never edit the same file concurrently.

## Spawn rules (checklist)
Before dispatching any worker, the orchestrator must confirm:
- [ ] No file appears in two agents' ownership sets.
- [ ] Each agent's ownership is declared on the board **before** any edit.
- [ ] Each agent id and lane is recorded on the board.
- [ ] Each worker will report exactly these four fields:
  - root cause
  - files touched
  - verification command
  - result

## Task board format
Maintain one canonical board per session (in the session notes, or in [BACKLOG.md](BACKLOG.md) for cross-session work):

| id | lane | owner agent | files owned | status | verification command | result |
|----|------|-------------|-------------|--------|----------------------|--------|
| T1 | L2 | agent-L2-1 | src/OpenGLBackend.cpp, include/OpenGLBackend.h | in progress | `cmake --build build` | — |

Status lifecycle (strict, no skipping):
`decomposed → in progress → ready for review → verified → merged`
- `blocked` is a side state: the row must name the blocking task id and the conflicting file.
- A row may move to `merged` only after its verification command passes **and** the cumulative `cmake --build build` + test run still passes post-merge.

## Merge protocol
- Fixed merge order: **L1 → L2 → L3 → L4 → L5**.
- After each merge, the orchestrator re-runs the full build and tests.
- If a merge breaks the build: revert **only that task's files**, re-decompose the failing workstream, and re-dispatch. Workers must not hot-patch other lanes' files.

## Safe startup sequence for a session
1. Build the project once to establish a clean baseline.
2. Classify the request:
   - content/code generation or multi-file change → **default path** (auto-dispatch);
   - single-file change under ~50 diff lines → **fallback path** (solo agent, still one board row).
3. Default path:
   - Decompose into 2–4 lane workstreams with disjoint file sets.
   - Create the board with declared ownership for every row.
   - Dispatch workers concurrently.
4. Fallback path: create one board row, execute solo, record verification.
5. Resume rule: if board rows already exist, do **not** re-decompose — resume from the first non-`merged` row.
6. Merge in fixed lane order; a task is done only when the cumulative build and tests pass.

## Red flags
- Multiple `project(...)` declarations in one CMake tree
- Mismatched return types between factories and interfaces
- Silent broad refactors during a bug fix
- Parallel edits to the same file
- Claiming success without fresh command output
- Agent edits a file outside its declared ownership
- Task marked `merged` without a passing verification command
- Two board rows claiming the same file
- Worker agent re-decomposing work or dispatching other agents