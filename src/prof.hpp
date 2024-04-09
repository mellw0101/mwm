#ifndef PROF_HPP
#define PROF_HPP

#include <chrono>
#include <map>
#include <string>
#include <vector>

class ProfilerStats {
public:
    void record(double value);
    double mean() const;
    double stddev() const;
    double min() const;
    double max() const;
    size_t count() const;

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
    void record(const std::string& name, double duration);
    void report(const std::string& filename);
    static GlobalProfiler *createNewGprof();

private:
    std::map<std::string, ProfilerStats> stats;
    GlobalProfiler() {}
};
static GlobalProfiler *gProf(nullptr);

void init_gProf();

class AutoTimer {
public:
    AutoTimer(const std::string& name);/*  : name(name), start(std::chrono::high_resolution_clock::now()) {} */
    ~AutoTimer();

private:
    std::string name;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};

// Register at-exit handler to generate the report
void setupReportGeneration();

#endif/* PROF_HPP */