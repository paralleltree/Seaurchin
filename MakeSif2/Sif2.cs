using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace MakeSif2
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Sif2Header
    {
        public UInt16 Magic;
        public UInt16 Images;
        public UInt32 Glyphs;
        public Single FontSize;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct Sif2Glyph
    {
        public UInt32 Codepoint;
        public UInt16 ImageNumber;
        public UInt16 GlyphWidth;
        public UInt16 GlyphHeight;
        public UInt16 GlyphX;
        public UInt16 GlyphY;
        public Int16 BearX;
        public Int16 BearY;
        public UInt16 WholeAdvance;
    };
    
    /*
     * [Sif2Header]
     * [Sif2Glyph]
     * ...
     * ...
     * [UInt32] PngSize
     * [any bytes] PNG Image File
     * [UInt32] PngSize
     * [any bytes] PNG Image File
     * ...
     * ...
     */
}
