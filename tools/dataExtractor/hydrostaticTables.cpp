#include "hydrostaticTables.h"

#include "loadicator/core/error.h"
#include "loadicator/io/csv.h"
#include "loadicator/text/numberParsing.h"

#include <array>
#include <fstream>
#include <map>
#include <optional>
#include <utility>

namespace loadicator::extractor {

namespace {

enum class Property { draught, displacement, lcf, lcb, vcb, kmt, kml, mctOneCm, tpc };
enum class TrimSection { displacement, lcb, kmt };

struct PropertyLabel {
    std::string_view prefix;
    Property property;
};

/// Row labels of the even-keel block, after the leading row number has been removed.
constexpr std::array propertyLabels{
    PropertyLabel{"DRAUGHT RFP", Property::draught},
    PropertyLabel{"DISP TOTAL SW", Property::displacement},
    PropertyLabel{"LCF FWD OF RFP", Property::lcf},
    PropertyLabel{"LCB FWD OF RFP", Property::lcb},
    PropertyLabel{"VCB ABOVE RFP", Property::vcb},
    PropertyLabel{"KMT", Property::kmt},
    PropertyLabel{"KML", Property::kml},
    PropertyLabel{"MCT SW", Property::mctOneCm},
    PropertyLabel{"TPM SW", Property::tpc},
};

[[nodiscard]] std::optional<double> firstNumberAfterColon(std::string_view text) {
    const std::size_t colon = text.find(':');
    if (colon == std::string_view::npos) {
        return std::nullopt;
    }
    const auto numbers = text::parseNumbers(text.substr(colon + 1));
    return numbers.empty() ? std::nullopt : std::optional<double>(numbers.front());
}

/// One printed page: up to five draughts, each property as a row across them.
class PageBlock {
public:
    void setProperty(Property property, std::vector<double> values) {
        properties[property] = std::move(values);
    }
    void setTrimRow(TrimSection section, double trim, std::vector<double> values) {
        trimRows[{section, trim}] = std::move(values);
    }
    [[nodiscard]] bool empty() const { return properties.empty(); }

    void appendTo(HydrostaticData& data, double referencePointX, const std::string& context) const {
        const auto draughts = valuesOf(Property::draught, context);
        const auto column = [&](Property property) {
            return valuesOf(property, context);
        };
        const auto displacement = column(Property::displacement);
        const auto lcf = column(Property::lcf);
        const auto lcb = column(Property::lcb);
        const auto vcb = column(Property::vcb);
        const auto kmt = column(Property::kmt);
        const auto kml = column(Property::kml);
        const auto mct = column(Property::mctOneCm);
        const auto tpc = column(Property::tpc);

        for (std::size_t index = 0; index < draughts.size(); ++index) {
            data.evenKeel.push_back(EvenKeelRow{.draught = draughts[index],
                                                .displacement = displacement[index],
                                                .lcf = referencePointX + lcf[index],
                                                .lcb = referencePointX + lcb[index],
                                                .vcb = vcb[index],
                                                .kmt = kmt[index],
                                                .kml = kml[index],
                                                .mctOneCm = mct[index],
                                                .tpc = tpc[index]});
        }

        std::map<double, std::array<std::vector<double>, 3>> byTrim;
        for (const auto& [key, values] : trimRows) {
            if (values.size() != draughts.size()) {
                throw LoadicatorError(context + ": trim row has " + std::to_string(values.size()) +
                                      " values for " + std::to_string(draughts.size()) + " draughts");
            }
            byTrim[key.second][static_cast<std::size_t>(key.first)] = values;
        }
        for (const auto& [trim, sections] : byTrim) {
            for (const auto& section : sections) {
                if (section.empty()) {
                    throw LoadicatorError(context + ": incomplete trim table at trim " +
                                          std::to_string(trim));
                }
            }
            for (std::size_t index = 0; index < draughts.size(); ++index) {
                data.trimGrid.push_back(TrimGridPoint{.draught = draughts[index],
                                                      .trim = trim,
                                                      .displacement = sections[0][index],
                                                      .lcb = referencePointX + sections[1][index],
                                                      .kmt = sections[2][index]});
            }
        }
    }

private:
    [[nodiscard]] std::vector<double> valuesOf(Property property, const std::string& context) const {
        const auto found = properties.find(property);
        if (found == properties.end()) {
            throw LoadicatorError(context + ": page is missing a hydrostatic property");
        }
        if (found->second.size() != properties.at(Property::draught).size()) {
            throw LoadicatorError(context + ": property rows have different lengths");
        }
        return found->second;
    }

    std::map<Property, std::vector<double>> properties;
    std::map<std::pair<TrimSection, double>, std::vector<double>> trimRows;
};

[[nodiscard]] std::optional<Property> propertyFor(std::string_view label) {
    for (const auto& entry : propertyLabels) {
        if (label.starts_with(entry.prefix)) {
            return entry.property;
        }
    }
    return std::nullopt;
}

/// "12 KMT ...............[m]:" ⇒ "KMT ...............[m]:"; nullopt for any other label.
[[nodiscard]] std::optional<std::string_view> evenKeelRowName(std::string_view label) {
    const auto tokens = tokenize(label);
    if (tokens.size() < 2 || label.find("]:") == std::string_view::npos ||
        tokens[0].find_first_not_of("0123456789") != std::string_view::npos ||
        tokens[1].find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") != 0) {
        return std::nullopt;
    }
    return label.substr(static_cast<std::size_t>(tokens[1].data() - label.data()));
}

[[nodiscard]] std::optional<TrimSection> trimSectionFor(std::string_view label) {
    if (label.starts_with("DISP TOTAL SW [t]")) {
        return TrimSection::displacement;
    }
    if (label.starts_with("LCB FWD OF RFP [m]")) {
        return TrimSection::lcb;
    }
    if (label.starts_with("KMT [m]")) {
        return TrimSection::kmt;
    }
    return std::nullopt;
}

[[nodiscard]] std::vector<double> rowValues(const std::vector<std::string_view>& fields,
                                            const SourceText& source, const SourceLine& line) {
    std::vector<double> values;
    for (std::size_t index = 2; index < fields.size(); ++index) {
        const std::string_view field = text::trim(fields[index]);
        if (field.empty()) {
            continue;
        }
        const auto value = text::parseNumber(field);
        if (!value) {
            throw LoadicatorError(source.where(line, "not a number: '" + std::string(field) + "'"));
        }
        values.push_back(*value);
    }
    return values;
}

void writeShip(const ShipParticularsData& ship, const std::filesystem::path& path) {
    std::ofstream file(path, std::ios::binary);
    io::CsvWriter writer(file);
    writer.metadata("source", "data/source/hydrostaticTables.pdf (principal data, references)");
    writer.header({"parameter", "value", "unit"});
    const auto row = [&](std::string name, double value) {
        const std::vector<std::string> fields{std::move(name), io::formatNumber(value, 3), "m"};
        writer.row(fields);
    };
    row("lengthBetweenPerpendiculars", ship.lengthBetweenPerpendiculars);
    row("breadthMoulded", ship.breadthMoulded);
    row("depthMoulded", ship.depthMoulded);
    row("designDraught", ship.designDraught);
    row("referencePointX", ship.referencePointX);
}

} // namespace

HydrostaticData extractHydrostatics(const SourceText& source) {
    HydrostaticData data;
    bool inReferencePointBlock = false;
    std::optional<double> referencePointX;
    std::vector<PageBlock> pages(1);
    std::optional<TrimSection> trimSection;

    for (const SourceLine& line : source.lines()) {
        const std::string_view text = text::trim(line.text);

        if (text.starts_with("LENGTH BETWEEN PERPENDICULARS")) {
            data.ship.lengthBetweenPerpendiculars = firstNumberAfterColon(text).value_or(0.0);
        } else if (text.starts_with("BREADTH MOULDED")) {
            data.ship.breadthMoulded = firstNumberAfterColon(text).value_or(0.0);
        } else if (text.starts_with("DEPTH MOULDED")) {
            data.ship.depthMoulded = firstNumberAfterColon(text).value_or(0.0);
        } else if (text.starts_with("DESIGN DRAUGHT")) {
            data.ship.designDraught = firstNumberAfterColon(text).value_or(0.0);
        }
        if (text.find("OUTPUT REFERENCE POINT (RFP)") != std::string_view::npos) {
            inReferencePointBlock = true;
        } else if (inReferencePointBlock && text.find("LONGITUDINAL..:") != std::string_view::npos) {
            referencePointX = firstNumberAfterColon(text);
            inReferencePointBlock = false;
        }

        if (!text.starts_with('|')) {
            continue;
        }
        const auto fields = text::split(text, '|');
        if (fields.size() < 3) {
            continue;
        }
        const std::string_view label = text::trim(fields[1]);
        if (label.empty()) {
            continue;
        }

        if (const auto name = evenKeelRowName(label)) {
            const auto property = propertyFor(*name);
            if (!property) {
                continue;
            }
            if (*property == Property::draught) {
                pages.emplace_back();
                trimSection.reset();
            }
            pages.back().setProperty(*property, rowValues(fields, source, line));
        } else if (const auto section = trimSectionFor(label)) {
            trimSection = section;
        } else if (trimSection && label.find("[m]:") != std::string_view::npos) {
            const auto trims = text::parseNumbers(label);
            if (trims.size() != 1) {
                throw LoadicatorError(source.where(line, "cannot read the trim of this row"));
            }
            pages.back().setTrimRow(*trimSection, trims.front(), rowValues(fields, source, line));
        }
    }

    if (!referencePointX || data.ship.lengthBetweenPerpendiculars <= 0.0) {
        throw LoadicatorError(source.name() + ": principal data or RFP definition not found");
    }
    data.ship.referencePointX = *referencePointX;
    for (const PageBlock& page : pages) {
        if (!page.empty()) {
            page.appendTo(data, *referencePointX, source.name());
        }
    }
    if (data.evenKeel.size() < 2) {
        throw LoadicatorError(source.name() + ": no hydrostatic rows found");
    }
    return data;
}

void writeHydrostatics(const HydrostaticData& data, const std::filesystem::path& outputDirectory) {
    writeShip(data.ship, outputDirectory / "ship.csv");

    const std::filesystem::path directory = outputDirectory / "hydrostatics";
    std::filesystem::create_directories(directory);
    {
        std::ofstream file(directory / "evenKeel.csv", std::ios::binary);
        io::CsvWriter writer(file);
        writer.metadata("source", "data/source/hydrostaticTables.pdf (heel 0, trim 0)");
        writer.comment("draught [m at RFP], displacement [t, sea water 1.025], lcf/lcb [m from AP], "
                       "vcb/kmt/kml [m above BL], mctOneCm [t*m/cm], tpc [t/cm]");
        writer.header({"draught", "displacement", "lcf", "lcb", "vcb", "kmt", "kml", "mctOneCm", "tpc"});
        for (const EvenKeelRow& row : data.evenKeel) {
            const std::vector<std::string> fields{
                io::formatNumber(row.draught, 3), io::formatNumber(row.displacement, 1),
                io::formatNumber(row.lcf, 3),     io::formatNumber(row.lcb, 3),
                io::formatNumber(row.vcb, 3),     io::formatNumber(row.kmt, 3),
                io::formatNumber(row.kml, 2),     io::formatNumber(row.mctOneCm, 2),
                io::formatNumber(row.tpc, 3)};
            writer.row(fields);
        }
    }
    {
        std::ofstream file(directory / "trimGrid.csv", std::ios::binary);
        io::CsvWriter writer(file);
        writer.metadata("source", "data/source/hydrostaticTables.pdf (cross tables, heel 0)");
        writer.comment("draught [m at RFP], trim [m, + by the stern], displacement [t, sea water 1.025], "
                       "lcb [m from AP], kmt [m above BL]");
        writer.header({"draught", "trim", "displacement", "lcb", "kmt"});
        for (const TrimGridPoint& point : data.trimGrid) {
            const std::vector<std::string> fields{
                io::formatNumber(point.draught, 3), io::formatNumber(point.trim, 3),
                io::formatNumber(point.displacement, 1), io::formatNumber(point.lcb, 3),
                io::formatNumber(point.kmt, 3)};
            writer.row(fields);
        }
    }
}

} // namespace loadicator::extractor
