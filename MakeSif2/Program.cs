using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace MakeSif2
{
    class Program
    {
        static int Main(string[] args)
        {
            if (args.Length == 0)
            {
                Console.WriteLine("Usage: MakeSif2 <sif2 generate text file>");
                Console.WriteLine("       MakeSif2 --extract <sif2 file>");
                return 1;
            }
            if (args.Length >= 2 && args[0] == "--extract")
            {
                new Sif2Extractor().Extract(args[1]);
            }
            else if (args.Length >= 1 && args[0] != "--extract")
            {
                new Sif2Generator().Generate(args[0]);
            }
            else
            {
                Console.WriteLine("Usage: MakeSif2 <sif2 generate text file>");
                Console.WriteLine("       MakeSif2 --extract <sif2 file>");
                return 1;
            }
            return 0;
        }
    }
}
