using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;

namespace RowingSite
{
    public class RowingSample
    {
        //mac address of this client
        public String mac{get;set;} 
        /// <summary>
        /// time in milliseconds from the start
        /// </summary>
        public TimeSpan TimeFromStart { get; set; }

        /// <summary>
        /// time spent on the drive (ms)
        /// </summary>
        public TimeSpan DriveTime
        {
            get; set;
        }
        /// <summary>
        /// time spent on the recovery (ms)
        /// </summary>
        public TimeSpan RecoveryTime
        {
            get; set;
        } //
        /// <summary>
        /// split (seconds).
        /// </summary>
        public TimeSpan Split
        {
            get
            {
                TimeSpan retval = TimeSpan.FromSeconds((DriveTime.TotalMilliseconds + RecoveryTime.TotalMilliseconds)/(SplitDistancem*2));
            }
        }
        /// <summary>
        /// 
        /// </summary>
        public float Distancem { get; set; }
        /// <summary>
        /// split Distance
        /// </summary>
        public float SplitDistancem
        {
            get; set;
        }
    }
}