using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using Microsoft.AspNet.SignalR;

namespace RowingSite
{
    public class rowHub : Hub
    {
        public void Hello()
        {
            Clients.All.hello();
        }
    }
}