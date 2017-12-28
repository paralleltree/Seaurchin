#pragma once

#include <angelscript.h>

#define SeaurchinExport extern "C" __declspec(dllexport) __stdcall
#define SeaurchinImport __stdcall

typedef void(SeaurchinImport *SeaurchinExtensionEntryPoint)(asIScriptEngine*);