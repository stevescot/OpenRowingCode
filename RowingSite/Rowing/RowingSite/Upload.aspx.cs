using System;
using System.Collections.Generic;
using Microsoft.AspNet.SignalR;
using System.Linq;

namespace RowingSite
{
    public partial class Upload : System.Web.UI.Page
    {
        protected void Page_Load(object sender, EventArgs e)
        {
            RowingSample rowSample = RowingSample.FromQueryString(Request.QueryString);
            var rowers = ((Dictionary<string, Rower>)Application["Rowers"]);
            if(!rowers.ContainsKey(rowSample.mac))
            {
                var rower = new Rower();
                rower.Session.Add(rowSample);
                rowers.Add(rowSample.mac, rower);
            }
            else
            {
                rowers[rowSample.mac].Session.Add(rowSample);
            }
            var x = GlobalHost.ConnectionManager.GetHubContext <rowHub>();
            x.Clients.Group(rowSample.mac).update(rowSample.TimeFromStartStr, rowSample.Spm, rowSample.SplitStr, rowSample.Distancem);
        }
    }
}