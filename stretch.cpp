#include "stretch.h"

extern "C"
{
    ca_Stretch::ca_Stretch(stk::StkFloat &one,stk::StkFloat &two):
    stk::StkFrames(one,two)
    {
        stk::StkFrames *in, *out;
        in = new stk::StkFrames(one,two);
        out = new stk::StkFrames(one,two);

    }
}
