#ifndef BENCHMARK_H
#define BENCHMARK_H
#include <vector>
#include <string>

class Benchmark{
public:
   Benchmark();
   void update(std::string& result);
private:
    size_t counter=100;
    std::vector<std::string> results;
    std::string res_path;
};

#endif // BENCHMARK_H