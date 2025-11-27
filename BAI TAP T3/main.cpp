#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

// -----------------------------------------------------------------------------
// 1. Tracing example
// -----------------------------------------------------------------------------
struct TraceEvent {
    std::string region;
    std::string action;
    long long timestamp;
};

class Tracer {
public:
    explicit Tracer(std::string region)
        : region_(std::move(region)) {
        log("enter");
    }

    ~Tracer() { log("exit"); }

    void log(const std::string& action) {
        TraceEvent evt{region_, action, now()};
        std::ofstream("trace.log", std::ios::app)
            << evt.region << ' ' << evt.action << ' ' << evt.timestamp << '\n';
    }

private:
    static long long now() {
        using namespace std::chrono;
        return duration_cast<microseconds>(
                   steady_clock::now().time_since_epoch())
            .count();
    }

    std::string region_;
};

int compute(int n) {
    Tracer scope("compute");
    int total = 0;
    for (int i = 0; i < n; ++i) {
        Tracer iter("loop");
        total += i;
    }
    return total;
}

// -----------------------------------------------------------------------------
// 2. Dynamic slicing example
// -----------------------------------------------------------------------------
struct Statement {
    int id;
    std::vector<int> uses;
    int def;
};

class SliceRecorder {
public:
    void exec(int stmtId, int defVar, std::vector<int> uses) {
        history_.push_back({stmtId, std::move(uses), defVar});
    }

    std::vector<int> slice(int stmtId, int var) const {
        std::vector<int> sliceIds;
        int targetVar = var;
        for (auto it = history_.rbegin(); it != history_.rend(); ++it) {
            if (it->id == stmtId && (it->def == var || contains(it->uses, var))) {
                sliceIds.push_back(it->id);
                break;
            }
            if (it->def == targetVar) {
                sliceIds.push_back(it->id);
                if (!it->uses.empty()) {
                    targetVar = it->uses.front();
                }
            }
        }
        return sliceIds;
    }

private:
    static bool contains(const std::vector<int>& vec, int v) {
        for (int x : vec) {
            if (x == v) {
                return true;
            }
        }
        return false;
    }

    std::vector<Statement> history_;
};

std::vector<int> recordAccumulateTrace(SliceRecorder& recorder, int n) {
    // Variable ids: 1=sum, 2=i.
    recorder.exec(1, 1, {});      // sum = 0;
    recorder.exec(2, 2, {});      // i = 1;
    while (n-- > 0) {
        recorder.exec(3, 1, {1, 2}); // sum = sum + i;
        recorder.exec(4, 2, {2});    // i = i + 1;
    }
    recorder.exec(5, -1, {1});   // return sum;
    return recorder.slice(5, 1); // slice for <stmt 5, var sum>.
}

// -----------------------------------------------------------------------------
// 3. Execution indexing example
// -----------------------------------------------------------------------------
struct IndexEntry {
    int regionId;
    int stmtId;
    int instance;
};

class ExecutionIndex {
public:
    void enterRegion(int regionId) {
        stack_.push({regionId, -1, 0});
    }

    void exitRegion() {
        if (!stack_.empty()) {
            stack_.pop();
        }
    }

    IndexEntry mark(int stmtId) {
        if (stack_.empty()) {
            enterRegion(0);
        }
        auto top = stack_.top();
        top.stmtId = stmtId;
        top.instance = ++counters_[stmtId];
        history_.push_back(top);
        return top;
    }

    const std::vector<IndexEntry>& history() const { return history_; }

private:
    std::stack<IndexEntry> stack_;
    std::unordered_map<int, int> counters_;
    std::vector<IndexEntry> history_;
};

void simulateExecution(ExecutionIndex& indexer) {
    indexer.enterRegion(1);     // main
    indexer.mark(101);          // stmt 101

    indexer.enterRegion(2);     // loop region
    for (int iter = 0; iter < 2; ++iter) {
        indexer.mark(201);      // loop body statement
    }
    indexer.exitRegion();

    indexer.enterRegion(3);     // if region
    indexer.mark(301);
    indexer.exitRegion();

    indexer.mark(102);          // final statement
    indexer.exitRegion();
}

// -----------------------------------------------------------------------------
// 4. Fault localization example
// -----------------------------------------------------------------------------
struct Spectrum {
    int passed = 0;
    int failed = 0;
};

class FaultLocalizer {
public:
    void report(int stmtId, bool failed) {
        auto& s = spectra_[stmtId];
        if (failed) {
            ++s.failed;
        } else {
            ++s.passed;
        }
    }

    void endTest(bool failed) {
        if (failed) {
            ++failedTests_;
        } else {
            ++passedTests_;
        }
    }

    double suspiciousness(int stmtId) const {
        auto it = spectra_.find(stmtId);
        if (it == spectra_.end() || failedTests_ == 0 || passedTests_ == 0) {
            return 0.0;
        }
        double f = static_cast<double>(it->second.failed) / failedTests_;
        double p = static_cast<double>(it->second.passed) / passedTests_;
        double denom = f + p;
        return denom == 0.0 ? 0.0 : f / denom;
    }

private:
    std::map<int, Spectrum> spectra_;
    double failedTests_ = 0;
    double passedTests_ = 0;
};

void runTest(FaultLocalizer& localizer, bool shouldFail) {
    // Example statements executed in a test run.
    localizer.report(10, shouldFail);
    localizer.report(20, shouldFail);
    localizer.report(30, shouldFail);
    localizer.endTest(shouldFail);
}

int main() {
    std::cout << "--- Tracing demo ---\n";
    int result = compute(5);
    std::cout << "compute(5) = " << result << " (trace written to trace.log)\n\n";

    std::cout << "--- Dynamic slicing demo ---\n";
    SliceRecorder recorder;
    auto slice = recordAccumulateTrace(recorder, 3);
    std::cout << "Slice affecting return sum: ";
    for (int stmt : slice) {
        std::cout << stmt << ' ';
    }
    std::cout << "\n\n";

    std::cout << "--- Execution indexing demo ---\n";
    ExecutionIndex indexer;
    simulateExecution(indexer);
    for (const auto& entry : indexer.history()) {
        std::cout << "region " << entry.regionId << ", stmt " << entry.stmtId
                  << ", instance " << entry.instance << '\n';
    }
    std::cout << '\n';

    std::cout << "--- Fault localization demo ---\n";
    FaultLocalizer localizer;
    runTest(localizer, false);
    runTest(localizer, true);
    runTest(localizer, true);
    for (int stmtId : {10, 20, 30}) {
        std::cout << "Suspiciousness stmt " << stmtId << " = "
                  << localizer.suspiciousness(stmtId) << '\n';
    }

    return 0;
}
