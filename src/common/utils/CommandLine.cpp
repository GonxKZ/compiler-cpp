/**
 * @file CommandLine.cpp
 * @brief Parsing de argumentos de línea de comandos
 */

#include <compiler/common/utils/CommandLine.h>
#include <compiler/common/utils/StringUtils.h>
#include <algorithm>
#include <iostream>

namespace cpp20::compiler::common::utils {

// ========================================================================
// CommandLineParser implementation
// ========================================================================

CommandLineParser::CommandLineParser(const std::string& programName)
    : programName_(programName) {
}

void CommandLineParser::addOption(const std::string& name, const std::string& description,
                                OptionType type, bool required) {
    Option option{name, description, type, required, {}};
    options_[name] = option;
}

void CommandLineParser::addFlag(const std::string& name, const std::string& description) {
    Option option{name, description, OptionType::Flag, false, {}};
    options_[name] = option;
}

bool CommandLineParser::parse(int argc, char* argv[]) {
    parsedOptions_.clear();
    positionalArgs_.clear();

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (startsWith(arg, "--")) {
            // Long option
            if (!parseLongOption(arg, argc, argv, i)) {
                return false;
            }
        } else if (startsWith(arg, "-")) {
            // Short option
            if (!parseShortOption(arg, argc, argv, i)) {
                return false;
            }
        } else {
            // Positional argument
            positionalArgs_.push_back(arg);
        }
    }

    // Check required options
    for (const auto& [name, option] : options_) {
        if (option.required && parsedOptions_.find(name) == parsedOptions_.end()) {
            std::cerr << "Error: Required option '" << name << "' not provided\n";
            return false;
        }
    }

    return true;
}

bool CommandLineParser::hasOption(const std::string& name) const {
    return parsedOptions_.find(name) != parsedOptions_.end();
}

std::string CommandLineParser::getOptionValue(const std::string& name,
                                           const std::string& defaultValue) const {
    auto it = parsedOptions_.find(name);
    return (it != parsedOptions_.end()) ? it->second : defaultValue;
}

std::vector<std::string> CommandLineParser::getPositionalArgs() const {
    return positionalArgs_;
}

void CommandLineParser::printHelp() const {
    std::cout << "Usage: " << programName_ << " [options] [files...]\n\n";
    std::cout << "Options:\n";

    for (const auto& [name, option] : options_) {
        std::cout << "  " << name << "\t" << option.description << "\n";
    }

    std::cout << "\n";
}

bool CommandLineParser::parseLongOption(const std::string& arg, int argc, char* argv[], int& index) {
    size_t equalsPos = arg.find('=');
    std::string optionName;
    std::string value;

    if (equalsPos != std::string::npos) {
        optionName = arg.substr(2, equalsPos - 2);
        value = arg.substr(equalsPos + 1);
    } else {
        optionName = arg.substr(2);
    }

    auto it = options_.find(optionName);
    if (it == options_.end()) {
        std::cerr << "Error: Unknown option '" << optionName << "'\n";
        return false;
    }

    const Option& option = it->second;

    if (option.type == OptionType::Flag) {
        parsedOptions_[optionName] = "true";
    } else {
        if (value.empty()) {
            if (index + 1 >= argc) {
                std::cerr << "Error: Option '" << optionName << "' requires a value\n";
                return false;
            }
            value = argv[++index];
        }
        parsedOptions_[optionName] = value;
    }

    return true;
}

bool CommandLineParser::parseShortOption(const std::string& arg, int argc, char* argv[], int& index) {
    std::string optionName = arg.substr(1);

    // Handle combined short options (e.g., -abc)
    if (optionName.length() > 1) {
        for (char c : optionName) {
            std::string singleOption(1, c);
            if (options_.find(singleOption) == options_.end()) {
                std::cerr << "Error: Unknown option '-" << c << "'\n";
                return false;
            }
            parsedOptions_[singleOption] = "true";
        }
        return true;
    }

    auto it = options_.find(optionName);
    if (it == options_.end()) {
        std::cerr << "Error: Unknown option '-" << optionName << "'\n";
        return false;
    }

    const Option& option = it->second;

    if (option.type == OptionType::Flag) {
        parsedOptions_[optionName] = "true";
    } else {
        if (index + 1 >= argc) {
            std::cerr << "Error: Option '-" << optionName << "' requires a value\n";
            return false;
        }
        parsedOptions_[optionName] = argv[++index];
    }

    return true;
}

} // namespace cpp20::compiler::common::utils
