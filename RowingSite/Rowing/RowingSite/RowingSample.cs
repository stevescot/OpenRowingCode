using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;

namespace RowingSite
{
    public class RowingSample
    {
        public static RowingSample FromQueryString(System.Collections.Specialized.NameValueCollection QueryString)
        {
            RowingSample newSample = new RowingSample();
            newSample.mac = QueryString["m"];//mac address of this client
            newSample.TimeFromStart = TimeSpan.FromMilliseconds(double.Parse(QueryString["t"])); //time to this sample (ms from start)
            newSample.Distancem = float.Parse(QueryString["d"]);//distance (m)
            newSample.SplitDistancem = float.Parse(QueryString["sd"]);//stroke Distance
            newSample.DriveTime = TimeSpan.FromMilliseconds(double.Parse(QueryString["msD"]));// time spent on the drive (ms)
            newSample.RecoveryTime = TimeSpan.FromMilliseconds(double.Parse(QueryString["msR"])); //time spent on the recovery (ms)
            return newSample;
    }
        //mac address of this client
        public String mac{get;set;} 
        /// <summary>
        /// time in milliseconds from the start
        /// </summary>
        /// 
        public TimeSpan TimeFromStart { get; set; }

        public string TimeFromStartStr
        {
            get
            {
                return TimeFromStart.Hours.ToString("DD") + ":"
                    + TimeFromStart.Minutes.ToString("DD") + ":"
                    + TimeFromStart.Seconds.ToString("DD") ;
            }
        }

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

        public string SplitStr
        {
            get
            {
                return Split.Minutes.ToString("DD") + ":" + Split.Seconds.ToString("DD");
            }
        }

        /// <summary>
        /// split (seconds).
        /// </summary>
        public TimeSpan Split
        {
            get
            {
                return TimeSpan.FromSeconds((DriveTime.TotalMilliseconds + RecoveryTime.TotalMilliseconds)/(SplitDistancem*2));
            }
        }

        public string AverageSplitStr
        {
            get
            {
                return AverageSplit.Minutes.ToString("DD") + ":" + AverageSplit.Seconds.ToString("DD");
            }
        }
        public TimeSpan AverageSplit
        {
            get
            {
                return TimeSpan.FromSeconds(TimeFromStart.TotalSeconds / Distancem * 500);
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

        public int Spm
        {
            get
            {
                return (int)(1.0 / (DriveTime.TotalMinutes + RecoveryTime.TotalMinutes));
            }
        }
    }
}