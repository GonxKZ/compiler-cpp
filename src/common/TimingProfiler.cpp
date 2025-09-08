/**
 * @file TimingProfiler.cpp
 * @brief Implementación del sistema de profiling de tiempos
 */

#include <compiler/common/TimingProfiler.h>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif

namespace cpp20::compiler {

// ============================================================================
// PhaseTiming - Implementación
// ============================================================================

// Ya definido en header

// ============================================================================
// PhaseStats - Implementación
// ============================================================================

// Ya definido en header

// ============================================================================
// AutoTimer - Implementación
// ============================================================================

AutoTimer::AutoTimer(TimingProfiler& profiler, CompilationPhase phase, const std::string& details)
    : profiler_(profiler), phase_(phase), details_(details),
      operations_(0), memoryUsed_(0) {
    startTime_ = std::chrono::steady_clock::now();
    profiler_.startPhase(phase_, details_);
}

AutoTimer::~AutoTimer() {
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime_);

    profiler_.recordPhaseTiming(phase_, duration, memoryUsed_, operations_, details_);
}

void AutoTimer::addDetails(const std::string& details) {
    if (!details_.empty()) {
        details_ += "; ";
    }
    details_ += details;
}

void AutoTimer::incrementOperations(size_t count) {
    operations_ += count;
}

void AutoTimer::recordMemoryUsage(size_t bytes) {
    memoryUsed_ = std::max(memoryUsed_, bytes);
}

// ============================================================================
// TimingProfiler - Implementación
// ============================================================================

TimingProfiler::TimingProfiler() : enabled_(true) {
    initializePhaseNames();
}

TimingProfiler::~TimingProfiler() = default;

void TimingProfiler::initializePhaseNames() {
    phaseNames_ = {
        {CompilationPhase::Lexing, "Lexing"},
        {CompilationPhase::Parsing, "Parsing"},
        {CompilationPhase::Preprocessing, "Preprocessing"},
        {CompilationPhase::SemanticAnalysis, "Semantic Analysis"},
        {CompilationPhase::TemplateInstantiation, "Template Instantiation"},
        {CompilationPhase::ConstexprEvaluation, "Constexpr Evaluation"},
        {CompilationPhase::ConceptChecking, "Concept Checking"},
        {CompilationPhase::IRGeneration, "IR Generation"},
        {CompilationPhase::InstructionSelection, "Instruction Selection"},
        {CompilationPhase::RegisterAllocation, "Register Allocation"},
        {CompilationPhase::CodeOptimization, "Code Optimization"},
        {CompilationPhase::PeepholeOptimization, "Peephole Optimization"},
        {CompilationPhase::ObjectEmission, "Object Emission"},
        {CompilationPhase::UnwindGeneration, "Unwind Generation"},
        {CompilationPhase::ExceptionHandling, "Exception Handling"},
        {CompilationPhase::SymbolResolution, "Symbol Resolution"},
        {CompilationPhase::RelocationProcessing, "Relocation Processing"},
        {CompilationPhase::FinalLinking, "Final Linking"},
        {CompilationPhase::ModuleInterfaceGeneration, "Module Interface Generation"},
        {CompilationPhase::ModuleImportProcessing, "Module Import Processing"},
        {CompilationPhase::HeaderUnitProcessing, "Header Unit Processing"},
        {CompilationPhase::DiagnosticEmission, "Diagnostic Emission"},
        {CompilationPhase::FileIO, "File I/O"},
        {CompilationPhase::MemoryManagement, "Memory Management"},
        {CompilationPhase::TotalCompilation, "Total Compilation"},
        {CompilationPhase::CustomPhase, "Custom Phase"}
    };
}

std::string TimingProfiler::getPhaseName(CompilationPhase phase) const {
    auto it = phaseNames_.find(phase);
    return it != phaseNames_.end() ? it->second : "Unknown Phase";
}

void TimingProfiler::startPhase(CompilationPhase phase, const std::string& details) {
    if (!enabled_) return;

    std::lock_guard<std::mutex> lock(mutex_);
    activeTimers_[phase] = std::chrono::steady_clock::now();
    activeDetails_[phase] = details;
    activeOperations_[phase] = 0;
    activeMemory_[phase] = 0;
}

void TimingProfiler::endPhase(CompilationPhase phase) {
    if (!enabled_) return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = activeTimers_.find(phase);
    if (it != activeTimers_.end()) {
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - it->second);

        recordPhaseTiming(phase, duration,
                         activeMemory_[phase],
                         activeOperations_[phase],
                         activeDetails_[phase]);

        activeTimers_.erase(it);
        activeDetails_.erase(phase);
        activeOperations_.erase(phase);
        activeMemory_.erase(phase);
    }
}

void TimingProfiler::recordPhaseTiming(CompilationPhase phase, std::chrono::microseconds duration,
                                      size_t memoryUsed, size_t operations,
                                      const std::string& details) {
    if (!enabled_) return;

    std::lock_guard<std::mutex> lock(mutex_);

    PhaseTiming timing(phase, getPhaseName(phase));
    timing.duration = duration;
    timing.memoryUsed = memoryUsed;
    timing.operationsCount = operations;
    timing.details = details;
    timing.peakMemory = memoryUsed; // Simplificado

    phaseHistory_[phase].push_back(timing);
    phaseStats_[phase].update(timing);
}

const PhaseTiming* TimingProfiler::getLastPhaseTiming(CompilationPhase phase) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = phaseHistory_.find(phase);
    if (it != phaseHistory_.end() && !it->second.empty()) {
        return &it->second.back();
    }
    return nullptr;
}

const PhaseStats* TimingProfiler::getPhaseStats(CompilationPhase phase) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = phaseStats_.find(phase);
    if (it != phaseStats_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<CompilationPhase> TimingProfiler::getMeasuredPhases() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<CompilationPhase> phases;
    for (const auto& [phase, _] : phaseStats_) {
        phases.push_back(phase);
    }
    return phases;
}

std::string TimingProfiler::generateTimingReport(bool detailed) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::stringstream ss;
    ss << "=== Compilation Timing Report ===\n\n";

    ss << generateSummarySection();

    if (detailed) {
        ss << "\n" << generateDetailedSection();
        ss << "\n" << generateStatsSection();
    }

    return ss.str();
}

std::string TimingProfiler::generateJSONReport() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::stringstream ss;
    ss << "{\n";
    ss << "  \"compilation_report\": {\n";
    ss << "    \"total_time\": \"" << formatDuration(getTotalCompilationTime()) << "\",\n";
    ss << "    \"phases\": [\n";

    auto phases = getMeasuredPhases();
    for (size_t i = 0; i < phases.size(); ++i) {
        const auto& phase = phases[i];
        const auto* stats = getPhaseStats(phase);

        if (stats) {
            ss << "      {\n";
            ss << "        \"name\": \"" << getPhaseName(phase) << "\",\n";
            ss << "        \"total_time\": \"" << formatDuration(stats->totalTime) << "\",\n";
            ss << "        \"avg_time\": \"" << formatDuration(stats->avgTime) << "\",\n";
            ss << "        \"min_time\": \"" << formatDuration(stats->minTime) << "\",\n";
            ss << "        \"max_time\": \"" << formatDuration(stats->maxTime) << "\",\n";
            ss << "        \"call_count\": " << stats->callCount << ",\n";
            ss << "        \"total_memory\": \"" << formatMemorySize(stats->totalMemory) << "\",\n";
            ss << "        \"total_operations\": " << stats->totalOperations << "\n";
            ss << "      }";

            if (i < phases.size() - 1) {
                ss << ",";
            }
            ss << "\n";
        }
    }

    ss << "    ]\n";
    ss << "  }\n";
    ss << "}\n";

    return ss.str();
}

void TimingProfiler::reset() {
    std::lock_guard<std::mutex> lock(mutex_);

    activeTimers_.clear();
    activeDetails_.clear();
    activeOperations_.clear();
    activeMemory_.clear();
    phaseHistory_.clear();
    phaseStats_.clear();
}

std::chrono::microseconds TimingProfiler::getTotalCompilationTime() const {
    const auto* totalStats = getPhaseStats(CompilationPhase::TotalCompilation);
    return totalStats ? totalStats->totalTime : std::chrono::microseconds(0);
}

CompilationPhase TimingProfiler::getSlowestPhase() const {
    std::lock_guard<std::mutex> lock(mutex_);

    CompilationPhase slowest = CompilationPhase::TotalCompilation;
    std::chrono::microseconds maxTime(0);

    for (const auto& [phase, stats] : phaseStats_) {
        if (stats.totalTime > maxTime) {
            maxTime = stats.totalTime;
            slowest = phase;
        }
    }

    return slowest;
}

CompilationPhase TimingProfiler::getMostMemoryIntensivePhase() const {
    std::lock_guard<std::mutex> lock(mutex_);

    CompilationPhase mostMemory = CompilationPhase::TotalCompilation;
    size_t maxMemory = 0;

    for (const auto& [phase, stats] : phaseStats_) {
        if (stats.totalMemory > maxMemory) {
            maxMemory = stats.totalMemory;
            mostMemory = phase;
        }
    }

    return mostMemory;
}

std::string TimingProfiler::formatDuration(std::chrono::microseconds duration) const {
    auto micros = duration.count();

    if (micros < 1000) {
        return std::to_string(micros) + " μs";
    } else if (micros < 1000000) {
        return std::to_string(micros / 1000) + " ms";
    } else {
        double seconds = micros / 1000000.0;
        return std::to_string(seconds) + " s";
    }
}

std::string TimingProfiler::formatMemorySize(size_t bytes) const {
    const char* units[] = {"B", "KB", "MB", "GB"};
    size_t unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024 && unitIndex < 3) {
        size /= 1024;
        unitIndex++;
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
    return ss.str();
}

double TimingProfiler::calculatePercentage(std::chrono::microseconds duration) const {
    auto total = getTotalCompilationTime();
    if (total.count() == 0) return 0.0;

    return (static_cast<double>(duration.count()) / total.count()) * 100.0;
}

std::vector<std::pair<CompilationPhase, std::chrono::microseconds>>
TimingProfiler::getPhasesSortedByTime() const {
    std::vector<std::pair<CompilationPhase, std::chrono::microseconds>> phases;

    for (const auto& [phase, stats] : phaseStats_) {
        phases.emplace_back(phase, stats.totalTime);
    }

    std::sort(phases.begin(), phases.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    return phases;
}

std::vector<std::pair<CompilationPhase, size_t>>
TimingProfiler::getPhasesSortedByMemory() const {
    std::vector<std::pair<CompilationPhase, size_t>> phases;

    for (const auto& [phase, stats] : phaseStats_) {
        phases.emplace_back(phase, stats.totalMemory);
    }

    std::sort(phases.begin(), phases.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    return phases;
}

std::string TimingProfiler::generateSummarySection() const {
    std::stringstream ss;

    ss << "Compilation Summary:\n";
    ss << "==================\n";
    ss << "Total time: " << formatDuration(getTotalCompilationTime()) << "\n";
    ss << "Slowest phase: " << getPhaseName(getSlowestPhase()) << "\n";
    ss << "Most memory-intensive: " << getPhaseName(getMostMemoryIntensivePhase()) << "\n\n";

    ss << "Phase Breakdown (sorted by time):\n";
    ss << "---------------------------------\n";

    auto sortedPhases = getPhasesSortedByTime();
    for (const auto& [phase, duration] : sortedPhases) {
        if (duration.count() > 0) {
            ss << std::left << std::setw(25) << getPhaseName(phase)
               << std::right << std::setw(12) << formatDuration(duration)
               << " (" << std::fixed << std::setprecision(1)
               << calculatePercentage(duration) << "%)\n";
        }
    }

    return ss.str();
}

std::string TimingProfiler::generateDetailedSection() const {
    std::stringstream ss;

    ss << "Detailed Phase Information:\n";
    ss << "==========================\n";

    for (const auto& [phase, history] : phaseHistory_) {
        if (history.empty()) continue;

        ss << "\n" << getPhaseName(phase) << ":\n";

        for (size_t i = 0; i < history.size(); ++i) {
            const auto& timing = history[i];
            ss << "  Run " << (i + 1) << ": "
               << formatDuration(timing.duration);

            if (timing.memoryUsed > 0) {
                ss << ", " << formatMemorySize(timing.memoryUsed) << " memory";
            }

            if (timing.operationsCount > 0) {
                ss << ", " << timing.operationsCount << " operations";
            }

            if (!timing.details.empty()) {
                ss << " (" << timing.details << ")";
            }

            ss << "\n";
        }
    }

    return ss.str();
}

std::string TimingProfiler::generateStatsSection() const {
    std::stringstream ss;

    ss << "Phase Statistics:\n";
    ss << "================\n";

    for (const auto& [phase, stats] : phaseStats_) {
        if (stats.callCount == 0) continue;

        ss << "\n" << getPhaseName(phase) << ":\n";
        ss << "  Calls: " << stats.callCount << "\n";
        ss << "  Total: " << formatDuration(stats.totalTime) << "\n";
        ss << "  Average: " << formatDuration(stats.avgTime) << "\n";
        ss << "  Min: " << formatDuration(stats.minTime) << "\n";
        ss << "  Max: " << formatDuration(stats.maxTime) << "\n";

        if (stats.totalMemory > 0) {
            ss << "  Memory: " << formatMemorySize(stats.totalMemory) << "\n";
        }

        if (stats.totalOperations > 0) {
            ss << "  Operations: " << stats.totalOperations << "\n";
        }
    }

    return ss.str();
}

// ============================================================================
// MemoryProfiler - Implementación
// ============================================================================

size_t MemoryProfiler::lastCheckpoint_ = 0;

size_t MemoryProfiler::getCurrentMemoryUsage() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
#endif
    return 0; // Placeholder para otros sistemas
}

size_t MemoryProfiler::getPeakMemoryUsage() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.PeakWorkingSetSize;
    }
#endif
    return 0; // Placeholder para otros sistemas
}

void MemoryProfiler::recordMemoryCheckpoint(const std::string& label) {
    lastCheckpoint_ = getCurrentMemoryUsage();
    // En un compilador real, aquí se guardaría el label para debugging
}

size_t MemoryProfiler::getMemoryDelta() {
    size_t current = getCurrentMemoryUsage();
    return current > lastCheckpoint_ ? current - lastCheckpoint_ : 0;
}

// ============================================================================
// CompilationTelemetry - Implementación
// ============================================================================

CompilationTelemetry::CompilationTelemetry(TimingProfiler& profiler)
    : profiler_(profiler) {
}

void CompilationTelemetry::recordCompilationEvent(const std::string& eventType,
                                                const std::string& details) {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.emplace_back(eventType, details);
}

void CompilationTelemetry::recordMetric(const std::string& name, double value) {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_[name].push_back(value);
}

void CompilationTelemetry::recordCompilationError(const std::string& errorType,
                                                const std::string& file,
                                                size_t line) {
    std::lock_guard<std::mutex> lock(mutex_);
    errors_.emplace_back(errorType, file, line);
}

std::string CompilationTelemetry::generateTelemetryReport() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::stringstream ss;
    ss << "=== Compilation Telemetry Report ===\n\n";

    ss << "Events:\n";
    for (const auto& [type, details] : events_) {
        ss << "  " << type;
        if (!details.empty()) {
            ss << ": " << details;
        }
        ss << "\n";
    }

    ss << "\nMetrics:\n";
    for (const auto& [name, values] : metrics_) {
        if (!values.empty()) {
            double sum = std::accumulate(values.begin(), values.end(), 0.0);
            double avg = sum / values.size();
            double min_val = *std::min_element(values.begin(), values.end());
            double max_val = *std::max_element(values.begin(), values.end());

            ss << "  " << name << ": "
               << "avg=" << avg << ", "
               << "min=" << min_val << ", "
               << "max=" << max_val << ", "
               << "count=" << values.size() << "\n";
        }
    }

    if (!errors_.empty()) {
        ss << "\nErrors:\n";
        for (const auto& [type, file, line] : errors_) {
            ss << "  " << type << " in " << file << ":" << line << "\n";
        }
    }

    return ss.str();
}

std::string CompilationTelemetry::exportToJSON() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::stringstream ss;
    ss << "{\n";
    ss << "  \"telemetry\": {\n";

    // Events
    ss << "    \"events\": [\n";
    for (size_t i = 0; i < events_.size(); ++i) {
        const auto& [type, details] = events_[i];
        ss << "      {\"type\": \"" << type << "\", \"details\": \"" << details << "\"}";
        if (i < events_.size() - 1) ss << ",";
        ss << "\n";
    }
    ss << "    ],\n";

    // Metrics
    ss << "    \"metrics\": {\n";
    size_t metricCount = 0;
    for (const auto& [name, values] : metrics_) {
        if (!values.empty()) {
            ss << "      \"" << name << "\": [";
            for (size_t i = 0; i < values.size(); ++i) {
                ss << values[i];
                if (i < values.size() - 1) ss << ",";
            }
            ss << "]";

            if (++metricCount < metrics_.size()) ss << ",";
            ss << "\n";
        }
    }
    ss << "    },\n";

    // Errors
    ss << "    \"errors\": [\n";
    for (size_t i = 0; i < errors_.size(); ++i) {
        const auto& [type, file, line] = errors_[i];
        ss << "      {\"type\": \"" << type << "\", \"file\": \"" << file << "\", \"line\": " << line << "}";
        if (i < errors_.size() - 1) ss << ",";
        ss << "\n";
    }
    ss << "    ]\n";

    ss << "  }\n";
    ss << "}\n";

    return ss.str();
}

} // namespace cpp20::compiler
