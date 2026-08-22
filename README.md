# Sudoku Solver & Generator

A C89-compatible Sudoku solver and generator implemented in C, with a compact bitmask-based grid representation, incremental constraint propagation, recursive backtracking, and randomized puzzle generation.

The project was developed as part of a university assignment. The externally defined `Grid_T` API was preserved at the interface level, while the internal representation was redesigned to reduce per-cell state and maintain constraint information incrementally.
## Features

* Sudoku solving using constraint propagation and recursive backtracking.
* Minimum Remaining Values (MRV) cell selection.
* Bitmask-based representation of cell candidates.
* Incremental row, column, and 3×3 box constraint tracking.
* Detection of whether a solution was reached without backtracking.
* Sudoku validation and error reporting.
* Sudoku puzzle generation with a configurable number of clues.
* Optional generation using the solver's deterministic-solution criterion.
* No global grid state.

---
## 1. Architecture

The implementation is divided into two main layers.

```text
+-----------------------------+
|        Sudoku Layer         |
|-----------------------------|
| solve                       |
| generate                    |
| validate                    |
| input / output              |
+--------------+--------------+
               |
               v
+-----------------------------+
|          Grid Layer         |
|-----------------------------|
| cell representation         |
| candidate masks             |
| row / column / box masks    |
| constraint propagation      |
| candidate iteration         |
+-----------------------------+
```

The grid layer is responsible for maintaining a consistent Sudoku state.

The Sudoku layer builds the solving, validation, and generation logic on top of that state representation.

This separation keeps the low-level constraint operations independent from the higher-level solving and generation procedures.

---

## 2. Grid Representation

The original assignment represents each cell using an array of integers describing its available choices.

This implementation replaces that representation with:

```c
typedef struct cell_s {
    unsigned short avail_mask
    unsigned char value;
    unsigned char num_choices;
} Cell_T;
```

Each cell therefore contains:

* `value`: the currently assigned value, or `0` if unresolved.
* `avail_mask`: a nine-bit mask representing the values that can still be assigned.
* `num_choices`: the number of available values.

The nine bits correspond directly to Sudoku values:

```text
bit 0 -> value 1
bit 1 -> value 2
...
bit 8 -> value 9
```

For example, if values `1`, `3`, `4`, and `7` are available:

```text
value:       9 8 7 6 5 4 3 2 1
availability 0 0 1 0 0 1 1 0 1
```

The same information can be represented using a single nine-bit mask.

### Grid-level constraint masks

The grid additionally maintains:

```c
unsigned short rows[9];
unsigned short cols[9];
unsigned short boxes[9];
```

Each mask records the values already present in the corresponding row, column, or 3×3 box.

Consequently, the grid maintains both:

```text
Cell-level information
    available values for each cell

Grid-level information
    values already occupied in each constraint group
```

This allows constraint information to be updated incrementally rather than reconstructed from the entire grid after every assignment.

---

## 3. Memory Layout

The current representation contains:

```text
81 × Cell_T                         324 bytes
9 × unsigned short rows             18 bytes
9 × unsigned short cols             18 bytes
9 × unsigned short boxes            18 bytes
3 × unsigned char state fields        3 bytes
                                     ---
                                     381 bytes
```

The final size of `Grid_T` is ABI-dependent because structure alignment and padding are determined by the compiler and target platform.

For this reason, the implementation treats `sizeof(Grid_T)` as the authoritative value rather than relying solely on the theoretical member total.

The compact representation was also considered in the context of recursive solving, where multiple grid states can exist as active stack frames due to the value-based API.

The relevant performance consideration is therefore the size of the **active recursion state**, not the size of the entire search tree.

---

## 4. Constraint Propagation

The central operation of the grid layer is:

```c
Grid_T grid_update(Grid_T g, Choice_T c);
```

When a value is assigned to a cell, `grid_update()` performs several operations:

```text
                    Choice
                      |
                      v
              +---------------+
              | Target Cell   |
              +---------------+
                      |
          +-----------+-----------+
          |           |           |
          v           v           v
       Row mask   Column mask   Box mask
          |           |           |
          +-----------+-----------+
                      |
                      v
             Peer candidate removal
                      |
                      v
             Updated num_choices
```

The assigned value is removed from the candidate masks of all cells sharing the same:

* row,
* column,
* 3×3 box.

The corresponding constraint masks are updated at the same time.

This means that the solver does not need to repeatedly scan the entire grid to reconstruct the current constraint state.

---

## 5. Candidate Selection

`grid_iterate()` has two related responsibilities.

### Selecting the next cell

When called with an empty choice:

```c
Choice_T empty = {0, 0, 0};
grid_iterate(g, empty);
```

the function searches unresolved cells and selects one with the smallest number of available candidates.

This is a form of the **Minimum Remaining Values (MRV)** heuristic.

Conceptually:

```text
Unresolved cells:

A -> 5 candidates
B -> 2 candidates
C -> 7 candidates
D -> 1 candidate

             ↓

         choose D
```

Selecting the most constrained cell tends to expose contradictions earlier and reduces unnecessary branching in the recursive search.

A cell with exactly one remaining candidate can be returned immediately.

A cell with no remaining candidates is treated as a dead end.

### Enumerating candidates

When `grid_iterate()` receives a non-zero `t.n`, it instead searches for the next valid candidate for the same cell.

This allows the solver to use the same function both for cell selection and for candidate enumeration during backtracking.

---

## 6. Solver

The solver combines deterministic propagation with recursive search.

Its high-level process is:

```text
                 Current Grid
                      |
                      v
             Apply current choice
                      |
                      v
             Select MRV cell
                      |
              +-------+-------+
              |               |
          no cell          cell found
              |               |
              v               v
       solved / dead end   enumerate values
                              |
                     +--------+--------+
                     |        |        |
                    v1       v2       v3 ...
                     |        |        |
                     v        v        v
                  recurse  recurse  recurse
                     |
                  failure
                     |
                     v
               next candidate
```

The recursive function receives and returns `Grid_T` values.

This follows the value-based API required by the assignment and naturally isolates the state of each recursive branch.

The trade-off is that copying a `Grid_T` introduces memory-copying overhead.

A pointer-based implementation could instead modify a shared grid in place and explicitly undo changes during backtracking. That approach would reduce copying but would require a different state-management strategy and would not match the provided value-based interface.

---

## 7. Deterministic Solution and `unique`

The `unique` field requires an important distinction.

In this implementation:

```text
unique = 1
```

means that the puzzle was solved without requiring a backtracking guess.

If the solver has to branch over multiple candidates, the flag is cleared:

```text
unique = 0
```

Therefore, the flag does **not** constitute a general mathematical proof that the Sudoku has exactly one solution.

There are Sudoku puzzles with a mathematically unique solution that require more advanced deduction techniques or search to solve. Such a puzzle may therefore result in:

```text
unique = 0
```

even though its actual solution is unique.

The implementation consequently uses the following interpretation:

> `unique == 1` means that the puzzle was solved completely by the deterministic deduction mechanisms implemented by this solver.

This distinction is particularly relevant to the `-u` generator option.

---

## 8. Puzzle Generation

Puzzle generation follows a two-stage approach.

### 8.1 Generate a complete grid

The generator first creates a randomized starting configuration by shuffling the first Sudoku row.

The solver then completes the grid.

During this phase, random tie-breaking can be enabled when multiple cells have the same MRV score.

This provides variation between generated complete grids without requiring a randomized candidate ordering at every recursive level.

### 8.2 Dig values from the completed grid

Once a complete grid exists, the generator creates a shuffled list of cell indices:

```text
0, 1, 2, ..., 80
```

The indices determine the order in which values are removed.

Conceptually:

```text
Complete grid
      |
      v
shuffle cell indices
      |
      v
remove one value
      |
      v
optional solver verification
      |
      +---- accepted ---> continue
      |
      +---- rejected ---> restore value
```

For ordinary generation, values are removed until the requested number of elements remains.

When `-u` is enabled, every tentative removal is tested using the solver. If the resulting puzzle cannot be solved without backtracking, the removal is reverted.

This approach prioritizes a simple and memory-conscious generator over exhaustive exploration or uniform sampling of the Sudoku solution space.

---

## 9. Performance Considerations

Several implementation decisions were made with the cost of frequently executed operations in mind.

### Bitmask operations

Candidate and constraint information can be manipulated using integer bit operations:

```c
mask & value_mask
mask |= value_mask
mask &= ~value_mask
```

This provides a compact representation and avoids maintaining larger arrays of candidate values.

### Incremental constraint updates

Row, column, and box masks are updated when a value is inserted.

Candidate counts are also updated only for affected peer cells.

This avoids repeatedly deriving the same information from the complete board.

### Avoiding repeated division and modulo

The `grid_update()` hot path needs to determine the corresponding 3×3 box.

Small lookup tables are used for the `/ 3` and `% 3` related calculations instead of repeatedly performing those operations inside the peer-update loop.

This is a targeted micro-optimization. Its actual benefit depends on compiler optimization and target architecture and is not assumed to be significant without measurement.

### Active recursive state

Because the solver passes `Grid_T` by value, recursive calls can require multiple grid copies to coexist in the active call stack.

The compact grid representation therefore also reduces the size of each recursive state.

The original design considered the L1 data cache size of the target system when evaluating this memory footprint, but no claim of guaranteed cache residency is made without runtime measurement.

---

## 10. Design Trade-offs

The implementation intentionally makes several trade-offs.

| Decision | Benefit | Cost |
|---|---|---|
| Bitmask candidate representation | Compact state and efficient bitwise operations | Less immediately readable than an array of integers |
| Row/column/box masks | Fast incremental constraint tracking | Additional state that must remain consistent |
| MRV cell selection | Usually reduces unnecessary branching | Requires scanning unresolved cells |
| Value-based `Grid_T` | Simple state isolation and compatibility with assignment API | Copies grid state during calls |
| Lookup tables for box coordinates | Avoids repeated arithmetic in a hot path | Slightly more indirect code |
| Randomized tie-breaking | Variation during generation | Non-deterministic generation behavior |
| Seed-and-dig generation | Simple generation procedure with limited auxiliary state | Does not uniformly sample all valid Sudoku puzzles |
| Deterministic `unique` flag | Simple criterion compatible with implemented solver | Not equivalent to a mathematical uniqueness proof |

---

## 11. Validation

The program provides a separate validation mode:

```bash
./sudoku -c
```

The validation procedure checks:

* whether every cell is filled,
* whether duplicate values exist in any row,
* whether duplicate values exist in any column,
* whether duplicate values exist in any 3×3 box.

Detected errors are reported to standard error.

---

## 12. Command-Line Interface

### Solve

```bash
./sudoku
```

Reads a Sudoku grid from standard input, solves it, and prints the resulting grid.

### Validate

```bash
./sudoku -c
```

Validates the supplied Sudoku grid and reports detected errors.

### Generate

```bash
./sudoku -g <number_of_elements>
```

Generates a Sudoku puzzle containing the requested number of filled elements.

### Generate with deterministic-solution verification

```bash
./sudoku -g <number_of_elements> -u
```

Generates a puzzle while retaining removals only when the implemented solver can still solve the resulting puzzle without backtracking.

---

## 13. Limitations

The implementation has several deliberate limitations.

### Solver completeness

The recursive search is capable of finding a solution by exploring the remaining candidate choices when deterministic propagation is insufficient, the `unique` flag is not a complete uniqueness test.

It describes whether the implemented deterministic deduction process was sufficient to solve the puzzle without branching.

### Generator distribution

The generator does not attempt to uniformly sample the space of valid Sudoku puzzles.

Its purpose is to generate valid puzzles efficiently under the requirements of the assignment.

### Value-based state management

Passing `Grid_T` by value simplifies recursive state isolation but introduces copying overhead.

A production-oriented solver could use pointer-based mutation combined with explicit state restoration or a reversible update mechanism.

### Target-dependent memory layout

The exact size and alignment of `Grid_T` depend on the target compiler and ABI.

Memory-layout assumptions should therefore be verified using `sizeof()` on the target environment.

---

## 14. Development and Technical Focus

The most significant implementation work was concentrated in the grid representation and constraint-update mechanism.

The main areas of investigation were:

* compact representation of Sudoku state,
* bitwise candidate manipulation,
* incremental constraint propagation,
* recursive solver state management,
* candidate-selection heuristics,
* memory layout,
* and refactoring of the grid-update logic.

The project also involved evaluating implementation alternatives rather than optimizing solely for source-code size or apparent algorithmic complexity.

For example, the generator deliberately uses a seed-and-dig approach instead of performing randomized candidate selection throughout the entire recursive search. The choice reflects the intended balance between randomness, implementation complexity, and the constraints of the assignment API.

---

## 15. AI Assistance

An LLM was used as a development aid during the project, primarily in two areas:

### Bitwise representation

The LLM was used to explore possible ways of representing Sudoku candidate sets with bitmasks and to investigate techniques for inspecting and counting active bits.

The final representation and its integration into the grid architecture were implemented and tested within the project.

### Refactoring

The LLM was also used to review refactoring alternatives for the grid-update logic.

In particular, it was used to evaluate whether extracting repeated operations into macros would improve the implementation. The resulting discussion highlighted drawbacks related to type safety, debugging, and maintainability, leading to a function-based/refactored implementation instead.

The LLM was used as a development and review aid rather than as the source of truth for the implementation. The repository's source code and assignment specification remain the authoritative sources for the project's behavior and requirements.

---

## 16. Project Context

This repository was developed as a university C programming assignment and is not intended to represent a production Sudoku engine.

Its primary purpose is to demonstrate:

* low-level data representation,
* bitwise operations,
* constraint-based problem solving,
* recursive search,
* memory-aware design,
* API-driven implementation,
* and evaluation of engineering trade-offs.

The implementation deliberately preserves the constraints imposed by the assignment while replacing the internal cell representation with a more compact representation tailored to the Sudoku problem.
