#include "AngelScriptManager.h"
#include "Debug.h"

using namespace std;

static int ScriptIncludeCallback(const wchar_t *include, const wchar_t *from, CWScriptBuilder *builder, void *userParam);

AngelScript::AngelScript()
{
    engine = asCreateScriptEngine();
    RegisterScriptMath(engine);
    RegisterScriptArray(engine, true);
    RegisterStdString(engine);
    RegisterStdStringUtils(engine);
    RegisterScriptDictionary(engine);
    engine->SetMessageCallback(asMETHOD(AngelScript, ScriptMessageCallback), this, asCALL_THISCALL);

    //Script Interface

    sharedContext = engine->CreateContext();
    sharedContext->AddRef();
    builder.SetIncludeCallback(ScriptIncludeCallback, this);
}

AngelScript::~AngelScript()
{
    sharedContext->Release();
    // engine->ShutDownAndRelease();
}

void AngelScript::StartBuildModule(const string &name, IncludeCallback callback)
{
    includeFunc = callback;
    builder.StartNewModule(engine, name.c_str());
}

void AngelScript::LoadFile(const wstring &filename)
{
    builder.AddSectionFromFile(filename.c_str());
}

bool AngelScript::IncludeFile(const wstring &include, const wstring &from)
{
    return includeFunc(include, from, &builder);
}

bool AngelScript::FinishBuildModule()
{
    return builder.BuildModule() >= 0;
}

bool AngelScript::CheckMetaData(asITypeInfo *type, std::string meta)
{
    auto df = builder.GetMetadataStringForType(type->GetTypeId());
    return df == meta;
}

bool AngelScript::CheckMetaData(asIScriptFunction *func, std::string meta)
{
    auto df = builder.GetMetadataStringForFunc(func);
    return df == meta;
}

asIScriptObject * AngelScript::InstantiateObject(asITypeInfo * type)
{
    auto factory = type->GetFactoryByIndex(0);
    sharedContext->Prepare(factory);
    sharedContext->Execute();
    return *(asIScriptObject**)sharedContext->GetAddressOfReturnValue();
}

void AngelScript::ScriptMessageCallback(const asSMessageInfo * message)
{
    using namespace std;
    auto log = spdlog::get("main");
    switch (message->type) {
        case asEMsgType::asMSGTYPE_INFORMATION:
            log->info(u8"{0} ({1:d}s{2:d}—ñ): {3}", message->section, message->row, message->col, message->message);
            break;
        case asEMsgType::asMSGTYPE_WARNING:
            log->warn(u8"{0} ({1:d}s{2:d}—ñ): {3}", message->section, message->row, message->col, message->message);
            break;
        case asEMsgType::asMSGTYPE_ERROR:
            log->error(u8"{0} ({1:d}s{2:d}—ñ): {3}", message->section, message->row, message->col, message->message);
            break;
    }
}

int ScriptIncludeCallback(const wchar_t *include, const wchar_t *from, CWScriptBuilder *builder, void *userParam)
{
    auto as = reinterpret_cast<AngelScript*>(userParam);
    return as->IncludeFile(include, from) ? 1 : -1;
}