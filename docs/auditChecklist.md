# Audit checklist

Every finding from the repository audit, with the recommended fix and where it is resolved.
Locations in the "Finding" column refer to the **original** code (`Source/`, `Data/`), which this
change replaces.

Legend: ✅ resolved · ➖ intentionally out of scope (see note)

## C — Correctness defects

| ID | Finding | Recommended fix | Status |
|----|---------|-----------------|--------|
| C1 | Hold tables read from `Data/Cargo hold data/Hold (N).txt`, which does not exist; 25 of 31 conditions fail (`LoadingCondition.cpp:152`). | Load every compartment table from one data directory through one loader; no paths built inside calculations. | ✅ `io/dataLoader.cpp` loads `data/compartments/*.csv` |
| C2 | Error from C1 surfaces as the unrelated `"Error instantiating SoundingTablesReader."` (`LoadingCondition.cpp:74-76`). | Never swallow the original exception; add context, keep the cause. | ✅ `LoadicatorError` carries file/line/context; no catch-and-replace |
| C3 | Sounding parser reads across `-----` terminators and `Compartment ident:` headers; 41 of 76 tanks empty or merged (`SoundingTablesReader.cpp:80-89`). | Parse each compartment as a bounded block; never read past its terminator. | ✅ `tools/dataExtractor/compartmentTables.cpp` collects each block, then parses it |
| C4 | Sounding source file is a corrupt PDF→text conversion: 304 rows truncated to 8 fields, their tails glued on a later line (`35.798.6`). | Repair deterministically (IMOM has one decimal) and validate FILL against volume/capacity. | ✅ repaired and validated in the extractor; all 76 tanks complete, liquid-tank centroids match the book within 0.03 m |
| C5 | Empty sounding data silently falls back to book mass/centres (`LoadingCondition.cpp:41`). | Unknown compartment ⇒ hard error. | ✅ `resolveLoadingCondition` throws for unknown ids |
| C6 | Free-surface moment added to the transverse moment (`Ship.cpp:34`); GM never corrected; spurious port heel. | `GMfluid = KMT − KG − ΣFSM/Δ`; FSM never enters TCG. | ✅ `calc/massSummary.cpp`, `calc/stability.cpp` |
| C7 | "Two nearest rows" interpolation: unordered seed pair, and pair need not bracket x (×3 copies). VCG off by 1.07 m at V = 2000 m³ in Hold 1. | One bracketing interpolator (`std::upper_bound` on strictly increasing x). | ✅ `numeric/interpolation.h` + regression test |
| C8 | `HydrostaticsReader::interpolate` leaves LCF uninitialised, assigns LCB twice (`HydrostaticsReader.cpp:77-78`). | Return a struct built in one place. | ✅ `HydrostaticTable::evenKeelAt` |
| C9 | Regex drops the sign of `-.116` (`HydrostaticsReader.cpp:91`); 12 values affected incl. LCF. | One number parser that accepts `-.5`, `.5`, `-0.5`. | ✅ `text/numberParsing.cpp` + test |
| C10 | Density looked up "two lines below RHO", `nextLine[2]`; fails for single-tank groups (10 conditions); `densities[index-1]` / `values[0]` UB; `resize(index+1)` off-by-one ⇒ silent density 0. | Store density explicitly per loading entry. | ✅ `density` column in condition CSVs; validated `> 0` |
| C11 | Hold interpolation has no zero-division guard; `cargoData[11]` hard-coded (`LoadingCondition.cpp:156,178`). | Capacity = last row; table validation at load. | ✅ `CompartmentTable` constructor validates |
| C12 | Zero displacement only prints a warning and continues (`Ship.cpp:44-46`). | Throw. | ✅ `summarizeWeights` throws |
| C13 | `atan(TCG/GM)` meaningless for GM ≤ 0. | Report "unstable", heel undefined. | ✅ `std::optional<Angle> heel` |
| C14 | Hydrostatic matrix pre-filled 83×30 with zeros; padding rows can be selected. | Size from data; validate monotonic displacement. | ✅ `HydrostaticTable` constructor validates |
| C15 | `static_cast<char>` truncates UTF-16 code points (`U+010A` → `'\n'`). | Remove runtime PDF text handling. | ✅ no PDF code at runtime |
| C16 | `sprintf_s` is MSVC-only; does not compile with GCC/Clang. | Standard C++ only. | ✅ builds warning-free (`-Werror`) on GCC 13 and Clang 18; MSVC covered by CI |

## N — Numerical model and physics

| ID | Finding | Recommended fix | Status |
|----|---------|-----------------|--------|
| N1 | TA/TF split trim 50/50 about the even-keel draught, ignoring LCF (`Ship.cpp:56-57`). | First order: rotate about LCF. Free trim: draughts from amidships draught. | ✅ both solvers |
| N2 | Only first-order (even-keel) equilibrium; trim tables in the hydrostatics unused. | Free-trim solve on the draught × trim grid. | ✅ `calc/equilibrium.cpp` (`freeTrim`, default; see E1) |
| N3 | Tank state interpolated against FILL %, which is rounded to 0.1 %. | `V = fill · capacity`; interpolate centres against volume. | ✅ `CompartmentTable::stateAt(Volume)` |
| N4 | Hold FSM column ignored (`fsm = 0`). | Use the inertia column for liquid-filled holds. | ✅ holds use the same table type as tanks |
| N5 | MCT printed as `[tons/cm]`. | `t·m/cm`. | ✅ `report/reportWriter.cpp` |
| N6 | Heel sign convention undocumented. | Document: y and heel positive to port (book convention). | ✅ `data/README.md`, report header |
| N7 | Magic `139.1` (LBP/2) in the reader. | `ShipParticulars` from `data/ship.csv`; data normalised to x-from-AP. | ✅ |

## A — Architecture

| ID | Finding | Recommended fix | Status |
|----|---------|-----------------|--------|
| A1 | Input limited to the book's 31 predefined conditions; not a general loadicator. | Loading condition is a user-editable CSV. | ✅ `loadicator myCondition.csv` |
| A2 | Book's own results parsed then discarded. | Use them as a regression oracle. | ✅ `data/conditions/expected.csv` + `tests/regressionTests.cpp` |
| A3 | Static reference data scraped from PDFs on every run; hydrostatics parsed twice. | One-off extraction to reviewed CSV; runtime has no PDF dependency. | ✅ `tools/dataExtractor`, `data/` |
| A4 | Readers compute, calculators do I/O, report prints to stdout. | Layers: `io` → `domain` → `calc` → `report`; calc is pure. | ✅ |
| A5 | `Ship` does all work in its constructor; 18 members, mostly temporaries. | Free functions returning value types. | ✅ `solveEquilibrium(...) → FloatingPosition` |
| A6 | Two-phase init (`LoadingCondition` + `calculate()`). | Construct complete objects or return values. | ✅ |
| A7 | Meaning encoded in strings (`"R1."`, `key[1]`, `key[3]`, magic names). | Explicit `kind`, `CompartmentId`, explicit density. | ✅ |
| A8 | Positional `vector<double>` records with per-key semantics. | Named structs. | ✅ |
| A9 | Anonymous `tuple<double×5/6>`; slots reused for different quantities. | Named structs with typed quantities. | ✅ `units/quantity.h` |
| A10 | Magic numbers: 83, 30, 19, 13, 6, row 11, page 3, 31, paths. | Gone with the PDF/text scrapers; remaining constants named. | ✅ |
| A11 | Catch-and-rethrow discarding `e.what()`. | One error type with context; catch only in `main`. | ✅ |
| A12 | pdfium handles without RAII; library init/destroy ×3. | Removed from runtime (A3). | ✅ |

## D — Duplication and dead code

| ID | Finding | Recommended fix | Status |
|----|---------|-----------------|--------|
| D1 | Nearest-two search ×3, lerp boilerplate ×14. | Single `LinearTable` / `interpolateLinear`. | ✅ |
| D2 | PDF open/iterate/close ×2, page text extraction ×3. | Removed (A3). | ✅ |
| D3 | Two `extractNumericalValues` with diverging regexes. | Single `parseNumbers`. | ✅ |
| D4 | Whitespace trimming ×3. | `text::trim`. | ✅ |
| D5 | `tankPlan[X].insert(...)` ×6; duplicated Lightweight branch. | Uniform `WeightItem`. | ✅ |
| D6 | Dead params/members/functions (`skipFirst`, `skipNext`, `currentLineIndex`, `getData`, unused members, copy of map, `ostream&` alias, explicit `close()`, meaningless `[1].empty()`). | Delete. | ✅ |
| D7 | Redundant regex parts (`|0[1-9]`, nested optional). | Delete. | ✅ |
| D8 | Matrices passed/copied by value. | `const&` / `std::span`. | ✅ |
| D9 | `std::regex` constructed per call / inside loops. | `static const` (extractor only); runtime uses no regex. | ✅ |

## Q — C++ quality

| ID | Finding | Recommended fix | Status |
|----|---------|-----------------|--------|
| Q1 | Missing includes (`<algorithm>`, `<cmath>`, `<cctype>`, `<tuple>`); heavy transitive includes; pdfium in public headers. | Include what you use; minimal headers. | ✅ |
| Q2 | `constexpr double M_PI` in a header. | `std::numbers::pi`. | ✅ |
| Q3 | 12 warnings under `-Wall -Wextra -Wshadow -Wconversion`. | Build warning-clean with `-Werror`. | ✅ GCC + Clang |
| Q4 | Little `const`/`static`; mutable helpers. | Pure free functions, `const` members, `[[nodiscard]]`. | ✅ |
| Q5 | Noise comments (`// Implementing the X method`). | Comment intent only. | ✅ |
| Q6 | Inconsistent naming. | camelCase enforced by `.clang-tidy`. | ✅ |

## B — Build, portability, repository hygiene

| ID | Finding | Recommended fix | Status |
|----|---------|-----------------|--------|
| B1 | No build system. | CMake ≥ 3.20 with `ctest`. | ✅ `CMakeLists.txt`, `cmake/compilerWarnings.cmake` |
| B2 | No tests, no CI. | Unit + regression tests; GitHub Actions. | ✅ 31 tests; CI: GCC, Clang, GCC+ASan/UBSan, MSVC, clang-format, clang-tidy |
| B3 | No `.gitignore` / `.gitattributes`; CRLF; one file with BOM. | Add both; LF for sources; raw data kept byte-exact. | ✅ |
| B4 | File names with spaces and browser suffixes (`(1)`). | camelCase names under `data/source/`. | ✅ |
| B5 | Hard-coded relative paths; must run from repo root; no CLI. | `--data-dir`, `--output`, `--method`. | ✅ |
| B6 | README stale; build and dependencies undocumented. | Rewrite. | ✅ |
| B7 | Web-upload commit history. | Clear commits from now on. | ✅ |
| B8 | Shipyard documents (Bohai BH410-020-035A, BH410-020-005) committed. | Owner to confirm redistribution rights. | ➖ owner decision; files kept, flagged in README |

## E — Found while fixing

| ID | Finding | Resolution | Status |
|----|---------|------------|--------|
| E1 | Free-trim equilibrium must put B and G on one vertical in the *trimmed* attitude: `LCB − LCG = (VCB − KG)·tanθ`. Without it trim was up to 0.08 m short. | Term added; RMS trim error vs. the book 0.035 m. | ✅ |
| E2 | Rows split across page breaks and an empty-tank row without its sounding (R6.11). | Joined / restored in the extractor. | ✅ |
| E3 | Some tables stop at FILL 99.9 %: the last volume is not the capacity. | Capacity = V / FILL, stored as `# capacity:`. | ✅ |
| E4 | Book masses are rounded to 0.1 t, which puts full small tanks a hair above capacity. | 0.5 % capacity allowance. | ✅ |
| E5 | Linear interpolation overshoots the convex KMT curve at light draughts (1.2 m at COND01). | Natural cubic splines along draught and trim. | ✅ |
| E6 | Cargo-hold source tables are coarse and their TCG column is implausible; partly filled solid-cargo holds differ from the book by up to 0.08 m LCG / 0.26 m TCG. | Not fixable from the available data: documented in `data/README.md`, given named tolerances in the regression test. | ➖ data limitation |
