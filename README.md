# Loadicator

This repository contains a loading computer for a 174,000 DWT double-skin bulk carrier, written in C++. For any loading condition it computes the displacement and centre of gravity, the draughts and trim at equilibrium, the metacentric height with free-surface correction, and the static heel.

## Project Overview

Each loaded compartment is looked up in its capacity table, which gives the centroid and free-surface moment of its contents; summing these with the other weights gives the displacement and centre of gravity. The draught and trim are then solved together on the hydrostatic tables, interpolated with cubic splines over draught and trim, until the centres of buoyancy and gravity lie on one vertical. The tables are never extrapolated.

The ship data are extracted once from the vessel's documents into CSV files. The results are verified against the 31 loading conditions of the trim and stability book.

```bash
cmake -S . -B build && cmake --build build
build/loadicator 7                   # condition 7 of the book
build/loadicator myCondition.csv     # any loading condition
ctest --test-dir build               # comparison with the book
```

## Repository Structure

- `app/`: Command-line program and its report.
- `src/util/`: Dimension-checked quantities, errors and text parsing.
- `src/numeric/`: Linear and natural cubic spline interpolation.
- `src/domain/`: Hydrostatic tables, compartment capacity tables and loading conditions.
- `src/calc/`: Weight summary, equilibrium solver and initial stability.
- `src/io/`: CSV reading and the loaders of the ship data and loading conditions.
- `tests/bookComparison.cpp`: Deviation from the trim and stability book over its 31 loading conditions.
- `tools/`: One-off extraction of the ship data from the vessel's documents.
- `data/`: Ship data and loading conditions, described in `data/README.md`.
