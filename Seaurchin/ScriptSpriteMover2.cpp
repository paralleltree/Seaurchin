#include "ScriptSpriteMover2.h"
#include "ScriptSprite.h"

using namespace std;
using namespace crc32_constexpr;

ScriptSpriteMover2::ScriptSpriteMover2(SSprite *target)
{
    Target = target;
}

ScriptSpriteMover2::~ScriptSpriteMover2()
{}

void ScriptSpriteMover2::Tick(double delta)
{}

void ScriptSpriteMover2::Apply(const string &application)
{

}

void ScriptSpriteMover2::ApplyProperty(const string &prop, const string &value)
{
    switch (crc32_rec(0xffffffff, prop.c_str())) {
        case "x"_crc32:
            Target->Transform.X = ToDouble(value.c_str());
            break;
        case "y"_crc32:
            Target->Transform.Y = ToDouble(value.c_str());
            break;
        case "z"_crc32:
            Target->ZIndex = (int)ToDouble(value.c_str());
            break;
        case "origX"_crc32:
            Target->Transform.OriginX = ToDouble(value.c_str());
            break;
        case "origY"_crc32:
            Target->Transform.OriginY = ToDouble(value.c_str());
            break;
        case "scaleX"_crc32:
            Target->Transform.ScaleX = ToDouble(value.c_str());
            break;
        case "scaleY"_crc32:
            Target->Transform.ScaleY = ToDouble(value.c_str());
            break;
        case "angle"_crc32:
            Target->Transform.Angle = ToDouble(value.c_str());
            break;
        case "alpha"_crc32:
            Target->Color.A = (unsigned char)(ToDouble(value.c_str()) * 255.0);
            break;
        case "r"_crc32:
            Target->Color.R = (unsigned char)ToDouble(value.c_str());
            break;
        case "g"_crc32:
            Target->Color.G = (unsigned char)ToDouble(value.c_str());
            break;
        case "b"_crc32:
            Target->Color.B = (unsigned char)ToDouble(value.c_str());
            break;
        default:
            // TODO SSprite‚ÉƒJƒXƒ^ƒ€ŽÀ‘•
            break;
    }
}