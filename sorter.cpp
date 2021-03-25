#include <sorter.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <fftw3.h>
#include <fstream>
#include <world/common.h>
#include <world/constantnumbers.h>
#include <world/fft.h>
#include <world/dio.h>
#include <world/codec.h>
#include <world/macrodefinitions.h>
#include <world/d4c.h>
#include <world/cheaptrick.h>
#include <world/synthesis.h>

extern  "C"
{
    freqSorter::freqSorter()
    {
        /*order.resize(1);
        order.reserve(1);*/
    }
    freqSorter::~freqSorter()
    {
    }
    void freqSorter::sortFreq(double *raw, size_t inFrames, std::string freqOut)
    {
        std::ofstream freqFile;
        freqFile.open(freqOut +  ".stk");


        DioOption option;
        InitializeDioOption(&option);
        int sRate = 44100;
        option.frame_period=(double)5.0;
        double fPer = option.frame_period;
        int f0_length = GetSamplesForDIO(sRate,inFrames,fPer);
        std::cout<<GetSamplesForDIO(sRate,inFrames,fPer)<<std::endl;
        std::cout << option.allowed_range << std::endl;
        double *base = new double[GetSamplesForDIO(sRate,inFrames,fPer)];
        double *tp = new double[GetSamplesForDIO(sRate,inFrames,fPer)];
        Dio(raw,inFrames,sRate,&option,tp,base);
        double **platinum = new double* [GetSamplesForDIO(sRate,inFrames,fPer)];

        //Estimate freq avg

        double freqAvg = base[0];
        int avC = 1;
        int c=1;
        while(tp[c]!=(double)0)
        {
            if(base[c]==0)
            {}
            else
            {
                freqAvg+=base[c];

                avC++;
            }
            c++;
        }

        freqAvg /= avC;
        std::cout << freqAvg << std::endl;
        int peak = 128;
        double sws = (double)440*pow((double)2,(((double)peak-(double)69)/(double)12));
        while(sws>freqAvg)
        {
            peak--;
            sws = (double)440*pow((double)2,(((double)peak-(double)69)/(double)12));
        }

        freqFile << "basepitch=" << peak-23 << std::endl;
        freqFile << "period=" << fPer << std::endl;
        freqFile << "contour=" << f0_length <<std::endl;

        CheapTrickOption cho;

        InitializeCheapTrickOption(44100,&cho);

        double **sg = new double* [f0_length];

        int fftSize = GetFFTSizeForCheapTrick(sRate,&cho);

        for(int i=0; i<f0_length; i++)
        {
            sg[i] = new double [(fftSize/2)+1];
            std::cout<<(fftSize/2)+1<<std::endl;
        }

        std::cout<<fftSize<<std::endl;
        freqFile<<"size="<<fftSize<<std::endl;
        CheapTrick(raw,inFrames,sRate,tp,base,f0_length,&cho,sg);

        D4COption dcOption;
        InitializeD4COption(&dcOption);

        for(unsigned int i =0;i<f0_length;i++)
        {
            platinum[i] = new double[(fftSize/2)+1];
        }

        D4C(raw,inFrames,sRate,tp,base,f0_length,fftSize,&dcOption,platinum);

        int tpC=1;

        while(tp[tpC-1]!=tp[tpC])
        {
            tpC++;
        }

        int timestampStretch=round((double)GetSamplesForDIO(sRate,inFrames,fPer)/(double)tpC);
        int inStretch = 1, tPos = 0;
        std::ofstream apFile(freqOut+".d4c", std::ios::out | std::ios::binary);
        std::ofstream specFile(freqOut+".cheap",std::ios::out | std::ios::binary);
        std::ofstream fqFile(freqOut+".pitchinfo",std::ios::out | std::ios::binary);
        if(!specFile)
        {
            std::cout<< "Error: can't create" << freqOut << ".cheap. Location may not exist." <<std::endl;
            exit(2);
        }
        if(!apFile)
        {
            std::cout<< "Error: can't create" << freqOut << ".d4c. Location may not exist." <<std::endl;
            exit(2);
        }
        if(!fqFile)
        {
            std::cout<< "Error: can't create" << freqOut << ".pitchinfo. Location may not exist." <<std::endl;
            exit(2);
        }

        fqFile.write(reinterpret_cast<const char*>(base),std::streamsize(f0_length*sizeof(double)));

        specFile.write(reinterpret_cast<const char*>(&sRate),std::streamsize(sizeof(sRate)));
        specFile.write(reinterpret_cast<const char*>(&fPer),std::streamsize(sizeof(fPer)));

        for(unsigned int i=0; i<f0_length; i++)
        {
            if(inStretch>timestampStretch)
            {
                tPos++;
                inStretch = 1;
            }
            specFile.write(reinterpret_cast<const char*>(sg[i]),std::streamsize(((fftSize/2)+1)*sizeof(double)));
            apFile.write(reinterpret_cast<const char*>(platinum[i]),std::streamsize(((fftSize/2)+1)*sizeof(double)));
            freqFile << base[i]<< " " << tp[tPos] << std::endl;

            inStretch++;
        }
        freqFile.close();
        fqFile.close();
        specFile.close();
        apFile.close();

        delete[] *sg;
        delete[] *platinum;
        delete[] base;
    }
}
