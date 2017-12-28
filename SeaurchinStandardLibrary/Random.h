#pragma once

#include <cstdint>
#include <memory>
#include <random>
#include <new>

#include <angelscript.h>

class Random final {
private:
    std::unique_ptr<std::mt19937> random;

    Random();
    Random(uint32_t seed);
    ~Random();

    static void Factory(void *memory);
    static void Factory(void *memory, uint32_t seed);
    static void Descrutor(void *memory);

    uint32_t NextInt(uint32_t max);
    double NextDouble();
    double NextDouble(double max);

public:
    static void RegisterType(asIScriptEngine *engine);
};