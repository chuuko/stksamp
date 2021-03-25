#ifndef STRETCH_H_INCLUDED
#define STRETCH_H_INCLUDED
#include <stk/FileRead.h>

extern "C"{
    class ca_Stretch : public stk::StkFrames
    {

        public:
        explicit ca_Stretch(stk::StkFloat& one,stk::StkFloat &two);

    };
}
#endif // STRETCH_H_INCLUDED
