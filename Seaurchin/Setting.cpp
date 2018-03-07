#include "Setting.h"
#include "Config.h"
#include "Debug.h"
#include "Misc.h"

using namespace std;
using namespace boost::filesystem;
namespace ba = boost::algorithm;

std::wstring Setting::RootDirectory = L"";

Setting::Setting(HMODULE hModule)
{
    if (RootDirectory != "") return;
    wchar_t directory[MAX_PATH];
    GetModuleFileNameW(hModule, directory, MAX_PATH);
    PathRemoveFileSpecW(directory);
    RootDirectory = directory;
}

Setting::Setting()
{

}

void Setting::Load(const wstring &filename)
{
    auto log = spdlog::get("main");
    file = filename;
    if (!exists(RootDirectory / file)) {
        log->info(u8"設定ファイルを作成しました");
        Save();
    }
    std::ifstream ifs((RootDirectory / file).wstring(), ios::in);
    auto pr = toml::parse(ifs);
    if (!pr.valid()) {
        log->error(u8"設定ファイルの記述が不正です: {0}", pr.errorReason);
    } else {
        SettingTree = pr.value;
        log->info(u8"設定ファイルを読み込みました");
    }
    ifs.close();
}

const std::wstring Setting::GetRootDirectory()
{
    return RootDirectory;
}

void Setting::Save() const
{
    auto log = spdlog::get("main");
    std::ofstream ofs((RootDirectory / file).wstring(), ios::out);
    if (SettingTree.valid()) SettingTree.write(&ofs);
    log->info(u8"設定ファイルを保存しました");
    ofs.close();
}

namespace Setting2
{

// SettingItemManager

SettingItemManager::SettingItemManager(shared_ptr<Setting> setting)
{
    SettingInstance = setting;
}

void SettingItemManager::LoadItemsFromToml(boost::filesystem::path file)
{
    using namespace boost::filesystem;
    using namespace crc32_constexpr;

    auto log = spdlog::get("main");

    std::ifstream ifs(file.wstring(), ios::in);
    auto pr = toml::parse(ifs);
    ifs.close();
    if (!pr.valid()) {
        log->error(u8"設定定義 {0} は不正なファイルです", ConvertUnicodeToUTF8(file.wstring()));
        log->error(pr.errorReason);
        return;
    }
    auto &root = pr.value;
    auto items = root.find("SettingItems");
    if (!items || !items->is<toml::Array>()) {
        log->warn(u8"設定定義 {0} に設定項目がありません", ConvertUnicodeToUTF8(file.wstring()));
        return;
    }
    for (const auto &item : items->as<vector<toml::Value>>()) {
        if (item.type() != toml::Value::TABLE_TYPE) continue;
        shared_ptr<SettingItem> si = nullptr;
        auto group = item.get<string>("Group");
        auto key = item.get<string>("Key");
        auto type = item.get<string>("Type");
        
        switch (crc32_rec(0xffffffff, type.c_str())) {
            case "Integer"_crc32:
                si = make_shared<IntegerSettingItem>(SettingInstance, group, key);
                break;
            case "Float"_crc32:
                si = make_shared<FloatSettingItem>(SettingInstance, group, key);
                break;
            case "Boolean"_crc32:
                si = make_shared<BooleanSettingItem>(SettingInstance, group, key);
                break;
            case "String"_crc32:
                si = make_shared<StringSettingItem>(SettingInstance, group, key);
                break;
            case "IntegerSelect"_crc32:
                si = make_shared<IntegerSelectSettingItem>(SettingInstance, group, key);
                break;
            case "FloatSelect"_crc32:
                si = make_shared<FloatSelectSettingItem>(SettingInstance, group, key);
                break;
            case "StringSelect"_crc32:
                si = make_shared<StringSelectSettingItem>(SettingInstance, group, key);
                break;
            default:
                log->warn(u8"不明な設定タイプです: {0}", type);
                continue;
        }
        si->Build(item);
        Items[si->GetSettingName()] = si;
    }
}

void SettingItemManager::RetrieveAllValues()
{
    for (auto &si : Items) si.second->RetrieveValue();
}

void SettingItemManager::SaveAllValues()
{
    for (auto &si : Items) si.second->SaveValue();
}

shared_ptr<SettingItem> SettingItemManager::GetSettingItem(const string &group, const string &key)
{
    auto skey = fmt::format("{0}.{1}", group, key);
    if (Items.find(skey) != Items.end()) return Items[skey];
    return nullptr;
}

shared_ptr<SettingItem> SettingItemManager::GetSettingItem(const string &name)
{
    if (Items.find(name) != Items.end()) return Items[name];
    return nullptr;
}

// SettingItem

SettingItem::SettingItem(shared_ptr<Setting> setting, const string &group, const string &key)
{
    SettingInstance = setting;
    Group = group;
    Key = key;
    Description = u8"説明はありません";
    FindName = "";
}

void SettingItem::Build(const toml::Value &table)
{
    auto d = table.find("Description");
    if (d && d->is<string>()) {
        Description = d->as<string>();
    }
    auto n = table.find("Name");
    if (n && n->is<string>()) {
        FindName = n->as<string>();
    }
}

string SettingItem::GetSettingName()
{
    if (FindName != "") return FindName;
    return fmt::format("{0}.{1}", Group, Key);
}

string SettingItem::GetDescription()
{
    return Description;
}

// IntegerSettingItem

IntegerSettingItem::IntegerSettingItem(std::shared_ptr<Setting> setting, const std::string &group, const std::string &key) : SettingItem(setting, group, key)
{
    Value = Default = 0;
    MinValue = MaxValue = 0;
    Step = 1;
    Type = SettingItemType::Integer;
}

string IntegerSettingItem::GetItemString()
{
    return fmt::format("{0}", Value);
}

void IntegerSettingItem::MoveNext()
{
    Value += Step;
    if (Value > MaxValue) Value = MaxValue;
}

void IntegerSettingItem::MovePrevious()
{
    Value -= Step;
    if (Value < MinValue) Value = MinValue;
}

void IntegerSettingItem::SaveValue()
{
    SettingInstance->WriteValue<int64_t>(Group, Key, Value);
}

void IntegerSettingItem::RetrieveValue()
{
    Value = SettingInstance->ReadValue(Group, Key, Default);
}

void IntegerSettingItem::Build(const toml::Value &table)
{
    SettingItem::Build(table);
    auto r = table.find("Range");
    if (r && r->is<vector<int64_t>>()) {
        auto v = r->as<vector<int64_t>>();
        MinValue = v[0];
        MaxValue = v[1];
    }
    auto s = table.find("Step");
    if (s && s->is<int64_t>()) {
        Step = s->as<int64_t>();
    }
    auto d = table.find("Default");
    if (d && d->is<int64_t>()) {
        Default = d->as<int64_t>();
    }
}

// FloatSettingItem

FloatSettingItem::FloatSettingItem(std::shared_ptr<Setting> setting, const std::string &group, const std::string &key) : SettingItem(setting, group, key)
{
    Value = Default = 0;
    MinValue = MaxValue = 0;
    Step = 1;
    Type = SettingItemType::Float;
}

string FloatSettingItem::GetItemString()
{
    return fmt::format("{0}", Value);
}

void FloatSettingItem::MoveNext()
{
    Value += Step;
    if (Value > MaxValue) Value = MaxValue;
}

void FloatSettingItem::MovePrevious()
{
    Value -= Step;
    if (Value < MinValue) Value = MinValue;
}

void FloatSettingItem::SaveValue()
{
    SettingInstance->WriteValue<double>(Group, Key, Value);
}

void FloatSettingItem::RetrieveValue()
{
    Value = SettingInstance->ReadValue(Group, Key, Default);
}

void FloatSettingItem::Build(const toml::Value &table)
{
    SettingItem::Build(table);
    auto r = table.find("Range");
    if (r && r->is<vector<double>>()) {
        auto v = r->as<vector<double>>();
        MinValue = v[0];
        MaxValue = v[1];
    }
    auto s = table.find("Step");
    if (s && s->is<double>()) {
        Step = s->as<double>();
    }
    auto d = table.find("Default");
    if (d && d->is<double>()) {
        Default = d->as<double>();
    }
}

// BooleanSettingItem

BooleanSettingItem::BooleanSettingItem(std::shared_ptr<Setting> setting, const std::string &group, const std::string &key) : SettingItem(setting, group, key)
{
    Value = Default = false;
    Falsy = Truthy = "";
    Type = SettingItemType::Boolean;
}

string BooleanSettingItem::GetItemString()
{
    return fmt::format("{0}", Value ? Truthy : Falsy);
}

void BooleanSettingItem::MoveNext()
{
    Value = !Value;
}

void BooleanSettingItem::MovePrevious()
{
    Value = !Value;
}

void BooleanSettingItem::SaveValue()
{
    SettingInstance->WriteValue<bool>(Group, Key, Value);
}

void BooleanSettingItem::RetrieveValue()
{
    Value = SettingInstance->ReadValue(Group, Key, Default);
}

void BooleanSettingItem::Build(const toml::Value &table)
{
    SettingItem::Build(table);
    auto r = table.find("Values");
    if (r && r->is<vector<string>>()) {
        auto v = r->as<vector<string>>();
        Truthy = v[0];
        Falsy = v[1];
    }
    auto d = table.find("Default");
    if (d && d->is<bool>()) {
        Default = d->as<bool>();
    }
}

// StringSettingItem

StringSettingItem::StringSettingItem(std::shared_ptr<Setting> setting, const std::string &group, const std::string &key) : SettingItem(setting, group, key)
{
    Value = Default = "";
    Type = SettingItemType::String;
}

string StringSettingItem::GetItemString()
{
    return fmt::format("{0}", Value);
}

void StringSettingItem::MoveNext()
{
}

void StringSettingItem::MovePrevious()
{
}

void StringSettingItem::SaveValue()
{
    SettingInstance->WriteValue<string>(Group, Key, Value);
}

void StringSettingItem::RetrieveValue()
{
    Value = SettingInstance->ReadValue(Group, Key, Default);
}

void StringSettingItem::Build(const toml::Value &table)
{
    SettingItem::Build(table);
    auto d = table.find("Default");
    if (d && d->is<string>()) {
        Default = d->as<string>();
    }
}

// IntegerSelectSettingItem

IntegerSelectSettingItem::IntegerSelectSettingItem(std::shared_ptr<Setting> setting, const std::string &group, const std::string &key) : SettingItem(setting, group, key)
{
    Value = Default = 0;
    Type = SettingItemType::IntegerSelect;
}

string IntegerSelectSettingItem::GetItemString()
{
    return fmt::format("{0}", Value);
}

void IntegerSelectSettingItem::MoveNext()
{
    Selected = (Selected + Values.size() + 1) % Values.size();
    Value = Values[Selected];
}

void IntegerSelectSettingItem::MovePrevious()
{
    Selected = (Selected + Values.size() - 1) % Values.size();
    Value = Values[Selected];
}

void IntegerSelectSettingItem::SaveValue()
{
    SettingInstance->WriteValue<int64_t>(Group, Key, Value);
}

void IntegerSelectSettingItem::RetrieveValue()
{
    Value = SettingInstance->ReadValue(Group, Key, Default);
}

void IntegerSelectSettingItem::Build(const toml::Value &table)
{
    SettingItem::Build(table);
    auto r = table.find("Values");
    if (r && r->is<vector<int64_t>>()) {
        auto v = r->as<vector<int64_t>>();
        for (const auto &val : v) Values.push_back(val);
    }
    auto d = table.find("Default");
    if (d && d->is<int64_t>()) {
        Default = d->as<int64_t>();
    }
    Selected = 0;
    if (Values.size() == 0) Values.push_back(0);
}

// FloatSelectSettingItem

FloatSelectSettingItem::FloatSelectSettingItem(std::shared_ptr<Setting> setting, const std::string &group, const std::string &key) : SettingItem(setting, group, key)
{
    Value = Default = 0;
    Type = SettingItemType::FloatSelect;
}

string FloatSelectSettingItem::GetItemString()
{
    return fmt::format("{0}", Value);
}

void FloatSelectSettingItem::MoveNext()
{
    Selected = (Selected + Values.size() + 1) % Values.size();
    Value = Values[Selected];
}

void FloatSelectSettingItem::MovePrevious()
{
    Selected = (Selected + Values.size() - 1) % Values.size();
    Value = Values[Selected];
}

void FloatSelectSettingItem::SaveValue()
{
    SettingInstance->WriteValue<double>(Group, Key, Value);
}

void FloatSelectSettingItem::RetrieveValue()
{
    Value = SettingInstance->ReadValue(Group, Key, Default);
}

void FloatSelectSettingItem::Build(const toml::Value &table)
{
    SettingItem::Build(table);
    auto r = table.find("Values");
    if (r && r->is<vector<double>>()) {
        auto v = r->as<vector<double>>();
        for (const auto &val : v) Values.push_back(val);
    }
    auto d = table.find("Default");
    if (d && d->is<double>()) {
        Default = d->as<double>();
    }
    Selected = 0;
    if (Values.size() == 0) Values.push_back(0);
}

// StringSelectSettingItem

StringSelectSettingItem::StringSelectSettingItem(std::shared_ptr<Setting> setting, const std::string &group, const std::string &key) : SettingItem(setting, group, key)
{
    Value = Default = "";
    Type = SettingItemType::StringSelect;
}

string StringSelectSettingItem::GetItemString()
{
    return fmt::format("{0}", Value);
}

void StringSelectSettingItem::MoveNext()
{
    Selected = (Selected + Values.size() + 1) % Values.size();
    Value = Values[Selected];
}

void StringSelectSettingItem::MovePrevious()
{
    Selected = (Selected + Values.size() - 1) % Values.size();
    Value = Values[Selected];
}

void StringSelectSettingItem::SaveValue()
{
    SettingInstance->WriteValue<string>(Group, Key, Value);
}

void StringSelectSettingItem::RetrieveValue()
{
    Value = SettingInstance->ReadValue(Group, Key, Default);
}

void StringSelectSettingItem::Build(const toml::Value &table)
{
    SettingItem::Build(table);
    auto r = table.find("Values");
    if (r && r->is<vector<string>>()) {
        auto v = r->as<vector<string>>();
        for (const auto &val : v) Values.push_back(val);
    }
    auto d = table.find("Default");
    if (d && d->is<string>()) {
        Default = d->as<string>();
    }
    Selected = 0;
    if (Values.size() == 0) Values.push_back(0);
}

}