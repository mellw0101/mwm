#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
// #include <memory>
#include <numeric>
#include <string>
#include <vector>
#include <cmath>
#include "prof.hpp"

void
ProfilerStats::record(double value)
{
    values.push_back(value);
}

double
ProfilerStats::mean() const
{
    if (values.empty()) return 0.0;
    double sum = std::accumulate(values.begin(), values.end(), 0.0);
    return sum / values.size();
}

double
ProfilerStats::stddev() const
{
    if (values.size() < 2) return 0.0;
    double mean_val = mean();
    double sq_sum = std::accumulate(values.begin(), values.end(), 0.0, [mean_val](double a, double b) { return a + (b - mean_val) * (b - mean_val); });
    return std::sqrt(sq_sum / values.size());
}

double
ProfilerStats::min() const
{
    return values.empty() ? 0.0 : *std::min_element(values.begin(), values.end());
}

double
ProfilerStats::max() const
{
    return values.empty() ? 0.0 : *std::max_element(values.begin(), values.end());
}

size_t
ProfilerStats::count() const
{
    return values.size();
}

/* 
**********************************************************************
**********************************************************************
********************<<      GlobalProfiler      >>********************
**********************************************************************
**********************************************************************
*/

void
GlobalProfiler::record(const std::string& name, double duration)
{
    stats[name].record(duration);
}

void
GlobalProfiler::report(const std::string& filename)
{
    std::ofstream file(filename, std::ios::app);
    for (const auto& pair : stats)
    {
        file << 
            pair.first <<
            ": Mean = "  << pair.second.mean()   << " ms," <<
            " Stddev = " << pair.second.stddev() << " ms," <<
            " Min = "    << pair.second.min()    << " ms," <<
            " Max = "    << pair.second.max()    << " ms," <<
            " Count = "  << pair.second.count()  <<
        "\n";
    }
}

GlobalProfiler *
GlobalProfiler::createNewGprof()
{
    return new GlobalProfiler;
}

void
init_gProf()
{
    gProf = GlobalProfiler::createNewGprof();
}

AutoTimer::AutoTimer(const std::string& name)
: name(name), start(std::chrono::high_resolution_clock::now()) {}

AutoTimer::~AutoTimer()
{
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    gProf->record(name, duration.count());
}


void /* Register at-exit handler to generate the report */
setupReportGeneration()
{
    std::atexit([]
    {
        gProf->report("/home/mellw/profiling_report.txt");
    });
}

/* int
main()
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
} */