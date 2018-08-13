#include "Random.h"

using namespace std;

Random::Random()
{
    random_device seeder;
    random = make_unique<mt19937>(seeder());
}

Random::Random(uint32_t seed)
{
    random = make_unique<mt19937>(seed);
}

Random::~Random()
{}

void Random::Factory(void *memory)
{
    new(memory) Random();
}

void Random::Factory(void *memory, uint32_t seed)
{
    new(memory) Random(seed);
}

void Random::Descrutor(void * memory)
{
    reinterpret_cast<Random*>(memory)->~Random();
}

uint32_t Random::NextInt(uint32_t max)
{
    uniform_int_distribution<uint32_t> dist(0, max - 1);
    return dist(*random);
}

double Random::NextDouble()
{
    uniform_real_distribution<double> dist;
    return dist(*random);
}

double Random::NextDouble(double max)
{
    uniform_real_distribution<double> dist(0, max);
    return dist(*random);
}

void Random::RegisterType(asIScriptEngine *engine)
{
    engine->RegisterObjectType("Random", sizeof(Random), asOBJ_VALUE | asGetTypeTraits<Random>());
    engine->RegisterObjectBehaviour("Random", asBEHAVE_CONSTRUCT, "void f()", asFUNCTIONPR(Random::Factory, (void*), void), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectBehaviour("Random", asBEHAVE_CONSTRUCT, "void f(uint32)", asFUNCTIONPR(Random::Factory, (void*, uint32_t), void), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectBehaviour("Random", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Random::Descrutor), asCALL_CDECL_OBJFIRST);
    engine->RegisterObjectMethod("Random", "uint32 NextInt(uint32)", asMETHOD(Random, NextInt), asCALL_THISCALL);
    engine->RegisterObjectMethod("Random", "double NextDouble()", asMETHODPR(Random, NextDouble, (double), double), asCALL_THISCALL);
    engine->RegisterObjectMethod("Random", "double NextDouble(double)", asMETHODPR(Random, NextDouble, (void), double), asCALL_THISCALL);
}
