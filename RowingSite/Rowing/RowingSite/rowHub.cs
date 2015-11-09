using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using Microsoft.AspNet.SignalR;

namespace RowingSite
{
    public class rowHub : Hub
    {
        /// <summary>
        //
        /// </summary>
        public static Dictionary<string,List<string>> Users = new Dictionary<string, List<string>>();

        public void updateStats(RowingSample rowsample)
        {
            //Clients.All.;
        }

        /// <summary>
        /// The OnConnected event.
        /// </summary>
        /// <returns>
        /// The <see cref="Task"/>.
        /// </returns>
        public override System.Threading.Tasks.Task OnConnected()
        {
            string clientId = GetClientId();

            if (!Users.ContainsKey(clientId))
            {
                Users.Add(clientId, new List<string>());
            }

            // Send the current count of users
            return base.OnConnected();
        }

        /// <summary>
        /// The OnReconnected event.
        /// </summary>
        /// <returns>
        /// The <see cref="Task"/>.
        /// </returns>
        public override System.Threading.Tasks.Task OnReconnected()
        {
            string clientId = GetClientId();
            if (!Users.ContainsKey(clientId))
            {
                Users.Add(clientId, new List<string>());
            }

            // Send the current count of users
            return base.OnReconnected();
        }

        /// <summary>
        /// The OnDisconnected event.
        /// </summary>
        /// <returns>
        /// The <see cref="Task"/>.
        /// </returns>
        public override System.Threading.Tasks.Task OnDisconnected(bool stopCalled)
        {
            string clientId = GetClientId();

            if (!Users.ContainsKey(clientId))
            {
                Users.Remove(clientId);
            }

            // Send the current count of users
            return base.OnDisconnected(stopCalled);
        }

        /// <summary>
        /// Get's the currently connected Id of the client.
        /// This is unique for each client and is used to identify
        /// a connection.
        /// </summary>
        /// <returns>The client Id.</returns>
        private string GetClientId()
        {
            string clientId = "";
            if (Context.QueryString["clientId"] != null)
            {
                // clientId passed from application 
                clientId = this.Context.QueryString["clientId"];
            }
            if (string.IsNullOrEmpty(clientId.Trim()))
            {
                clientId = Context.ConnectionId;
            }
            return clientId;
        }
    }
}