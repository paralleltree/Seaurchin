#include "ScriptResource.h"
#include "Interfaces.h"
#include "ExecutionManager.h"
#include "Misc.h"

using namespace std;

SResource::SResource()
{}

SResource::~SResource()
{}

void SResource::AddRef()
{
    Reference++;
}

void SResource::Release()
{
    if (--Reference == 0) delete this;
}

// SImage ----------------------

void SImage::ObtainSize()
{
    GetGraphSize(Handle, &Width, &Height);
}

SImage::SImage(int ih)
{
    Handle = ih;
}

SImage::~SImage()
{
    if (Handle) DeleteGraph(Handle);
    Handle = 0;
}

int SImage::get_Width()
{
    if (!Width) ObtainSize();
    return Width;
}

int SImage::get_Height()
{
    if (!Height) ObtainSize();
    return Height;
}

SImage * SImage::CreateBlankImage()
{
    auto result = new SImage(0);
    result->AddRef();
    return result;
}

SImage * SImage::CreateLoadedImageFromFile(const string &file, bool async)
{
    if (async) SetUseASyncLoadFlag(TRUE);
    auto result = new SImage(LoadGraph(reinterpret_cast<const char*>(ConvertUTF8ToUnicode(file).c_str())));
    if (async) SetUseASyncLoadFlag(FALSE);
    result->AddRef();
    return result;
}

SImage * SImage::CreateLoadedImageFromMemory(void * buffer, size_t size)
{
    auto result = new SImage(CreateGraphFromMem(buffer, size));
    result->AddRef();
    return result;
}

// SRenderTarget -----------------------------

SRenderTarget::SRenderTarget(int w, int h) : SImage(0)
{
    Width = w;
    Height = h;
    if (w * h) Handle = MakeScreen(w, h, TRUE);
}

SRenderTarget * SRenderTarget::CreateBlankTarget(int w, int h)
{
    auto result = new SRenderTarget(w, h);
    result->AddRef();
    return result;
}

// SNinePatchImage ----------------------------
SNinePatchImage::SNinePatchImage(int ih) : SImage(ih)
{}

SNinePatchImage::~SNinePatchImage()
{
    DeleteGraph(Handle);
    Handle = 0;
    LeftSideWidth = TopSideHeight = BodyWidth = BodyHeight = 0;
}

void SNinePatchImage::SetArea(int leftw, int toph, int bodyw, int bodyh)
{
    LeftSideWidth = leftw;
    TopSideHeight = toph;
    BodyWidth = bodyw;
    BodyHeight = bodyh;
}

// SAnimatedImage --------------------------------

SAnimatedImage::SAnimatedImage(int w, int h, int count, double time) : SImage(0)
{
    CellWidth = Width = w;
    CellHeight = Height = h;
    FrameCount = count;
    SecondsPerFrame = time;
}

SAnimatedImage::~SAnimatedImage()
{
    for (auto &img : Images) DeleteGraph(img);
}

SAnimatedImage * SAnimatedImage::CreateLoadedImageFromFile(const std::string & file, int xc, int yc, int w, int h, int count, double time)
{
    auto result = new SAnimatedImage(w, h, count, time);
    result->Images.resize(count);
    LoadDivGraph(reinterpret_cast<const char*>(ConvertUTF8ToUnicode(file).c_str()), count, xc, yc, w, h, result->Images.data());
    result->AddRef();
    return result;
}


// SFont --------------------------------------

SFont::SFont()
{

}

SFont::~SFont()
{
    for (auto &i : Glyphs) if (i.second) delete i.second;
    for (auto &i : Images) i->Release();
}

tuple<double, double, int> SFont::RenderRaw(SRenderTarget *rt, const string &utf8str)
{
    double cx = 0, cy = 0;
    double mx = 0, my = 0;
    int line = 1;
    if (rt) {
        BEGIN_DRAW_TRANSACTION(rt->GetHandle());
        ClearDrawScreen();
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
        SetDrawBright(255, 255, 255);
    }
    const uint8_t *ccp = (const uint8_t*)utf8str.c_str();
    while (*ccp) {
        uint32_t gi = 0;
        if (*ccp >= 0xF0) {
            gi = (*ccp & 0x07) << 18 | (*(ccp + 1) & 0x3F) << 12 | (*(ccp + 2) & 0x3F) << 6 | (*(ccp + 3) & 0x3F);
            ccp += 4;
        } else if (*ccp >= 0xE0) {
            gi = (*ccp & 0x0F) << 12 | (*(ccp + 1) & 0x3F) << 6 | (*(ccp + 2) & 0x3F);
            ccp += 3;
        } else if (*ccp >= 0xC2) {
            gi = (*ccp & 0x1F) << 6 | (*(ccp + 1) & 0x3F);
            ccp += 2;
        } else {
            gi = *ccp & 0x7F;
            ccp++;
        }
        if (gi == 0x0A) {
            line++;
            mx = max(mx, cx);
            my = line * Size;
            cx = 0;
            cy += Size;
            continue;
        }
        auto sg = Glyphs[gi];
        if (!sg) continue;
        if (rt) DrawRectGraph(
            cx + sg->BearX, cy + sg->BearY,
            sg->GlyphX, sg->GlyphY,
            sg->GlyphWidth, sg->GlyphHeight,
            Images[sg->ImageNumber]->GetHandle(),
            TRUE, FALSE);
        cx += sg->WholeAdvance;
    }
    if (rt) {
        FINISH_DRAW_TRANSACTION;
    }
    mx = max(mx, cx);
    my = line * Size;
    return make_tuple(mx, my, line);
}

tuple<double, double, int> SFont::RenderRich(SRenderTarget *rt, const string &utf8str, const ColorTint &defcol)
{
    namespace bx = boost::xpressive;
    using namespace crc32_constexpr;

    bx::sregex cmd = bx::bos >> "${" >> (bx::s1 = -+bx::_w) >> "}";
    double cx = 0, cy = 0;
    double mx = 0, my = 0;
    int line = 1;

    uint8_t cr = defcol.R, cg = defcol.G, cb = defcol.B;
    double cw = 1;

    if (rt) {
        BEGIN_DRAW_TRANSACTION(rt->GetHandle());
        ClearDrawScreen();
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
        SetDrawBright(cr, cg, cb);
        SetDrawMode(DX_DRAWMODE_ANISOTROPIC);
    }
    auto ccp = utf8str.begin();
    bx::smatch match;
    while (ccp != utf8str.end()) {
        boost::sub_range<const string> sr(ccp, utf8str.end());
        if (bx::regex_search(sr, match, cmd)) {
            auto al = match.length();
            auto cmd = match[1].str();
            switch (crc32_rec(0xffffffff, cmd.c_str())) {
                case "reset"_crc32:
                    cr = defcol.R;
                    cg = defcol.G;
                    cb = defcol.B;
                    cw = 1;
                    break;
                case "red"_crc32:
                    cr = 255;
                    cg = cb = 0;
                    break;
                case "green"_crc32:
                    cg = 255;
                    cr = cb = 0;
                    break;
                case "blue"_crc32:
                    cb = 255;
                    cr = cg = 0;
                    break;
                case "defcolor"_crc32:
                    cr = defcol.R;
                    cg = defcol.G;
                    cb = defcol.B;
                    break;
                case "bold"_crc32:
                    cw = 1.2;
                    break;
                case "normal"_crc32:
                    cw = 1;
                    break;
            }
            ccp += match[0].length();
            continue;
        }

        uint32_t gi = 0;
        if ((uint8_t)*ccp >= 0xF0) {
            gi = ((uint8_t)*ccp & 0x07) << 18 | ((uint8_t)*(ccp + 1) & 0x3F) << 12 | ((uint8_t)*(ccp + 2) & 0x3F) << 6 | ((uint8_t)*(ccp + 3) & 0x3F);
            ccp += 4;
        } else if ((uint8_t)*ccp >= 0xE0) {
            gi = ((uint8_t)*ccp & 0x0F) << 12 | ((uint8_t)*(ccp + 1) & 0x3F) << 6 | ((uint8_t)*(ccp + 2) & 0x3F);
            ccp += 3;
        } else if ((uint8_t)*ccp >= 0xC2) {
            gi = ((uint8_t)*ccp & 0x1F) << 6 | ((uint8_t)*(ccp + 1) & 0x3F);
            ccp += 2;
        } else {
            gi = (uint8_t)*ccp & 0x7F;
            ccp++;
        }
        if (gi == 0x0A) {
            line++;
            mx = max(mx, cx);
            my = line * Size;
            cx = 0;
            cy += Size;
            continue;
        }
        auto sg = Glyphs[gi];
        if (!sg) continue;
        if (rt) {
            SetDrawBright(cr, cg, cb);
            DrawRectRotaGraph3F(
                cx + sg->BearX - (cw - 1.0) * 0.5 * sg->GlyphWidth, cy + sg->BearY,
                sg->GlyphX, sg->GlyphY,
                sg->GlyphWidth, sg->GlyphHeight,
                0, 0, 
                cw, 1, 0,
                Images[sg->ImageNumber]->GetHandle(),
                TRUE, FALSE);
        }
        cx += sg->WholeAdvance;
    }
    if (rt) {
        SetDrawMode(DX_DRAWMODE_NEAREST);
        FINISH_DRAW_TRANSACTION;
    }
    mx = max(mx, cx);
    my = line * Size;
    return make_tuple(mx, my, line);
}


SFont * SFont::CreateBlankFont()
{
    auto result = new SFont();
    result->AddRef();
    return result;
}

SFont * SFont::CreateLoadedFontFromFile(const string & file)
{
    auto result = new SFont();
    ifstream font(ConvertUTF8ToUnicode(file), ios::in | ios::binary);

    Sif2Header header;
    font.read((char*)&header, sizeof(Sif2Header));
    result->Size = header.FontSize;

    for (int i = 0; i < header.Glyphs; i++) {
        Sif2Glyph *info = new Sif2Glyph();
        font.read((char*)info, sizeof(Sif2Glyph));
        result->Glyphs[info->Codepoint] = info;
    }
    uint32_t size;
    for (int i = 0; i < header.Images; i++) {
        font.read((char*)&size, sizeof(uint32_t));
        uint8_t *pngdata = new uint8_t[size];
        font.read((char*)pngdata, size);
        result->Images.push_back(SImage::CreateLoadedImageFromMemory(pngdata, size));
        delete[] pngdata;
    }
    result->AddRef();
    return result;
}

// SSoundMixer ------------------------------

SSoundMixer::SSoundMixer(SoundMixerStream * mixer)
{
    this->mixer = mixer;
}

SSoundMixer::~SSoundMixer()
{
    delete mixer;
}

void SSoundMixer::Update()
{
    mixer->Update();
}

void SSoundMixer::Play(SSound * sound)
{
    mixer->Play(sound->sample);
}

void SSoundMixer::Stop(SSound * sound)
{
    mixer->Stop(sound->sample);
}

SSoundMixer * SSoundMixer::CreateMixer(SoundManager * manager)
{
    auto result = new SSoundMixer(manager->CreateMixerStream());
    result->AddRef();
    return result;
}


// SSound -----------------------------------
SSound::SSound(SoundSample *smp)
{
    sample = smp;
}

SSound::~SSound()
{
    delete sample;
}

void SSound::SetLoop(bool looping)
{
    sample->SetLoop(looping);
}

void SSound::SetVolume(float vol)
{
    sample->SetVolume(vol);
}

SSound * SSound::CreateSound(SoundManager *smanager)
{
    auto result = new SSound(nullptr);
    result->AddRef();
    return result;
}

SSound * SSound::CreateSoundFromFile(SoundManager *smanager, const std::string & file, int simul)
{
    auto hs = SoundSample::CreateFromFile(ConvertUTF8ToUnicode(file), simul);
    auto result = new SSound(hs);
    result->AddRef();
    return result;
}

// SSettingItem --------------------------------------------

SSettingItem::SSettingItem(shared_ptr<Setting2::SettingItem> s) : setting(s)
{

}

SSettingItem::~SSettingItem()
{
    setting->SaveValue();
}

void SSettingItem::Save()
{
    setting->SaveValue();
}

void SSettingItem::MoveNext()
{
    setting->MoveNext();
}

void SSettingItem::MovePrevious()
{
    setting->MovePrevious();
}

std::string SSettingItem::GetItemText()
{
    return setting->GetItemString();
}

std::string SSettingItem::GetDescription()
{
    return setting->GetDescription();
}


void RegisterScriptResource(ExecutionManager *exm)
{
    auto engine = exm->GetScriptInterfaceUnsafe()->GetEngine();

    engine->RegisterObjectType(SU_IF_IMAGE, 0, asOBJ_REF);
    engine->RegisterObjectBehaviour(SU_IF_IMAGE, asBEHAVE_FACTORY, SU_IF_IMAGE "@ f()", asFUNCTION(SImage::CreateBlankImage), asCALL_CDECL);
    engine->RegisterObjectBehaviour(SU_IF_IMAGE, asBEHAVE_FACTORY, SU_IF_IMAGE "@ f(const string &in, bool = false)", asFUNCTION(SImage::CreateLoadedImageFromFile), asCALL_CDECL);
    engine->RegisterObjectBehaviour(SU_IF_IMAGE, asBEHAVE_ADDREF, "void f()", asMETHOD(SImage, AddRef), asCALL_THISCALL);
    engine->RegisterObjectBehaviour(SU_IF_IMAGE, asBEHAVE_RELEASE, "void f()", asMETHOD(SImage, Release), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_IMAGE, "int get_Width()", asMETHOD(SImage, get_Width), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_IMAGE, "int get_Height()", asMETHOD(SImage, get_Height), asCALL_THISCALL);
    //engine->RegisterObjectMethod(SU_IF_IMAGE, SU_IF_IMAGE "& opAssign(" SU_IF_IMAGE "&)", asFUNCTION(asAssign<SImage>), asCALL_CDECL_OBJFIRST);

    engine->RegisterObjectType(SU_IF_FONT, 0, asOBJ_REF);
    engine->RegisterObjectBehaviour(SU_IF_FONT, asBEHAVE_FACTORY, SU_IF_FONT "@ f()", asFUNCTION(SFont::CreateBlankFont), asCALL_CDECL);
    engine->RegisterObjectBehaviour(SU_IF_FONT, asBEHAVE_ADDREF, "void f()", asMETHOD(SFont, AddRef), asCALL_THISCALL);
    engine->RegisterObjectBehaviour(SU_IF_FONT, asBEHAVE_RELEASE, "void f()", asMETHOD(SFont, Release), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_FONT, "int get_Size()", asMETHOD(SFont, get_Size), asCALL_THISCALL);

    engine->RegisterObjectType(SU_IF_SOUND, 0, asOBJ_REF);
    engine->RegisterObjectBehaviour(SU_IF_SOUND, asBEHAVE_ADDREF, "void f()", asMETHOD(SSound, AddRef), asCALL_THISCALL);
    engine->RegisterObjectBehaviour(SU_IF_SOUND, asBEHAVE_RELEASE, "void f()", asMETHOD(SSound, Release), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SOUND, "void SetLoop(bool)", asMETHOD(SSound, SetLoop), asCALL_THISCALL);

    engine->RegisterObjectType(SU_IF_SOUNDMIXER, 0, asOBJ_REF);
    engine->RegisterObjectBehaviour(SU_IF_SOUNDMIXER, asBEHAVE_ADDREF, "void f()", asMETHOD(SSoundMixer, AddRef), asCALL_THISCALL);
    engine->RegisterObjectBehaviour(SU_IF_SOUNDMIXER, asBEHAVE_RELEASE, "void f()", asMETHOD(SSoundMixer, Release), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SOUNDMIXER, "void Play(" SU_IF_SOUND "@)", asMETHOD(SSoundMixer, Play), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SOUNDMIXER, "void Stop(" SU_IF_SOUND "@)", asMETHOD(SSoundMixer, Stop), asCALL_THISCALL);

    engine->RegisterObjectType(SU_IF_ANIMEIMAGE, 0, asOBJ_REF);
    engine->RegisterObjectBehaviour(SU_IF_ANIMEIMAGE, asBEHAVE_ADDREF, "void f()", asMETHOD(SAnimatedImage, AddRef), asCALL_THISCALL);
    engine->RegisterObjectBehaviour(SU_IF_ANIMEIMAGE, asBEHAVE_RELEASE, "void f()", asMETHOD(SAnimatedImage, Release), asCALL_THISCALL);

    engine->RegisterObjectType(SU_IF_SETTING_ITEM, 0, asOBJ_REF);
    engine->RegisterObjectBehaviour(SU_IF_SETTING_ITEM, asBEHAVE_ADDREF, "void f()", asMETHOD(SSettingItem, AddRef), asCALL_THISCALL);
    engine->RegisterObjectBehaviour(SU_IF_SETTING_ITEM, asBEHAVE_RELEASE, "void f()", asMETHOD(SSettingItem, Release), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SETTING_ITEM, "void Save()", asMETHOD(SSettingItem, Save), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SETTING_ITEM, "void MoveNext()", asMETHOD(SSettingItem, MoveNext), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SETTING_ITEM, "void MovePrevious()", asMETHOD(SSettingItem, MovePrevious), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SETTING_ITEM, "string GetItemText()", asMETHOD(SSettingItem, GetItemText), asCALL_THISCALL);
    engine->RegisterObjectMethod(SU_IF_SETTING_ITEM, "string GetDescription()", asMETHOD(SSettingItem, GetDescription), asCALL_THISCALL);
}