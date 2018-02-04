using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MakeSif2
{
    class Sif2Extractor
    {
        public void Extract(string filename)
        {
            var fontdir = Path.GetDirectoryName(Path.GetFullPath(filename));
            float size;
            ushort images;
            var glist = new List<Sif2Glyph>();

            using (var stream = File.OpenRead(filename))
            using (var br = new BinaryReader(stream))
            {
                var magic = br.ReadUInt16();
                if (magic != 0xA45F)
                {
                    br.Dispose();
                    stream.Dispose();
                    Console.WriteLine("不正なSif2ファイルです！");
                    return;
                }
                images = br.ReadUInt16();
                var glyphs = br.ReadUInt32();
                size = br.ReadSingle();

                for (uint i = 0; i < glyphs; i++)
                {
                    Sif2Glyph glyph;
                    glyph.Codepoint = br.ReadUInt32();
                    glyph.ImageNumber = br.ReadUInt16();
                    glyph.GlyphWidth = br.ReadUInt16();
                    glyph.GlyphHeight = br.ReadUInt16();
                    glyph.GlyphX = br.ReadUInt16();
                    glyph.GlyphY = br.ReadUInt16();
                    glyph.BearX = br.ReadInt16();
                    glyph.BearY = br.ReadInt16();
                    glyph.WholeAdvance = br.ReadUInt16();
                    glist.Add(glyph);
                }

                for (int i = 0; i < images; i++)
                {
                    var pngs = br.ReadUInt32();
                    var pngd = br.ReadBytes((int)pngs);
                    var path = Path.Combine(fontdir, $"font{i}.png");
                    File.WriteAllBytes(path, pngd);
                }
            }

            var lines = new List<string>();
            lines.Add($"output \"{Path.GetFileName(filename)}\"");
            lines.Add($"size {size}");
            for (int i = 0; i < images; i++) lines.Add($"image \"font{i}.png\"");
            foreach(var g in glist) lines.Add($"glyph U+{g.Codepoint:X4}, {g.ImageNumber}, {g.GlyphX}, {g.GlyphY}, {g.GlyphWidth}, {g.GlyphHeight}, {g.BearX}, {g.BearY}, {g.WholeAdvance}");
            File.WriteAllLines(Path.Combine(fontdir, $"{filename}.txt"), lines);
        }
    }
}
