#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
// #include <memory>
#include <numeric>
#include <string>
#include <vector>
#include <cmath>

class ProfilerStats {
public:
    void record( double value )
    {
        values.push_back(value);
    }

    double mean( ) const
    {
        if (values.empty()) return 0.0;
        double sum = std::accumulate(values.begin(), values.end(), 0.0);
        return sum / values.size();
    }

    double stddev( ) const
    {
        if (values.size() < 2) return 0.0;
        double mean_val = mean();
        double sq_sum = std::accumulate(values.begin(), values.end(), 0.0, [mean_val](double a, double b) { return a + (b - mean_val) * (b - mean_val); });
        return std::sqrt(sq_sum / values.size());
    }

    double min( ) const
    {
        return values.empty() ? 0.0 : *std::min_element(values.begin(), values.end());
    }

    double max( ) const
    {
        return values.empty() ? 0.0 : *std::max_element(values.begin(), values.end());
    }

    size_t count( ) const
    {
        return values.size();
    }

private:
    std::vector<double> values;
};

class GlobalProfiler {
public:
    /* static GlobalProfiler& getInstance()
    {
        static GlobalProfiler instance;
        return instance;
    } */

    void record(const std::string& name, double duration)
    {
        stats[name].record(duration);
    }

    void report(const std::string& filename)
    {
        std::ofstream file(filename);
        for (const auto& pair : stats)
        {
            file << pair.first << ": Mean = " << pair.second.mean() << " ms, Stddev = " << pair.second.stddev()
                 << " ms, Min = " << pair.second.min() << " ms, Max = " << pair.second.max() << " ms, Count = " << pair.second.count() << "\n";
        }
    }

private:
    std::map<std::string, ProfilerStats> stats;
    GlobalProfiler() {}
};
static GlobalProfiler *gProf(nullptr);

class AutoTimer
{
public:
    AutoTimer(const std::string& name) : name(name), start(std::chrono::high_resolution_clock::now()) {}

    ~AutoTimer()
    {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        gProf->record(name, duration.count());
    }

private:
    std::string name;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};

// Register at-exit handler to generate the report
void setupReportGeneration()
{
    std::atexit([] {
        gProf->report("profiling_report.txt");
    });
}

int main()
{
    setupReportGeneration();

    {
        AutoTimer timer("FunctionA");
        // Simulate work
    }

    {
        AutoTimer timer("FunctionB");
        // Simulate work
    }

    // More application code...

    return 0;
}