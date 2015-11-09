using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.UI;
using System.Web.UI.WebControls;

namespace RowingSite
{
    public partial class Upload : System.Web.UI.Page
    {
        protected void Page_Load(object sender, EventArgs e)
        {
            var mac = Request.QueryString["m"];//mac address of this client
            var t = Request.QueryString["t"]; //time to this sample (ms from start)
            var Distancem = Request.QueryString["d"];//distance (m)
            var SplitDistancem = Request.QueryString["sd"];//split Distance
            var DriveTimems = Request.QueryString["msD"];// time spent on the drive (ms)
            var RecoveryTimems = Request.QueryString["msR"]; //time spent on the recovery (ms)
            var s = Request.QueryString["s"];//split (seconds).
        }
    }
}