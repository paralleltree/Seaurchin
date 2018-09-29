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
            MessageBox.Show("ちんこ");
        }
    }
}
