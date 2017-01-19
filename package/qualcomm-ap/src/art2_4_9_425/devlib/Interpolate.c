




//
// Returns the interpolated y value corresponding to the specified x value
// from the np ordered pairs of data (px,py).
// The pairs do not have to be in any order.
// If the specified x value is less than any of the px,
// the returned y value is equal to the py for the lowest px.
// If the specified x value is greater than any of the px,
// the returned y value is equal to the py for the highest px.
//
int Interpolate(int x, int *px, int *py, int np)
{
    int ip;
    int lx,ly,lhave;
    int hx,hy,hhave;
    int dx;
    int y;

    lhave=0;
    hhave=0;
    //
    // identify best lower and higher x calibration measurement
    //
    for(ip=0; ip<np; ip++)
    {
        dx=x-px[ip];
        //
        // this measurement is higher than our desired x
        //
        if(dx<=0)
        {
            if(!hhave || dx>(x-hx))
            {
                //
                // new best higher x measurement
                //
                hx=px[ip];
                hy=py[ip];
                hhave=1;
            }
        }
        //
        // this measurement is lower than our desired x
        //
        if(dx>=0)
        {
            if(!lhave || dx<(x-lx))
            {
                //
                // new best lower x measurement
                //
                lx=px[ip];
                ly=py[ip];
                lhave=1;
            }
        }
    }
    //
    // the low x is good
    //
    if(lhave)
    {
        //
        // so is the high x
        //
        if(hhave)
        {
            //
            // they're the same, so just pick one
            //
            if(hx==lx)
            {
                y=ly;
            }
            //
            // interpolate 
            //
            else
            {
                y=ly+(((x-lx)*(hy-ly))/(hx-lx));
            }
        }
        //
        // only low is good, use it
        //
        else
        {
            y=ly;
        }
    }
    //
    // only high is good, use it
    //
    else if(hhave)
    {
        y=hy;
    }
    //
    // nothing is good,this should never happen unless np=0, ????
    //
    else
    {
        y= -(1<<30);
    }

    return y;
}
