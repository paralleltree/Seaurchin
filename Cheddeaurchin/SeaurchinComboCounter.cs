using System;
using System.Collections.Generic;
using System.ComponentModel.Composition;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using Ched.Components;
using Ched.Plugins;

namespace Cheddeaurchin
{
    [Export(typeof(IScorePlugin))]
    public class SeaurchinComboCounter : IScorePlugin
    {
        public string DisplayName => "コンボ計算 (Seaurchin仕様)";

        public void Run(Score score)
        {
            var csc = new ChedScoreConverter(score);
            var suNotes = csc.GenerateSusNotesList();

            var allNotes = 0;
            var followMask = SusNoteType.End | SusNoteType.Step | SusNoteType.Injection;
            foreach (var note in suNotes)
            {
                var type = note.Type;
                if ((type & ChedScoreConverter.LongNoteMask) != 0)
                {
                    if ((type & SusNoteType.AirAction) == 0) ++allNotes;
                    foreach (var tail in note.FollowingNotes) if ((tail.Type & followMask) != 0) ++allNotes;
                }
                else if ((type & ChedScoreConverter.ShortNoteMask) != 0)
                {
                    ++allNotes;
                }
            }

            MessageBox.Show($"Seaurchinでの総ノーツ数\n{allNotes}");
        }
    }
}
