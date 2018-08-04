#include "Setting.h"
#include "Misc.h"

using namespace std;
using namespace boost::filesystem;
namespace ba = boost::algorithm;

std::wstring Setting::rootDirectory = L"";

Setting::Setting(const HMODULE hModule)
{
    if (rootDirectory != "") return;
    wchar_t directory[MAX_PATH];
    GetModuleFileNameW(hModule, directory, MAX_PATH);
    PathRemoveFileSpecW(directory);
    rootDirectory = directory;
}

Setting::Setting()
{

}

void Setting::Load(const wstring &filename)
{
    auto log = spdlog::get("main");
    file = filename;
    if (!exists(rootDirectory / file)) {
        log->info(u8"設定ファイルを作成しました");
        Save();
    }
    std::ifstream ifs((rootDirectory / file).wstring(), ios::in);
    auto pr = toml::parse(ifs);
    if (!pr.valid()) {
        log->error(u8"設定ファイルの記述が不正です: {0}", pr.errorReason);
    } else {
        settingTree = pr.value;
        log->info(u8"設定ファイルを読み込みました");
    }
    ifs.close();
}

std::wstring Setting::GetRootDirectory()
{
    return rootDirectory;
}

void Setting::Save() const
{
    auto log = spdlog::get("main");
    std::ofstream ofs((rootDirectory / file).wstring(), ios::out);
    if (settingTree.valid()) settingTree.write(&ofs);
    log->info(u8"設定ファイルを保存しました");
    ofs.close();
}

namespace setting2
{

// SettingItemManager

SettingItemManager::SettingItemManager(const shared_ptr<Setting>& setting)
{
    settingInstance = setting;
}

void SettingItemManager::LoadItemsFromToml(const path& file)
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
    const auto items = root.find("SettingItems");
    if (!items || !items->is<toml::Array>()) {
        log->warn(u8"設定定義 {0} に設定項目がありません", ConvertUnicodeToUTF8(file.wstring()));
        return;
    }
    for (const auto &item : items->as<vector<toml::Value>>()) {
        if (item.type() != toml::Value::TABLE_TYPE) continue;
        shared_ptr<SettingItem> si;
        auto group = item.get<string>("Group");
        auto key = item.get<string>("Key");
        auto type = item.get<string>("Type");

        switch (crc32_rec(0xffffffff, type.c_str())) {
            case "Integer"_crc32:
                si = make_shared<IntegerSettingItem>(settingInstance, group, key);
                break;
            case "Float"_crc32:
                si = make_shared<FloatSettingItem>(settingInstance, group, key);
                break;
            case "Boolean"_crc32:
                si = make_shared<BooleanSettingItem>(settingInstance, group, key);
                break;
            case "String"_crc32:
                si = make_shared<StringSettingItem>(settingInstance, group, key);
                break;
            case "IntegerSelect"_crc32:
                si = make_shared<IntegerSelectSettingItem>(settingInstance, group, key);
                break;
            case "FloatSelect"_crc32:
                si = make_shared<FloatSelectSettingItem>(settingInstance, group, key);
                break;
            case "StringSelect"_crc32:
                si = make_shared<StringSelectSettingItem>(settingInstance, group, key);
                break;
            default:
                log->warn(u8"不明な設定タイプです: {0}", type);
                continue;
        }
        si->Build(item);
        const auto name = si->GetSettingName();
        this->items[name] = si;
    }
}

void SettingItemManager::RetrieveAllValues()
{
    for (auto &si : items) si.second->RetrieveValue();
}

void SettingItemManager::SaveAllValues()
{
    for (auto &si : items) si.second->SaveValue();
}

shared_ptr<SettingItem> SettingItemManager::GetSettingItem(const string &group, const string &key)
{
    const auto skey = fmt::format("{0}.{1}", group, key);
    if (items.find(skey) != items.end()) return items[skey];
    return nullptr;
}

shared_ptr<SettingItem> SettingItemManager::GetSettingItem(const string &name)
{
    if (items.find(name) != items.end()) return items[name];
    return nullptr;
}

// SettingItem

SettingItem::SettingItem(const shared_ptr<Setting> setting, const string &igroup, const string &ikey) : type(SettingItemType::Integer)
{
    settingInstance = setting;
    group = igroup;
    key = ikey;
    description = u8"説明はありません";
    findName = "";
}

void SettingItem::Build(const toml::Value &table)
{
    const auto d = table.find("Description");
    if (d && d->is<string>()) {
        description = d->as<string>();
    }
    const auto n = table.find("Name");
    if (n && n->is<string>()) {
        findName = n->as<string>();
    }
}

string SettingItem::GetSettingName() const
{
    if (findName != "") return findName;
    return fmt::format("{0}.{1}", group, key);
}

string SettingItem::GetDescription() const
{
    return description;
}

// IntegerSettingItem

IntegerSettingItem::IntegerSettingItem(const std::shared_ptr<Setting> setting, const std::string &group, const std::string &key) : SettingItem(setting, group, key)
{
    value = defaultValue = 0;
    minValue = maxValue = 0;
    step = 1;
    type = SettingItemType::Integer;
}

string IntegerSettingItem::GetItemString()
{
    return fmt::format("{0}", value);
}

void IntegerSettingItem::MoveNext()
{
    value += step;
    if (value > maxValue) value = maxValue;
}

void IntegerSettingItem::MovePrevious()
{
    value -= step;
    if (value < minValue) value = minValue;
}

void IntegerSettingItem::SaveValue()
{
    settingInstance->WriteValue<int64_t>(group, key, value);
}

void IntegerSettingItem::RetrieveValue()
{
    value = settingInstance->ReadValue(group, key, defaultValue);
}

void IntegerSettingItem::Build(const toml::Value &table)
{
    SettingItem::Build(table);
    const auto r = table.find("Range");
    if (r && r->is<vector<int64_t>>()) {
        auto v = r->as<vector<int64_t>>();
        minValue = v[0];
        maxValue = v[1];
    }
    const auto s = table.find("Step");
    if (s && s->is<int64_t>()) {
        step = s->as<int64_t>();
    }
    const auto d = table.find("Default");
    if (d && d->is<int64_t>()) {
        defaultValue = d->as<int64_t>();
    }
}

// FloatSettingItem

FloatSettingItem::FloatSettingItem(const std::shared_ptr<Setting> setting, const std::string &group, const std::string &key) : SettingItem(setting, group, key)
{
    value = defaultValue = 0;
    minValue = maxValue = 0;
    step = 1;
    type = SettingItemType::Float;
}

string FloatSettingItem::GetItemString()
{
    return fmt::format("{0}", value);
}

void FloatSettingItem::MoveNext()
{
    value += step;
    if (value > maxValue) value = maxValue;
}

void FloatSettingItem::MovePrevious()
{
    value -= step;
    if (value < minValue) value = minValue;
}

void FloatSettingItem::SaveValue()
{
    settingInstance->WriteValue<double>(group, key, value);
}

void FloatSettingItem::RetrieveValue()
{
    value = settingInstance->ReadValue(group, key, defaultValue);
}

void FloatSettingItem::Build(const toml::Value &table)
{
    SettingItem::Build(table);
    const auto r = table.find("Range");
    if (r && r->is<vector<double>>()) {
        auto v = r->as<vector<double>>();
        minValue = v[0];
        maxValue = v[1];
    }
    const auto s = table.find("Step");
    if (s && s->is<double>()) {
        step = s->as<double>();
    }
    const auto d = table.find("Default");
    if (d && d->is<double>()) {
        defaultValue = d->as<double>();
    }
}

// BooleanSettingItem

BooleanSettingItem::BooleanSettingItem(const std::shared_ptr<Setting> setting, const std::string &group, const std::string &key) : SettingItem(setting, group, key)
{
    value = defaultValue = false;
    falsy = truthy = "";
    type = SettingItemType::Boolean;
}

string BooleanSettingItem::GetItemString()
{
    return fmt::format("{0}", value ? truthy : falsy);
}

void BooleanSettingItem::MoveNext()
{
    value = !value;
}

void BooleanSettingItem::MovePrevious()
{
    value = !value;
}

void BooleanSettingItem::SaveValue()
{
    settingInstance->WriteValue<bool>(group, key, value);
}

void BooleanSettingItem::RetrieveValue()
{
    value = settingInstance->ReadValue(group, key, defaultValue);
}

void BooleanSettingItem::Build(const toml::Value &table)
{
    SettingItem::Build(table);
    const auto r = table.find("Values");
    if (r && r->is<vector<string>>()) {
        auto v = r->as<vector<string>>();
        truthy = v[0];
        falsy = v[1];
    }
    const auto d = table.find("Default");
    if (d && d->is<bool>()) {
        defaultValue = d->as<bool>();
    }
}

// StringSettingItem

StringSettingItem::StringSettingItem(const std::shared_ptr<Setting> setting, const std::string &group, const std::string &key) : SettingItem(setting, group, key)
{
    value = defaultValue = "";
    type = SettingItemType::String;
}

string StringSettingItem::GetItemString()
{
    return fmt::format("{0}", value);
}

void StringSettingItem::MoveNext()
{}

void StringSettingItem::MovePrevious()
{}

void StringSettingItem::SaveValue()
{
    settingInstance->WriteValue<string>(group, key, value);
}

void StringSettingItem::RetrieveValue()
{
    value = settingInstance->ReadValue(group, key, defaultValue);
}

void StringSettingItem::Build(const toml::Value &table)
{
    SettingItem::Build(table);
    const auto d = table.find("Default");
    if (d && d->is<string>()) {
        defaultValue = d->as<string>();
    }
}

// IntegerSelectSettingItem

IntegerSelectSettingItem::IntegerSelectSettingItem(const std::shared_ptr<Setting> setting, const std::string &group, const std::string &key) : SettingItem(setting, group, key)
{
    value = defaultValue = 0;
    type = SettingItemType::IntegerSelect;
}

string IntegerSelectSettingItem::GetItemString()
{
    return fmt::format("{0}", value);
}

void IntegerSelectSettingItem::MoveNext()
{
    selected = (selected + values.size() + 1) % values.size();
    value = values[selected];
}

void IntegerSelectSettingItem::MovePrevious()
{
    selected = (selected + values.size() - 1) % values.size();
    value = values[selected];
}

void IntegerSelectSettingItem::SaveValue()
{
    settingInstance->WriteValue<int64_t>(group, key, value);
}

void IntegerSelectSettingItem::RetrieveValue()
{
    value = settingInstance->ReadValue(group, key, defaultValue);
}

void IntegerSelectSettingItem::Build(const toml::Value &table)
{
    SettingItem::Build(table);
    const auto r = table.find("Values");
    if (r && r->is<vector<int64_t>>()) {
        auto v = r->as<vector<int64_t>>();
        for (const auto &val : v) values.push_back(val);
    }
    const auto d = table.find("Default");
    if (d && d->is<int64_t>()) {
        defaultValue = d->as<int64_t>();
    }
    selected = 0;
    if (values.size() == 0) values.push_back(0);
}

// FloatSelectSettingItem

FloatSelectSettingItem::FloatSelectSettingItem(const std::shared_ptr<Setting> setting, const std::string &group, const std::string &key) : SettingItem(setting, group, key)
{
    value = defaultValue = 0;
    type = SettingItemType::FloatSelect;
}

string FloatSelectSettingItem::GetItemString()
{
    return fmt::format("{0}", value);
}

void FloatSelectSettingItem::MoveNext()
{
    selected = (selected + values.size() + 1) % values.size();
    value = values[selected];
}

void FloatSelectSettingItem::MovePrevious()
{
    selected = (selected + values.size() - 1) % values.size();
    value = values[selected];
}

void FloatSelectSettingItem::SaveValue()
{
    settingInstance->WriteValue<double>(group, key, value);
}

void FloatSelectSettingItem::RetrieveValue()
{
    value = settingInstance->ReadValue(group, key, defaultValue);
}

void FloatSelectSettingItem::Build(const toml::Value &table)
{
    SettingItem::Build(table);
    const auto r = table.find("Values");
    if (r && r->is<vector<double>>()) {
        auto v = r->as<vector<double>>();
        for (const auto &val : v) values.push_back(val);
    }
    const auto d = table.find("Default");
    if (d && d->is<double>()) {
        defaultValue = d->as<double>();
    }
    selected = 0;
    if (values.size() == 0) values.push_back(0);
}

// StringSelectSettingItem

StringSelectSettingItem::StringSelectSettingItem(const std::shared_ptr<Setting> setting, const std::string &group, const std::string &key) : SettingItem(setting, group, key)
{
    value = defaultValue = "";
    type = SettingItemType::StringSelect;
}

string StringSelectSettingItem::GetItemString()
{
    return fmt::format("{0}", value);
}

void StringSelectSettingItem::MoveNext()
{
    selected = (selected + values.size() + 1) % values.size();
    value = values[selected];
}

void StringSelectSettingItem::MovePrevious()
{
    selected = (selected + values.size() - 1) % values.size();
    value = values[selected];
}

void StringSelectSettingItem::SaveValue()
{
    settingInstance->WriteValue<string>(group, key, value);
}

void StringSelectSettingItem::RetrieveValue()
{
    value = settingInstance->ReadValue(group, key, defaultValue);
}

void StringSelectSettingItem::Build(const toml::Value &table)
{
    SettingItem::Build(table);
    const auto r = table.find("Values");
    if (r && r->is<vector<string>>()) {
        auto v = r->as<vector<string>>();
        for (const auto &val : v) values.push_back(val);
    }
    const auto d = table.find("Default");
    if (d && d->is<string>()) {
        defaultValue = d->as<string>();
    }
    selected = 0;
    if (values.size() == 0) values.push_back(nullptr);
}

}