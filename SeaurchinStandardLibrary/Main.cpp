#include "Main.h"
#include "Random.h"

static asIScriptEngine *ScriptEngine;

void SeaurchinAPI InitializeExtension(asIScriptEngine *engine)
{
    ScriptEngine = engine;
}

void SeaurchinAPI RegisterInterfaces()
{
    Random::RegisterType(ScriptEngine);
}