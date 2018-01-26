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
    Selected = Skills.size() ? 0 : -1;
}

void SkillManager::Next()
{
    Selected = (Selected + Skills.size() + 1) % Skills.size();
}

void SkillManager::Previous()
{
    Selected = (Selected + Skills.size() + 1) % Skills.size();
}

SkillParameter *SkillManager::GetSkillParameter(int relative)
{
    return nullptr;
}

void SkillManager::LoadFromToml(boost::filesystem::path file)
{
    auto log = spdlog::get("main");
    auto result = make_shared<SkillParameter>();

    ifstream ifs(file.wstring(), ios::in);
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