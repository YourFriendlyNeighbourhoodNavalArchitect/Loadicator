#!/usr/bin/env bash
set -euo pipefail

repository="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="${1:-$repository/build}"
source="$repository/data/source"

cmake -S "$repository" -B "$build" >/dev/null
cmake --build "$build" --target dataExtractor bookComparison

pdfText="$(mktemp -d)"
trap 'rm -rf "$pdfText"' EXIT

pdftotext -raw "$source/trimAndStabilityBook.pdf" "$pdfText/trimAndStabilityBook.txt" 2>/dev/null
pdftotext -raw "$source/hydrostaticTables.pdf" "$pdfText/hydrostaticTables.txt" 2>/dev/null

"$build/dataExtractor" "$source" "$pdfText" "$repository/data"
"$build/bookComparison" "$repository/data"
