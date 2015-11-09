using System;
using System.Collections.Generic;
using System.Linq;

namespace RowingSite
{
    public partial class Upload : System.Web.UI.Page
    {
        protected void Page_Load(object sender, EventArgs e)
        {
            RowingSample thisSample = RowingSample.FromQueryString(Request.QueryString);
            var rowers = ((Dictionary<string, Rower>)Application["Rowers"]);
            if(!rowers.ContainsKey(thisSample.mac))
            {
                var rower = new Rower();
                rower.Session.Add(thisSample);
                rowers.Add(thisSample.mac, rower);
            }
            else
            {
                rowers[thisSample.mac].Session.Add(thisSample);
            }
            rowHub x = new rowHub();
            x.updateStats(thisSample);
        }
    }
}