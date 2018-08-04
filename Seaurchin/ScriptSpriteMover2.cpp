#include "ScriptSpriteMover2.h"
#include "ScriptSprite.h"
#include "Misc.h"
#include "MoverFunction.h"

using namespace std;
using namespace crc32_constexpr;

static const double qNaN = numeric_limits<double>::quiet_NaN();

SpriteMoverArgument::SpriteMoverArgument()
{
    X = Y = Z = qNaN;
    Duration = Wait = 0;
    Ease = easing::Easing::Linear;
}

SpriteMoverData::SpriteMoverData()
{
    Extra1 = Extra2 = Extra3 = qNaN;
    Now = 0;
    IsStarted = false;
}

ScriptSpriteMover2::ScriptSpriteMover2(SSprite *starget)
{
    target = starget;
}

ScriptSpriteMover2::~ScriptSpriteMover2()
{
    moves.clear();
}

void ScriptSpriteMover2::Tick(const double delta)
{

    auto i = moves.begin();
    while (i != moves.end()) {
        auto mobj = i->get();
        if (mobj->Data.Now < 0) {
            //余ったdeltaで呼び出すべきなのかもしれないけどまあいいかって
            mobj->Data.Now += delta;
            ++i;
            continue;
        }
        if (!mobj->Data.IsStarted) {
            //多分初期化処理はこのタイミングの方がいい
            mobj->Data.IsStarted = true;
            mobj->Function(target, mobj->Argument, mobj->Data, 0);
        }
        const auto result = mobj->Function(target, mobj->Argument, mobj->Data, delta);
        mobj->Data.Now += delta;
        if (mobj->Data.Now >= mobj->Argument.Duration || result) {
            mobj->Function(target, mobj->Argument, mobj->Data, -1);
            i = moves.erase(i);
        } else {
            ++i;
        }
    }

}

void ScriptSpriteMover2::Apply(const string &application) const
{
    auto source = application;
    source.erase(remove(source.begin(), source.end(), ' '), source.end());

    vector<tuple<string, string>> params;
    params.reserve(8);
    SplitProps(source, params);
    for (const auto &param : params) ApplyProperty(get<0>(param), get<1>(param));
}

void ScriptSpriteMover2::AddMove(const string &mover)
{
    auto source = mover;
    source.erase(remove(source.begin(), source.end(), ' '), source.end());
    const auto lpp = source.find('('), rpp = source.find(')');
    if (lpp == string::npos || rpp == string::npos) return;

    const auto funcname = source.substr(0, lpp);
    const auto paramstr = source.substr(lpp + 1, rpp - lpp - 1);
    vector<tuple<string, string>> params;
    params.reserve(8);
    SplitProps(paramstr, params);

    auto mobj = BuildMoverObject(funcname, params);
    if (!mobj) return;
    moves.push_back(move(mobj));
}

void ScriptSpriteMover2::Abort(const bool completeMove)
{
    if (completeMove) for (const auto &mobj : moves) mobj->Function(target, mobj->Argument, mobj->Data, -1);
    moves.clear();
}

void ScriptSpriteMover2::ApplyProperty(const string &prop, const string &value) const
{
    switch (crc32_rec(0xffffffff, prop.c_str())) {
        case "x"_crc32:
            target->Transform.X = ToDouble(value.c_str());
            break;
        case "y"_crc32:
            target->Transform.Y = ToDouble(value.c_str());
            break;
        case "z"_crc32:
            target->ZIndex = int(ToDouble(value.c_str()));
            break;
        case "origX"_crc32:
            target->Transform.OriginX = ToDouble(value.c_str());
            break;
        case "origY"_crc32:
            target->Transform.OriginY = ToDouble(value.c_str());
            break;
        case "scaleX"_crc32:
            target->Transform.ScaleX = ToDouble(value.c_str());
            break;
        case "scaleY"_crc32:
            target->Transform.ScaleY = ToDouble(value.c_str());
            break;
        case "angle"_crc32:
            target->Transform.Angle = ToDouble(value.c_str());
            break;
        case "alpha"_crc32:
            target->Color.A = static_cast<unsigned char>(ToDouble(value.c_str()) * 255.0);
            break;
        case "r"_crc32:
            target->Color.R = static_cast<unsigned char>(ToDouble(value.c_str()));
            break;
        case "g"_crc32:
            target->Color.G = static_cast<unsigned char>(ToDouble(value.c_str()));
            break;
        case "b"_crc32:
            target->Color.B = static_cast<unsigned char>(ToDouble(value.c_str()));
            break;
        default:
            // TODO SSpriteにカスタム実装
            break;
    }
}

std::unique_ptr<SpriteMoverObject> ScriptSpriteMover2::BuildMoverObject(const string &func, const PropList &props) const
{
    auto result = make_unique<SpriteMoverObject>();
    // MoverFunctionを決定
    if (mover_function::actions.find(func) != mover_function::actions.end()) {
        result->Function = mover_function::actions[func];
    } else {
        // TODO: カスタムMoverに対応
        const auto custom = target->GetCustomAction(func);
        if (!custom) return nullptr;
        result->Function = custom;
    }

    for (const auto &prop : props) {
        switch (crc32_rec(0xffffffff, get<0>(prop).c_str())) {
            case "x"_crc32:
            case "r"_crc32:
                result->Argument.X = ToDouble(get<1>(prop).c_str());
                break;
            case "y"_crc32:
            case "g"_crc32:
                result->Argument.Y = ToDouble(get<1>(prop).c_str());
                break;
            case "z"_crc32:
            case "b"_crc32:
                result->Argument.Z = ToDouble(get<1>(prop).c_str());
                break;
            case "time"_crc32:
                result->Argument.Duration = ToDouble(get<1>(prop).c_str());
                break;
            case "wait"_crc32:
                result->Argument.Wait = ToDouble(get<1>(prop).c_str());
                break;
            case "ease"_crc32:
                result->Argument.Ease = easing::easings[get<1>(prop)];
                break;
            default: break;
        }
    }

    result->Data.Now = -result->Argument.Wait;

    return result;
}

