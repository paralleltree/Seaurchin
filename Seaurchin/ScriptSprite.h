#pragma once

#include "ScriptSpriteMisc.h"
#include "MoverFunction.h"
#include "ScriptSpriteMover2.h"
#include "ScriptResource.h"

#define SU_IF_COLOR "Color"
#define SU_IF_TF2D "Transform2D"
#define SU_IF_SHAPETYPE "ShapeType"
#define SU_IF_TEXTALIGN "TextAlign"
#define SU_IF_9TYPE "NinePatchType"

#define SU_IF_SPRITE "Sprite"
#define SU_IF_SHAPE "Shape"
#define SU_IF_TXTSPRITE "TextSprite"
#define SU_IF_SYHSPRITE "SynthSprite"
#define SU_IF_CLPSPRITE "ClipSprite"
#define SU_IF_ANIMESPRITE "AnimeSprite"
#define SU_IF_CONTAINER "Container"

struct Mover;
//基底がImageSpriteでもいい気がしてるんだよね正直
class SSprite {
private:
    virtual void DrawBy(const Transform2D &tf, const ColorTint &ct);

protected:
    int reference;
    ScriptSpriteMover2 *mover;

    void CopyParameterFrom(SSprite *original);

public:
    //値(CopyParameterFromで一括)
    Transform2D Transform;
    int32_t ZIndex;
    ColorTint Color;
    bool IsDead;
    bool HasAlpha;

    //参照(手動コピー)
    SImage *Image;

    SSprite();
    virtual ~SSprite();
    void AddRef();
    void Release();
    int GetRefCount() const { return reference; }

    virtual void Dismiss() { IsDead = true; }
    void Revive() { IsDead = false; }

    void SetImage(SImage *img);
    const SImage* GetImage() const;

    void Apply(const std::string &dict);
    void Apply(const CScriptDictionary *dict);
    virtual bool Apply(const std::string &key, double value);
    void Apply(std::vector<std::pair<const std::string, double>> &props);
    void SetPosition(double x, double y);
    void SetOrigin(double x, double y);
    void SetAngle(double rad);
    void SetScale(double scale);
    void SetScale(double scaleX, double scaleY);
    void SetAlpha(double alpha);
    void SetColor(uint8_t r, uint8_t g, uint8_t b);
    void SetColor(double a, uint8_t r, uint8_t g, uint8_t b);

    virtual mover_function::Action GetCustomAction(const std::string &name);
    void AddMove(const std::string &move) const;
    void AbortMove(bool terminate) const;

    virtual void Tick(double delta);
    virtual void Draw();
    virtual void Draw(const Transform2D &parent, const ColorTint &color);
    virtual SSprite* Clone();

    static SSprite* Factory();
    static SSprite* Factory(SImage *img);
    static void RegisterType(asIScriptEngine *engine);
    struct Comparator {
        bool operator()(const SSprite* lhs, const SSprite* rhs) const
        {
            return lhs->ZIndex < rhs->ZIndex;
        }
    };
};

enum class SShapeType {
    Pixel,
    Box,
    BoxFill,
    Oval,
    OvalFill,
};

//任意の多角形などを表示できる
class SShape : public SSprite {
private :
    typedef SSprite Base;

private:
    void DrawBy(const Transform2D &tf, const ColorTint &ct) override;

public:
    SShapeType Type;
    double Width;
    double Height;

    SShape();

    using Base::Apply;
    bool Apply(const std::string &key, double value) override;

    void Draw() override;
    void Draw(const Transform2D &parent, const ColorTint &color) override;
    SShape* Clone() override;

    static SShape* Factory();
    static void RegisterType(asIScriptEngine *engine);
};

enum class STextAlign {
    Top = 0,
    Center = 1,
    Bottom = 2,
    Left = 0,
    Right = 2
};

//文字列をスプライトとして扱います
class STextSprite : public SSprite {
protected:
    SRenderTarget *target;
    SRenderTarget *scrollBuffer;
    std::tuple<double, double, int> size;
    STextAlign horizontalAlignment;
    STextAlign verticalAlignment;
    bool isScrolling;
    int scrollWidth;
    int scrollMargin;
    double scrollSpeed;
    double scrollPosition;
    bool isRich;

    void Refresh();
    void DrawNormal(const Transform2D &tf, const ColorTint &ct);
    void DrawScroll(const Transform2D &tf, const ColorTint &ct);

public:
    SFont * Font;
    std::string Text;

    void SetFont(SFont* font);
    void SetText(const std::string &txt);
    void SetAlignment(STextAlign hori, STextAlign vert);
    void SetRangeScroll(int width, int margin, double pps);
    void SetRich(bool enabled);
    double GetWidth();
    double GetHeight();

    STextSprite();
    ~STextSprite() override;
    void Tick(double delta) override;
    void Draw() override;
    void Draw(const Transform2D &parent, const ColorTint &color) override;
    STextSprite *Clone() override;

    static STextSprite* Factory();
    static STextSprite* Factory(SFont *img, const std::string &str);
    static void RegisterType(asIScriptEngine *engine);
};

//文字入力を扱うスプライトです
//他と違ってDXライブラリのリソースをナマで取得するのであんまりボコボコ使わないでください。
class STextInput : public SSprite {
protected:
    int inputHandle = 0;
    SFont *font = nullptr;
    int selectionStart = -1, selectionEnd = -1;
    int cursor = 0;
    std::string currentRawString = "";

public:
    STextInput();
    ~STextInput() override;
    void SetFont(SFont *font);

    void Activate() const;
    void Draw() override;
    void Tick(double delta) override;

    std::string GetUTF8String() const;

    static STextInput* Factory();
    static STextInput* Factory(SFont *img);
    static void RegisterType(asIScriptEngine *engine);
};

//画像を任意のスプライトから合成してウェイできます
class SSynthSprite : public SSprite {
protected:
    SRenderTarget * target;
    int width;
    int height;

    void DrawBy(const Transform2D &tf, const ColorTint &ct) override;

public:
    SSynthSprite(int w, int h);
    ~SSynthSprite() override;
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

    void Clear();
    void Transfer(SSprite *sprite);
    void Transfer(SImage *image, double x, double y);
    void Draw() override;
    void Draw(const Transform2D &parent, const ColorTint &color) override;
    SSynthSprite *Clone() override;

    static SSynthSprite *Factory(int w, int h);
    static void RegisterType(asIScriptEngine *engine);
};

//画像を任意のスプライトから合成してウェイできます
class SClippingSprite : public SSynthSprite {
private:
    typedef SSynthSprite Base;

protected:
    double u1, v1;
    double u2, v2;

    void DrawBy(const Transform2D &tf, const ColorTint &ct) override;

    static bool ActionMoveRangeTo(SSprite *thisObj, SpriteMoverArgument &args, SpriteMoverData &data, double delta);

public:
    SClippingSprite(int w, int h);

    using Base::Apply;
    bool Apply(const std::string &key, double value) override;

    mover_function::Action GetCustomAction(const std::string &name) override;
    void SetRange(double tx, double ty, double w, double h);
    void Draw() override;
    void Draw(const Transform2D &parent, const ColorTint &color) override;
    SClippingSprite *Clone() override;

    static SClippingSprite *Factory(int w, int h);
    static void RegisterType(asIScriptEngine *engine);
};

class SAnimeSprite : public SSprite {
private:
    typedef SSprite Base;

protected:
    SAnimatedImage *images;
    int loopCount, count;
    double speed;
    double time;

    void DrawBy(const Transform2D &tf, const ColorTint &ct) override;

public:
    SAnimeSprite(SAnimatedImage *img);
    ~SAnimeSprite() override;

    using Base::Apply;
    bool Apply(const std::string &key, double value) override;

    void Draw() override;
    void Draw(const Transform2D &parent, const ColorTint &color) override;
    void Tick(double delta) override;
    SAnimeSprite *Clone() override;
    void SetSpeed(double speed);
    void SetLoopCount(int lc);

    static SAnimeSprite* Factory(SAnimatedImage *image);
    static void RegisterType(asIScriptEngine *engine);
};

enum NinePatchType : uint32_t {
    StretchByRatio = 1,
    StretchByPixel,
    Repeat,
    RepeatAndStretch,
};

class SContainer : public SSprite {
protected:
    std::multiset<SSprite*, SSprite::Comparator> children;

public:
    SContainer();
    ~SContainer() override;

    void AddChild(SSprite *child);
    void Dismiss() override;
    void Tick(double delta) override;
    void Draw() override;
    void Draw(const Transform2D &parent, const ColorTint &color) override;
    SContainer *Clone() override;

    static SContainer* Factory();
    static void RegisterType(asIScriptEngine *engine);
};

template<typename T>
void RegisterSpriteBasic(asIScriptEngine *engine, const char *name)
{
    using namespace std;
    engine->RegisterObjectType(name, 0, asOBJ_REF);
    engine->RegisterObjectBehaviour(name, asBEHAVE_ADDREF, "void f()", asMETHOD(T, AddRef), asCALL_THISCALL);
    engine->RegisterObjectBehaviour(name, asBEHAVE_RELEASE, "void f()", asMETHOD(T, Release), asCALL_THISCALL);

    engine->RegisterObjectProperty(name, SU_IF_COLOR " Color", asOFFSET(T, Color));
    engine->RegisterObjectProperty(name, "bool HasAlpha", asOFFSET(T, HasAlpha));
    engine->RegisterObjectProperty(name, "int Z", asOFFSET(T, ZIndex));
    engine->RegisterObjectProperty(name, SU_IF_TF2D " Transform", asOFFSET(T, Transform));
    engine->RegisterObjectMethod(name, "void SetImage(" SU_IF_IMAGE "@)", asMETHOD(T, SetImage), asCALL_THISCALL);
    //engine->RegisterObjectMethod(name, SU_IF_IMAGE "@ get_Image()", asMETHOD(T, get_Image), asCALL_THISCALL);
    engine->RegisterObjectMethod(name, "void Dismiss()", asMETHOD(T, Dismiss), asCALL_THISCALL);
    engine->RegisterObjectMethod(name, "void Apply(const string &in)", asMETHODPR(T, Apply, (const std::string&), void), asCALL_THISCALL);
    engine->RegisterObjectMethod(name, "void Apply(const dictionary@)", asMETHODPR(T, Apply, (const CScriptDictionary*), void), asCALL_THISCALL);
    engine->RegisterObjectMethod(name, "void SetPosition(double, double)", asMETHOD(T, SetPosition), asCALL_THISCALL);
    engine->RegisterObjectMethod(name, "void SetOrigin(double, double)", asMETHOD(T, SetOrigin), asCALL_THISCALL);
    engine->RegisterObjectMethod(name, "void SetAngle(double)", asMETHOD(T, SetAngle), asCALL_THISCALL);
    engine->RegisterObjectMethod(name, "void SetScale(double)", asMETHODPR(T, SetScale, (double), void), asCALL_THISCALL);
    engine->RegisterObjectMethod(name, "void SetScale(double, double)", asMETHODPR(T, SetScale, (double, double), void), asCALL_THISCALL);
    engine->RegisterObjectMethod(name, "void SetAlpha(double)", asMETHOD(T, SetAlpha), asCALL_THISCALL);
    engine->RegisterObjectMethod(name, "void SetColor(uint8, uint8, uint8)", asMETHODPR(T, SetColor, (uint8_t, uint8_t, uint8_t), void), asCALL_THISCALL);
    engine->RegisterObjectMethod(name, "void SetColor(double, uint8, uint8, uint8)", asMETHODPR(T, SetColor, (double, uint8_t, uint8_t, uint8_t), void), asCALL_THISCALL);
    engine->RegisterObjectMethod(name, "void AddMove(const string &in)", asMETHOD(T, AddMove), asCALL_THISCALL);
    engine->RegisterObjectMethod(name, "void AbortMove(bool = true)", asMETHOD(T, AbortMove), asCALL_THISCALL);
    engine->RegisterObjectMethod(name, (std::string(name) + "@ Clone()").c_str(), asMETHOD(T, Clone), asCALL_THISCALL);
    //engine->RegisterObjectMethod(name, "void Tick(double)", asMETHOD(T, Tick), asCALL_THISCALL);
    //engine->RegisterObjectMethod(name, "void Draw()", asMETHOD(T, Draw), asCALL_THISCALL);
}



class ExecutionManager;
void RegisterScriptSprite(ExecutionManager *exm);
