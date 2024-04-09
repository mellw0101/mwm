#include <chrono>
#include <fstream>
#include <ios>
#include <iostream>
#include <map>
// #include <memory>
#include <numeric>
#include <sstream>
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

/**
**********************************************************************
**********************************************************************
****************<<    @class @c GlobalProfiler      >>****************
**********************************************************************
**********************************************************************
*/

void
GlobalProfiler::record(const std::string& name, double duration)
{
    stats[name].record(duration);
}

std::string
makeNamePadding(const std::string &__s)
{
    std::stringstream ss;
    for (int i = 0; (i + __s.length()) < 30; ++i)
    {
        ss << ' ';
    }
    return ss.str();
}

/* std::string
makeDoublePadding(const double &__d)
{
    std::stringstream ss;
    for (int i = 0; (i + std::to_string(__d).length()) < 12; ++i)
    {
        ss << ' ';
    }
    return ss.str();
} */

/* std::string
makeDoublePadding(const double &__d)
{
    std::ostringstream oss;
    oss << std::setprecision(2) << __d; // Set desired precision for the double's string representation
    std::string doubleAsString = oss.str(); // Convert the double to a string with fixed precision

    std::stringstream ss;
    for (size_t i = doubleAsString.length(); i < 12; ++i) // Calculate the padding based on the length of the double's string representation
    {
        ss << ' ';
    }
    return ss.str();
} */

void
GlobalProfiler::report(const std::string &filename)
{
    std::ofstream file(filename, std::ios::app);
    file << "\nProfiling report:\n";
    for (const auto& pair : stats)
    {
        file <<
            pair.first << makeNamePadding(pair.first) <<
            ": Mean = " << pair.second.mean()   << " ms," << /* makeDoublePadding(pair.second.mean())   << */
            "Stddev = " << pair.second.stddev() << " ms," << /* makeDoublePadding(pair.second.stddev()) << */
            "   Min = " << pair.second.min()    << " ms," << /* makeDoublePadding(pair.second.min())    << */
            "   Max = " << pair.second.max()    << " ms," << /* makeDoublePadding(pair.second.max())    << */
            " Count = " << pair.second.count()  <<           /* makeDoublePadding(pair.second.count())  << */
        "\n";
    }
    (void)file.close();
    
    for (const auto &i : stats)
    {
        std::ofstream File("/home/mellw/gprof/" + i.first, std::ios::app);
        File <<
            i.second.mean()   << ':' <<
            i.second.stddev() << ':' <<
            i.second.min()    << ':' <<
            i.second.max()    << ':' <<
            i.second.count()  << ':' <<
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

/** 
**********************************************************************
**********************************************************************
*****************<<      @class @c AutoTimer      >>******************
**********************************************************************
**********************************************************************
**/

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