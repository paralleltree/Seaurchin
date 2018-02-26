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

// SettingItem -----------------------------------------------------------------

SettingItem::SettingItem(shared_ptr<Setting> setting, const string &group, const string &key) : SettingInstance(setting), SettingGroup(group), SettingKey(key)
{

}

// NumberSettingItem -------------------------------------------------------------------

function<string(NumberSettingItem*)> NumberSettingItem::DefaultFormatter = [](NumberSettingItem *item) {

    ostringstream ss;
    ss << setprecision(item->GetFloatDigits()) << item->GetValue();
    return ss.str();
};

NumberSettingItem::NumberSettingItem(shared_ptr<Setting> setting, const string &group, const string &key) : SettingItem(setting, group, key)
{
    Formatter = DefaultFormatter;
    Type = SettingType::IntegerSetting;
}

std::string NumberSettingItem::GetItemString()
{
    return Formatter(this);
}

void NumberSettingItem::MoveNext()
{
    Value += Step;
    if (Value > MaxValue) Value = MaxValue;
}

void NumberSettingItem::MovePrevious()
{
    Value -= Step;
    if (Value < MinValue) Value = MinValue;
}

void NumberSettingItem::SaveValue()
{
    SettingInstance->WriteValue<double>(SettingGroup, SettingKey, Value);
}

void NumberSettingItem::RetrieveValue()
{
    Value = SettingInstance->ReadValue<double>(SettingGroup, SettingKey, 0);
}

void NumberSettingItem::SetStep(double step)
{
    Step = fabs(step);
}

void NumberSettingItem::SetRange(double min, double max)
{
    if (max < min) return;
    MinValue = min;
    MaxValue = max;
}

void NumberSettingItem::SetFloatDigits(int digits)
{
    if (digits < 0) return;
    FloatDigits = digits;
    Type = digits > 0 ? SettingType::FloatSetting : SettingType::IntegerSetting;
}

BooleanSettingItem::BooleanSettingItem(shared_ptr<Setting> setting, const string &group, const string &key) : SettingItem(setting, group, key)
{
    Type = SettingType::BooleanSetting;
}

string BooleanSettingItem::GetItemString()
{
    return Value ? TrueText : FalseText;
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
    SettingInstance->WriteValue<bool>(SettingGroup, SettingKey, Value);
}

void BooleanSettingItem::RetrieveValue()
{
    Value = SettingInstance->ReadValue<bool>(SettingGroup, SettingKey, false);
}

void BooleanSettingItem::SetText(const string &t, const string &f)
{
    TrueText = t;
    FalseText = f;
}

SettingItemManager::SettingItemManager(std::shared_ptr<Setting> setting) : ReferredSetting(setting)
{

}

void SettingItemManager::AddSettingByString(const string &proc)
{
    //B:<Group>:<Key>:Description:YesText:NoText:Default
    //FI:<Group>:<Key>:Description:Min:Max:Step:Default
    auto str = proc;
    vector<string> params;
    ba::erase_all(str, " ");
    ba::split(params, str, boost::is_any_of(":"));
    if (params.size() < 4) return;
    if (params[0] == "B") {
        ReferredSetting->ReadValue(params[1], params[2], ConvertBoolean(params[6]));
        auto si = make_shared<BooleanSettingItem>(ReferredSetting, params[1], params[2]);
        si->SetDescription(params[3]);
        si->SetText(params[4], params[5]);
        si->RetrieveValue();
        Items[params[1] + "." + params[2]] = si;
    } else if (params[0] == "I") {
        ReferredSetting->ReadValue<double>(params[1], params[2], ConvertInteger(params[7]));
        auto si = make_shared<NumberSettingItem>(ReferredSetting, params[1], params[2]);
        si->SetDescription(params[3]);
        si->SetFloatDigits(0);
        si->SetRange(ConvertInteger(params[4]), ConvertInteger(params[5]));
        si->SetStep(ConvertInteger(params[6]));
        si->RetrieveValue();
        Items[params[1] + "." + params[2]] = si;
    } else if (params[0] == "F") {
        ReferredSetting->ReadValue(params[1], params[2], ConvertFloat(params[7]));
        auto si = make_shared<NumberSettingItem>(ReferredSetting, params[1], params[2]);
        si->SetDescription(params[3]);
        si->SetFloatDigits(5);
        si->SetRange(ConvertFloat(params[4]), ConvertFloat(params[5]));
        si->SetStep(ConvertFloat(params[6]));
        si->RetrieveValue();
        Items[params[1] + "." + params[2]] = si;
    }
}

std::shared_ptr<SettingItem> SettingItemManager::GetSettingItem(const string &group, const string &key)
{
    return Items[group + "." + key];
}

void SettingItemManager::RetrieveAllValues()
{
    for (auto &si : Items) si.second->RetrieveValue();
}

void SettingItemManager::SaveAllValues()
{
    for (auto &si : Items) si.second->SaveValue();
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
        shared_ptr<SettingItem> si;
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
                si = make_shared<SettingItem>(SettingInstance, group, key);
                break;
                // TODO: 実装しろ
            case "IntegerSelect"_crc32:
                break;
            case "FloatSelect"_crc32:
                break;
            case "StringSelect"_crc32:
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
    auto d = table.find("Name");
    if (d && d->is<string>()) {
        FindName = d->as<string>();
    }
}

string SettingItem::GetSettingName()
{
    if (FindName != "") return FindName;
    return fmt::format("{0}.{1}", Group, Key);
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
        Falsy = v[0];
        Truthy = v[1];
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
    if (d && d->is<bool>()) {
        Default = d->as<bool>();
    }
}

}