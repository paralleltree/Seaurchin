#include "Skill.h"
#include "Setting.h"
#include "Misc.h"
#include "ExecutionManager.h"

using namespace std;

SkillManager::SkillManager(ExecutionManager *exm)
{
    manager = exm;
    Selected = -1;
}

void SkillManager::LoadAllSkills()
{
    using namespace boost;
    using namespace boost::filesystem;
    path skillroot = Setting::GetRootDirectory() / SU_SKILL_DIR / SU_SKILL_DIR;

    directory_iterator dit(skillroot);
    for (const auto& fdata : make_iterator_range(dit, {})) {
        if (is_directory(fdata)) continue;
        auto filename = ConvertUnicodeToUTF8(fdata.path().wstring());
        if (!ends_with(filename, ".toml")) continue;
        LoadFromToml(fdata.path());
    }
    spdlog::get("main")->info(u8"スキル総数: {0:d}", Skills.size());
    Selected = Skills.size() ? 0 : -1;
}

void SkillManager::Next()
{
    Selected = (Selected + Skills.size() + 1) % Skills.size();
}

void SkillManager::Previous()
{
    Selected = (Selected + Skills.size() - 1) % Skills.size();
}

SkillParameter *SkillManager::GetSkillParameter(int relative)
{
    int ri = Selected + relative;
    while (ri < 0) ri += Skills.size();
    return Skills[ri % Skills.size()].get();
}

shared_ptr<SkillParameter> SkillManager::GetSkillParameterSafe(int relative)
{
    int ri = Selected + relative;
    while (ri < 0) ri += Skills.size();
    return Skills[ri % Skills.size()];
}

void SkillManager::LoadFromToml(boost::filesystem::path file)
{
    using namespace boost::filesystem;
    auto log = spdlog::get("main");
    auto result = make_shared<SkillParameter>();
    auto iconRoot = Setting::GetRootDirectory() / SU_SKILL_DIR / SU_ICON_DIR;

    std::ifstream ifs(file.wstring(), ios::in);
    auto pr = toml::parse(ifs);
    ifs.close();
    if (!pr.valid()) {
        log->error(u8"スキル {0} は不正なファイルです", ConvertUnicodeToUTF8(file.wstring()));
        log->error(pr.errorReason);
        return;
    }
    auto &root = pr.value;

    try {
        result->Name = root.get<string>("Name");
        result->Description = root.get<string>("Description");
        result->IconPath = ConvertUnicodeToUTF8((iconRoot / ConvertUTF8ToUnicode(root.get<string>("Icon"))).wstring());

        auto abilities = root.get<vector<toml::Table>>("Abilities");
        for (const auto &ability : abilities) {
            AbilityParameter ai;
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
        log->error(u8"スキル {0} の読み込みに失敗しました", ConvertUnicodeToUTF8(file.wstring()));
        return;
    }
    Skills.push_back(result);
}


SkillIndicators::SkillIndicators()
{
    CallbackFunction = nullptr;
}

SkillIndicators::~SkillIndicators()
{
    for (const auto &i : IndicatorIcons) i->Release();
    if (CallbackFunction) {
        auto engine = CallbackContext->GetEngine();
        CallbackContext->Release();
        CallbackFunction->Release();
        engine->ReleaseScriptObject(CallbackObject, CallbackObjectType);
    }
}

void SkillIndicators::SetCallback(asIScriptFunction *func)
{
    if (!func || func->GetFuncType() != asFUNC_DELEGATE) return;

    if (CallbackFunction) {
        auto engine = CallbackContext->GetEngine();
        CallbackContext->Release();
        CallbackFunction->Release();
        engine->ReleaseScriptObject(CallbackObject, CallbackObjectType);
    }

    auto ctx = asGetActiveContext();
    auto engine = ctx->GetEngine();
    CallbackContext = engine->CreateContext();
    CallbackFunction = func->GetDelegateFunction();
    CallbackFunction->AddRef();
    CallbackObject = (asIScriptObject*)func->GetDelegateObject();
    CallbackObjectType = func->GetDelegateObjectType();
}

int SkillIndicators::AddSkillIndicator(const string &icon)
{
    using namespace boost::filesystem;
    auto path = Setting::GetRootDirectory() / SU_SKILL_DIR / SU_ICON_DIR / ConvertUTF8ToUnicode(icon);
    auto image = SImage::CreateLoadedImageFromFile(ConvertUnicodeToUTF8(path.wstring()), true);
    IndicatorIcons.push_back(image);
    return IndicatorIcons.size() - 1;
}

void SkillIndicators::TriggerSkillIndicator(int index)
{
    CallbackContext->Prepare(CallbackFunction);
    CallbackContext->SetObject(CallbackObject);
    CallbackContext->SetArgDWord(0, index);
    CallbackContext->Execute();
    // CallbackContext->Unprepare();
}

int SkillIndicators::GetSkillIndicatorCount()
{
    return IndicatorIcons.size();
}

SImage* SkillIndicators::GetSkillIndicatorImage(int index)
{
    if (index >= IndicatorIcons.size()) return nullptr;
    auto result = IndicatorIcons[index];
    result->AddRef();
    return result;
}

void RegisterSkillTypes(asIScriptEngine *engine)
{
    engine->RegisterEnum(SU_IF_NOTETYPE);
    engine->RegisterEnumValue(SU_IF_NOTETYPE, "Tap", (int)AbilityNoteType::Tap);
    engine->RegisterEnumValue(SU_IF_NOTETYPE, "ExTap", (int)AbilityNoteType::ExTap);
    engine->RegisterEnumValue(SU_IF_NOTETYPE, "Flick", (int)AbilityNoteType::Flick);
    engine->RegisterEnumValue(SU_IF_NOTETYPE, "HellTap", (int)AbilityNoteType::HellTap);
    engine->RegisterEnumValue(SU_IF_NOTETYPE, "Air", (int)AbilityNoteType::Air);
    engine->RegisterEnumValue(SU_IF_NOTETYPE, "Hold", (int)AbilityNoteType::Hold);
    engine->RegisterEnumValue(SU_IF_NOTETYPE, "Slide", (int)AbilityNoteType::Slide);
    engine->RegisterEnumValue(SU_IF_NOTETYPE, "AirAction", (int)AbilityNoteType::AirAction);

    engine->RegisterFuncdef("void " SU_IF_SKILL_CALLBACK "(int)");
    engine->RegisterObjectType(SU_IF_SKILL_INDICATORS, 0, asOBJ_REF | asOBJ_NOCOUNT);
    engine->RegisterObjectMethod(SU_IF_SKILL_INDICATORS, "int AddIndicator(const string &in)", asMETHOD(SkillIndicators, AddSkillIndicator), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SKILL_INDICATORS, "void TriggerIndicator(int)", asMETHOD(SkillIndicators, TriggerSkillIndicator), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SKILL_INDICATORS, "void SetCallback(" SU_IF_SKILL_CALLBACK "@)", asMETHOD(SkillIndicators, SetCallback), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SKILL_INDICATORS, "int GetIndicatorCount()", asMETHOD(SkillIndicators, GetSkillIndicatorCount), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SKILL_INDICATORS, SU_IF_IMAGE "@ GetIndicatorImage(int)", asMETHOD(SkillIndicators, GetSkillIndicatorImage), asCALL_THISCALL);

    engine->RegisterInterface(SU_IF_ABILITY);
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void Initialize(dictionary@, " SU_IF_SKILL_INDICATORS "@)");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnStart(" SU_IF_RESULT "@)");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnFinish(" SU_IF_RESULT "@)");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnJusticeCritical(" SU_IF_RESULT "@, " SU_IF_NOTETYPE ")");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnJustice(" SU_IF_RESULT "@, " SU_IF_NOTETYPE ")");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnAttack(" SU_IF_RESULT "@, " SU_IF_NOTETYPE ")");
    engine->RegisterInterfaceMethod(SU_IF_ABILITY, "void OnMiss(" SU_IF_RESULT "@, " SU_IF_NOTETYPE ")");

    engine->RegisterObjectType(SU_IF_SKILL, 0, asOBJ_REF | asOBJ_NOCOUNT);
    engine->RegisterObjectProperty(SU_IF_SKILL, "string Name", asOFFSET(SkillParameter, Name));
    engine->RegisterObjectProperty(SU_IF_SKILL, "string IconPath", asOFFSET(SkillParameter, IconPath));
    engine->RegisterObjectProperty(SU_IF_SKILL, "string Description", asOFFSET(SkillParameter, Description));

    engine->RegisterObjectType(SU_IF_SKILL_MANAGER, 0, asOBJ_REF | asOBJ_NOCOUNT);
    engine->RegisterObjectMethod(SU_IF_SKILL_MANAGER, "void Next()", asMETHOD(SkillManager, Next), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SKILL_MANAGER, "void Previous()", asMETHOD(SkillManager, Previous), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SKILL_MANAGER, SU_IF_SKILL "@ GetSkill(int)", asMETHOD(SkillManager, GetSkillParameter), asCALL_THISCALL);
}
