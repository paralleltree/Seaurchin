#pragma once

#define SU_SETTING_GENERAL "General"
#define SU_SETTING_SKIN "Skin"

class Setting final {
private:
    static std::wstring rootDirectory;
    std::wstring file;
    toml::Value settingTree;

public:
    Setting();
    explicit Setting(HMODULE hModule);
    void Load(const std::wstring &filename);
    void Reload() { Load(file); }
    void Save() const;
    static std::wstring GetRootDirectory();

    template<typename T>
    T ReadValue(const std::string &group, const std::string &key, T defValue)
    {
        auto v = settingTree.find(group + "." + key);
        if (v && v->is<T>()) {
            return v->as<T>();
        }
        WriteValue(group, key, defValue);
        return defValue;
    }

    template<typename T>
    void WriteValue(const std::string &group, const std::string &key, T value)
    {
        settingTree.set(group + "." + key, value);
    }
};

namespace setting2
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
    std::shared_ptr<Setting> settingInstance;
    SettingItemType type;
    std::string description;
    std::string group;
    std::string key;
    std::string findName;

public:
    virtual ~SettingItem() = default;
    SettingItem(std::shared_ptr<Setting> setting, const std::string &group, const std::string &key);
    std::string GetSettingName() const;
    std::string GetDescription() const;

    virtual std::string GetItemString() = 0;
    virtual void MoveNext() = 0;
    virtual void MovePrevious() = 0;
    virtual void SaveValue() = 0;
    virtual void RetrieveValue() = 0;
    virtual void Build(const toml::Value &table);
};

class IntegerSettingItem final : public SettingItem {
private:
    int64_t value;
    int64_t minValue;
    int64_t maxValue;
    int64_t step;
    int64_t defaultValue;

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
    double value;
    double minValue;
    double maxValue;
    double step;
    double defaultValue;

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
    bool value;
    std::string truthy;
    std::string falsy;
    bool defaultValue;

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
    std::string value;
    std::string defaultValue;

public:
    StringSettingItem(std::shared_ptr<Setting> setting, const std::string &group, const std::string &key);
    std::string GetItemString() override;
    void MoveNext() override;
    void MovePrevious() override;
    void SaveValue() override;
    void RetrieveValue() override;
    void Build(const toml::Value &table) override;
};

class IntegerSelectSettingItem final : public SettingItem {
private:
    int64_t value;
    int64_t defaultValue;
    int selected = -1;
    std::vector<int64_t> values;

public:
    IntegerSelectSettingItem(std::shared_ptr<Setting> setting, const std::string &group, const std::string &key);
    std::string GetItemString() override;
    void MoveNext() override;
    void MovePrevious() override;
    void SaveValue() override;
    void RetrieveValue() override;
    void Build(const toml::Value &table) override;
};

class FloatSelectSettingItem final : public SettingItem {
private:
    double value;
    double defaultValue;
    int selected = -1;
    std::vector<double> values;

public:
    FloatSelectSettingItem(std::shared_ptr<Setting> setting, const std::string &group, const std::string &key);
    std::string GetItemString() override;
    void MoveNext() override;
    void MovePrevious() override;
    void SaveValue() override;
    void RetrieveValue() override;
    void Build(const toml::Value &table) override;
};

class StringSelectSettingItem final : public SettingItem {
private:
    std::string value;
    std::string defaultValue;
    int selected = -1;
    std::vector<std::string> values;

public:
    StringSelectSettingItem(std::shared_ptr<Setting> setting, const std::string &group, const std::string &key);
    std::string GetItemString() override;
    void MoveNext() override;
    void MovePrevious() override;
    void SaveValue() override;
    void RetrieveValue() override;
    void Build(const toml::Value &table) override;
};


class SettingItemManager final {
private:
    std::shared_ptr<Setting> settingInstance;
    std::unordered_map<std::string, std::shared_ptr<SettingItem>> items;

public:
    explicit SettingItemManager(const std::shared_ptr<Setting>& setting);
    void LoadItemsFromToml(const boost::filesystem::path& file);
    void RetrieveAllValues();
    void SaveAllValues();

    std::shared_ptr<SettingItem> GetSettingItem(const std::string &group, const std::string &key);
    std::shared_ptr<SettingItem> GetSettingItem(const std::string &name);
};

}