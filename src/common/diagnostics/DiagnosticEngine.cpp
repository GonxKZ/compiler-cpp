/**
 * @file DiagnosticEngine.cpp
 * @brief Implementación del DiagnosticEngine
 */

#include <compiler/common/diagnostics/DiagnosticEngine.h>
#include <iostream>
#include <format>

namespace cpp20::compiler::diagnostics {

// DiagnosticEngine implementation
DiagnosticEngine::DiagnosticEngine(std::shared_ptr<SourceManager> sourceManager)
    : sourceManager_(std::move(sourceManager)) {
    // Add default stream consumer to stderr
    addConsumer(std::make_unique<StreamConsumer>(std::cerr, true));
}

DiagnosticEngine::~DiagnosticEngine() = default;

void DiagnosticEngine::addConsumer(std::unique_ptr<Consumer> consumer) {
    std::lock_guard<std::mutex> lock(mutex_);
    consumers_.push_back(std::move(consumer));
}

void DiagnosticEngine::clearConsumers() {
    std::lock_guard<std::mutex> lock(mutex_);
    consumers_.clear();
}

void DiagnosticEngine::emit(const Diagnostic& diagnostic) {
    std::lock_guard<std::mutex> lock(mutex_);
    processDiagnostic(diagnostic);
}

void DiagnosticEngine::emit(DiagnosticLevel level, DiagnosticCode code,
                           SourceLocation location, std::string message) {
    Diagnostic diagnostic(level, code, location, std::move(message));
    emit(diagnostic);
}

void DiagnosticEngine::reportError(DiagnosticCode code, SourceLocation loc, std::string msg) {
    emit(DiagnosticLevel::Error, code, loc, std::move(msg));
}

void DiagnosticEngine::reportWarning(DiagnosticCode code, SourceLocation loc, std::string msg) {
    emit(DiagnosticLevel::Warning, code, loc, std::move(msg));
}

void DiagnosticEngine::reportNote(DiagnosticCode code, SourceLocation loc, std::string msg) {
    emit(DiagnosticLevel::Note, code, loc, std::move(msg));
}

void DiagnosticEngine::reportFatal(DiagnosticCode code, SourceLocation loc, std::string msg) {
    emit(DiagnosticLevel::Fatal, code, loc, std::move(msg));
}

void DiagnosticEngine::clearDiagnostics() {
    std::lock_guard<std::mutex> lock(mutex_);
    diagnostics_.clear();
    errorCount_ = 0;
    warningCount_ = 0;
    noteCount_ = 0;
    fatalCount_ = 0;
}

std::string DiagnosticEngine::formatDiagnostic(const Diagnostic& diagnostic) const {
    std::string result;

    // Add level prefix
    switch (diagnostic.level()) {
        case DiagnosticLevel::Error:
        case DiagnosticLevel::Fatal:
            result += "error: ";
            break;
        case DiagnosticLevel::Warning:
            result += "warning: ";
            break;
        case DiagnosticLevel::Note:
            result += "note: ";
            break;
    }

    // Add code
    result += std::format("[{}] ", static_cast<int>(diagnostic.code()));

    // Add message
    result += diagnostic.message();

    // Add location if valid
    if (diagnostic.location().isValid()) {
        const SourceFile* file = sourceManager_->getFileForLocation(diagnostic.location());
        if (file) {
            result += std::format("\n  --> {}:{}", file->displayName, diagnostic.location().toString());
        }
    }

    return result;
}

std::string DiagnosticEngine::formatSourceLine(SourceLocation location, int contextLines) const {
    if (!location.isValid()) {
        return "";
    }

    return sourceManager_->getContextLines(location, contextLines, contextLines);
}

void DiagnosticEngine::processDiagnostic(const Diagnostic& diagnostic) {
    // Check if we should emit this diagnostic
    if (!shouldEmit(diagnostic)) {
        return;
    }

    // Add to diagnostics list
    diagnostics_.push_back(diagnostic);

    // Update statistics
    updateStatistics(diagnostic);

    // Emit to all consumers
    emitToConsumers(diagnostic);

    // Handle fatal errors
    if (diagnostic.level() == DiagnosticLevel::Fatal) {
        // TODO: Implement proper fatal error handling
    }
}

void DiagnosticEngine::emitToConsumers(const Diagnostic& diagnostic) {
    for (const auto& consumer : consumers_) {
        if (consumer) {
            consumer->handleDiagnostic(diagnostic);
        }
    }
}

void DiagnosticEngine::updateStatistics(const Diagnostic& diagnostic) {
    switch (diagnostic.level()) {
        case DiagnosticLevel::Error:
            ++errorCount_;
            break;
        case DiagnosticLevel::Warning:
            ++warningCount_;
            break;
        case DiagnosticLevel::Note:
            ++noteCount_;
            break;
        case DiagnosticLevel::Fatal:
            ++fatalCount_;
            ++errorCount_;
            break;
    }
}

bool DiagnosticEngine::shouldEmit(const Diagnostic& diagnostic) const {
    // Check warning level
    if (diagnostic.level() == DiagnosticLevel::Warning && !options_.showWarnings) {
        return false;
    }

    // Check note level
    if (diagnostic.level() == DiagnosticLevel::Note && !options_.showNotes) {
        return false;
    }

    // Check error limit
    if (diagnostic.isError() && errorCount_ >= static_cast<size_t>(options_.maxErrors)) {
        return false;
    }

    return true;
}

// StreamConsumer implementation
StreamConsumer::StreamConsumer(std::ostream& stream, bool useColors)
    : stream_(stream), useColors_(useColors) {
}

bool StreamConsumer::handleDiagnostic(const Diagnostic& diagnostic) {
    // Format the diagnostic
    std::string formatted = formatDiagnostic(diagnostic);

    // Apply colors if enabled
    if (useColors_) {
        switch (diagnostic.level()) {
            case DiagnosticLevel::Error:
            case DiagnosticLevel::Fatal:
                formatted = formatWithColor(formatted, "red");
                break;
            case DiagnosticLevel::Warning:
                formatted = formatWithColor(formatted, "yellow");
                break;
            case DiagnosticLevel::Note:
                formatted = formatWithColor(formatted, "cyan");
                break;
        }
    }

    stream_ << formatted << std::endl;
    return true;
}

void StreamConsumer::finish() {
    // Nothing to do for stream consumer
}

std::string StreamConsumer::formatWithColor(const std::string& text,
                                           const std::string& color) const {
    // Basic ANSI color codes (simplified)
    std::string colorCode;
    if (color == "red") colorCode = "\033[31m";
    else if (color == "yellow") colorCode = "\033[33m";
    else if (color == "cyan") colorCode = "\033[36m";
    else return text;

    return colorCode + text + "\033[0m";
}

// MemoryConsumer implementation
bool MemoryConsumer::handleDiagnostic(const Diagnostic& diagnostic) {
    diagnostics_.push_back(diagnostic);
    return true;
}

} // namespace cpp20::compiler::diagnostics
