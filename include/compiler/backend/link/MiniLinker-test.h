#pragma once

// Test file to verify COFF types are accessible
#include "compiler/backend/coff/COFFTypes.h"
#include <vector>
#include <string>

// Test if we can access COFF types
using TestCOFFHeader = cpp20::compiler::backend::coff::IMAGE_FILE_HEADER;
using TestCOFFSymbol = cpp20::compiler::backend::coff::COFFSymbol;
using TestSectionHeader = cpp20::compiler::backend::coff::IMAGE_SECTION_HEADER;

namespace cpp20::compiler::backend::link {

// Test function using COFF types
bool testCOFFTypes() {
    TestCOFFHeader header;
    TestCOFFSymbol symbol("test");
    TestSectionHeader section;

    std::vector<TestCOFFSymbol> symbols;
    symbols.push_back(symbol);

    return true;
}

} // namespace cpp20::compiler::backend::link
