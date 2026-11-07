#!/usr/bin/env bash
# Regenerates the CSV files under data/ from the original documents in data/source/.
# Requires pdftotext (poppler-utils) and a built dataExtractor.
#
# Usage: tools/extractData.sh [path/to/dataExtractor]
set -euo pipefail

repository="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
extractor="${1:-$repository/build/tools/dataExtractor/dataExtractor}"

if ! command -v pdftotext >/dev/null; then
    echo "pdftotext not found (install poppler-utils)" >&2
    exit 1
fi
if [[ ! -x "$extractor" ]]; then
    echo "dataExtractor not found at $extractor (build the project first)" >&2
    exit 1
fi

workDirectory="$(mktemp -d)"
trap 'rm -rf "$workDirectory"' EXIT

# pdftotext reports harmless font warnings for these documents; keep stderr quiet.
pdftotext -raw "$repository/data/source/trimAndStabilityBook.pdf" "$workDirectory/trimAndStabilityBook.txt" 2>/dev/null
pdftotext -raw "$repository/data/source/hydrostaticTables.pdf" "$workDirectory/hydrostaticTables.txt" 2>/dev/null

"$extractor" "$repository/data/source" "$workDirectory" "$repository/data"
