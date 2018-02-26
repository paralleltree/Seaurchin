#pragma once

#define SU_SETTING_GENERAL "General"
#define SU_SETTING_SKIN "Skin"

class Setting final {
private:
    static std::wstring RootDirectory;
    std::wstring file;
    toml::Value SettingTree;

public:
    Setting();
    Setting(HMODULE hModule);
    void Load(const std::wstring &filename);
    inline void Reload() { Load(file); }
    void Save() const;
    static const std::wstring GetRootDirectory();

    template<typename T>
    T ReadValue(const std::string &group, const std::string &key, T defValue)
    {
        auto v = SettingTree.find(group + "." + key);
        if (v && v->is<T>()) {
            return v->as<T>();
        } else {
            WriteValue(group, key, defValue);
            return defValue;
        }
    }

    template<typename T>
    void WriteValue(const std::string &group, const std::string &key, T value)
    {
        SettingTree.set(group + "." + key, value);
    }
};

namespace Setting2
{

enum class SettingItemType {
    Integer,
    Float,
    Boolean,
    String,
    IntegerSelect,
    FloatSelect,
    StringSelect,
};

class SettingItem {
protected:
    std::shared_ptr<Setting> SettingInstance;
    SettingItemType Type;
    std::string Description;
    std::string Group;
    std::string Key;
    std::string FindName;

public:
    SettingItem(std::shared_ptr<Setting> setting, const std::string &group, const std::string &key);
    std::string GetSettingName();
    std::string GetDescription();

    virtual std::string GetItemString() = 0;
    virtual void MoveNext() = 0;
    virtual void MovePrevious() = 0;
    virtual void SaveValue() = 0;
    virtual void RetrieveValue() = 0;
    virtual void Build(const toml::Value &table);
};

class IntegerSettingItem final : public SettingItem {
private:
    int64_t Value;
    int64_t MinValue;
    int64_t MaxValue;
    int64_t Step;
    int64_t Default;

public:
    IntegerSettingItem(std::shared_ptr<Setting> setting, const std::string &group, const std::string &key);
    std::string GetItemString() override;
    void MoveNext() override;
    void MovePrevious() override;
    void SaveValue() override;
    void RetrieveValue() override;
    void Build(const toml::Value &table) override;
};

class FloatSettingItem final : public SettingItem {
private:
    double Value;
    double MinValue;
    double MaxValue;
    double Step;
    double Default;

public:
    FloatSettingItem(std::shared_ptr<Setting> setting, const std::string &group, const std::string &key);
    std::string GetItemString() override;
    void MoveNext() override;
    void MovePrevious() override;
    void SaveValue() override;
    void RetrieveValue() override;
    void Build(const toml::Value &table) override;
};

class BooleanSettingItem final : public SettingItem {
private:
    bool Value;
    std::string Truthy;
    std::string Falsy;
    bool Default;

public:
    BooleanSettingItem(std::shared_ptr<Setting> setting, const std::string &group, const std::string &key);
    std::string GetItemString() override;
    void MoveNext() override;
    void MovePrevious() override;
    void SaveValue() override;
    void RetrieveValue() override;
    void Build(const toml::Value &table) override;
};

class StringSettingItem final : public SettingItem {
private:
    std::string Value;
    bool Default;

public:
    StringSettingItem(std::shared_ptr<Setting> setting, const std::string &group, const std::string &key);
    std::string GetItemString() override;
    void MoveNext() override;
    void MovePrevious() override;
    void SaveValue() override;
    void RetrieveValue() override;
    void Build(const toml::Value &table) override;
};

class SettingItemManager final {
private:
    std::shared_ptr<Setting> SettingInstance;
    std::unordered_map<std::string, std::shared_ptr<SettingItem>> Items;

public:
    SettingItemManager(std::shared_ptr<Setting> setting);
    void LoadItemsFromToml(boost::filesystem::path file);
    void RetrieveAllValues();
    void SaveAllValues();

    std::shared_ptr<SettingItem> GetSettingItem(const std::string &group, const std::string &key);
    std::shared_ptr<SettingItem> GetSettingItem(const std::string &name);
};

}