using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.IO;

namespace MakeSif2
{
    public class Sif2Generator
    {
        public Regex Sif2Command { get; } = new Regex(@"^(\w+)\s+(.+)");
        public Regex Sif2Codepoint { get; } = new Regex(@"U\+([\dA-Fa-f]+)");

        public void Generate(string sif2gfile)
        {
            var images = new List<string>();
            var glyphs = new List<Sif2Glyph>();
            var output = "output.sif";
            float size = 32;

            var lines = File.ReadAllLines(sif2gfile, Encoding.UTF8);
            foreach (var l in lines)
            {
                var match = Sif2Command.Match(l);
                if (!match.Success) continue;
                switch (match.Groups[1].Value.ToLower())
                {
                    case "output":
                        output = ConvertRawString(match.Groups[2].Value);
                        break;
                    case "size":
                        size = Convert.ToSingle(match.Groups[2].Value);
                        break;
                    case "image":
                        images.Add(ConvertRawString(match.Groups[2].Value));
                        break;
                    case "glyph":
                        var pr = match.Groups[2].Value.Split(',').Select(s => s.Trim()).ToList();
                        if (pr.Count < 9)
                        {
                            Console.WriteLine("パラメーターが足りません: {0}", l);
                            break;
                        }

                        Sif2Glyph glyph;

                        var cm = Sif2Codepoint.Match(pr[0]);
                        if (!cm.Success) break;
                        glyph.Codepoint = Convert.ToUInt32(cm.Groups[1].Value, 16);

                        glyph.ImageNumber = Convert.ToUInt16(pr[1]);
                        glyph.GlyphX = Convert.ToUInt16(pr[2]);
                        glyph.GlyphY = Convert.ToUInt16(pr[3]);
                        glyph.GlyphWidth = Convert.ToUInt16(pr[4]);
                        glyph.GlyphHeight = Convert.ToUInt16(pr[5]);
                        glyph.BearX = Convert.ToInt16(pr[6]);
                        glyph.BearY = Convert.ToInt16(pr[7]);
                        glyph.WholeAdvance = Convert.ToUInt16(pr[8]);

                        glyphs.Add(glyph);
                        break;
                    default:
                        Console.WriteLine("不正なコマンドです: {0}", match.Groups[1].Value);
                        break;
                }
            }

            if (glyphs.Max(p => p.ImageNumber) >= images.Count)
            {
                Console.WriteLine("指定されている番号に対して画像が不足しています");
                return;
            }

            Console.WriteLine("{0}画像 {1}グリフ", images.Count, glyphs.Count);

            var fontdir = Path.GetDirectoryName(Path.GetFullPath(sif2gfile));

            using (var fs = File.Create(Path.Combine(fontdir, output)))
            using (var bw = new BinaryWriter(fs))
            {
                // ヘッダ
                bw.Write((ushort)0xA45F);
                bw.Write((ushort)images.Count);
                bw.Write((uint)glyphs.Count);
                bw.Write(size);
                // グリフ情報
                foreach (var g in glyphs)
                {
                    bw.Write(g.Codepoint);
                    bw.Write(g.ImageNumber);
                    bw.Write(g.GlyphWidth);
                    bw.Write(g.GlyphHeight);
                    bw.Write(g.GlyphX);
                    bw.Write(g.GlyphY);
                    bw.Write(g.BearX);
                    bw.Write(g.BearY);
                    bw.Write(g.WholeAdvance);
                }
                foreach (var i in images)
                {
                    if (!File.Exists(Path.Combine(fontdir, i)))
                    {
                        Console.WriteLine("{0}が存在しません");
                        return;
                    }
                    byte[] imgbuffer;
                    var isize = 0;
                    using (var imgdata = File.OpenRead(Path.Combine(fontdir, i)))
                    {
                        imgdata.Seek(0, SeekOrigin.End);
                        isize = (int)imgdata.Position;
                        imgdata.Seek(0, SeekOrigin.Begin);
                        imgbuffer = new byte[isize];
                        imgdata.Read(imgbuffer, 0, isize);
                    }
                    bw.Write((uint)isize);
                    bw.Write(imgbuffer);
                }
            }
        }

        private string ConvertRawString(string raw)
        {
            raw = raw.Trim();
            if (raw.StartsWith("\""))
            {
                raw = raw.Trim('"');
                var result = new StringBuilder(raw.Length);
                int i = 0;
                while (i < raw.Length)
                {
                    if (raw[i] != '\\')
                    {
                        result.Append(raw[i]);
                        i++;
                        continue;
                    }
                    i++;
                    if (i >= raw.Length) return result.ToString();
                    switch (raw[i])
                    {
                        case '"':
                            result.Append('\"');
                            i++;
                            break;
                        case 't':
                            result.Append('\t');
                            i++;
                            break;
                        case 'n':
                            result.Append('\n');
                            i++;
                            break;
                        case 'u':
                            {
                                i++;
                                var num = (char)Convert.ToUInt16(raw.Substring(i, 4), 16);
                                result.Append(num);
                                break;
                            }
                        default:
                            result.Append(raw[i]);
                            i++;
                            break;
                    }
                }
                return result.ToString();
            }
            else
            {
                return raw;
            }
        }
    }
}
