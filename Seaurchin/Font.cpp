#include "Font.h"
#include "Misc.h"

using namespace std;

void RectPacker::Init(int w, int h, int rowh)
{
    width = w;
    height = h;
    row = 0;
    cursorX = 0;
    cursorY = 0;
}

RectPacker::Rect RectPacker::Insert(int w, int h)
{
    if (cursorX + w >= width) {
        cursorX = 0;
        cursorY += row;
    }
    if (cursorY + h > height) return Rect { 0 };
    if (w > width) return Rect { 0 };
    Rect r;
    r.x = cursorX;
    r.y = cursorY;
    r.width = w;
    r.height = h;
    cursorX += w;
    row = max(h, row);
    return r;
}

// Sif2Creator ----------------------------------------------

void Sif2Creator::InitializeFace(string fontpath)
{
    auto log = spdlog::get("main");
    if (faceMemory) return;
    ifstream fontfile(ConvertUTF8ToUnicode(fontpath));
    fontfile.seekg(0, ios_base::end);
    faceMemorySize = fontfile.tellg();
    fontfile.clear();
    fontfile.seekg(0, ios_base::beg);
    faceMemory = new uint8_t[faceMemorySize];
    fontfile.read((char*)faceMemory, faceMemorySize);
    fontfile.close();

    error = FT_New_Memory_Face(freetype, faceMemory, faceMemorySize, 0, &face);
    if (error) {
        log->error(u8"フォント {0} を読み込めませんでした", fontpath);
        delete[] faceMemory;
        faceMemory = nullptr;
        return;
    }
}

void Sif2Creator::FinalizeFace()
{
    if (!faceMemory) return;
    FT_Done_Face(face);
    delete[] faceMemory;
    faceMemory = nullptr;
}

void Sif2Creator::RequestFace(float size)
{
    FT_Size_RequestRec req;
    req.width = 0;
    req.height = (int)(size * 64.0f);
    req.horiResolution = 0;
    req.vertResolution = 0;
    req.type = FT_SIZE_REQUEST_TYPE_BBOX;

    FT_Request_Size(face, &req);
    FT_Select_Charmap(face, FT_ENCODING_UNICODE);
}

void Sif2Creator::OpenSif2(boost::filesystem::path sif2path)
{
    sif2stream.open(sif2path.wstring(), ios::out | ios::trunc | ios::binary);
    sif2stream.seekp(sizeof(Sif2Header));
}

void Sif2Creator::CloseSif2()
{
    sif2stream.close();
}

void Sif2Creator::NewBitmap(uint16_t width, uint16_t height)
{
    bitmapWidth = width;
    bitmapHeight = height;
    if (bitmapMemory) delete[] bitmapMemory;
    bitmapMemory = new uint8_t[width * height * 2];
    packer.Init(width, height, 0);
}

void Sif2Creator::SaveBitmapCache(boost::filesystem::path cachepath)
{
    FILE *file;
    fopen_s(&file, cachepath.string().c_str(), "wb");
    if (file == nullptr) return;
    auto png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    auto info = png_create_info_struct(png);
    png_init_io(png, file);
    png_set_IHDR(png, info, bitmapWidth, bitmapHeight, 8, PNG_COLOR_TYPE_GRAY_ALPHA, NULL, PNG_COMPRESSION_TYPE_DEFAULT, NULL);
    auto rows = new png_byte*[bitmapHeight];
    for (int i = 0; i < bitmapHeight; i++) rows[i] = bitmapMemory + bitmapWidth * i * 2;
    png_set_rows(png, info, rows);
    png_write_png(png, info, PNG_TRANSFORM_IDENTITY, nullptr);
    png_destroy_write_struct(&png, &info);
    if (file) fclose(file);
    delete[] rows;
}

bool Sif2Creator::RenderGlyph(uint32_t cp)
{
    Sif2Glyph ginfo;
    FT_GlyphSlot gslot;
    FT_UInt gidx = FT_Get_Char_Index(face, cp);
    int baseline = -face->size->metrics.descender >> 6;

    FT_Load_Glyph(face, gidx, FT_LOAD_DEFAULT);
    gslot = face->glyph;
    FT_Render_Glyph(gslot, FT_RENDER_MODE_NORMAL);
    ginfo.GlyphWidth = gslot->bitmap.width;
    ginfo.GlyphHeight = gslot->bitmap.rows;
    ginfo.WholeAdvance = gslot->metrics.horiAdvance >> 6;
    ginfo.BearX = gslot->metrics.horiBearingX >> 6;
    ginfo.BearY = currentSize - (baseline + (gslot->metrics.horiBearingY >> 6));
    ginfo.Codepoint = cp;
    ginfo.ImageNumber = imageIndex;
    ginfo.GlyphX = 0;
    ginfo.GlyphY = 0;

    if (ginfo.GlyphWidth * ginfo.GlyphWidth == 0) {
        //まさか' 'がグリフを持たないとは思わなかった(いや当たり前でしょ)
        sif2stream.write((const char*)&ginfo, sizeof(GlyphInfo));
        writtenGlyphs++;
        return true;
    }

    RectPacker::Rect rect;
    rect = packer.Insert(ginfo.GlyphWidth, ginfo.GlyphHeight);
    if (rect.height == 0) return false;
    ginfo.GlyphX = rect.x;
    ginfo.GlyphY = rect.y;

    uint8_t *buffer = new uint8_t[rect.width * 2];
    for (int y = 0; y < gslot->bitmap.rows; y++) {
        for (int x = 0; x < rect.width; x++) {
            buffer[x * 2] = 0xff;
            buffer[x * 2 + 1] = gslot->bitmap.buffer[y * rect.width + x];
        }
        int py = rect.y + y;
        memcpy_s(bitmapMemory + (py * bitmapHeight * 2 + rect.x * 2), rect.width * 2, buffer, rect.width * 2);
    }
    delete[] buffer;

    writtenGlyphs++;
    return true;
}

Sif2Creator::Sif2Creator()
{
    FT_Init_FreeType(&freetype);
}

Sif2Creator::~Sif2Creator()
{
    FinalizeFace();
    FT_Done_FreeType(freetype);
}

void Sif2Creator::CreateSif2(const FontCreationOption &option, boost::filesystem::path outputPath)
{
    // using namespace boost::filesystem;
    auto log = spdlog::get("main");

    ofstream sifstream;
    Sif2Header header;
    header.Magic = Sif2Magic;
    header.FontSize = option.Size;

    InitializeFace(option.FontPath);
    RequestFace(option.Size);
    log->info(u8"フォント\"{0}\"内に{1}グリフあります", face->family_name, face->num_glyphs);

    imageIndex = 0;
    writtenGlyphs = 0;
    OpenSif2(outputPath);

    CloseSif2();
}