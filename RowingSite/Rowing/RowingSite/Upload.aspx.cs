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
            var mac = Request.QueryString("m");//mac address of this client
            var t = Request.QueryString("t"); //time to this sample
            var d = Request.QueryString("d");//distance (m)
            var s = Request.QueryString("s");//split (seconds).
        }
    }
}