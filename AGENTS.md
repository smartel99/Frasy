# Frasy — agent guidelines

Rules for AI agents working on this repository. They must be followed at all times.

## Project at a glance
- Windows desktop application for automated PCBA testing: a C++23 host (`Frasy/src`, on the `Brigerad` engine) and a Lua layer (`Frasy/lua`) where test logic lives.
- Frasy is used as a git submodule by product repositories. The Lua SDK, the C++ classes products derive from and `config.json` are public API: a change there can break every product.
- Build: CMake 3.22+, MSVC, Ninja. Tests: GoogleTest, enabled with `-DFRASY_BUILD_TESTS=ON`, one executable per module (`tests/<module>/`, `FrasyTest_<Module>`).
- `demo_mode/`: a standalone CMake product that consumes Frasy from the parent directory. Its demo products (`src/lua/user/<feature>/`) and app layer (`src/layers/`, `src/my_frasy_interpreter.cpp`) show and exercise the API.
- Docs: MkDocs site in `docs/` (architecture, developer guide, Lua reference, panels). Implementation plans live in `docs/planification/`.

## Workflow
- Doubts should be questioned.
- Issues found should be brought up and discussed, not silently fixed on your own.
- Tasks should be broken down into subtasks with the help of the user before beginning them. Larger plans are saved in `docs/planification/<feature>.md`.
- Each subtask is one or more small, focused commits. Don't make fat commits.
- Ask before you commit something. Explain the changes you want to commit and draft a commit message, but do not commit without approval. Commit messages are short and to the point.
- Before asking for permission to commit a subtask (once per subtask, before its commits), ask whether a code review of the changes should be performed. If agreed to, the review is done with the `code-reviewer` agent, which can be found [here](https://gitlab.com/conception-electronique-prive/coding-style/-/blob/main/code-reviewer.md?ref_type=heads).
- Don't push, open or update a PR unless asked.

## Code
- Code follows the shared coding style in `https://gitlab.com/conception-electronique-prive/coding-style` (`cpp-style.md`, `lua-style.md`, `.clang-format`, `.lua-format`). Project rules in this file take precedence. Implementation and review agents are pointed at these documents in their brief.
- Project deviations:
  - `.clang-tidy`: Frasy keeps its own configuration, not the shared one.
  - Legacy code is reformatted when touched, in its own commit before the functional change. Untouched files stay as they are.
- Public API changes (Lua SDK globals, C++ extension points, config keys) are flagged to the user before they are made. Prefer additive, backward-compatible changes; a breaking change is discussed first.
- New or changed public API is exercised in `demo_mode`, in the same subtask: a demo product in `src/lua/user/<feature>/` for the Lua SDK, the demo app for C++ extension points. After changing Lua files, regenerate the hashes with `demo_mode/generate_hashes.bat`.
- A feature or behaviour change that is user-visible updates the matching page in `docs/` in the same subtask.

## Tests
- Unit tests are written for every feature implemented. They aren't just surface-level pass tests: they check edge cases and failure cases on top of behaviour. Whole features get integration tests where possible (e.g. through the orchestrator fixture, generating and running a sequence end to end).
- Lua core logic is tested from C++ with sol2, using `lua_test_fixture.h` / `orchestrator_test_fixture.h`. Host bindings are mocked; tests never need hardware.
- Code that needs real instrumentation boards (CANopen, DAQ, PIO, …) cannot be validated by an agent. Say so, keep the hardware-independent part testable, and ask the user to validate the rest on the bench.
- Gates of a subtask, kept short on purpose:
  - Tests: only the subtask's own test executables (`FrasyTest_<Module>`, narrowed with `--gtest_filter` where useful), never the whole suite. A doubt that something broke elsewhere is asked first.
  - Builds: the branch tip, 0 new warnings. No build of each commit. If the subtask touches public API, `demo_mode` is built too, with 0 new warnings.
  - A fix pass after a review runs again only the gates its changes touch.
- The entire test suite (`ctest --test-dir <build> --output-on-failure`) runs only at the end of a task, before it is offered for merge.
