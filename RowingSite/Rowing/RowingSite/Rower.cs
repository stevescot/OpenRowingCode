using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;

namespace RowingSite
{
    public class Rower
    {
        public List<RowingSample> Session = new List<RowingSample>(1920);//number of strokes in an hour roughly.
    }
}