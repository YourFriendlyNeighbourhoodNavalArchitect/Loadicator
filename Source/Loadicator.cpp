#include "Ship.h"
#include <iostream>

int main() {
    // Paths to data files
    std::string trimStabilityBook = "Data/Trim and stability book.pdf";
    std::string soundingTables = "Data/Sounding tables (1).txt";
    std::string hydrostaticTables = "Data/Hydrostatic tables.pdf";
    std::string userInput;

    // DISCLAIMER: This code performs a first order approximation for the final equilibrium of the ship,
    // assuming the trim is equal to zero.
    // Having found an initial approximation for the trim of the ship, one can go back to SoundingTablesReader.h 
    // and recalculate each compartment's properties with a finer accuracy.
    // For this reason, the code slightly underperforms (regarding the trim and heel angle) for loading conditions
    // for which free surface effects are prominent.
    // Prompt user for loading condition input.
    std::cout << "Enter Loading Condition: ";
    std::getline(std::cin, userInput);

    try {
        Ship myShip(trimStabilityBook, soundingTables, userInput, hydrostaticTables);
        myShip.printResultsToFile();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}