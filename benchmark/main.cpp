/*
 * MIT License
 * Copyright (c) 2026 KAVA Team
 * 
 * KAVA 2.0 - Benchmark Runner
 * Executa todos os benchmarks e gera relatório comparativo
 */

#include "benchmark.h"
#include <iostream>

int main(int argc, char** argv) {
    std::cout << R"(
    ██╗  ██╗ █████╗ ██╗   ██╗ █████╗     ██████╗    ██████╗ 
    ██║ ██╔╝██╔══██╗██║   ██║██╔══██╗    ╚════██╗  ██╔═████╗
    █████╔╝ ███████║██║   ██║███████║     █████╔╝  ██║██╔██║
    ██╔═██╗ ██╔══██║╚██╗ ██╔╝██╔══██║    ██╔═══╝   ████╔╝██║
    ██║  ██╗██║  ██║ ╚████╔╝ ██║  ██║    ███████╗  ╚██████╔╝
    ╚═╝  ╚═╝╚═╝  ╚═╝  ╚═══╝  ╚═╝  ╚═╝    ╚══════╝   ╚═════╝ 
    
    Performance Benchmark Suite
    Comparing with Java 5 & Java 6
    )" << std::endl;
    
    Kava::BenchmarkRunner runner;
    
    // Registra benchmarks padrão
    Kava::StandardBenchmarks::registerAll(runner);
    
    // Executa todos
    runner.runAll();
    
    // Gera relatório comparativo
    Kava::BenchmarkReporter::generateReport(runner.getResults());
    
    return 0;
}
