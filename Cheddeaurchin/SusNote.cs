using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Cheddeaurchin
{
    /// <summary>
    /// ノーツ種別
    /// </summary>
    [Flags]
    public enum SusNoteType
    {
        Undefined = 0x00000001,
        Tap = 0x00000002,
        ExTap = 0x00000004,
        Flick = 0x00000008,
        Air = 0x00000010,
        HellTap = 0x00000020,
        AwesomeExTap = 0x00000040,
        Hold = 0x00000080,
        Slide = 0x00000100,
        AirAction = 0x00000200,
        Start = 0x00000400,
        Step = 0x00000800,
        Control = 0x00001000,
        End = 0x00002000,
        Up = 0x00004000,
        Down = 0x00008000,
        Left = 0x00010000,
        Right = 0x00020000,
        Injection = 0x00040000,
        Invisible = 0x00080000,
        MeasureLine = 0x00100000,
        Grounded = 0x00200000,
    }

    /// <summary>
    /// 相対時刻
    /// </summary>
    public struct SusRelativeTime
    {
        /// <summary>
        /// 小節
        /// </summary>
        public uint Measure;

        /// <summary>
        /// Tick
        /// </summary>
        public uint Tick;
    }

    public struct SusRawNote
    {
        /// <summary>
        /// 種別
        /// </summary>
        public SusNoteType Type;

        /// <summary>
        /// 定義番号か開始位置
        /// </summary>
        public ushort Primary;

        /// <summary>
        /// 長さ
        /// </summary>
        public ushort Secondary;

        /// <summary>
        /// その他
        /// </summary>
        public ushort Extra;
    }

    public class SusAbsoluteNote
    {
        /// <summary>
        /// 種別
        /// </summary>
        public SusNoteType Type { get; private set; }

        /// <summary>
        /// 開始位置
        /// </summary>
        public double StartTime { get; private set; }

        /// <summary>
        /// StartLane
        /// </summary>
        public ushort StartLane { get; private set; }

        /// <summary>
        /// Length
        /// </summary>
        public ushort Length { get; private set; }

        /// <summary>
        /// ExtraData
        /// </summary>
        public IReadOnlyList<SusAbsoluteNote> FollowingNotes { get; private set; }

        /// <summary>
        /// 隠蔽
        /// </summary>
        private SusAbsoluteNote() { }

        /// <summary>
        /// ショート作る
        /// </summary>
        /// <param name="type"></param>
        /// <param name="time"></param>
        /// <param name="lane"></param>
        /// <param name="length"></param>
        /// <returns></returns>
        public static SusAbsoluteNote MakeShortNote(SusNoteType type, double time, ushort lane, ushort length)
        {
            return new SusAbsoluteNote
            {
                Type = type,
                StartTime = time,
                StartLane = lane,
                Length = length,
            };
        }

        /// <summary>
        /// ロング作る
        /// </summary>
        /// <param name="type"></param>
        /// <param name="time"></param>
        /// <param name="lane"></param>
        /// <param name="length"></param>
        /// <returns></returns>
        public static SusAbsoluteNote MakeLongNote(SusNoteType type, double time, ushort lane, ushort length, IEnumerable<SusAbsoluteNote> tail)
        {
            return new SusAbsoluteNote
            {
                Type = type,
                StartTime = time,
                StartLane = lane,
                Length = length,
                FollowingNotes = new List<SusAbsoluteNote>(tail),
            };
        }
    }
}
