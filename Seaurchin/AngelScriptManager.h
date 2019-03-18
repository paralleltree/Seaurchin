#pragma once

typedef std::function<bool(std::wstring, std::wstring, CWScriptBuilder*)> IncludeCallback;

class AngelScript {
private:
    asIScriptEngine * const engine;
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

    //asITypeInfoからインスタンス作成
    asIScriptObject* InstantiateObject(asITypeInfo *type) const;
};

// メンバ関数を管理する
class MethodObject {
public:
    asIScriptContext *Context;
    asIScriptObject *Object;
    asIScriptFunction *Function;

public:
    explicit MethodObject(asIScriptEngine *engine, asIScriptObject *object, asIScriptFunction *method);
    ~MethodObject();
};

// コールバックを管理する
// デストラクタで開放するから心配いらない…はず
class CallbackObject {
public:
    asIScriptContext *Context;
    asIScriptObject *Object;
    asIScriptFunction *Function;
    asITypeInfo *Type;
private:
    bool exists;
    int refcount;

public:
    explicit CallbackObject(asIScriptFunction *callback);
    ~CallbackObject();

    void AddRef() { refcount++; }
    void Release() { if (--refcount == 0) delete this; }
    int GetRefCount() const { return refcount; }

    // オブジェクトが所有しているデリゲート「のみ」を解放する、オブジェクトそのものが解放されるわけではない
    // これを呼び出すとContext等は解放されるので参照しては行けない状態になる
    void Dispose();

    // オブジェクトが所有しているデリゲートが有効かどうかを返す
    // trueを返している間は登録したデリゲートを呼び出してよい
    // falseを返したなら速やかにこのオブジェクトをReleaseすることが望まれる(二重開放しないよう注意)
    bool IsExists() { return exists; }
};
