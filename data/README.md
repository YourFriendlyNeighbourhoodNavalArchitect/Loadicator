# Ship Data

This directory contains the ship data as CSV files, extracted once from the vessel's documents in `source/`. The extraction repairs the damaged rows of the sounding tables, and checks every loading condition against the total weight printed in the book. It requires `cmake`, a C++20 compiler and `pdftotext`, and its output is deterministic:

```bash
tools/extractData.sh
```

## Files

- `ship.csv`: Length between perpendiculars and the reference point of the hydrostatic tables.
- `hydrostatics/evenKeel.csv`: `lcf` and `vcb` by `draught` on even keel.
- `hydrostatics/trimGrid.csv`: `displacement`, `lcb` and `kmt` by `draught` and `trim`.
- `compartments/<id>.csv`: Centroid `lcg`, `tcg`, `vcg` and free-surface `inertia` of the contents by `volume`, from the lowest sounding to the full compartment.
- `conditions/condNN.csv`: The loading conditions of the trim and stability book.
- `conditions/expected.csv`: The results printed in the book for each condition.

Lengths are in metres, masses in tonnes. Longitudinal positions are measured from the aft perpendicular, transverse positions and heel are positive to port, and vertical positions are above the baseline. Trim is positive by the stern, and the hydrostatics are for sea water of 1.025 t/m³.

## Loading Conditions

A loading condition has one row per item, with its columns in any order:

```csv
kind,id,mass,fillPercent,density,phase,lcg,tcg,vcg,fsm
name,Ballast departure,,,,,,,,
tank,R2.01,,100,1.025,liquid,,,,
tank,R1.3,36695.8,,2.990,solid,,,,
weight,Lightweight,24641.9,,,,132.01,0.00,13.67,0
```

- `name`: Name of the condition; without it, the condition is named after its file.
- `tank`: Contents of a compartment, by `mass` or `fillPercent`, with `density` and `phase`. A `liquid` (the default) has a free surface.
- `weight`: Any other item, by `mass`, `lcg`, `tcg`, `vcg` and optionally its free-surface moment `fsm`.

Compartments are named as in the vessel's documents, with `P` or `S` for the port or starboard tank of a pair: `R1.x` cargo holds, `R2.xx` water ballast, `R3.x` heavy fuel oil, `R4.x` marine diesel oil and `R6.xx` sludge and bilge tanks.
