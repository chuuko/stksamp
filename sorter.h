#ifndef SORTER_H
#define SORTER_H
#include <fftw3.h>
#include <vector>
#include <string>

extern "C"
{
namespace freqs {
    class freqSorter;
}
    class freqSorter
    {
    public:
        explicit freqSorter();
        ~freqSorter();
        double f0;
        void sortFreq(double *raw, size_t inFrames, std::string freqOut);
        //std::vector<double> order;
        //std::vector<double> freqAvgs;
        //std::vector<std::vector<double>> bands;
    };
}
#endif // SORTER_H
