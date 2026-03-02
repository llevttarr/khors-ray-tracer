#include "benchmark.h"
#include "glsl_util.h"
#include <iostream>

Benchmark::Benchmark(){
    res_path="benchmark_res.csv";
    results.reserve(100);
    std::cout<<"Starting benchmark..."<<std::endl;
}
void Benchmark::update(std::string& r){
    if (counter==0){
        return;
    }
    results.push_back(r);
    --counter;
    if (counter==0){
        std::cout<<"Benchmark stop!"<<std::endl;
        std::string firstline="i,fps";
        glsl_util::out_csv(firstline,res_path,results);
        results.clear();
    }
}