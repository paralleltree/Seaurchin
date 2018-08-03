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

    inline asIScriptEngine* GetEngine() const { return engine; }
    inline asIScriptContext* GetContext() const { return sharedContext; }

    //新しくModuleする
    void StartBuildModule(const std::string &name, IncludeCallback callback);

    inline asIScriptModule* GetExistModule(std::string name) const { return engine->GetModule(name.c_str()); }
    
    //ファイル読み込み
    void LoadFile(const std::wstring &filename);
    
    //外から使わないで
    bool IncludeFile(const std::wstring &include, const std::wstring &from);
    
    //ビルドする
    bool FinishBuildModule();
    
    //FinishしたModuleを取得
    inline asIScriptModule* GetLastModule() { return builder.GetModule(); }

    //特定クラスにメタデータが付与されてるか
    bool CheckMetaData(asITypeInfo *type, std::string meta);
    
    //特定グロ関に(ry
    bool CheckMetaData(asIScriptFunction *type, std::string meta);


    //実装をチェック
    inline bool CheckImplementation(asITypeInfo *type, std::string name) const
    {
        return type->Implements(engine->GetTypeInfoByName(name.c_str()));
    }
    
    //asITypeInfoからインスタンス作成 リファレンス無しなのでさっさとAddRefしろ
    asIScriptObject* InstantiateObject(asITypeInfo *type) const;
};

struct CallbackObject {
    asIScriptObject *Object;
    asIScriptFunction *Function;
    asITypeInfo *Type;
    asIScriptContext *Context;

    explicit CallbackObject(asIScriptFunction *callback);
    ~CallbackObject();
};