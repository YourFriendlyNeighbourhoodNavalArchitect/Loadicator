# Ship Data

This directory contains the data of the ship as plain CSV files, together with the documents of the vessel from which they are extracted. The extraction is done once and its result is kept in the repository. It is repeated only when a document in `source/` changes.

## Data Extraction

The data extractor reads the hydrostatic tables, the sounding tables, the cargo hold tables and the trim and stability book of the vessel. From the hydrostatic tables it takes the length between perpendiculars, the reference point of the tables, the centre of flotation and the vertical centre of buoyancy on even keel, and the displacement, the longitudinal centre of buoyancy and the transverse metacentre over draught and trim. From the sounding tables and the cargo hold tables it takes the capacity table of each compartment at zero trim. From the trim and stability book it takes the 31 loading conditions, with the results printed for each.

Rows of the sounding tables that the text conversion has broken in two, cut short or left without their sounding are restored, and every row is checked against its printed filling. A table that stops short of the full tank is closed with a row at its capacity, and the row of an empty cargo hold takes the centre of the first cargo. Before any file is written, the hydrostatic tables and the capacity tables are loaded with the classes of the program, and every loading condition is resolved against the capacity tables and compared with the total weight printed in the book.

## Running the Extractor

The extractor requires `cmake`, a C++20 compiler and `pdftotext`. The following command builds it, converts the documents to text, writes the CSV files and compares the result with the trim and stability book:

```bash
tools/extractData.sh
```

The comparison lists, for each key metric, the mean deviation from the book over the 31 loading conditions, the largest deviation and the condition in which it occurs. The output files are deterministic, so `git diff` shows exactly what a new extraction has changed.

## Output Files

The program reads every one of these files by the names of its columns, in any order. A column that the file does not define, or that is named twice, is an error. The files may begin with a UTF-8 byte order mark.

- `ship.csv`: Length between perpendiculars and longitudinal position of the reference point of the hydrostatic tables.
- `hydrostatics/evenKeel.csv`: Centre of flotation `lcf` and vertical centre of buoyancy `vcb` at each `draught` on even keel.
- `hydrostatics/trimGrid.csv`: Values of `displacement`, `lcb` and `kmt` at each `draught` and `trim`, for trims from -2 m to 6 m.
- `compartments/<id>.csv`: Capacity table of a compartment, with the centre `lcg`, `tcg`, `vcg` and the free-surface second moment of area `inertia` of its contents at each `volume`. The last row is the full compartment.
- `conditions/condNN.csv`: Loading conditions of the trim and stability book.
- `conditions/expected.csv`: Results printed in the book for each loading condition, namely the total weight and its centre, the draughts, the trim, the heel, the transverse metacentre and the metacentric height without and with free-surface correction.

## Compartments

A compartment is named by its identifier in the documents of the vessel, with the suffix `P` or `S` for the port or the starboard tank of a pair.

- `R1.1` to `R1.9`: Cargo holds No. 1 to No. 9.
- `R2.01` to `R2.09`: Water ballast tanks No. 1 to No. 9.
- `R2.11` to `R2.19`: Double-side water ballast tanks No. 1 to No. 9.
- `R2.21` to `R2.27`: Topside water ballast tanks No. 1 to No. 7.
- `R2.31`, `R2.32` and `R2.33`: Fore peak tank, aft peak tank and double-bottom water ballast tank No. 10.
- `R3.1` to `R3.5`: Heavy fuel oil tanks No. 1 to No. 5.
- `R3.6` to `R3.9`: Heavy fuel oil service and settling tanks.
- `R4.1`, `R4.2` and `R4.3`: Marine diesel oil tank, service tank and settling tank.
- `R6.05` to `R6.16`: Sludge, drain, sump and bilge tanks.

## Units and Sign Conventions

- `lcg`, `lcb`, `lcf`: Metres forward of the aft perpendicular.
- `tcg`: Metres from the centreline, positive to port. The heel is positive to port as well.
- `vcg`, `vcb`, `kg`, `kmt`: Metres above the baseline.
- `draught`: Moulded draught in metres. In the hydrostatic tables it is measured at the reference point, which lies amidships.
- `trim`: Metres, positive by the stern.
- `mass`, `displacement`: Tonnes. The hydrostatics are for sea water of 1.025 t/m³.
- `volume`, `inertia`, `density`, `fsm`: Cubic metres, metres to the fourth power, tonnes per cubic metre and tonne-metres.

## Loading Conditions

A loading condition is a CSV file with one row for each item on board. Its columns are addressed by name, and only `kind`, `id` and the columns that its rows use need to be present. The columns are those of the example below, and a value in a column that a row of its kind does not use is an error.

```csv
kind,id,mass,fillPercent,density,phase,lcg,tcg,vcg,fsm
name,Ballast departure,,,,,,,,
tank,R2.01,,100,1.025,liquid,,,,
tank,R1.3,36695.8,,2.990,solid,,,,
weight,Lightweight,24641.9,,,,132.01,0.00,13.67,0
weight,CREW&ST,200,,,,39.10,0.00,30.00,0
```

- `name`: Name of the loading condition, given in the `id` column. Without this row, the condition is named after its file. A condition has at most one such row.
- `tank`: Contents of a compartment that has a capacity table. The row gives either `mass` or `fillPercent`, together with `density` and `phase`. A `liquid` has a free surface and a `solid` has none; without a `phase`, the contents are taken to be liquid. The centre and the free-surface moment follow from the capacity table. Each compartment is loaded by one row at most.
- `weight`: Any other item, given by `mass`, `lcg`, `tcg`, `vcg` and optionally the free-surface moment `fsm`.
