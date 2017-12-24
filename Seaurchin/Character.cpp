#include "Character.h"
#include "ExecutionManager.h"
#include "Result.h"
#include "Misc.h"

using namespace std;

void RegisterCharacterTypes(ExecutionManager *exm)
{
    auto engine = exm->GetScriptInterfaceUnsafe()->GetEngine();

    engine->RegisterInterface(SU_IF_ABILITY);
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void Initialize(array<string>@)");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnStart(" SU_IF_RESULT "@)");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnFinish(" SU_IF_RESULT "@)");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnJusticeCritical(" SU_IF_RESULT "@)");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnJustice(" SU_IF_RESULT "@)");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnAttack(" SU_IF_RESULT "@)");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnMiss(" SU_IF_RESULT "@)");
}

asIScriptObject* Character::LoadAbility(boost::filesystem::path spath)
{
    using namespace boost::filesystem;
    auto log = spdlog::get("main");
    auto abroot = Setting::GetRootDirectory() / SU_DATA_DIR / SU_CHARACTER_DIR / L"Abilities";
    //お茶を濁せ
    auto modulename = ConvertUnicodeToUTF8(spath.c_str());
    auto mod = ScriptInterface->GetExistModule(modulename);
    if (!mod) {
        ScriptInterface->StartBuildModule(modulename.c_str(), [=](wstring inc, wstring from, CWScriptBuilder *b) {
            if (!exists(abroot / inc)) return false;
            b->AddSectionFromFile((abroot / inc).wstring().c_str());
            return true;
        });
        ScriptInterface->LoadFile(spath.wstring().c_str());
        if (!ScriptInterface->FinishBuildModule()) {
            ScriptInterface->GetLastModule()->Discard();
            return nullptr;
        }
        mod = ScriptInterface->GetLastModule();
    }

    //エントリポイント検索
    int cnt = mod->GetObjectTypeCount();
    asITypeInfo *type = nullptr;
    for (int i = 0; i < cnt; i++) {
        auto cti = mod->GetObjectTypeByIndex(i);
        if (!(ScriptInterface->CheckMetaData(cti, "EntryPoint") || cti->GetUserData(SU_UDTYPE_ENTRYPOINT))) continue;
        type = cti;
        type->SetUserData((void*)0xFFFFFFFF, SU_UDTYPE_ENTRYPOINT);
        type->AddRef();
        break;
    }
    if (!type) {
        log->critical(u8"アビリティーにEntryPointがありません");
        return nullptr;
    }

    auto obj = ScriptInterface->InstantiateObject(type);
    obj->AddRef();
    type->Release();
    return obj;
}

Character::Character(shared_ptr<AngelScript> script, shared_ptr<Result> result, shared_ptr<CharacterInfo> info)
{
    ScriptInterface = script;
    Info = info;
    TargetResult = result;
    context = script->GetEngine()->CreateContext();
}

Character::~Character()
{
    context->Release();
    for (const auto &t : AbilityTypes) t->Release();
    for (const auto &o : Abilities) o->Release();
}

void Character::Initialize()
{
    using namespace boost::algorithm;
    using namespace boost::filesystem;
    auto log = spdlog::get("main");
    auto abroot = Setting::GetRootDirectory() / SU_DATA_DIR / SU_CHARACTER_DIR / L"Abilities";

    for (const auto &def : Info->Abilities) {
        vector<string> params;
        split(params, def, is_any_of(" "));
        auto scrpath = abroot / (params[0] + ".as");

        auto abo = LoadAbility(scrpath);
        if (!abo) continue;
        auto abt = abo->GetObjectType();
        abt->AddRef();
        Abilities.push_back(abo);
        AbilityTypes.push_back(abt);

        auto init = abt->GetMethodByDecl("void Initialize(array<string>@)");
        if (!init) continue;
        
        auto at = ScriptInterface->GetEngine()->GetTypeInfoByDecl("array<string>");
        auto args = CScriptArray::Create(at, params.size() - 1);
        for (int i = 0; i < params.size() - 1; ++i) args->SetValue(i, &(params[i + 1]));

        context->Prepare(init);
        context->SetObject(abo);
        context->SetArgAddress(0, args);
        context->Execute();
        log->info(u8"アビリティー " + ConvertUnicodeToUTF8(scrpath.c_str()));
    }
}

void Character::CallOnEvent(const char *decl)
{
    for (int i = 0; i < Abilities.size(); ++i) {
        auto func = AbilityTypes[i]->GetMethodByDecl(decl);
        if (!func) continue;
        context->Prepare(func);
        context->SetObject(Abilities[i]);
        context->SetArgAddress(0, TargetResult.get());
        context->Execute();
    }
}

void Character::OnStart()
{
    CallOnEvent("void OnStart(" SU_IF_RESULT "@)");
}

void Character::OnFinish()
{
    CallOnEvent("void OnFinish(" SU_IF_RESULT "@)");
}

void Character::OnJusticeCritical()
{
    CallOnEvent("void OnJusticeCritical(" SU_IF_RESULT "@)");
}

void Character::OnJustice()
{
    CallOnEvent("void OnJustice(" SU_IF_RESULT "@)");
}

void Character::OnAttack()
{
    CallOnEvent("void OnAttack(" SU_IF_RESULT "@)");
}

void Character::OnMiss()
{
    CallOnEvent("void OnMiss(" SU_IF_RESULT "@)");
}
