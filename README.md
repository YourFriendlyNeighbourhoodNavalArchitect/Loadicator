# Loadicator

This repository contains a loading computer for a 174,000 DWT double-skin bulk carrier, written in C++. It computes the floating equilibrium and initial stability of the ship for any loading condition: the displacement and centre of gravity, the draughts at the perpendiculars, the trim, the metacentric height with free-surface correction, and the static heel.

## Project Overview

A loading condition lists the contents of the ship's compartments, by mass or by percentage filling together with the cargo density, and any further weights such as the lightship and the stores. Each compartment is looked up in its capacity table, which gives the centroid and the free-surface moment of its contents. Summing all weights yields the displacement, the centre of gravity and the total free-surface moment.

The draught and the trim are then solved together on the hydrostatic tables of the ship, interpolated with natural cubic splines over draught and trim, until the centre of buoyancy lies on the same vertical as the centre of gravity in the trimmed attitude. The tables are never extrapolated: the equilibrium is sought only at the trims where the displacement lies within the tabulated draughts, and a condition with no equilibrium there is rejected. The transverse metacentric height is corrected for free surfaces, and the static heel follows from the transverse centre of gravity.

The ship data are taken from the trim and stability book, the hydrostatic tables, the sounding tables and the cargo hold tables of the vessel, and converted once into CSV files. During the conversion the damaged sounding tables are repaired, and every table is validated. Results are verified against the 31 loading conditions of the trim and stability book.

## Repository Structure

- `CMakeLists.txt`: Build configuration of the program, the comparison with the book and, on request, the data extractor. The library is built as one target per layer, and each layer has its own include directory, so that a source can only include the headers of the layers its target links.
- `app/main.cpp`: Command-line program, which reads the ship data and a loading condition, and writes the report.
- `app/commandLine.cpp`: Command-line options.
- `src/<layer>/`: The library, one directory per layer, with its headers under `include/loadicator/<layer>/`. Each layer depends only on the layers listed before it.
  - `util/`: Dimension-checked physical quantities (mass, length, volume, density and moment) and angles, the error type and text parsing.
  - `numeric/`: Bracketing interpolation, natural cubic splines over one and two variables, and root finding.
  - `domain/`: Ship particulars, hydrostatic tables, compartment capacity tables and loading conditions.
  - `calc/`: Resolution of a loading condition into weights, weight summary, equilibrium solver and initial stability.
  - `io/`: CSV reading, the names of the data files and their columns, and loaders for the ship data and the loading conditions. Depends on `domain/`.
  - `report/`: The plain-text report of an analysis. Depends on `calc/`.
  - `book/`: The columns of the results printed in the trim and stability book, used only by the data extractor and the comparison with the book.
- `tools/dataExtractor/`: One-off conversion of the ship's documents into the CSV data, with repair and verification of the tables.
- `tools/extractData.sh`: Extracts the text of the documents, runs the data extractor and compares the result with the book.
- `tests/bookComparison.cpp`: Comparison with the 31 loading conditions of the trim and stability book. For each key metric it reports the mean absolute deviation, the bias (the mean signed deviation), the worst deviation and its condition, and lists as outliers the conditions that deviate by more than three times the limit of the metric. The test fails when the mean absolute deviation of a metric exceeds its limit, or when a condition cannot be compared, such as an unstable one, which has no heel; the outliers are for information.
- `data/ship.csv`: Length between perpendiculars and the reference point of the hydrostatic tables.
- `data/hydrostatics/`: Centre of flotation and vertical centre of buoyancy on even keel, and the hydrostatics over draught and trim.
- `data/compartments/`: Capacity tables of the 76 tanks and the 9 cargo holds.
- `data/conditions/`: The 31 loading conditions of the trim and stability book, and its results for each.
- `data/source/`: Trim and stability book, hydrostatic tables, sounding tables and cargo hold tables of the vessel.
- `data/README.md`: The data extraction, how to run it and the format of its output.
