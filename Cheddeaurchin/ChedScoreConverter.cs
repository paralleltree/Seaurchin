using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Ched.Components;
using Ched.Components.Notes;
using Ched.Components.Events;

namespace Cheddeaurchin
{
    /// <summary>
    /// Chedの<see cref="Score"/>をSeaurchinの内部形式と互換性のある形式に変換
    /// </summary>
    public sealed class ChedScoreConverter
    {
        public static readonly SusNoteType LongNoteMask =
            SusNoteType.Hold
            | SusNoteType.Slide
            | SusNoteType.AirAction;
        public static readonly SusNoteType ShortNoteMask =
            SusNoteType.Tap
            | SusNoteType.ExTap
            | SusNoteType.Flick
            | SusNoteType.HellTap
            | SusNoteType.Air
            | SusNoteType.AwesomeExTap;
        public static readonly double DefaultBpm = 120.0;
        public static readonly double DefaultBeats = 4.0;
        public static readonly double InjectionsPerBeat = 2.0;

        private Dictionary<uint, double> beatsDefinitions = new Dictionary<uint, double>();
        private List<Tuple<SusRelativeTime, double>> bpmChanges = new List<Tuple<SusRelativeTime, double>>();

        /// <summary>
        /// tpb value
        /// </summary>
        public uint TicksPerBeat => (uint)ReferencialScore.TicksPerBeat;

        public Score ReferencialScore { get; }

        /// <summary>
        /// コンストラクタで全部生成します。
        /// </summary>
        /// <param name="chedScore"></param>
        public ChedScoreConverter(Score chedScore)
        {
            ReferencialScore = chedScore;

            InitializeBeatsDefinitions(chedScore);
            InitializeBpmChanges(chedScore);
        }

        /// <summary>
        /// 指定した小節の拍数を取得する
        /// </summary>
        /// <param name="measure"></param>
        /// <returns></returns>
        public double GetBeatsAt(uint measure)
        {
            double result = DefaultBeats;
            var last = 0u;
            foreach (var bdp in beatsDefinitions)
            {
                if (bdp.Key >= last && bdp.Key <= measure)
                {
                    result = bdp.Value;
                    last = bdp.Key;
                }
            }
            return result;
        }

        /// <summary>
        /// 総tickから小節番号を逆算する
        /// </summary>
        /// <param name="tick"></param>
        /// <returns></returns>
        public SusRelativeTime GetRelativeTimeFromTicks(uint tick)
        {
            var rest = tick;
            var measure = 0u;
            var ticksPerMeasure = 0u;
            while (rest > (ticksPerMeasure = (uint)(GetBeatsAt(measure) * TicksPerBeat)))
            {
                rest -= ticksPerMeasure;
                ++measure;
            }

            return new SusRelativeTime { Measure = measure, Tick = rest };
        }

        /// <summary>
        /// 絶対時刻に変換
        /// </summary>
        /// <param name="measure"></param>
        /// <param name="tick"></param>
        public double GetAbsoluteTime(uint measure, uint tick)
        {
            double time = 0.0;
            double lastBpm = DefaultBpm;

            while (tick >= GetBeatsAt(measure) * TicksPerBeat) tick -= (uint)(GetBeatsAt(measure++) * TicksPerBeat);
            for (uint i = 0u; i < measure + 1; ++i)
            {
                var beats = GetBeatsAt(i);
                var lastChangeTick = 0u;
                foreach (var bc in bpmChanges)
                {
                    if (bc.Item1.Measure != i) continue;
                    var bcTime = bc.Item1;
                    if (i == measure && bcTime.Tick >= tick) break;
                    time += (60.0 / lastBpm) * ((double)(bcTime.Tick - lastChangeTick) / TicksPerBeat);
                    lastChangeTick = bcTime.Tick;
                    lastBpm = bc.Item2;
                }
                if (i == measure)
                {
                    time += (60.0 / lastBpm) * ((double)(tick - lastChangeTick) / TicksPerBeat);
                }
                else
                {
                    time += (60.0 / lastBpm) * ((TicksPerBeat * beats - lastChangeTick) / TicksPerBeat);
                }
            }

            return time;
        }

        /// <summary>
        /// リスト化する
        /// </summary>
        /// <returns></returns>
        public List<SusAbsoluteNote> GenerateSusNotesList()
        {
            var resultBuffer = new List<SusAbsoluteNote>();

            // Tap
            foreach (var note in ReferencialScore.Notes.Taps)
            {
                var rt = GetRelativeTimeFromTicks((uint)note.Tick);
                var sn = SusAbsoluteNote.MakeShortNote(
                    SusNoteType.Tap,
                    GetAbsoluteTime(rt.Measure, rt.Tick),
                    (ushort)note.LaneIndex,
                    (ushort)note.Width);
                resultBuffer.Add(sn);
            }

            // ExTap
            foreach (var note in ReferencialScore.Notes.ExTaps)
            {
                var rt = GetRelativeTimeFromTicks((uint)note.Tick);
                var sn = SusAbsoluteNote.MakeShortNote(
                    SusNoteType.ExTap,
                    GetAbsoluteTime(rt.Measure, rt.Tick),
                    (ushort)note.LaneIndex,
                    (ushort)note.Width);
                resultBuffer.Add(sn);
            }

            // Flick
            foreach (var note in ReferencialScore.Notes.Flicks)
            {
                var rt = GetRelativeTimeFromTicks((uint)note.Tick);
                var sn = SusAbsoluteNote.MakeShortNote(
                    SusNoteType.Flick,
                    GetAbsoluteTime(rt.Measure, rt.Tick),
                    (ushort)note.LaneIndex,
                    (ushort)note.Width);
                resultBuffer.Add(sn);
            }

            // HellTap
            foreach (var note in ReferencialScore.Notes.Damages)
            {
                var rt = GetRelativeTimeFromTicks((uint)note.Tick);
                var sn = SusAbsoluteNote.MakeShortNote(
                    SusNoteType.HellTap,
                    GetAbsoluteTime(rt.Measure, rt.Tick),
                    (ushort)note.LaneIndex,
                    (ushort)note.Width);
                resultBuffer.Add(sn);
            }

            // Air
            foreach (var note in ReferencialScore.Notes.Airs)
            {
                var rt = GetRelativeTimeFromTicks((uint)note.Tick);
                var type = SusNoteType.Air;
                type |= (note.HorizontalDirection == HorizontalAirDirection.Left) ? SusNoteType.Left
                        : (note.HorizontalDirection == HorizontalAirDirection.Right) ? SusNoteType.Right
                        : 0;
                type |= (note.VerticalDirection == VerticalAirDirection.Up) ? SusNoteType.Up
                        : SusNoteType.Down;
                var sn = SusAbsoluteNote.MakeShortNote(
                    type,
                    GetAbsoluteTime(rt.Measure, rt.Tick),
                    (ushort)note.LaneIndex,
                    (ushort)note.Width);
                resultBuffer.Add(sn);
            }

            // Hold
            foreach (var note in ReferencialScore.Notes.Holds)
            {
                var headTime = GetRelativeTimeFromTicks((uint)note.StartNote.Tick);
                var tailTime = GetRelativeTimeFromTicks((uint)note.EndNote.Tick);
                var tail = new List<SusAbsoluteNote>();

                var injections = (double)(note.EndNote.Tick - note.StartNote.Tick) / TicksPerBeat * InjectionsPerBeat;
                for (int i = 1; i < injections; ++i)
                {
                    var insertAt = headTime.Tick + (TicksPerBeat / InjectionsPerBeat * i);
                    var injection = SusAbsoluteNote.MakeShortNote(
                        SusNoteType.Hold | SusNoteType.Injection,
                        GetAbsoluteTime(headTime.Measure, (uint)insertAt),
                        0,
                        0);
                    tail.Add(injection);
                }

                var end = SusAbsoluteNote.MakeShortNote(
                    SusNoteType.Hold | SusNoteType.End,
                    GetAbsoluteTime(tailTime.Measure, tailTime.Tick),
                    (ushort)note.EndNote.LaneIndex,
                    (ushort)note.EndNote.Width);
                tail.Add(end);


                var head = SusAbsoluteNote.MakeLongNote(
                    SusNoteType.Hold | SusNoteType.Start,
                    GetAbsoluteTime(headTime.Measure, headTime.Tick),
                    (ushort)note.StartNote.LaneIndex,
                    (ushort)note.StartNote.Width,
                    tail);
                resultBuffer.Add(head);
            }

            // Slide 
            foreach (var note in ReferencialScore.Notes.Slides)
            {
                var headTime = GetRelativeTimeFromTicks((uint)note.StartNote.Tick);
                var tail = new List<SusAbsoluteNote>();

                var lastTailTick = (uint)note.StartTick;
                var lastTailTime = headTime;
                var steps = note.StepNotes.OrderBy(n => n.Tick);
                foreach (var tn in steps)
                {
                    var tailTick = (uint)tn.Tick;
                    // 本当は最後のやつをEndにするべきだけど特に問題ないので全部Stepにする
                    var time = GetRelativeTimeFromTicks((uint)tn.Tick);
                    var type = SusNoteType.Slide;
                    type |= tn.IsVisible ? SusNoteType.Step : SusNoteType.Invisible;
                    var telem = SusAbsoluteNote.MakeShortNote(
                        type,
                        GetAbsoluteTime(time.Measure, time.Tick),
                        (ushort)tn.LaneIndex,
                        (ushort)tn.WidthChange);
                    if (tn.IsVisible)
                    {
                        var injections = (double)(tailTick - lastTailTick) / TicksPerBeat * InjectionsPerBeat;
                        for (int i = 1; i < injections; ++i)
                        {
                            var insertAt = lastTailTime.Tick + (TicksPerBeat / InjectionsPerBeat * i);
                            var injection = SusAbsoluteNote.MakeShortNote(
                                SusNoteType.Slide | SusNoteType.Injection,
                                GetAbsoluteTime(lastTailTime.Measure, (uint)insertAt),
                                0,
                                0);
                            tail.Add(injection);
                        }
                        lastTailTick = tailTick;
                        lastTailTime = time;
                    }
                    tail.Add(telem);
                }

                var head = SusAbsoluteNote.MakeLongNote(
                    SusNoteType.Slide | SusNoteType.Start,
                    GetAbsoluteTime(headTime.Measure, headTime.Tick),
                    (ushort)note.StartNote.LaneIndex,
                    (ushort)note.StartNote.Width,
                    tail.OrderBy(n => n.StartTime));
                resultBuffer.Add(head);
            }

            // AirAction
            foreach (var note in ReferencialScore.Notes.AirActions)
            {
                var headTime = GetRelativeTimeFromTicks((uint)note.StartTick);
                var tail = new List<SusAbsoluteNote>();
                var lastTailTick = (uint)note.StartTick;
                var lastTailTime = headTime;
                var steps = note.ActionNotes.OrderBy(n => n.Offset);
                foreach (var tn in steps)
                {
                    var tailTick = (uint)(note.ParentNote.Tick + tn.Offset);
                    // 本当は最後のやつをEndにするべきだけど特に問題ないので全部Stepにする
                    var time = GetRelativeTimeFromTicks((uint)(note.ParentNote.Tick + tn.Offset));
                    var type = SusNoteType.AirAction | SusNoteType.Step;
                    var telem = SusAbsoluteNote.MakeShortNote(
                        type,
                        GetAbsoluteTime(time.Measure, time.Tick),
                        (ushort)note.ParentNote.LaneIndex,
                        (ushort)note.ParentNote.Width);
                    var injections = (double)(tailTick - lastTailTick) / TicksPerBeat * InjectionsPerBeat;
                    for (int i = 1; i < injections; ++i)
                    {
                        var insertAt = lastTailTime.Tick + (TicksPerBeat / InjectionsPerBeat * i);
                        var injection = SusAbsoluteNote.MakeShortNote(
                            SusNoteType.Slide | SusNoteType.Injection,
                            GetAbsoluteTime(lastTailTime.Measure, (uint)insertAt),
                            0,
                            0);
                        tail.Add(injection);
                    }
                    tail.Add(telem);
                    lastTailTick = tailTick;
                    lastTailTime = time;
                }

                var head = SusAbsoluteNote.MakeLongNote(
                        SusNoteType.AirAction | SusNoteType.Start,
                        GetAbsoluteTime(headTime.Measure, headTime.Tick),
                        (ushort)note.ParentNote.LaneIndex,
                        (ushort)note.ParentNote.Width,
                        tail);
                resultBuffer.Add(head);
            }

            var result = resultBuffer
                .OrderBy(n => n.Type)
                .OrderBy(n => n.StartTime)
                .ToList();
            return result;
        }

        private void InitializeBeatsDefinitions(Score chedScore)
        {
            beatsDefinitions[0] = DefaultBeats;
            var tsigs = new List<TimeSignatureChangeEvent>(chedScore.Events.TimeSignatureChangeEvents).OrderBy(s => s.Tick);
            var lastBeats = DefaultBeats;
            var lastTick = 0;
            var measureSum = 0u;
            foreach (var bd in tsigs)
            {
                var diffTicks = bd.Tick - lastTick;
                var diffMeasures = diffTicks / (lastBeats * TicksPerBeat);
                if (diffTicks - (diffMeasures * lastBeats * TicksPerBeat) != 0)
                    throw new ArgumentException("TimeSignatureChangeEvent が小節先頭にアライメントされていません");

                var beats = 4.0 * bd.Numerator / bd.Denominator;
                beatsDefinitions[measureSum] = beats;
                measureSum += (uint)diffMeasures;
                lastBeats = beats;
                lastTick = bd.Tick;
            }
        }

        private void InitializeBpmChanges(Score chedScore)
        {
            foreach (var bc in chedScore.Events.BPMChangeEvents)
            {
                var time = GetRelativeTimeFromTicks((uint)bc.Tick);
                bpmChanges.Add(Tuple.Create(time, (double)bc.BPM));
            }
        }
    }
}