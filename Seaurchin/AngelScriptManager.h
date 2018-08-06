#pragma once

typedef std::function<bool(std::wstring, std::wstring, CWScriptBuilder*)> IncludeCallback;

class AngelScript
{
private:
    asIScriptEngine *engine;
    asIScriptContext *sharedContext;
    CWScriptBuilder builder;
    IncludeCallback includeFunc;

    void ScriptMessageCallback(const asSMessageInfo *message) const;

public:
    AngelScript();
    ~AngelScript();

    asIScriptEngine* GetEngine() const { return engine; }
    asIScriptContext* GetContext() const { return sharedContext; }

    //新しくModuleする
    void StartBuildModule(const std::string &name, IncludeCallback callback);

    asIScriptModule* GetExistModule(std::string name) const { return engine->GetModule(name.c_str()); }
    
    //ファイル読み込み
    void LoadFile(const std::wstring &filename);
    
    //外から使わないで
    bool IncludeFile(const std::wstring &include, const std::wstring &from);
    
    //ビルドする
    bool FinishBuildModule();
    
    //FinishしたModuleを取得
    asIScriptModule* GetLastModule() { return builder.GetModule(); }

    //特定クラスにメタデータが付与されてるか
    bool CheckMetaData(asITypeInfo *type, const std::string &meta);
    
    //特定グロ関に(ry
    bool CheckMetaData(asIScriptFunction *type, const std::string &meta);


    //実装をチェック
    bool CheckImplementation(asITypeInfo *type, std::string name) const
    {
        return type->Implements(engine->GetTypeInfoByName(name.c_str()));
    }
    
    //asITypeInfoからインスタンス作成 リファレンス無しなのでさっさとAddRefしろ
    asIScriptObject* InstantiateObject(asITypeInfo *type) const;
};

// コールバックを管理する
// デストラクタで開放するから心配いらない…はず
struct CallbackObject {
    asIScriptObject *Object;
    asIScriptFunction *Function;
    asITypeInfo *Type;
    asIScriptContext *Context;

    explicit CallbackObject(asIScriptFunction *callback);
    ~CallbackObject();
};