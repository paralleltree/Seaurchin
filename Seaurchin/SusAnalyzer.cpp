#include "SusAnalyzer.h"
#include <utility>
#include "Misc.h"

using namespace std;
using namespace crc32_constexpr;
namespace b = boost;
namespace ba = boost::algorithm;
namespace xp = boost::xpressive;

xp::sregex SusAnalyzer::regexSusCommand = "#" >> (xp::s1 = +xp::alnum) >> !(+xp::space >> (xp::s2 = +(~xp::_n)));
xp::sregex SusAnalyzer::regexSusData = "#" >> (xp::s1 = xp::repeat<3, 3>(xp::alnum)) >> (xp::s2 = xp::repeat<2, 3>(xp::alnum)) >> ":" >> *xp::space >> (xp::s3 = +(~xp::_n));

static xp::sregex allNumeric = xp::bos >> +(xp::digit) >> xp::eos;

auto toUpper = [](const char c) {
    return (c >= 'a' && c <= 'z') ? char(c - 0x20) : c;
};

static auto isUpperHexadecimalChar = [](const char c) {
    if (c >= '8' && c <= '9') return true;
    if (c >= 'A' && c <= 'F') return true;
    return false;
};

static auto convertRawString = [](const string &input) -> string {
    // TIL: ASCII文字範囲ではUTF-8と本来のASCIIを間違うことはない
    if (ba::starts_with(input, "\"")) {
        ostringstream result;
        auto rest = input;
        trim_if(rest, ba::is_any_of("\""));
        auto it = rest.begin();
        while (it != rest.end()) {
            if (*it != '\\') {
                result << *it;
                ++it;
                continue;
            }
            ++it;
            if (it == rest.end()) return "";
            switch (*it) {
                case '"':
                    result << "\"";
                    break;
                case 't':
                    result << "\t";
                    break;
                case 'n':
                    result << "\n";
                    break;
                case 'u': {
                    /*
                    //utf-8 4byte食う
                    char cp[5] = { 0 };
                    for (auto i = 0; i < 4; i++) {
                        cp[i] = *(++it);
                    }
                    */
                    // wchar_t r = stoi(cp, 0, 16);
                    //でも突っ込むのめんどくさいので🙅で代用します
                    result << u8"🙅";
                    break;
                }
                default:
                    break;
            }
            ++it;
        }
        return result.str();
    }
    return input;
};

SusAnalyzer::SusAnalyzer(const uint32_t tpb)
{
    ticksPerBeat = tpb;
    longInjectionPerBeat = 2;
    measureCountOffset = 0;
    timelineResolver = [=](const uint32_t number) { return hispeedDefinitions[number]; };
    errorCallbacks.emplace_back([](auto type, auto message) {
        auto log = spdlog::get("main");
        log->error(message);
    });
}

SusAnalyzer::~SusAnalyzer()
{
    Reset();
}

void SusAnalyzer::Reset()
{
    notes.clear();
    bpmChanges.clear();
    bpmDefinitions.clear();
    beatsDefinitions.clear();
    hispeedDefinitions.clear();
    extraAttributes.clear();
    ticksPerBeat = 192;
    longInjectionPerBeat = 2;
    measureCountOffset = 0;
    SharedMetaData.Reset();

    bpmDefinitions[1] = 120.0;
    beatsDefinitions[0] = 4.0;

    auto defhs = make_shared<SusHispeedTimeline>([&](const uint32_t m, const uint32_t t) { return GetAbsoluteTime(m, t); });
    defhs->AddKeysByString("0'0:1.0:v", timelineResolver);
    hispeedDefinitions[defaultHispeedNumber] = defhs;
    hispeedToApply = defhs;
    hispeedToMeasure = defhs;

    const auto defea = make_shared<SusNoteExtraAttribute>();
    defea->Priority = 0;
    defea->HeightScale = 1;
    extraAttributes[defaultExtraAttributeNumber] = defea;
    extraAttributeToApply = defea;
}

void SusAnalyzer::SetMessageCallBack(const function<void(string, string)>& func)
{
    errorCallbacks.push_back(func);
}

//一応UTF-8として処理することにしますがどうせ変わらないだろうなぁ
//あと列挙済みファイルを流し込む前提でエラーチェックしない
void SusAnalyzer::LoadFromFile(const wstring &fileName, const bool analyzeOnlyMetaData)
{
    auto log = spdlog::get("main");
    ifstream file;
    string rawline;
    xp::smatch match;
    uint32_t line = 0;

    Reset();
    if (!analyzeOnlyMetaData) log->info(u8"{0}の解析を開始…", ConvertUnicodeToUTF8(fileName));

    file.open(fileName, ios::in);
    char bom[3];
    file.read(bom, 3);
    if (bom[0] != char(0xEF) || bom[1] != char(0xBB) || bom[2] != char(0xBF)) file.seekg(0);
    while (getline(file, rawline)) {
        line++;
        if (!rawline.length()) continue;
        if (rawline[0] != '#') continue;
        if (xp::regex_match(rawline, match, regexSusCommand)) {
            ProcessCommand(match, analyzeOnlyMetaData, line);
        } else if (xp::regex_match(rawline, match, regexSusData)) {
            if (!analyzeOnlyMetaData || boost::starts_with(rawline, "#BPM")) ProcessData(match, line);
        } else {
            MakeMessage(line, u8"SUS有効行ですが解析できませんでした。");
        }
    }
    file.close();
    if (!analyzeOnlyMetaData) log->info(u8"…終了");
    if (!analyzeOnlyMetaData) {
        // いい感じにソート
        stable_sort(notes.begin(), notes.end(), [](tuple<SusRelativeNoteTime, SusRawNoteData> a, tuple<SusRelativeNoteTime, SusRawNoteData> b) {
            return get<1>(a).Type.to_ulong() > get<1>(b).Type.to_ulong();
        });
        stable_sort(notes.begin(), notes.end(), [](tuple<SusRelativeNoteTime, SusRawNoteData> a, tuple<SusRelativeNoteTime, SusRawNoteData> b) {
            return get<0>(a).Tick < get<0>(b).Tick;
        });
        stable_sort(notes.begin(), notes.end(), [](tuple<SusRelativeNoteTime, SusRawNoteData> a, tuple<SusRelativeNoteTime, SusRawNoteData> b) {
            return get<0>(a).Measure < get<0>(b).Measure;
        });

        // 小節線ノーツ
        // この時点でケツは最終ノーツのはず
        const auto lastMeasure = get<0>(notes[notes.size() - 1]).Measure + 2;
        for (auto i = 0u; i <= lastMeasure; i++) {
            SusRawNoteData ml;
            SusRelativeNoteTime t = { i, 0 };
            ml.Type.set(size_t(SusNoteType::MeasureLine));
            ml.Timeline = hispeedToMeasure;
            ml.ExtraAttribute = extraAttributes[defaultExtraAttributeNumber];
            notes.emplace_back(t, ml);
        }

        copy_if(notes.begin(), notes.end(), back_inserter(bpmChanges), [](tuple<SusRelativeNoteTime, SusRawNoteData> n) {
            return get<1>(n).Type.test(size_t(SusNoteType::Undefined));
        });

        for (auto &hs : hispeedDefinitions) hs.second->Finialize();
        if (SharedMetaData.BaseBpm == 0) SharedMetaData.BaseBpm = GetBpmAt(0, 0);
        for (const auto& bpm : bpmChanges) {
            SusRelativeNoteTime t = {};
            SusRawNoteData d = {};
            tie(t, d) = bpm;
            SharedBpmChanges.emplace_back(GetAbsoluteTime(t.Measure, t.Tick), bpmDefinitions[d.DefinitionNumber]);
        }
    }
}

void SusAnalyzer::ProcessCommand(const xp::smatch &result, const bool onlyMeta, const uint32_t line)
{
    auto name = result[1].str();
    transform(name.cbegin(), name.cend(), name.begin(), toUpper);
    if (ba::starts_with(name, "BPM")) {
        // #BPMxx yyy.yy
        const auto value = ConvertFloat(result[2].str());
        bpmDefinitions[ConvertHexatridecimal(name.substr(3))] = value;
        if (SharedMetaData.ShowBpm < 0) SharedMetaData.ShowBpm = value;
        return;
    }
    switch (Crc32Rec(0xffffffff, name.c_str())) {
        case "TITLE"_crc32:
            SharedMetaData.UTitle = convertRawString(result[2]);
            break;
        case "SUBTITLE"_crc32:
            SharedMetaData.USubTitle = convertRawString(result[2]);
            break;
        case "ARTIST"_crc32:
            SharedMetaData.UArtist = convertRawString(result[2]);
            break;
        case "GENRE"_crc32:
            // SharedMetaData.UGenre = ConvertRawString(result[2]);
            break;
        case "DESIGNER"_crc32:
        case "SUBARTIST"_crc32:  // BMS互換
            SharedMetaData.UDesigner = convertRawString(result[2]);
            break;
        case "PLAYLEVEL"_crc32: {
            string lstr = result[2];
            const auto pluspos = lstr.find('+');
            if (pluspos != string::npos) {
                SharedMetaData.UExtraDifficulty = u8"+";
                SharedMetaData.Level = ConvertInteger(lstr.substr(0, pluspos));
            } else {
                SharedMetaData.Level = ConvertInteger(lstr);
            }
            break;
        }
        case "DIFFICULTY"_crc32: {
            if (xp::regex_match(result[2], allNumeric)) {
                //通常記法
                SharedMetaData.DifficultyType = ConvertInteger(result[2]);
            } else {
                //WE記法
                auto dd = convertRawString(result[2]);
                vector<string> params;
                ba::split(params, dd, ba::is_any_of(":"));
                if (params.size() < 2) return;
                SharedMetaData.DifficultyType = 4;
                SharedMetaData.Level = ConvertInteger(params[0]);
                SharedMetaData.UExtraDifficulty = params[1];
            }
            break;
        }
        case "SONGID"_crc32:
            SharedMetaData.USongId = convertRawString(result[2]);
            break;
        case "WAVE"_crc32:
            SharedMetaData.UWaveFileName = convertRawString(result[2]);
            break;
        case "WAVEOFFSET"_crc32:
            SharedMetaData.WaveOffset = ConvertFloat(result[2]);
            break;
        case "MOVIE"_crc32:
            SharedMetaData.UMovieFileName = convertRawString(result[2]);
            break;
        case "MOVIEOFFSET"_crc32:
            SharedMetaData.MovieOffset = ConvertFloat(result[2]);
            break;
        case "JACKET"_crc32:
            SharedMetaData.UJacketFileName = convertRawString(result[2]);
            break;
        case "BACKGROUND"_crc32:
            SharedMetaData.UBackgroundFileName = convertRawString(result[2]);
            break;
        case "REQUEST"_crc32:
            ProcessRequest(convertRawString(result[2]), line);
            break;
        case "BASEBPM"_crc32:
            SharedMetaData.BaseBpm = ConvertFloat(result[2]);
            break;

            //此処から先はデータ内で使う用
        case "HISPEED"_crc32: {
            if (onlyMeta) break;
            const auto hsn = ConvertHexatridecimal(result[2]);
            if (hispeedDefinitions.find(hsn) == hispeedDefinitions.end()) {
                MakeMessage(line, u8"指定されたタイムラインが存在しません");
                break;
            }
            hispeedToApply = hispeedDefinitions[hsn];
            break;
        }
        case "NOSPEED"_crc32:
            if (!onlyMeta) hispeedToApply = hispeedDefinitions[defaultHispeedNumber];
            break;

        case "ATTRIBUTE"_crc32: {
            if (onlyMeta) break;
            const auto ean = ConvertHexatridecimal(result[2]);
            if (extraAttributes.find(ean) == extraAttributes.end()) {
                MakeMessage(line, u8"指定されたアトリビュートが存在しません");
                break;
            }
            extraAttributeToApply = extraAttributes[ean];
            break;
        }
        case "NOATTRIBUTE"_crc32:
            if (!onlyMeta) extraAttributeToApply = extraAttributes[defaultExtraAttributeNumber];
            break;

        case "MEASUREHS"_crc32: {
            if (onlyMeta) break;
            const auto hsn = ConvertHexatridecimal(result[2]);
            if (hispeedDefinitions.find(hsn) == hispeedDefinitions.end()) {
                MakeMessage(line, u8"指定されたタイムラインが存在しません");
                break;
            }
            hispeedToMeasure = hispeedDefinitions[hsn];
            break;
        }

        case "MEASUREBS"_crc32: {
            if (onlyMeta) break;
            const auto bsc = ConvertInteger(result[2]);
            if (bsc < 0) {
                MakeMessage(line, u8"小節オフセットの値が不正です");
                break;
            }
            measureCountOffset = bsc;
            break;
        }

        default:
            MakeMessage(line, u8"SUSコマンドが無効です");
            break;
    }

}

void SusAnalyzer::ProcessRequest(const string &cmd, const uint32_t line)
{
    auto str = cmd;
    b::trim_if(str, ba::is_any_of(" "));
    vector<string> params;
    ba::split(params, str, ba::is_any_of(" "), b::token_compress_on);

    if (params.empty()) return;
    switch (Crc32Rec(0xffffffff, params[0].c_str())) {
        case "mertonome"_crc32:
            SharedMetaData.ExtraFlags[size_t(SusMetaDataFlags::DisableMetronome)] = !ConvertBoolean(params[1]);
            break;
        case "ticks_per_beat"_crc32:
            ticksPerBeat = ConvertInteger(params[1]);
            break;
        case "enable_priority"_crc32:
            MakeMessage(line, u8"優先度つきノーツ描画が設定されます");
            SharedMetaData.ExtraFlags[size_t(SusMetaDataFlags::EnableDrawPriority)] = ConvertBoolean(params[1]);
            break;
        case "enable_moving_lane"_crc32:
            MakeMessage(line, u8"移動レーンサポートが設定されます");
            SharedMetaData.ExtraFlags[size_t(SusMetaDataFlags::EnableMovingLane)] = ConvertBoolean(params[1]);
            break;
        case "segments_per_second"_crc32:
            SharedMetaData.SegmentsPerSecond = ConvertInteger(params[1]);
            break;
        default:
            break;
    }
}

void SusAnalyzer::ProcessData(const xp::smatch &result, const uint32_t line)
{
    auto meas = result[1].str();
    auto lane = result[2].str();
    auto pattern = result[3].str();
    ba::erase_all(pattern, " ");

    /*
     判定順について
     0. #...** (BPMなど)
     1. #---0* (特殊データ、定義分割不可)
     2. #---1* (Short)
     3. #---5* (Air)
     4. #---[234]*. (Long)
    */

    const auto noteCount = pattern.length() / 2;
    const auto step = uint32_t(ticksPerBeat * GetBeatsAt(ConvertInteger(meas))) / (!noteCount ? 1 : noteCount);

    if (!regex_match(meas, allNumeric)) {
        // コマンドデータ
        transform(meas.cbegin(), meas.cend(), meas.begin(), toUpper);
        if (meas == "BPM") {
            const auto number = ConvertHexatridecimal(lane);
            const auto value = ConvertFloat(pattern);
            bpmDefinitions[number] = value;
            if (SharedMetaData.ShowBpm < 0) SharedMetaData.ShowBpm = value;
        } else if (meas == "TIL") {
            const auto number = ConvertHexatridecimal(lane);
            auto it = hispeedDefinitions.find(number);
            if (it == hispeedDefinitions.end()) {
                auto hs = make_shared<SusHispeedTimeline>([&](const uint32_t m, const uint32_t t) { return GetAbsoluteTime(m, t); });
                hs->AddKeysByString(convertRawString(pattern), timelineResolver);
                hispeedDefinitions[number] = hs;
            } else {
                it->second->AddKeysByString(convertRawString(pattern), timelineResolver);
            }
        } else if (meas == "ATR") {
            const auto number = ConvertHexatridecimal(lane);
            auto it = extraAttributes.find(number);
            if (it == extraAttributes.end()) {
                auto ea = make_shared<SusNoteExtraAttribute>();
                ea->Apply(convertRawString(pattern));
                extraAttributes[number] = ea;
            } else {
                it->second->Apply(convertRawString(pattern));
            }
        } else {
            MakeMessage(line, u8"不正なデータコマンドです");
        }
    } else if (lane[0] == '0') {
        switch (lane[1]) {
            case '2':
                // 小節長
                beatsDefinitions[measureCountOffset + ConvertInteger(meas)] = ConvertFloat(pattern);
                break;
            case '8': {
                // BPM
                for (auto i = 0u; i < noteCount; i++) {
                    const auto note = pattern.substr(i * 2, 2);
                    SusRawNoteData noteData;
                    SusRelativeNoteTime time = { measureCountOffset + ConvertInteger(meas), step * i };
                    noteData.Type.set(size_t(SusNoteType::Undefined));
                    noteData.DefinitionNumber = ConvertHexatridecimal(note);
                    if (noteData.DefinitionNumber) notes.emplace_back(time, noteData);
                }
                break;
            }
            default:
                MakeMessage(line, u8"不正なデータコマンドです");
                break;
        }
    } else if (lane[0] == '1') {
        // ショートノーツ
        for (auto i = 0u; i < noteCount; i++) {
            auto note = pattern.substr(i * 2, 2);
            SusRawNoteData noteData;
            SusRelativeNoteTime time = { measureCountOffset + ConvertInteger(meas), step * i };
            noteData.NotePosition.StartLane = ConvertHexatridecimal(lane.substr(1, 1));
            noteData.NotePosition.Length = ConvertHexatridecimal(note.substr(1, 1));
            noteData.Timeline = hispeedToApply;
            noteData.ExtraAttribute = extraAttributeToApply;

            switch (note[0]) {
                case '1':
                    noteData.Type.set(size_t(SusNoteType::Tap));
                    break;
                case '2':
                    noteData.Type.set(size_t(SusNoteType::ExTap));
                    break;
                case '3':
                    noteData.Type.set(size_t(SusNoteType::Flick));
                    break;
                case '4':
                    noteData.Type.set(size_t(SusNoteType::HellTap));
                    break;
                case '5':
                    noteData.Type.set(size_t(SusNoteType::AwesomeExTap));
                    noteData.Type.set(size_t(SusNoteType::Up));
                    break;
                case '6':
                    noteData.Type.set(size_t(SusNoteType::AwesomeExTap));
                    noteData.Type.set(size_t(SusNoteType::Down));
                    break;
                default:
                    if (note[1] == '0') continue;
                    MakeMessage(line, u8"ショートレーンの指定が不正です。");
                    continue;
            }
            notes.emplace_back(time, noteData);
        }
    } else if (lane[0] == '5') {
        // Airノーツ
        for (auto i = 0u; i < noteCount; i++) {
            auto note = pattern.substr(i * 2, 2);
            SusRawNoteData noteData;
            SusRelativeNoteTime time = { measureCountOffset + ConvertInteger(meas), step * i };
            noteData.NotePosition.StartLane = ConvertHexatridecimal(lane.substr(1, 1));
            noteData.NotePosition.Length = ConvertHexatridecimal(note.substr(1, 1));
            noteData.Timeline = hispeedToApply;
            noteData.ExtraAttribute = extraAttributeToApply;

            switch (note[0]) {
                case '1':
                    noteData.Type.set(size_t(SusNoteType::Air));
                    noteData.Type.set(size_t(SusNoteType::Up));
                    break;
                case '2':
                    noteData.Type.set(size_t(SusNoteType::Air));
                    noteData.Type.set(size_t(SusNoteType::Down));
                    break;
                case '3':
                    noteData.Type.set(size_t(SusNoteType::Air));
                    noteData.Type.set(size_t(SusNoteType::Up));
                    noteData.Type.set(size_t(SusNoteType::Left));
                    break;
                case '4':
                    noteData.Type.set(size_t(SusNoteType::Air));
                    noteData.Type.set(size_t(SusNoteType::Up));
                    noteData.Type.set(size_t(SusNoteType::Right));
                    break;
                case '5':
                    noteData.Type.set(size_t(SusNoteType::Air));
                    noteData.Type.set(size_t(SusNoteType::Down));
                    noteData.Type.set(size_t(SusNoteType::Right));
                    break;
                case '6':
                    noteData.Type.set(size_t(SusNoteType::Air));
                    noteData.Type.set(size_t(SusNoteType::Down));
                    noteData.Type.set(size_t(SusNoteType::Left));
                    break;
                case '7':
                    noteData.Type.set(size_t(SusNoteType::Air));
                    noteData.Type.set(size_t(SusNoteType::Up));
                    noteData.Type.set(size_t(SusNoteType::Grounded));
                    break;
                case '8':
                    noteData.Type.set(size_t(SusNoteType::Air));
                    noteData.Type.set(size_t(SusNoteType::Up));
                    noteData.Type.set(size_t(SusNoteType::Left));
                    noteData.Type.set(size_t(SusNoteType::Grounded));
                    break;
                case '9':
                    noteData.Type.set(size_t(SusNoteType::Air));
                    noteData.Type.set(size_t(SusNoteType::Up));
                    noteData.Type.set(size_t(SusNoteType::Right));
                    noteData.Type.set(size_t(SusNoteType::Grounded));
                    break;
                default:
                    if (note[1] == '0') continue;
                    MakeMessage(line, u8"Airレーンの指定が不正です。");
                    continue;
            }
            notes.emplace_back(time, noteData);
        }
    } else if (lane.length() == 3 && lane[0] >= '2' && lane[0] <= '4') {
        // ロングタイプ
        for (auto i = 0u; i < noteCount; i++) {
            auto note = pattern.substr(i * 2, 2);
            SusRawNoteData noteData;
            SusRelativeNoteTime time = { measureCountOffset + ConvertInteger(meas), step * i };
            noteData.NotePosition.StartLane = ConvertHexatridecimal(lane.substr(1, 1));
            noteData.NotePosition.Length = ConvertHexatridecimal(note.substr(1, 1));
            noteData.Extra = ConvertHexatridecimal(lane.substr(2, 1));
            noteData.Timeline = hispeedToApply;
            noteData.ExtraAttribute = extraAttributeToApply;

            switch (lane[0]) {
                case '2':
                    noteData.Type.set(size_t(SusNoteType::Hold));
                    break;
                case '3':
                    noteData.Type.set(size_t(SusNoteType::Slide));
                    break;
                case '4':
                    noteData.Type.set(size_t(SusNoteType::AirAction));
                    break;
                default:
                    MakeMessage(line, u8"ロングレーンの指定が不正です。");
                    continue;
            }
            switch (note[0]) {
                case '1':
                    noteData.Type.set(size_t(SusNoteType::Start));
                    break;
                case '2':
                    noteData.Type.set(size_t(SusNoteType::End));
                    break;
                case '3':
                    noteData.Type.set(size_t(SusNoteType::Step));
                    break;
                case '4':
                    noteData.Type.set(size_t(SusNoteType::Control));
                    break;
                case '5':
                    noteData.Type.set(size_t(SusNoteType::Invisible));
                    break;
                default:
                    if (note[1] == '0') continue;
                    MakeMessage(line, u8"ノーツ種類の指定が不正です。");
                    continue;
            }
            notes.emplace_back(time, noteData);
        }
    } else if (lane.length() == 3 && lane[0] == '9') {
        // z = 0のレーン指定(Tap) #mmm800~#mmm80f~#mmm8ff
        const auto endlane = lane.substr(1, 1);
        const auto startlane = lane.substr(2, 1);

        for (auto i = 0u; i < noteCount; i++) {
            auto note = pattern.substr(i * 2, 2);
            if (note[1] == '0') continue;
            if (note[0] != '1') {
                MakeMessage(line, u8"スタートレーンの指定が不正です。");
                continue;
            }
            SusRawNoteData noteData;
            SusRelativeNoteTime time = { measureCountOffset + ConvertInteger(meas), step * i };
            noteData.NotePosition.StartLane = ConvertHexatridecimal(endlane);
            noteData.Extra = ConvertHexatridecimal(startlane);
            noteData.NotePosition.Length = ConvertHexatridecimal(note.substr(1, 1));
            noteData.Type.set(size_t(SusNoteType::StartPosition));
            // noteData.Timeline = hispeedToApply;
            // noteData.ExtraAttribute = extraAttributeToApply;
            notes.emplace_back(time, noteData);
        }
    } else {
        // 不正
        MakeMessage(line, u8"不正なデータ定義です。");
    }
}

void SusAnalyzer::MakeMessage(const uint32_t line, const string &message)
{
    ostringstream ss;
    ss << line << u8"行目: " << message;
    for (const auto &cb : errorCallbacks) cb("Error", ss.str());
}

void SusAnalyzer::MakeMessage(const uint32_t meas, const uint32_t tick, const uint32_t lane, const std::string &message)
{
    ostringstream ss;
    ss << meas << u8"'" << tick << u8"@" << lane << u8": " << message;
    for (const auto &cb : errorCallbacks) cb("Error", ss.str());
}

float SusAnalyzer::GetBeatsAt(const uint32_t measure)
{
    float result = defaultBeats;
    auto last = 0;
    for (auto &t : beatsDefinitions) {
        if (t.first >= last && t.first <= measure) {
            result = t.second;
            last = t.first;
        }
    }
    return result;
}

double SusAnalyzer::GetBpmAt(const uint32_t measure, const uint32_t tick)
{
    auto result = defaultBpm;
    for (auto &t : bpmChanges) {
        if (get<0>(t).Measure != measure) continue;
        if (get<0>(t).Tick < tick) continue;
        result = bpmDefinitions[get<1>(t).DefinitionNumber];
    }
    return result;
}

double SusAnalyzer::GetAbsoluteTime(uint32_t meas, uint32_t tick)
{
    auto time = 0.0;
    auto lastBpm = defaultBpm;
    //超過したtick指定にも対応したほうが使いやすいよね
    while (tick >= GetBeatsAt(meas) * ticksPerBeat) tick -= GetBeatsAt(meas++) * ticksPerBeat;

    for (auto i = 0u; i < meas + 1; i++) {
        const auto beats = GetBeatsAt(i);
        auto lastChangeTick = 0u;
        for (auto& bc : bpmChanges) {
            if (get<0>(bc).Measure != i) continue;
            const auto timing = get<0>(bc);
            if (i == meas && timing.Tick >= tick) break;
            const auto dur = (60.0 / lastBpm) * (double(timing.Tick - lastChangeTick) / ticksPerBeat);
            time += dur;
            lastChangeTick = timing.Tick;
            lastBpm = bpmDefinitions[get<1>(bc).DefinitionNumber];
        }
        if (i == meas) {
            time += (60.0 / lastBpm) * (double(tick - lastChangeTick) / ticksPerBeat);
        } else {
            time += (60.0 / lastBpm) * (double(ticksPerBeat * beats - lastChangeTick) / ticksPerBeat);
        }
    }

    return time;
};

tuple<uint32_t, uint32_t> SusAnalyzer::GetRelativeTime(const double time)
{
    auto restTime = time;
    uint32_t meas = 0;
    auto secPerBeat = (60.0 / 120.0);

    while (true) {
        const auto beats = GetBeatsAt(meas);
        auto lastChangeTick = 0u;

        for (auto& bc : bpmChanges) {
            const auto timing = get<0>(bc);
            if (timing.Measure != meas) continue;
            const auto dur = secPerBeat * (double(timing.Tick - lastChangeTick) / ticksPerBeat);
            if (dur >= restTime) return make_tuple(meas, lastChangeTick + restTime / secPerBeat * ticksPerBeat);
            restTime -= dur;
            lastChangeTick = timing.Tick;
            secPerBeat = 60.0 / bpmDefinitions[get<1>(bc).DefinitionNumber];
        }
        const double restTicks = ticksPerBeat * beats - lastChangeTick;
        const auto restDuration = restTicks / ticksPerBeat * secPerBeat;
        if (restDuration >= restTime) return make_tuple(meas, lastChangeTick + restTime / secPerBeat * ticksPerBeat);
        restTime -= restDuration;
        meas++;
    }
}

uint32_t SusAnalyzer::GetRelativeTicks(const uint32_t measure, const uint32_t tick)
{
    uint32_t result = 0;
    for (auto i = 0u; i < measure; i++) result += GetBeatsAt(i) * ticksPerBeat;
    return result + tick;
}

void SusAnalyzer::RenderScoreData(DrawableNotesList &data, NoteCurvesList &curveData)
{
    // 不正チェックリスト
    // ショート: はみ出しは全部アウト
    // ホールド: ケツ無しアウト(ケツ連は無視)、Step/Control問答無用アウト、ケツ違いアウト
    // スライド、AA: ケツ無しアウト(ケツ連は無視)
    data.clear();
    for (auto& note : notes) {
        const auto time = get<0>(note);
        auto info = get<1>(note);
        if (info.Type[size_t(SusNoteType::Step)]) continue;
        if (info.Type[size_t(SusNoteType::Control)]) continue;
        if (info.Type[size_t(SusNoteType::Invisible)]) continue;
        if (info.Type[size_t(SusNoteType::End)]) continue;
        if (info.Type[size_t(SusNoteType::Undefined)]) continue;

        const auto bits = info.Type.to_ulong();
        auto noteData = make_shared<SusDrawableNoteData>();
        if (bits & SU_NOTE_LONG_MASK) {
            auto genCurve = true;
            noteData->Type = info.Type;
            noteData->StartTime = GetAbsoluteTime(time.Measure, time.Tick);
            noteData->StartLane = info.NotePosition.StartLane;
            noteData->Length = info.NotePosition.Length;
            noteData->CenterAtZero = noteData->StartLane + noteData->Length / 2.0;
            noteData->Timeline = info.Timeline;
            noteData->ExtraAttribute = info.ExtraAttribute;
            noteData->StartTimeEx = get<1>(noteData->Timeline->GetRawDrawStateAt(noteData->StartTime));

            auto ltype = SusNoteType::Undefined;
            switch ((bits >> 7) & 7) {
                case 1:
                    ltype = SusNoteType::Hold;
                    genCurve = false;
                    break;
                case 2:
                    ltype = SusNoteType::Slide;
                    break;
                case 4:
                    ltype = SusNoteType::AirAction;
                    break;
                default: break;
            }

            auto completed = false;
            auto lastStep = note;
            for (const auto &it : notes) {
                const auto curPos = get<0>(it);
                auto curNo = get<1>(it);
                if (!curNo.Type.test(size_t(ltype)) || curNo.Extra != info.Extra) continue;
                if (curPos.Measure < time.Measure) continue;
                if (curPos.Measure == time.Measure && curPos.Tick < time.Tick) continue;
                switch (ltype) {
                    case SusNoteType::Hold: {
                        if (curNo.Type.test(size_t(SusNoteType::Control)) || curNo.Type.test(size_t(SusNoteType::Invisible)))
                            MakeMessage(curPos.Measure, curPos.Tick, curNo.NotePosition.StartLane, u8"HoldでControl/Invisibleは指定できません。");
                        if (curNo.NotePosition.StartLane != info.NotePosition.StartLane || curNo.NotePosition.Length != info.NotePosition.Length)
                            MakeMessage(curPos.Measure, curPos.Tick, curNo.NotePosition.StartLane, u8"Holdの長さ/位置が始点と一致していません。");
                    }
                                            /* ホールドだけ追加チェックしてフォールスルー */
                    case SusNoteType::Slide:
                    case SusNoteType::AirAction: {
                        if (curNo.Type.test(size_t(SusNoteType::Start))) break;

                        auto nextNote = make_shared<SusDrawableNoteData>();
                        nextNote->StartTime = GetAbsoluteTime(curPos.Measure, curPos.Tick);
                        nextNote->StartLane = curNo.NotePosition.StartLane;
                        nextNote->Length = curNo.NotePosition.Length;
                        // 暫定
                        nextNote->CenterAtZero = nextNote->StartLane + nextNote->Length / 2.0;
                        nextNote->Type = curNo.Type;
                        nextNote->Timeline = curNo.Timeline;
                        nextNote->ExtraAttribute = curNo.ExtraAttribute;
                        nextNote->StartTimeEx = get<1>(nextNote->Timeline->GetRawDrawStateAt(nextNote->StartTime));

                        if (curNo.Type.test(size_t(SusNoteType::Step)) || curNo.Type.test(size_t(SusNoteType::End))) {
                            const auto lsrt = get<0>(lastStep);
                            const auto injc = double(GetRelativeTicks(curPos.Measure, curPos.Tick) - GetRelativeTicks(lsrt.Measure, lsrt.Tick)) / ticksPerBeat * longInjectionPerBeat;
                            for (auto i = 1; i < injc; i++) {
                                const auto insertAt = lsrt.Tick + (ticksPerBeat / longInjectionPerBeat * i);
                                auto injection = make_shared<SusDrawableNoteData>();
                                injection->Type.set(size_t(SusNoteType::Injection));
                                injection->StartTime = GetAbsoluteTime(lsrt.Measure, insertAt);
                                noteData->ExtraData.push_back(injection);
                            }
                        }

                        if (nextNote->Type.test(size_t(SusNoteType::Step))) lastStep = it;

                        if (curNo.Type.test(size_t(SusNoteType::End))) {
                            noteData->Duration = nextNote->StartTime - noteData->StartTime;
                            completed = true;
                        }
                        noteData->ExtraData.push_back(nextNote);
                        break;
                    }
                    default:
                        // TODO: 多分エラー
                        break;
                }
                if (completed) break;
            }
            if (!completed) {
                MakeMessage(time.Measure, time.Tick, info.NotePosition.StartLane, u8"ロングノーツに終点がありません。");
            } else {
                data.push_back(noteData);
                SharedMetaData.ScoreDuration = max(SharedMetaData.ScoreDuration, noteData->StartTime + noteData->Duration);
                if (genCurve) CalculateCurves(noteData, curveData);
            }
        } else if (bits & SU_NOTE_SHORT_MASK) {
            // ショート
            if (info.NotePosition.StartLane + info.NotePosition.Length > 16) {
                MakeMessage(time.Measure, time.Tick, info.NotePosition.StartLane, u8"ショートノーツがはみ出しています。");
            }
            noteData->Type = info.Type;
            noteData->StartTime = GetAbsoluteTime(time.Measure, time.Tick);
            noteData->Duration = 0;
            noteData->StartLane = info.NotePosition.StartLane;
            noteData->Length = info.NotePosition.Length;
            noteData->Timeline = info.Timeline;
            noteData->ExtraAttribute = info.ExtraAttribute;
            noteData->StartTimeEx = get<1>(noteData->Timeline->GetRawDrawStateAt(noteData->StartTime));
            // RollHispeed組み込み
            if (noteData->ExtraAttribute->RollHispeedNumber >= 0) {
                if (hispeedDefinitions.find(noteData->ExtraAttribute->RollHispeedNumber) != hispeedDefinitions.end()) {
                    noteData->ExtraAttribute->RollTimeline = hispeedDefinitions[noteData->ExtraAttribute->RollHispeedNumber];
                } else {
                    MakeMessage(0, u8"指定されたタイムラインが存在しません");
                    noteData->ExtraAttribute->RollHispeedNumber = -1;
                }
            }
            // Air接地処理
            // if ((下に別ノーツがある && それはロング終点) || 下に別ノーツがない)
            if (info.Type[size_t(SusNoteType::Air)] && !info.Type[size_t(SusNoteType::Grounded)]) {
                auto found = false;
                for (const auto &target : notes) {
                    if (get<1>(target) == get<1>(note)) continue;
                    const auto gtime = get<0>(target);
                    auto ginfo = get<1>(target);
                    const auto tbits = ginfo.Type.to_ulong();
                    if (time != gtime) continue;
                    if (info.NotePosition.StartLane != ginfo.NotePosition.StartLane
                        || info.NotePosition.Length != ginfo.NotePosition.Length)
                        continue;

                    found = true;
                    if (tbits & SU_NOTE_LONG_MASK && ginfo.Type[size_t(SusNoteType::End)]) {
                        noteData->Type.set(size_t(SusNoteType::Grounded));
                    }
                    break;
                }
                if (!found) noteData->Type.set(size_t(SusNoteType::Grounded));
            }
            // 移動レーン処理
            noteData->CenterAtZero = noteData->StartLane + noteData->Length / 2.0;
            if (SharedMetaData.ExtraFlags[size_t(SusMetaDataFlags::EnableMovingLane)]) {
                for (const auto &startSource : notes) {
                    const auto mltime = get<0>(startSource);
                    const auto &mlinfo = get<1>(startSource);
                    if (mltime != time) continue;
                    if (mlinfo.NotePosition.StartLane != info.NotePosition.StartLane) continue;
                    if (!mlinfo.Type[size_t(SusNoteType::StartPosition)]) continue;
                    noteData->CenterAtZero = mlinfo.Extra + mlinfo.NotePosition.Length / 2.0;
                }
            }
            data.push_back(noteData);
            SharedMetaData.ScoreDuration = max(SharedMetaData.ScoreDuration, noteData->StartTime);
        } else if (info.Type[size_t(SusNoteType::MeasureLine)]) {
            noteData->Type = info.Type;
            noteData->ExtraAttribute = info.ExtraAttribute;
            noteData->StartTime = GetAbsoluteTime(time.Measure, 0);
            noteData->Duration = 0;
            noteData->Timeline = info.Timeline;
            noteData->StartTimeEx = get<1>(noteData->Timeline->GetRawDrawStateAt(noteData->StartTime));
            data.push_back(noteData);
        } else if (!info.Type[size_t(SusNoteType::StartPosition)]) {
            MakeMessage(time.Measure, time.Tick, info.NotePosition.StartLane, u8"致命的なノーツエラー(不正な内部表現です)。");
        }
    }
}

void SusAnalyzer::CalculateCurves(const shared_ptr<SusDrawableNoteData>& note, NoteCurvesList &curveData) const
{
    auto lastStep = note;
    vector<tuple<double, double>> controlPoints;    // lastStepからの時間, X中央位置(0~1)
    vector<tuple<double, double>> bezierBuffer;

    controlPoints.emplace_back(0, (lastStep->StartLane + lastStep->Length / 2.0) / 16.0);
    for (auto &slideElement : note->ExtraData) {
        if (slideElement->Type.test(size_t(SusNoteType::Injection))) continue;
        if (slideElement->Type.test(size_t(SusNoteType::Control))) {
            const auto cpi = make_tuple(slideElement->StartTime - lastStep->StartTime, (slideElement->StartLane + slideElement->Length / 2.0) / 16.0);
            controlPoints.push_back(cpi);
            continue;
        }
        // EndかStepかInvisible
        controlPoints.emplace_back(slideElement->StartTime - lastStep->StartTime, (slideElement->StartLane + slideElement->Length / 2.0) / 16.0);
        const int segmentPoints = SharedMetaData.SegmentsPerSecond * (slideElement->StartTime - lastStep->StartTime) + 2;
        vector<tuple<double, double>> segmentPositions;
        for (auto j = 0; j < segmentPoints; j++) {
            const auto relativeTimeInBlock = j / double(segmentPoints - 1);
            bezierBuffer.clear();
            copy(controlPoints.begin(), controlPoints.end(), back_inserter(bezierBuffer));
            for (int k = controlPoints.size() - 1; k >= 0; k--) {
                for (auto l = 0; l < k; l++) {
                    auto derivedTime = glm::mix(get<0>(bezierBuffer[l]), get<0>(bezierBuffer[l + 1]), relativeTimeInBlock);
                    auto derivedPosition = glm::mix(get<1>(bezierBuffer[l]), get<1>(bezierBuffer[l + 1]), relativeTimeInBlock);
                    bezierBuffer[l] = make_tuple(derivedTime, derivedPosition);
                }
            }
            segmentPositions.push_back(bezierBuffer[0]);
        }
        curveData[slideElement] = segmentPositions;
        lastStep = slideElement;
        controlPoints.clear();
        controlPoints.emplace_back(0, (slideElement->StartLane + slideElement->Length / 2.0) / 16.0);
    }
}

// SusHispeedTimeline ------------------------------------------------------------------

const double SusHispeedData::keepSpeed = numeric_limits<double>::quiet_NaN();

SusHispeedTimeline::SusHispeedTimeline(function<double(uint32_t, uint32_t)> func) : relToAbs(std::move(func))
{
    keys.emplace_back(SusRelativeNoteTime { 0, 0 }, SusHispeedData { SusHispeedData::Visibility::Visible, 1.0 });
}

void SusHispeedTimeline::AddKeysByString(const string &def, const function<shared_ptr<SusHispeedTimeline>(uint32_t)>& resolver)
{
    // int'int:double:v/i
    auto str = def;
    vector<string> ks;

    ba::erase_all(str, " ");
    split(ks, str, b::is_any_of(","));
    for (const auto &k : ks) {
        vector<string> params;
        split(params, k, b::is_any_of(":"));
        if (params.size() < 2) return;
        if (params[0] == "inherit") {
            // データ流用
            const auto from = ConvertHexatridecimal(params[1]);
            auto parent = resolver(from);
            if (!parent) continue;
            for (auto &parentKey : parent->keys) keys.push_back(parentKey);
            continue;
        }

        vector<string> timing;
        split(timing, params[0], b::is_any_of("'"));
        SusRelativeNoteTime time = { ConvertInteger(timing[0]), ConvertInteger(timing[1]) };
        SusHispeedData data = { SusHispeedData::Visibility::Keep, SusHispeedData::keepSpeed };
        for (auto i = 1u; i < params.size(); i++) {
            if (params[i] == "v" || params[i] == "visible") {
                data.VisibilityState = SusHispeedData::Visibility::Visible;
            } else if (params[i] == "i" || params[i] == "invisible") {
                data.VisibilityState = SusHispeedData::Visibility::Invisible;
            } else {
                data.Speed = ConvertFloat(params[i]);
            }
        }
        auto found = false;
        for (auto &p : keys) {
            if (p.first == time) {
                p.second = data;
                found = true;
                break;
            }
        }
        if (!found) keys.emplace_back(time, data);
    }
}

void SusHispeedTimeline::AddKeyByData(const uint32_t meas, const uint32_t tick, const double hs)
{
    SusRelativeNoteTime time = { meas, tick };
    for (auto &p : keys) {
        if (p.first != time) continue;
        p.second.Speed = hs;
        return;
    }
    SusHispeedData data = { SusHispeedData::Visibility::Keep, hs };
    keys.emplace_back(time, data);
}

void SusHispeedTimeline::AddKeyByData(const uint32_t meas, const uint32_t tick, const bool vis)
{
    SusRelativeNoteTime time = { meas, tick };
    const auto vv = vis ? SusHispeedData::Visibility::Visible : SusHispeedData::Visibility::Invisible;
    for (auto &p : keys) {
        if (p.first != time) continue;
        p.second.VisibilityState = vv;
        return;
    }
    SusHispeedData data = { vv, SusHispeedData::keepSpeed };
    keys.emplace_back(time, data);
}

void SusHispeedTimeline::Finialize()
{
    stable_sort(keys.begin(), keys.end(), [](const pair<SusRelativeNoteTime, SusHispeedData> &a, const pair<SusRelativeNoteTime, SusHispeedData> &b) {
        return a.first.Tick < b.first.Tick;
    });
    stable_sort(keys.begin(), keys.end(), [](const pair<SusRelativeNoteTime, SusHispeedData> &a, const pair<SusRelativeNoteTime, SusHispeedData> &b) {
        return a.first.Measure < b.first.Measure;
    });
    auto hs = 1.0;
    auto vis = true;
    for (auto &key : keys) {
        if (!isnan(key.second.Speed)) {
            hs = key.second.Speed;
        } else {
            key.second.Speed = hs;
        }
        if (key.second.VisibilityState != SusHispeedData::Visibility::Keep) {
            vis = key.second.VisibilityState == SusHispeedData::Visibility::Visible;
        } else {
            key.second.VisibilityState = vis ? SusHispeedData::Visibility::Visible : SusHispeedData::Visibility::Invisible;
        }
    }

    auto it = keys.begin();
    double sum = 0;
    double lastAt = 0;
    auto lastSpeed = 1.0;
    for (auto &rd : keys) {
        auto t = relToAbs(rd.first.Measure, rd.first.Tick);
        sum += (t - lastAt) * lastSpeed;
        data.emplace_back(t, sum, rd.second);
        lastAt = t;
        lastSpeed = rd.second.Speed;
    }
    keys.clear();
}

tuple<bool, double> SusHispeedTimeline::GetRawDrawStateAt(const double time)
{
    auto lastData = data[0];
    auto check = 0;
    for (auto &d : data) {
        if (!check++) continue;
        const auto keyTime = get<0>(d);
        if (keyTime >= time) break;
        lastData = d;
    }
    const auto lastDifference = time - get<0>(lastData);
    return make_tuple(get<2>(lastData).VisibilityState == SusHispeedData::Visibility::Visible, get<1>(lastData) + lastDifference * get<2>(lastData).Speed);
}

double SusHispeedTimeline::GetSpeedAt(const double time)
{
    auto lastData = data[0];
    auto check = 0;
    for (auto &d : data) {
        if (!check++) continue;
        const auto keyTime = get<0>(d);
        if (keyTime >= time) break;
        lastData = d;
    }
    return get<2>(lastData).Speed;
}

tuple<bool, double> SusDrawableNoteData::GetStateAt(const double time)
{
    auto result = Timeline->GetRawDrawStateAt(time);
    ModifiedPosition = StartTimeEx - get<1>(result);
    for (auto &ex : ExtraData) {
        if (!ex->Timeline) {
            ex->ModifiedPosition = numeric_limits<double>::quiet_NaN();
            continue;
        }
        auto eres = ex->Timeline->GetRawDrawStateAt(time);
        ex->ModifiedPosition = ex->StartTimeEx - get<1>(eres);
    }
    return make_tuple(get<0>(result), StartTimeEx - get<1>(result));
}

void SusNoteExtraAttribute::Apply(const string &props)
{
    using namespace boost::algorithm;
    auto list = props;
    list.erase(remove(list.begin(), list.end(), ' '), list.end());
    vector<string> params;
    split(params, list, is_any_of(","));

    vector<string> pr;
    for (auto& p : params) {
        pr.clear();
        split(pr, p, is_any_of(":"));
        if (pr.size() != 2) continue;
        switch (Crc32Rec(0xffffffff, pr[0].c_str())) {
            case "priority"_crc32:
            case "pr"_crc32:
                Priority = uint32_t(strtol(pr[1].c_str(), nullptr, 10));
                break;
            case "rollhs"_crc32:
            case "rh"_crc32:
                RollHispeedNumber = ConvertHexatridecimal(pr[1]);
                break;
            case "height"_crc32:
            case "h"_crc32:
                HeightScale = double(strtod(pr[1].c_str(), nullptr));
                break;
            default:
                // TODO: メッセージ出す
                break;
        }
    }
}
