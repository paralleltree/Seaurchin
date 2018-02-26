#include "ScriptSprite.h"
#include "ExecutionManager.h"
#include "Misc.h"

using namespace std;
using namespace crc32_constexpr;
// 一般

void RegisterScriptSprite(ExecutionManager *exm)
{
    auto engine = exm->GetScriptInterfaceUnsafe()->GetEngine();

    SSprite::RegisterType(engine);
    SShape::RegisterType(engine);
    STextSprite::RegisterType(engine);
    SSynthSprite::RegisterType(engine);
    SClippingSprite::RegisterType(engine);
    SAnimeSprite::RegisterType(engine);
    SContainer::RegisterType(engine);
}

//SSprite ------------------

void SSprite::CopyParameterFrom(SSprite * original)
{
    Color = original->Color;
    Transform = original->Transform;
    ZIndex = original->ZIndex;
    IsDead = original->IsDead;
    HasAlpha = original->HasAlpha;
}

void SSprite::set_Image(SImage * img)
{
    if (Image) Image->Release();
    Image = img;
}

const SImage *SSprite::get_Image()
{
    if (Image) Image->AddRef();
    return Image;
}

SSprite::SSprite()
{
    //ZIndex = 0;
    Color = Colors::White;
    mover = new ScriptSpriteMover2(this);
}

SSprite::~SSprite()
{
    //WriteDebugConsole("Destructing ScriptSprite\n");
    if (Image) Image->Release();
    Image = nullptr;
    delete mover;
}

void SSprite::AddRef()
{
    ++Reference;
}

void SSprite::Release()
{
    if (--Reference == 0) delete this;
}

MoverFunction::Action SSprite::GetCustomAction(const string & name)
{
    return nullptr;
}

void SSprite::AddMove(const string & move)
{
    mover->AddMove(move);
}

void SSprite::AbortMove(bool terminate)
{
    mover->Abort(terminate);
}

void SSprite::Apply(const string & dict)
{
    mover->Apply(dict);
}

void SSprite::Apply(const CScriptDictionary & dict)
{
    using namespace crc32_constexpr;
    ostringstream aps;

    auto i = dict.begin();
    while (i != dict.end()) {
        auto key = i.GetKey();
        aps << key << ":";
        double dv = 0;
        i.GetValue(dv);
        aps << dv << ", ";
        i++;
    }

    Apply(aps.str());
}

void SSprite::Tick(double delta)
{
    mover->Tick(delta);
}

void SSprite::Draw()
{
    DrawBy(Transform, Color);
}

void SSprite::Draw(const Transform2D &parent, const ColorTint &color)
{
    auto tf = Transform.ApplyFrom(parent);
    auto cl = Color.ApplyFrom(color);
    DrawBy(tf, cl);
}

void SSprite::DrawBy(const Transform2D &tf, const ColorTint &ct)
{
    if (!Image) return;
    SetDrawBright(ct.R, ct.G, ct.B);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, ct.A);
    DrawRotaGraph3F(
        tf.X, tf.Y,
        tf.OriginX, tf.OriginY,
        tf.ScaleX, tf.ScaleY,
        tf.Angle, Image->GetHandle(),
        HasAlpha ? TRUE : FALSE, FALSE);
}

SSprite * SSprite::Clone()
{
    //コピコンで良くないかこれ
    auto clone = new SSprite();
    clone->AddRef();
    clone->CopyParameterFrom(this);
    if (Image) {
        Image->AddRef();
        clone->set_Image(Image);
    }
    return clone;
}

SSprite * SSprite::Factory()
{
    auto result = new SSprite();
    result->AddRef();
    return result;
}

SSprite * SSprite::Factory(SImage * img)
{
    auto result = new SSprite();
    result->set_Image(img);
    result->AddRef();
    return result;
}

ColorTint GetColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return ColorTint { a, r, g, b };
}

void SSprite::RegisterType(asIScriptEngine * engine)
{
    //Transform2D
    engine->RegisterObjectType(SU_IF_TF2D, sizeof(Transform2D), asOBJ_VALUE | asOBJ_APP_CLASS_CD);
    engine->RegisterObjectBehaviour(SU_IF_TF2D, asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(AngelScriptValueConstruct<Transform2D>), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectBehaviour(SU_IF_TF2D, asBEHAVE_DESTRUCT, "void f()", asFUNCTION(AngelScriptValueDestruct<Transform2D>), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectProperty(SU_IF_TF2D, "double X", asOFFSET(Transform2D, X));
    engine->RegisterObjectProperty(SU_IF_TF2D, "double Y", asOFFSET(Transform2D, Y));
    engine->RegisterObjectProperty(SU_IF_TF2D, "double Angle", asOFFSET(Transform2D, Angle));
    engine->RegisterObjectProperty(SU_IF_TF2D, "double OriginX", asOFFSET(Transform2D, OriginX));
    engine->RegisterObjectProperty(SU_IF_TF2D, "double OriginY", asOFFSET(Transform2D, OriginY));
    engine->RegisterObjectProperty(SU_IF_TF2D, "double ScaleX", asOFFSET(Transform2D, ScaleX));
    engine->RegisterObjectProperty(SU_IF_TF2D, "double ScaleY", asOFFSET(Transform2D, ScaleY));

    //Color
    engine->RegisterObjectType(SU_IF_COLOR, sizeof(ColorTint), asOBJ_VALUE | asOBJ_APP_CLASS_CD);
    engine->RegisterObjectBehaviour(SU_IF_COLOR, asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(AngelScriptValueConstruct<ColorTint>), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectBehaviour(SU_IF_COLOR, asBEHAVE_DESTRUCT, "void f()", asFUNCTION(AngelScriptValueConstruct<ColorTint>), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectProperty(SU_IF_COLOR, "uint8 A", asOFFSET(ColorTint, A));
    engine->RegisterObjectProperty(SU_IF_COLOR, "uint8 R", asOFFSET(ColorTint, R));
    engine->RegisterObjectProperty(SU_IF_COLOR, "uint8 G", asOFFSET(ColorTint, G));
    engine->RegisterObjectProperty(SU_IF_COLOR, "uint8 B", asOFFSET(ColorTint, B));

    //ShapeType
    engine->RegisterEnum(SU_IF_SHAPETYPE);
    engine->RegisterEnumValue(SU_IF_SHAPETYPE, "Pixel", SShapeType::Pixel);
    engine->RegisterEnumValue(SU_IF_SHAPETYPE, "Box", SShapeType::Box);
    engine->RegisterEnumValue(SU_IF_SHAPETYPE, "BoxFill", SShapeType::BoxFill);
    engine->RegisterEnumValue(SU_IF_SHAPETYPE, "Oval", SShapeType::Oval);
    engine->RegisterEnumValue(SU_IF_SHAPETYPE, "OvalFill", SShapeType::OvalFill);

    //ShapeType
    engine->RegisterEnum(SU_IF_TEXTALIGN);
    engine->RegisterEnumValue(SU_IF_TEXTALIGN, "Top", (int)STextAlign::Top);
    engine->RegisterEnumValue(SU_IF_TEXTALIGN, "Bottom", (int)STextAlign::Bottom);
    engine->RegisterEnumValue(SU_IF_TEXTALIGN, "Left", (int)STextAlign::Left);
    engine->RegisterEnumValue(SU_IF_TEXTALIGN, "Right", (int)STextAlign::Right);
    engine->RegisterEnumValue(SU_IF_TEXTALIGN, "Center", (int)STextAlign::Center);

    RegisterSpriteBasic<SSprite>(engine, SU_IF_SPRITE);
    engine->RegisterObjectBehaviour(SU_IF_SPRITE, asBEHAVE_FACTORY, SU_IF_SPRITE "@ f()", asFUNCTIONPR(SSprite::Factory, (), SSprite*), asCALL_CDECL);
    engine->RegisterObjectBehaviour(SU_IF_SPRITE, asBEHAVE_FACTORY, SU_IF_SPRITE "@ f(" SU_IF_IMAGE "@)", asFUNCTIONPR(SSprite::Factory, (SImage*), SSprite*), asCALL_CDECL);
    engine->RegisterObjectMethod(SU_IF_SPRITE, SU_IF_SPRITE "@ Clone()", asMETHOD(SSprite, Clone), asCALL_THISCALL);
}

// Shape -----------------

void SShape::DrawBy(const Transform2D & tf, const ColorTint & ct)
{
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, ct.A);
    switch (Type) {
        case SShapeType::Pixel:
            DrawPixel(tf.X, tf.Y, GetColor(ct.R, ct.G, ct.B));
            break;
        case SShapeType::Box:
            DrawBoxAA(
                tf.X - Width / 2, tf.Y - Height / 2,
                tf.X + Width / 2, tf.Y + Height / 2,
                GetColor(ct.R, ct.G, ct.B), FALSE);
            break;
        case SShapeType::BoxFill:
            DrawBoxAA(
                tf.X - Width / 2, tf.Y - Height / 2,
                tf.X + Width / 2, tf.Y + Height / 2,
                GetColor(ct.R, ct.G, ct.B), TRUE);
            break;
        case SShapeType::Oval:
            DrawOvalAA(
                tf.X, tf.Y,
                Width / 2, Height / 2,
                256, GetColor(ct.R, ct.G, ct.B), FALSE);
            break;
        case SShapeType::OvalFill:
            DrawOvalAA(
                tf.X, tf.Y,
                Width / 2, Height / 2,
                256, GetColor(ct.R, ct.G, ct.B), TRUE);
            break;
    }
}

void SShape::Draw()
{
    DrawBy(Transform, Color);
}

void SShape::Draw(const Transform2D & parent, const ColorTint & color)
{
    auto tf = Transform.ApplyFrom(parent);
    auto cl = Color.ApplyFrom(color);
    DrawBy(tf, cl);
}

SShape * SShape::Factory()
{
    auto result = new SShape();
    result->AddRef();
    return result;
}

void SShape::RegisterType(asIScriptEngine * engine)
{
    RegisterSpriteBasic<SShape>(engine, SU_IF_SHAPE);
    engine->RegisterObjectBehaviour(SU_IF_SHAPE, asBEHAVE_FACTORY, SU_IF_SHAPE "@ f()", asFUNCTIONPR(SShape::Factory, (), SShape*), asCALL_CDECL);
    engine->RegisterObjectMethod(SU_IF_SPRITE, SU_IF_SHAPE "@ opCast()", asFUNCTION((CastReferenceType<SSprite, SShape>)), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod(SU_IF_SHAPE, SU_IF_SPRITE "@ opImplCast()", asFUNCTION((CastReferenceType<SShape, SSprite>)), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectProperty(SU_IF_SHAPE, "double Width", asOFFSET(SShape, Width));
    engine->RegisterObjectProperty(SU_IF_SHAPE, "double Height", asOFFSET(SShape, Height));
    engine->RegisterObjectProperty(SU_IF_SHAPE, SU_IF_SHAPETYPE " Type", asOFFSET(SShape, Type));
}

// TextSprite -----------

void STextSprite::Refresh()
{
    if (!Font) return;
    if (Target) delete Target;
    if (ScrollBuffer) delete ScrollBuffer;
    
    Size = IsRich ? Font->RenderRich(nullptr, Text, Color) : Font->RenderRaw(nullptr, Text);
    if (IsScrolling) {
        ScrollBuffer = new SRenderTarget(ScrollWidth, (int)get<1>(Size));
    }
    
    Target = new SRenderTarget((int)get<0>(Size), (int)get<1>(Size));
    if (IsRich) {
        Font->RenderRich(Target, Text, Color);
    } else {
        Font->RenderRaw(Target, Text);
    }
}

void STextSprite::DrawNormal(const Transform2D &tf, const ColorTint &ct)
{
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, ct.A);
    SetDrawMode(DX_DRAWMODE_ANISOTROPIC);
    if (IsRich) {
        SetDrawBright(255, 255, 255);
    } else {
        SetDrawBright(ct.R, ct.G, ct.B);
    }
    double tox = get<0>(Size) / 2 * (int)HorizontalAlignment;
    double toy = get<1>(Size) / 2 * (int)VerticalAlignment;
    DrawRotaGraph3F(
        tf.X, tf.Y,
        tf.OriginX + tox, tf.OriginY + toy,
        tf.ScaleX, tf.ScaleY,
        tf.Angle, Target->GetHandle(), TRUE, FALSE);
}

void STextSprite::DrawScroll(const Transform2D &tf, const ColorTint &ct)
{
    int pds = GetDrawScreen();
    SetDrawScreen(ScrollBuffer->GetHandle());
    SetDrawBright(255, 255, 255);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
    ClearDrawScreen();
    if (ScrollSpeed >= 0) {
        double reach = -ScrollPosition + (int)(ScrollPosition / (get<0>(Size) + ScrollMargin)) * (get<0>(Size) + ScrollMargin);
        while (reach < ScrollWidth) {
            DrawRectGraphF(
                reach, 0,
                0, 0, get<0>(Size), get<1>(Size),
                Target->GetHandle(), TRUE, FALSE);
            reach += get<0>(Size) + ScrollMargin;
        }
    } else {
        double reach = -ScrollPosition - (int)(ScrollPosition / (get<0>(Size) + ScrollMargin)) * (get<0>(Size) + ScrollMargin);
        while (reach > 0) {
            DrawRectGraphF(
                reach, 0,
                0, 0, get<0>(Size), get<1>(Size),
                Target->GetHandle(), TRUE, FALSE);
            reach -= get<0>(Size) + ScrollMargin;
        }
    }
    SetDrawScreen(pds);

    if (IsRich) {
        SetDrawBright(255, 255, 255);
    } else {
        SetDrawBright(ct.R, ct.G, ct.B);
    }
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, ct.A);
    SetDrawMode(DX_DRAWMODE_ANISOTROPIC);
    double tox = ScrollWidth / 2 * (int)HorizontalAlignment;
    double toy = get<1>(Size) / 2 * (int)VerticalAlignment;
    DrawRotaGraph3F(
        tf.X, tf.Y,
        tf.OriginX + tox, tf.OriginY + toy,
        tf.ScaleX, tf.ScaleY,
        tf.Angle, ScrollBuffer->GetHandle(), TRUE, FALSE);
}

void STextSprite::set_Font(SFont * font)
{
    if (Font) Font->Release();
    Font = font;
    Refresh();
}

void STextSprite::set_Text(const string & txt)
{
    Text = txt;
    Refresh();
}

void STextSprite::SetAlignment(STextAlign hori, STextAlign vert)
{
    HorizontalAlignment = hori;
    VerticalAlignment = vert;
}

void STextSprite::SetRangeScroll(int width, int margin, double pps)
{
    IsScrolling = width > 0;
    ScrollWidth = width;
    ScrollMargin = margin;
    ScrollSpeed = pps;
    ScrollPosition = 0;
    Refresh();
}

void STextSprite::SetRich(bool enabled)
{
    IsRich = enabled;
    Refresh();
}

STextSprite::~STextSprite()
{
    if (Font) Font->Release();
    if (Target) delete Target;
    if (ScrollBuffer) delete ScrollBuffer;
}

void STextSprite::Tick(double delta)
{
    SSprite::Tick(delta);
    if (IsScrolling) ScrollPosition += ScrollSpeed * delta;
}

void STextSprite::Draw()
{
    if (!Target) return;
    if (IsScrolling && Target->get_Width() >= ScrollWidth) {
        DrawScroll(Transform, Color);
    } else {
        DrawNormal(Transform, Color);
    }
}

void STextSprite::Draw(const Transform2D & parent, const ColorTint & color)
{
    auto tf = Transform.ApplyFrom(parent);
    auto cl = Color.ApplyFrom(color);
    if (!Target) return;
    if (IsScrolling && Target->get_Width() >= ScrollWidth) {
        DrawScroll(tf, cl);
    } else {
        DrawNormal(tf, cl);
    }
}

STextSprite * STextSprite::Clone()
{
    //やっぱりコピコンで良くないかこれ
    auto clone = new STextSprite();
    clone->CopyParameterFrom(this);
    clone->AddRef();
    if (Font) {
        Font->AddRef();
        clone->set_Font(Font);
    }
    if (Target) {
        clone->set_Text(Text);
    }
    return clone;
}

STextSprite *STextSprite::Factory()
{
    auto result = new STextSprite();
    result->AddRef();
    return result;
}

STextSprite *STextSprite::Factory(SFont * img, const string & str)
{
    auto result = new STextSprite();
    result->set_Font(img);
    result->set_Text(str);
    result->AddRef();
    return result;
}

void STextSprite::RegisterType(asIScriptEngine * engine)
{
    RegisterSpriteBasic<STextSprite>(engine, SU_IF_TXTSPRITE);
    engine->RegisterObjectBehaviour(SU_IF_TXTSPRITE, asBEHAVE_FACTORY, SU_IF_TXTSPRITE "@ f()", asFUNCTIONPR(STextSprite::Factory, (), STextSprite*), asCALL_CDECL);
    engine->RegisterObjectBehaviour(SU_IF_TXTSPRITE, asBEHAVE_FACTORY, SU_IF_TXTSPRITE "@ f(" SU_IF_FONT "@, const string &in)", asFUNCTIONPR(STextSprite::Factory, (SFont*, const string&), STextSprite*), asCALL_CDECL);
    engine->RegisterObjectMethod(SU_IF_SPRITE, SU_IF_TXTSPRITE "@ opCast()", asFUNCTION((CastReferenceType<SSprite, STextSprite>)), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod(SU_IF_TXTSPRITE, SU_IF_SPRITE "@ opImplCast()", asFUNCTION((CastReferenceType<STextSprite, SSprite>)), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod(SU_IF_TXTSPRITE, SU_IF_TXTSPRITE "@ Clone()", asMETHOD(STextSprite, Clone), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_TXTSPRITE, "void SetFont(" SU_IF_FONT "@)", asMETHOD(STextSprite, set_Font), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_TXTSPRITE, "void SetText(const string &in)", asMETHOD(STextSprite, set_Text), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_TXTSPRITE, "void SetAlignment(" SU_IF_TEXTALIGN ", " SU_IF_TEXTALIGN ")", asMETHOD(STextSprite, SetAlignment), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_TXTSPRITE, "void SetRangeScroll(int, int, double)", asMETHOD(STextSprite, SetRangeScroll), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_TXTSPRITE, "void SetRich(bool)", asMETHOD(STextSprite, SetRich), asCALL_THISCALL);
}

// STextInput ---------------------------------------

STextInput::STextInput()
{
    //TODO: デフォルト値の引数化
    InputHandle = MakeKeyInput(1024, TRUE, FALSE, FALSE, FALSE, FALSE);
}

STextInput::~STextInput()
{
    if (InputHandle) DeleteKeyInput(InputHandle);
}

void STextInput::set_Font(SFont * font)
{
    if (Font) Font->Release();
    Font = font;
}

void STextInput::Activate()
{
    SetActiveKeyInput(InputHandle);
}

void STextInput::Draw()
{
    int wcc = MultiByteToWideChar(CP_OEMCP, 0, CurrentRawString.c_str(), 1024, nullptr, 0);
    wchar_t *widestr = new wchar_t[wcc];
    MultiByteToWideChar(CP_OEMCP, 0, CurrentRawString.c_str(), 1024, widestr, 0);
    for (int i = 0; i < wcc; i++) {

    }
    delete[] widestr;
}

void STextInput::Tick(double delta)
{
    TCHAR buffer[1024] = { 0 };
    switch (CheckKeyInput(InputHandle)) {
        case 0:
            GetKeyInputString(buffer, InputHandle);
            CurrentRawString = buffer;
            Cursor = GetKeyInputCursorPosition(InputHandle);
            GetKeyInputSelectArea(&SelectionStart, &SelectionEnd, InputHandle);
            return;
        case 1:
        case 2:
            SelectionStart = SelectionEnd = Cursor = -1;
            return;
    }
}

std::string STextInput::GetUTF8String()
{
    return std::string();
}

STextInput * STextInput::Factory()
{
    return nullptr;
}

STextInput * STextInput::Factory(SFont * img)
{
    return nullptr;
}

void STextInput::RegisterType(asIScriptEngine * engine)
{}


// SSynthSprite -------------------------------------

void SSynthSprite::DrawBy(const Transform2D & tf, const ColorTint & ct)
{
    if (!Target) return;
    SetDrawBright(ct.R, ct.G, ct.B);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, ct.A);
    DrawRotaGraph3F(
        tf.X, tf.Y,
        tf.OriginX, tf.OriginY,
        tf.ScaleX, tf.ScaleY,
        tf.Angle, Target->GetHandle(),
        HasAlpha ? TRUE : FALSE, FALSE);
}

SSynthSprite::SSynthSprite(int w, int h)
{
    Width = w;
    Height = h;
}

SSynthSprite::~SSynthSprite()
{
    if (Target) delete Target;
}

void SSynthSprite::Clear()
{
    if (Target) delete Target;
    Target = new SRenderTarget(Width, Height);
}

void SSynthSprite::Transfer(SSprite *sprite)
{
    if (!sprite) return;
    BEGIN_DRAW_TRANSACTION(Target->GetHandle());
    sprite->Draw();
    FINISH_DRAW_TRANSACTION;
    sprite->Release();
}

void SSynthSprite::Transfer(SImage * image, double x, double y)
{
    if (!image) return;
    BEGIN_DRAW_TRANSACTION(Target->GetHandle());
    SetDrawBright(255, 255, 255);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
    DrawGraphF(x, y, image->GetHandle(), HasAlpha ? TRUE : FALSE);
    FINISH_DRAW_TRANSACTION;
    image->Release();
}

void SSynthSprite::Draw()
{
    DrawBy(Transform, Color);
}

void SSynthSprite::Draw(const Transform2D & parent, const ColorTint & color)
{
    auto tf = Transform.ApplyFrom(parent);
    auto cl = Color.ApplyFrom(color);
    DrawBy(tf, cl);
}

SSynthSprite* SSynthSprite::Clone()
{
    auto clone = new SSynthSprite(Width, Height);
    clone->CopyParameterFrom(this);
    clone->AddRef();
    if (Target) {
        clone->Transfer(this);
    }
    return clone;
}

SSynthSprite *SSynthSprite::Factory(int w, int h)
{
    auto result = new SSynthSprite(w, h);
    result->Clear();
    result->AddRef();
    return result;
}

void SSynthSprite::RegisterType(asIScriptEngine * engine)
{
    RegisterSpriteBasic<SSynthSprite>(engine, SU_IF_SYHSPRITE);
    engine->RegisterObjectBehaviour(SU_IF_SYHSPRITE, asBEHAVE_FACTORY, SU_IF_SYHSPRITE "@ f(int, int)", asFUNCTION(SSynthSprite::Factory), asCALL_CDECL);
    engine->RegisterObjectMethod(SU_IF_SPRITE, SU_IF_SYHSPRITE "@ opCast()", asFUNCTION((CastReferenceType<SSprite, SSynthSprite>)), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod(SU_IF_SYHSPRITE, SU_IF_SPRITE "@ opImplCast()", asFUNCTION((CastReferenceType<SSynthSprite, SSprite>)), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod(SU_IF_SYHSPRITE, "int get_Width()", asMETHOD(SSynthSprite, get_Width), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SYHSPRITE, "int get_Height()", asMETHOD(SSynthSprite, get_Width), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SYHSPRITE, "void Clear()", asMETHOD(SSynthSprite, Clear), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SYHSPRITE, "void Transfer(" SU_IF_SPRITE "@)", asMETHODPR(SSynthSprite, Transfer, (SSprite*), void), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SYHSPRITE, "void Transfer(" SU_IF_IMAGE "@, double, double)", asMETHODPR(SSynthSprite, Transfer, (SImage*, double, double), void), asCALL_THISCALL);
}

// SClippingSprite ------------------------------------------

void SClippingSprite::DrawBy(const Transform2D & tf, const ColorTint & ct)
{
    if (!Target) return;
    double x = Width * U1, y = Height * V1, w = Width * U2, h = Height * V2;
    SetDrawBright(ct.R, ct.G, ct.B);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, ct.A);
    DrawRectRotaGraph3F(
        tf.X, tf.Y,
        x, y, w, h,
        tf.OriginX, tf.OriginY,
        tf.ScaleX, tf.ScaleY,
        tf.Angle, Target->GetHandle(),
        HasAlpha ? TRUE : FALSE, FALSE);
}

bool SClippingSprite::ActionMoveRangeTo(SSprite *thisObj, SpriteMoverArgument &args, SpriteMoverData &data, double delta)
{
    auto target = static_cast<SClippingSprite*>(thisObj);
    if (delta == 0) {
        data.Extra1 = target->U2;
        data.Extra2 = target->V2;
        return false;
    } else if (delta >= 0) {
        target->U2 = args.Ease(data.Now, args.Duration, data.Extra1, args.X - data.Extra1);
        target->V2 = args.Ease(data.Now, args.Duration, data.Extra2, args.Y - data.Extra2);
        return false;
    } else {
        target->U2 = args.X;
        target->V2 = args.Y;
        return true;
    }
}

SClippingSprite::SClippingSprite(int w, int h) : SSynthSprite(w, h)
{

}

MoverFunction::Action SClippingSprite::GetCustomAction(const string & name)
{
    switch (crc32_rec(0xffffffff, name.c_str())) {
        case "range_size"_crc32:
            return ActionMoveRangeTo;
    }
    return nullptr;
}

void SClippingSprite::SetRange(double tx, double ty, double w, double h)
{
    U1 = tx;
    V1 = ty;
    U2 = w;
    V2 = h;
}

void SClippingSprite::Draw()
{
    DrawBy(Transform, Color);
}

void SClippingSprite::Draw(const Transform2D & parent, const ColorTint & color)
{
    auto tf = Transform.ApplyFrom(parent);
    auto cl = Color.ApplyFrom(color);
    DrawBy(tf, cl);
}

SClippingSprite *SClippingSprite::Clone()
{
    auto clone = new SClippingSprite(Width, Height);
    clone->CopyParameterFrom(this);
    clone->AddRef();
    if (Target) {
        clone->Transfer(this);
    }
    clone->U1 = U1;
    clone->V1 = V1;
    clone->U2 = U2;
    clone->V2 = V2;
    return clone;
}

SClippingSprite *SClippingSprite::Factory(int w, int h)
{
    auto result = new SClippingSprite(w, h);
    result->Clear();
    result->AddRef();
    return result;
}

void SClippingSprite::RegisterType(asIScriptEngine * engine)
{
    RegisterSpriteBasic<SClippingSprite>(engine, SU_IF_CLPSPRITE);
    engine->RegisterObjectBehaviour(SU_IF_CLPSPRITE, asBEHAVE_FACTORY, SU_IF_CLPSPRITE "@ f(int, int)", asFUNCTION(SClippingSprite::Factory), asCALL_CDECL);
    engine->RegisterObjectMethod(SU_IF_SPRITE, SU_IF_CLPSPRITE "@ opCast()", asFUNCTION((CastReferenceType<SSprite, SClippingSprite>)), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod(SU_IF_CLPSPRITE, SU_IF_SPRITE "@ opImplCast()", asFUNCTION((CastReferenceType<SClippingSprite, SSprite>)), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod(SU_IF_CLPSPRITE, "int get_Width()", asMETHOD(SClippingSprite, get_Width), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_CLPSPRITE, "int get_Height()", asMETHOD(SClippingSprite, get_Width), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_CLPSPRITE, "void Clear()", asMETHOD(SClippingSprite, Clear), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_CLPSPRITE, "void Transfer(" SU_IF_SPRITE "@)", asMETHODPR(SClippingSprite, Transfer, (SSprite*), void), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_CLPSPRITE, "void Transfer(" SU_IF_IMAGE "@, double, double)", asMETHODPR(SClippingSprite, Transfer, (SImage*, double, double), void), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_CLPSPRITE, "void SetRange(double, double, double, double)", asMETHOD(SClippingSprite, SetRange), asCALL_THISCALL);
}

// SAnimeSprite -------------------------------------------
void SAnimeSprite::DrawBy(const Transform2D & tf, const ColorTint & ct)
{
    double at = Images->get_CellTime() * Images->get_FrameCount() - Time;
    auto ih = Images->GetImageHandleAt(at);
    if (!ih) return;
    SetDrawBright(Color.R, Color.G, Color.B);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, Color.A);
    DrawRotaGraph3F(
        Transform.X, Transform.Y,
        Transform.OriginX, Transform.OriginY,
        Transform.ScaleX, Transform.ScaleY,
        Transform.Angle, ih,
        HasAlpha ? TRUE : FALSE, FALSE);
}

SAnimeSprite::SAnimeSprite(SAnimatedImage * img)
{
    Images = img;
    LoopCount = 1;
    Speed = 1;
    Time = img->get_CellTime() * img->get_FrameCount();
}

SAnimeSprite::~SAnimeSprite()
{
    if (Images) Images->Release();
}

void SAnimeSprite::Draw()
{
    DrawBy(Transform, Color);
}

void SAnimeSprite::Draw(const Transform2D & parent, const ColorTint & color)
{
    auto tf = Transform.ApplyFrom(parent);
    auto cl = Color.ApplyFrom(color);
    DrawBy(tf, cl);
}

void SAnimeSprite::Tick(double delta)
{
    Time -= delta * Speed;
    if (Time > 0) return;
    LoopCount--;
    if (!LoopCount) Dismiss();
    Time = Images->get_CellTime() * Images->get_FrameCount();
}

void SAnimeSprite::SetSpeed(double speed)
{
    Speed = speed;
}

void SAnimeSprite::SetLoopCount(int lc)
{
    LoopCount = lc;
}

SAnimeSprite * SAnimeSprite::Factory(SAnimatedImage * image)
{
    auto result = new SAnimeSprite(image);
    result->AddRef();
    return result;
}

void SAnimeSprite::RegisterType(asIScriptEngine * engine)
{
    RegisterSpriteBasic<SAnimeSprite>(engine, SU_IF_ANIMESPRITE);
    engine->RegisterObjectBehaviour(SU_IF_ANIMESPRITE, asBEHAVE_FACTORY, SU_IF_ANIMESPRITE "@ f(" SU_IF_ANIMEIMAGE "@)", asFUNCTIONPR(SAnimeSprite::Factory, (SAnimatedImage*), SAnimeSprite*), asCALL_CDECL);
    engine->RegisterObjectMethod(SU_IF_SPRITE, SU_IF_ANIMESPRITE "@ opCast()", asFUNCTION((CastReferenceType<SSprite, SAnimeSprite>)), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod(SU_IF_ANIMESPRITE, SU_IF_SPRITE "@ opImplCast()", asFUNCTION((CastReferenceType<SAnimeSprite, SSprite>)), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod(SU_IF_ANIMESPRITE, "void SetSpeed(double)", asMETHOD(SAnimeSprite, SetSpeed), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_ANIMESPRITE, "void SetLoopCount(int)", asMETHOD(SAnimeSprite, SetLoopCount), asCALL_THISCALL);
}

SContainer::SContainer() : SSprite()
{
}

SContainer::~SContainer()
{
    for (const auto &s : children) if (s) s->Release();
}

void SContainer::AddChild(SSprite *child)
{
    if (!child) return;
    children.emplace(child);
}

void SContainer::Tick(double delta)
{
    mover->Tick(delta);
    for (const auto &s : children) s->Tick(delta);
}

void SContainer::Draw()
{
    for (const auto &s : children) s->Draw(Transform, Color);
}

void SContainer::Draw(const Transform2D & parent, const ColorTint &color)
{
    auto tf = Transform.ApplyFrom(parent);
    auto cl = Color.ApplyFrom(color);
    for (const auto &s : children) s->Draw(tf, cl);
}

SContainer* SContainer::Factory()
{
    auto result = new SContainer();
    result->AddRef();
    return result;
}

void SContainer::RegisterType(asIScriptEngine *engine)
{
    RegisterSpriteBasic<SContainer>(engine, SU_IF_CONTAINER);
    engine->RegisterObjectBehaviour(SU_IF_CONTAINER, asBEHAVE_FACTORY, SU_IF_CONTAINER "@ f()", asFUNCTION(SContainer::Factory), asCALL_CDECL);
    engine->RegisterObjectMethod(SU_IF_SPRITE, SU_IF_CONTAINER "@ opCast()", asFUNCTION((CastReferenceType<SSprite, SContainer>)), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod(SU_IF_CONTAINER, SU_IF_SPRITE "@ opImplCast()", asFUNCTION((CastReferenceType<SContainer, SSprite>)), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod(SU_IF_CONTAINER, "void AddChild(" SU_IF_SPRITE "@)", asMETHOD(SContainer, AddChild), asCALL_THISCALL);
}
