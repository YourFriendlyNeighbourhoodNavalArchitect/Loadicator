# Ship data

Everything the program knows about the ship, as plain CSV generated from the original documents
in [`source/`](source). Regenerate with `tools/extractData.sh` (needs `pdftotext`); the output is
deterministic, so `git diff` shows exactly what changed.

## Conventions

| Axis / quantity | Convention |
|---|---|
| x (LCG, LCB, LCF) | metres **forward of the aft perpendicular** |
| y (TCG) | metres, **positive to port** (the book's convention; heel is positive to port too) |
| z (VCG, KG, KMT) | metres above the baseline |
| trim | metres, **positive by the stern** |
| draught | moulded, metres; `draught` in hydrostatic tables is measured at the reference point (RFP, amidships) |
| mass, displacement | tonnes; hydrostatics are for sea water of 1.025 t/m³ |

CSV dialect: comma-separated, no quoting; `# key: value` lines are metadata, other `#` lines are
comments; columns are addressed by header name, so their order does not matter.

## Files

| Path | Content | Source |
|---|---|---|
| `ship.csv` | principal particulars and the RFP position | hydrostatic tables, p. 1–2 |
| `hydrostatics/evenKeel.csv` | draught, displacement, LCF, LCB, VCB, KMT, KML, MCT 1 cm, TPC (83 draughts) | hydrostatic tables |
| `hydrostatics/trimGrid.csv` | displacement, LCB, KMT at 83 draughts × trims −2 … 6 m | hydrostatic tables, cross tables |
| `compartments/<id>.csv` | capacity table: sounding, volume, LCG, TCG, VCG, free-surface inertia; `# capacity:` is the volume at 100 % | sounding tables (76 tanks), `source/cargoHolds` (9 holds) |
| `conditions/condNN.csv` | the book's 31 loading conditions | trim and stability book |
| `conditions/expected.csv` | the book's results for each condition (regression reference) | trim and stability book |

## Loading-condition format

```csv
# name: My voyage, departure
kind,id,mass,fillPercent,density,phase,lcg,tcg,vcg,fsm
tank,R2.01,,100,1.025,liquid,,,,
tank,R1.3,36695.8,,2.990,solid,,,,
weight,Lightweight,24641.9,,,,132.01,0.00,13.67,0
weight,CREW&ST,200,,,,39.10,0.00,30.00,0
```

- `tank` — a compartment with a table in `compartments/`. Give **either** `mass` [t] **or**
  `fillPercent` [% of capacity], plus `density` [t/m³] and `phase` (`liquid` has a free surface,
  `solid` does not). Centres and free-surface moment come from the table.
- `weight` — anything else: `mass`, `lcg`, `tcg`, `vcg` and optionally `fsm` [t·m].

## Provenance and repairs

`source/soundingTables.txt` is a damaged PDF-to-text conversion. The extractor repairs three
defects and validates every row against `FILL = volume / capacity` (±0.15 %):

1. **Truncated rows** (304 rows in R2.02S–R2.11S): only sounding + 7 volumes survive; the FILL,
   LCG, TCG, VCG, IMOM tails follow later on one line with IMOM glued to the next FILL
   (`35.798.6` → `35.7`, `98.6`). IMOM has one decimal, so the split is unambiguous.
2. **Rows split by page breaks** (`190` / `1048.72 … 58265.2`): joined.
3. **R6.11's empty-tank row** printed without its zero sounding: restored.

Tables that stop short of 100 % (last row 99.9 %) get their true capacity from `V / FILL`.

## Known limitations

- **Cargo-hold tables are coarse** (12 rows) and their TCG column is implausible between 9 and
  18 m sounding (e.g. 0.526 → 0.797 → 0.098 m in hold 3). For partly filled holds of solid cargo
  the book's centres differ by up to 0.08 m (LCG) and 0.26 m (TCG), which can move the computed
  heel by up to ~1.3°. Liquid tanks agree with the book within 0.03 m.
- The hold files label their last column `fsm`; it is treated as a free-surface second moment of
  area [m⁴], like the sounding tables' IMOM. It only matters for holds carrying liquid.
- Hydrostatics cover trims −2 … 6 m; beyond that they are extrapolated linearly (the report says
  so).

## Rights

`source/` contains the shipyard's documents (Bohai Shipbuilding BH410-020-035A and BH410-020-005).
Confirm you may redistribute them before publishing this repository.
