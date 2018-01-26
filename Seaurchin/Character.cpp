#include "Character.h"
#include "ExecutionManager.h"
#include "Result.h"
#include "Misc.h"
#include "Setting.h"

using namespace std;

void RegisterCharacterTypes(ExecutionManager *exm)
{
    auto engine = exm->GetScriptInterfaceUnsafe()->GetEngine();

    engine->RegisterEnum(SU_IF_NOTETYPE);
    engine->RegisterEnumValue(SU_IF_NOTETYPE, "Tap", (int)CharacterNoteType::Tap);
    engine->RegisterEnumValue(SU_IF_NOTETYPE, "ExTap", (int)CharacterNoteType::ExTap);
    engine->RegisterEnumValue(SU_IF_NOTETYPE, "Flick", (int)CharacterNoteType::Flick);
    engine->RegisterEnumValue(SU_IF_NOTETYPE, "HellTap", (int)CharacterNoteType::HellTap);
    engine->RegisterEnumValue(SU_IF_NOTETYPE, "Air", (int)CharacterNoteType::Air);
    engine->RegisterEnumValue(SU_IF_NOTETYPE, "Hold", (int)CharacterNoteType::Hold);
    engine->RegisterEnumValue(SU_IF_NOTETYPE, "Slide", (int)CharacterNoteType::Slide);
    engine->RegisterEnumValue(SU_IF_NOTETYPE, "AirAction", (int)CharacterNoteType::AirAction);

    engine->RegisterInterface(SU_IF_ABILITY);
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void Initialize(dictionary@)");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnStart(" SU_IF_RESULT "@)");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnFinish(" SU_IF_RESULT "@)");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnJusticeCritical(" SU_IF_RESULT "@, " SU_IF_NOTETYPE ")");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnJustice(" SU_IF_RESULT "@, " SU_IF_NOTETYPE ")");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnAttack(" SU_IF_RESULT "@, " SU_IF_NOTETYPE ")");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnMiss(" SU_IF_RESULT "@, " SU_IF_NOTETYPE ")");

    engine->RegisterObjectType(SU_IF_CHARACTER_MANAGER, 0, asOBJ_REF | asOBJ_NOCOUNT);
    engine->RegisterObjectMethod(SU_IF_CHARACTER_MANAGER, "void Next()", asMETHOD(CharacterManager, Next), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_CHARACTER_MANAGER, "void Previous()", asMETHOD(CharacterManager, Previous), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_CHARACTER_MANAGER, "string GetName(int)", asMETHOD(CharacterManager, GetName), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_CHARACTER_MANAGER, "string GetSkillName(int)", asMETHOD(CharacterManager, GetSkillName), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_CHARACTER_MANAGER, "string GetDescription(int)", asMETHOD(CharacterManager, GetDescription), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_CHARACTER_MANAGER, "string GetImagePath(int)", asMETHOD(CharacterManager, GetImagePath), asCALL_THISCALL);
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
        auto scrpath = abroot / (def.Name + ".as");

        auto abo = LoadAbility(scrpath);
        if (!abo) continue;
        auto abt = abo->GetObjectType();
        abt->AddRef();
        Abilities.push_back(abo);
        AbilityTypes.push_back(abt);

        auto init = abt->GetMethodByDecl("void Initialize(dictionary@)");
        if (!init) continue;
        
        auto args = CScriptDictionary::Create(ScriptInterface->GetEngine());
        for (const auto &arg : def.Arguments) {
            auto key = arg.first;
            auto value = arg.second;
            auto &vid = value.type();
            if (vid == typeid(int)) {
                asINT64 avalue = boost::any_cast<int>(value);
                args->Set(key, avalue);
            } else if (vid == typeid(double)) {
                double avalue = boost::any_cast<double>(value);
                args->Set(key, avalue);
            } else if (vid == typeid(string)) {
                string avalue = boost::any_cast<string>(value);
                args->Set(key, &avalue, ScriptInterface->GetEngine()->GetTypeIdByDecl("string"));
            }
        }

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

void Character::CallOnEvent(const char *decl, CharacterNoteType type)
{
    for (int i = 0; i < Abilities.size(); ++i) {
        auto func = AbilityTypes[i]->GetMethodByDecl(decl);
        if (!func) continue;
        context->Prepare(func);
        context->SetObject(Abilities[i]);
        context->SetArgAddress(0, TargetResult.get());
        context->SetArgDWord(1, (asDWORD)type);
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

void Character::OnJusticeCritical(CharacterNoteType type)
{
    CallOnEvent("void OnJusticeCritical(" SU_IF_RESULT "@, " SU_IF_NOTETYPE ")", type);
}

void Character::OnJustice(CharacterNoteType type)
{
    CallOnEvent("void OnJustice(" SU_IF_RESULT "@, " SU_IF_NOTETYPE ")", type);
}

void Character::OnAttack(CharacterNoteType type)
{
    CallOnEvent("void OnAttack(" SU_IF_RESULT "@, " SU_IF_NOTETYPE ")", type);
}

void Character::OnMiss(CharacterNoteType type)
{
    CallOnEvent("void OnMiss(" SU_IF_RESULT "@, " SU_IF_NOTETYPE ")", type);
}

shared_ptr<CharacterInfo> CharacterInfo::LoadFromToml(const boost::filesystem::path &path)
{
    auto log = spdlog::get("main");
    auto result = make_shared<CharacterInfo>();
    
    ifstream ifs(path.wstring(), ios::in);
    auto pr = toml::parse(ifs);
    ifs.close();
    
    if (!pr.valid()) return nullptr;
    auto &ci = pr.value;
    try {
        result->Name = ci.get<string>("Name");
        result->Description = ci.get<string>("Description");
        result->ImagePath = path.parent_path() / ConvertUTF8ToUnicode(ci.get<string>("Image"));
        result->SkillName = ci.get<string>("SkillName");
        auto abilities = ci.get<vector<toml::Table>>("Abilities");
        for (const auto &ability : abilities) {
            AbilityInfo ai;
            ai.Name = ability.at("Type").as<string>();
            auto args = ability.at("Arguments").as<toml::Table>();
            for (const auto &p : args) {
                switch (p.second.type()) {
                    case toml::Value::INT_TYPE:
                        ai.Arguments[p.first] = p.second.as<int>();
                        break;
                    case toml::Value::DOUBLE_TYPE:
                        ai.Arguments[p.first] = p.second.as<double>();
                        break;
                    case toml::Value::STRING_TYPE:
                        ai.Arguments[p.first] = p.second.as<string>();
                        break;
                    default:
                        break;
                }
            }
            result->Abilities.push_back(ai);
        }

    } catch (exception) {
        log->error(u8"キャラクター {0} の読み込みに失敗しました", ConvertUnicodeToUTF8(path.wstring()));
        return nullptr;
    }
    return result;
}

CharacterManager::CharacterManager(ExecutionManager *exm)
{
    manager = exm;
}

void CharacterManager::Load()
{
    using namespace boost;
    using namespace boost::filesystem;
    using namespace boost::xpressive;
    auto log = spdlog::get("main");

    path sepath = Setting::GetRootDirectory() / SU_DATA_DIR / SU_CHARACTER_DIR;

    for (const auto& fdata : make_iterator_range(directory_iterator(sepath), {})) {
        if (is_directory(fdata)) continue;
        auto filename = ConvertUnicodeToUTF8(fdata.path().wstring());
        if (!ends_with(filename, ".toml")) continue;
        Characters.push_back(CharacterInfo::LoadFromToml(fdata.path()));
    }
    log->info(u8"キャラクター総数: {0:d}", Characters.size());
    Selected = 0;
}

shared_ptr<Character> CharacterManager::CreateCharacterInstance(shared_ptr<Result> result)
{
    return make_shared<Character>(manager->GetScriptInterfaceSafe(), result, Characters[Selected]);
}

void CharacterManager::Next()
{
    Selected = (Selected + Characters.size() + 1) % Characters.size();
}

void CharacterManager::Previous()
{
    Selected = (Selected + Characters.size() - 1) % Characters.size();
}

string CharacterManager::GetName(int relative)
{
    int ri = Selected + relative;
    while (ri < 0) ri += Characters.size();
    return Characters[ri % Characters.size()]->Name;
}

string CharacterManager::GetDescription(int relative)
{
    int ri = Selected + relative;
    while (ri < 0) ri += Characters.size();
    return Characters[ri % Characters.size()]->Description;
}

string CharacterManager::GetImagePath(int relative)
{
    int ri = Selected + relative;
    while (ri < 0) ri += Characters.size();
    return ConvertUnicodeToUTF8(Characters[ri % Characters.size()]->ImagePath.wstring());
}

string CharacterManager::GetSkillName(int relative)
{
    int ri = Selected + relative;
    while (ri < 0) ri += Characters.size();
    return Characters[ri % Characters.size()]->SkillName;
}

