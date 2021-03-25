/*
 * stksamp: a resampler based on libstk and WORLD
 * Created by Squeekers
 * License: MIT
 * Dependencies: libcantamus, WORLD, libstk
 * stksamp provides both wavtool and resampler functionality akin to smsvoice
 * As the Cantamus UI does not use scientific notation, both libcantamus and stksamp have to compensate for it
 * UI notation is 23 pitches lower than scientific notation
 * TODO:
 * - implement more flags;
 * - implement local flags;
 * - fix formantless mode;
 * - implement envelopes and crossfade;
 * - implement proper stretching algorhytm;
*/

#include <iostream>
#include <stk/FormSwep.h>
#include <stk/FileWrite.h>
#include <stk/FileRead.h>
#include <stk/Function.h>
#include <stk/Fir.h>
#include <stk/Sampler.h>
#include <stk/ADSR.h>
#include <stk/Stk.h>
#include <stk/FileLoop.h>
#include <stk/PitShift.h>
#include <fftw3.h>
#include <fstream>
#include <string>
#include <sstream>
#include <strings.h>
#include <string.h>
#include <strstream>
#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <cstdlib>
#include <handleust.h>
#include <sorter.h>
#include <world/common.h>
#include <world/constantnumbers.h>
#include <world/fft.h>
#include <world/dio.h>
#include <world/codec.h>
#include <world/macrodefinitions.h>
#include <world/synthesis.h>
#include <world/cheaptrick.h>

using namespace std;
using namespace stk;


ustHandler u;

extern "C"
{

    int main(int argc, char** argv)
    {
        freqSorter fqs;
        if(argv[1]==NULL)
        {
            std::cout << "No arguments given, aborting" << std::endl << "Use --help for usage" << std::endl;
            exit(1);
        }
        std::string help = argv[1];
        if(help=="--help")
        {
            std::cout << "stksamp <flags> <path_to_voicebank> <path_to_sheet> <path_to_render>" << std::endl << "stksamp --genfreq <path_to_sample>" << std::endl;
            exit(0);
        }
        if(help=="--genfreq")
        {
            if(argv[2]==NULL)
            {
                std::cout << "No sample given to generate frequencies for, aborting" << std::endl << "Use --help for usage" << std::endl;
                exit(1);
            }
            //The frequency generation option takes a sample and sends it to the frequency rendering algorhytm at sorter.cpp

            stk::FileRead inSample;
            inSample.setSampleRate(44100);
            inSample.setRawwavePath(argv[2]);

            std::string resFile = argv[2];
            inSample.open(argv[2]);
            stk::StkFrames inSf(inSample.fileSize(),inSample.channels());


            inSf.resize(inSample.fileSize(),inSample.channels());
            inSample.read(inSf);
            std::cout<< inSample.fileSize() << " " << inSf.frames()<<std::endl;
            //Skulk for f0
            double *ic;
            ic = fftw_alloc_real(inSf.frames());

            size_t freqFrames = 0;

            for(unsigned int cd = 0; cd <inSf.frames();cd++)
            {
                ic[cd] = inSf[cd];
                freqFrames++;
            }
            std::cout << freqFrames<< std::endl;
            std::cout << "Generating frequencies for " << argv[2] << "..." << std::endl;
            fqs.sortFreq(ic, freqFrames, resFile.substr(0,resFile.size()-4));
            inSample.close();
            exit(0);
        }

        /*
         * Variables for flags, oto handling, full file size, etc...
        */

        int foff = atoi(argv[1]);
        char* vb = argv[2];
        char* in = argv[3];
        char* op = argv[4];

        stk::FileRead r;
        stk::FileRead rt;
        stk::FileWrite w;
        stk::FileWrite wt;
        stk::StkFrames l;
        stk::PitShift p;
        std::string on;
        on = new char;
        ifstream oto, bank;
        std::string *entry;
        std::string *cf;
        cf = new std::string;
        entry = new std::string;
        char *nl;
        char *cm;

        std::string noteB,noteR,vbString,rootd;
        rootd=std::getenv("HOME");
        vbString.append(vb);
        nl = new char;
        cm = new char;
        const char newl = "\n";
        const char comma = ",";
        std::string ca;
        ca.reserve(1);
        ca.assign(&comma);
        strcpy(cm,&comma);
        strcpy(nl,&newl);
        u.ustLoader(in);
        float fulls;
        oto=std::ifstream(vbString+(const char*)"/oto.ini",std::ios::in);
        if(!(oto))
        {
            std::cout<<"ERROR: oto.ini not found at " << vbString << std::endl;
            exit(2);
        }
        double prA[u.ustNoteCount+1];

        /*
         * stksamp renders full sheets instead of individual samples
        */

        for(int ustN = 0;ustN<u.ustNoteCount;ustN++)
        {
            fulls += ((float)u.ustNoteLength[ustN]/1000)*44100;

            double prt;
            if(u.isRest[ustN]==1)
            {
                std::string nada = "";

                while(!(on.substr(on.find("=")+1,u.ustNoteLyric[ustN].length())==u.ustNoteLyric[ustN]||(on.substr(0,u.ustNoteLyric[ustN].length())==u.ustNoteLyric[ustN])))
                {
                    std::getline(oto,on);
                }

                oto.close();
                oto.open(vbString+(const char*)"/oto.ini");
                int bf = 0;
                bf = on.find(".");
                /*
                 * In addition to the three usual WORLD outputs, stksamp uses a plain-text description file to store certain sample parameters
                */

                std::string basefile = on.substr(0,bf);
                bank = std::ifstream(vbString+(const char*)"/"+basefile+(const char*)".stk",std::ios::in);
                if(!bank)
                {
                    std::cout << "Sample description file " << vbString+(const char*)"/"+basefile+(const char*)".stk" << " not found" << std::endl;
                    exit(2);
                }

                std::getline(bank,noteB);
                while(!(noteB.substr(0,10)=="basepitch="))
                {
                    std::getline(bank,noteB);
                    std::cout << vb <<(const char*)"/"+basefile+(const char*)".stk" << endl;
                    if(noteB==noteR)
                    {
                        break;
                    }
                    noteR=noteB;
                }
                if(noteB.length()<10)
                {
                    std::cout << "Error: no base note set. Aborting.\n";
                    exit(13);
                }
                std::string pS;
                std::string pS2;
                std::getline(bank,pS);
                while(!(pS.substr(0,7)=="period="))
                {
                    std::getline(bank,pS);
                    std::cout << vb <<(const char*)"/"+basefile+(const char*)".stk" << endl;
                    if(pS==pS2)
                    {
                        break;
                    }
                    pS2=pS;
                }
                if(pS.length()<7)
                {
                    std::cout << "Error: no frame period set. Aborting.\n";
                    exit(13);
                }
                std::string cS, cS2;
                std::getline(bank,cS);
                while(!(cS.substr(0,8)=="contour="))
                {
                    std::getline(bank,cS);
                    std::cout << vb <<(const char*)"/"+basefile+(const char*)".stk" << endl;
                    if(cS==cS2)
                    {
                        break;
                    }
                    cS2=cS;
                }
                if(cS.length()<8)
                {
                    std::cout << "Error: f0 contour not set. Aborting.\n";
                    exit(13);
                }

                std::string fS, fS2;
                std::getline(bank,fS);
                while(!(fS.substr(0,5)=="size="))
                {
                    std::getline(bank,fS);
                    std::cout << vb <<(const char*)"/"+basefile+(const char*)".stk" << endl;
                    if(fS==fS2)
                    {
                        break;
                    }
                    fS2=fS;
                }
                if(fS.length()<5)
                {
                    std::cout << "Error: fft size not set. Aborting.\n";
                    exit(13);
                }

                std::string freqInter;
                int fq = 0;
                while(!(bank.eof()))
                {
                    std::getline(bank,freqInter);
                    fq++;
                }
                std::cout << fq << std::endl;

                int sRate;

                double fPeriod;
                int fContour= stoi(cS.substr(8));
                int fftSize = stoi(fS.substr(5));

                fPeriod = 5.0;

                std::ifstream specFile(vbString+(const char*)"/"+basefile+(const char*)".cheap",std::ios::in|std::ios::binary);
                if(!specFile)
                {
                    std::cout<<"Error: can't find file: "+ vbString+(const char*)"/"+basefile+(const char*)".cheap"<<std::endl;
                    exit(2);
                }
                std::ifstream apFile(vbString+(const char*)"/"+basefile+(const char*)".d4c",std::ios::in|std::ios::binary);
                if(!apFile)
                {
                    std::cout<<"Error: can't find file: "+ vbString+(const char*)"/"+basefile+(const char*)".d4c"<<std::endl;
                    exit(2);
                }


                bank.close();
                bank.open(vbString+(const char*)"/"+basefile+(const char*)".stk");
                for (int i=0; i < 4; i++)
                {
                    std::getline(bank,freqInter);
                }

                //Sample base pitch
                int bNote = stoi(noteB.substr(10,12));


                bank.close();

                std::cout << fq<<std::endl;


                if(!specFile.is_open())
                {
                    std::cout<<"Error: can't read spectrogram file"<<std::endl;
                    exit(2);
                }

                specFile.read(reinterpret_cast<char*>(&sRate),streamsize(sizeof(sRate)));
                specFile.read(reinterpret_cast<char*>(&fPeriod),streamsize(sizeof(fPeriod)));

                //Read binary files in preparation to actual synthesis

                double **fq2 = new double* [fContour];
                double **ap = new double* [fContour];
                double *destFreq = new double[fContour];

                std::ifstream fqFile(vbString+(const char*)"/"+basefile+(const char*)".pitchinfo",std::ifstream::in|std::ifstream::binary);

                if(!fqFile.is_open())
                {
                    std::cout<<"Error: can't read pitch file"<<std::endl;
                    exit(2);
                }

                fqFile.read(reinterpret_cast<char*>(destFreq),streamsize(fContour*sizeof(double)));

                for(int i=0;i<fContour;i++)
                {
                    fq2[i]=new double[(fftSize/2)+1];
                    ap[i]=new double[(fftSize/2)+1];
                    if(foff==1)
                    {
                        destFreq[i] = pow(2,(double)(u.ustNotePos[ustN]+23-69)/12)*440;
                        std::cout<<destFreq[i]<<std::endl;
                    }
                    else
                    {
                        destFreq[i] = pow(2,(double)(bNote+23-69)/12)*440;
                    }
                }
                for(int i=0;i<fContour;i++)
                {

                    specFile.read(reinterpret_cast<char*>(fq2[i]),streamsize(((fftSize/2)+1)*sizeof(double)));

                }

                if(!apFile.is_open())
                {
                    std::cout<<"Error: can't read aperiodicity file"<<std::endl;
                    exit(2);
                }

                for(int i=0;i<fContour;i++)
                {
                    apFile.read(reinterpret_cast<char*>(ap[i]),streamsize(((fftSize/2)+1)*sizeof(double)));
                }

                fqFile.close();
                specFile.close();
                apFile.close();

                //Length of first intermediary file
                int resLength = static_cast<int>((double)(fContour-1)*(double)fPeriod/(double)1000*44100)+1;

                //Fetch and prepare oto parameters
                int sep = 0;
                sep = on.find(",");
                int cns = 0;
                cns = on.find(",",(int)sep+1);
                int ctf = 0;
                ctf = on.find(",",(int)cns+1);
                int put = 0;
                put = on.find(",",(int)ctf+1);
                int ovl = 0;
                ovl = on.find(",",(int)put+1);
                entry->reserve(3);
                cf->reserve(3);
                std::string ofs = on.substr(sep+1,cns-sep);
                std::string con = on.substr(cns+1,ctf-cns);
                std::string cof = on.substr(ctf+1,put-ctf);
                std::string pre = on.substr(put+1,ovl-put);
                int os = atoi(ofs.c_str());
                int cs = atoi(con.c_str());
                int cu = atoi(cof.c_str());
                int pu = atoi(pre.c_str());
                double off = (((float)os/1000)*44100);
                double cst = ((float)cs/1000)*44100;
                double cut = ((float)cu/1000)*44100;
                double cut2 = cut;
                prt = ((float)pu/1000)*44100;

                //Array for resampled audio
                double* resampled= new double [resLength];
                for (int i = 0;i<resLength;++i)
                {
                    resampled[i] = 0.0;
                }

                Synthesis(destFreq,fContour,fq2,ap,fftSize,fPeriod,44100,resLength,resampled);
                for(int i=0;i<resLength;i++)
                {
                    std::cout<<resampled[i]<<" ";
                }
                std::cout << std::endl;

                //Create first intermediary sample
                stk::FileRead origWav;
                origWav.setRawwavePath(vbString+(const char*)"/"+u.ustNoteLyric[ustN]+(const char*)".wav");
                origWav.open(vbString+(const char*)"/"+u.ustNoteLyric[ustN]+(const char*)".wav");
                stk::FileWrite testFile;
                stk::StkFrames test(resLength,origWav.channels());
                testFile.open(rootd+(const char*)"/.cantamus/int0.wav");
                testFile.setSampleRate(44100);
                test.resize(resLength);
                for(int i=0;i<resLength;i++)
                {
                    test[i] = resampled[i];
                }
                testFile.write(test);
                testFile.close();
                origWav.close();

                r.setRawwavePath(rootd+(const char*)"/.cantamus/int0.wav");
                r.open(rootd+(const char*)"/.cantamus/int0.wav");

                //Apply oto to first intermediary and export second intermediary
                stk::StkFrames f(r.fileSize(),r.channels());



                if(cut < 0)
                {
                    f.resize(-(r.fileSize()-off-(r.fileSize()-off-cut)),r.channels());
                }
                else
                {
                    f.resize(r.fileSize()-off-cut2,r.channels());
                }
                r.read(f,off);
                r.close();

                w.open(rootd+(const char*)"/.cantamus/int.wav");
                w.setSampleRate(44100);
                w.write(f);
                w.close();

                //Stretch the sample if needed and export note

                rt.setRawwavePath(rootd+(const char*)"/.cantamus/int.wav");
                rt.setSampleRate(44100);
                if(rt.isOpen()==false)
                {
                    rt.open(rootd+(const char*)"/.cantamus/int.wav");
                }
                l.resize(rt.fileSize(),rt.channels());
                rt.setSampleRate(44100);
                //(480/1000)*44100

                float len = (float)u.ustNoteLength[ustN];

                int nf =((float)len/1000)*44100;
                stk::StkFrames lpp(1000,l.channels());
                lpp.resize(nf,l.channels());
                rt.read(l);
                rt.close();
                lpp.setDataRate(l.dataRate());
                unsigned int ll = 0;
                int diff = 0;
                for(unsigned int li = 0;li<lpp.frames();li++)
                {
                    //Stretching algorhytm that makes a monster out of a voicebank
                    /*lpp[li]=l[ll];
                    if(ll==l.frames())
                    {
                        ll = cst;
                    }
                    if(li>cst)
                    {
                        if(diff==10)
                        {
                            ll-= 5;
                            diff = 0;
                        }
                        if(cc<rint(nlen))
                        {
                            cc++;
                            ll++;
                        }
                        else
                        {
                            diff++;
                            ll++;
                        cc = 1;
                        }
                    }
                    else
                    {
                        ll++;
                    }*/
                    //Stupid looping algorhytm
                    if(ll==l.frames())
                    {
                        ll = cst+diff;
                        diff = 0;
                    }
                    lpp[li]=l[ll];
                    if(ll>l.frames()-1000)
                    {
                        for (unsigned int ch=0;ch<l.channels();ch++)
                        {
                            lpp[li] = l.interpolate(cst+diff,ch);
                        }
                        diff++;
                    }
                    ll++;
                }
                //Mechanism for formantless mode
                if(foff==0)
                {
                    float pt;

                    if(u.ustNotePos[ustN]<bNote)
                    {
                        pt = (float)(u.ustNotePos[ustN]-bNote)/24;
                    }
                    else
                    {
                        pt = (((float)u.ustNotePos[ustN]-bNote)/12)+1;
                    }


                    stk::OnePole vol;
                    float lpt = 1+pt;

                    p.setSampleRate(44100);
                    if(pt<0)
                    {
                        p.setShift(lpt);
                    }
                    else
                    {
                        p.setShift(pt);
                    }

                    if(pt==1)
                    {
                        vol.setGain(0.5);
                        vol.tick(lpp);
                    }
                    p.setEffectMix(1.0);
                    p.tick(lpp);
                }

                finish:
                {
                    wt.setRawwavePath(rootd+(const char*)"/.cantamus/"+std::to_string(ustN)+(const char*)".wav");
                    wt.open(rootd+(const char*)"/.cantamus/"+std::to_string(ustN)+(const char*)".wav");
                    wt.setSampleRate(44100);
                    wt.write(lpp);
                    lpp.empty();
                    rt.close();
                    wt.close();
                    //std::cout << os << " "<< cs << " "<< cu<< endl;
                }

                delete[] resampled;
                delete[] destFreq;
                delete[] *fq2;
                delete[] *ap;

            }
            if(ustN!=0&&u.isRest[ustN]==1)
            {
                prA[ustN-1]=prt;
            }

        }
        prA[u.ustNoteCount]=0;
        stk::StkFrames rendered(fulls,0);
        rendered.resize(fulls);

        unsigned int atFrame = 0;
        for(int i = 0;i < u.ustNoteCount;i++)
        {
            float corL = (((float)u.ustNoteLength[i]/1000)*44100)-prA[i];
            stk::FileRead noteIn;
            noteIn.setRawwavePath(rootd+(const char*)"/.cantamus/"+std::to_string(i)+(const char*)".wav");
            noteIn.open(rootd+(const char*)"/.cantamus/"+std::to_string(i)+(const char*)".wav");
            noteIn.setSampleRate(44100);
            stk::StkFrames noteFr(1000,noteIn.channels());
            noteFr.resize(corL,noteIn.channels());
            noteIn.read(noteFr);
            for(int ns = 0;ns<(int)corL-1;ns++)
            {
                if(atFrame>rendered.frames())
                {
                    break;
                }
                if(u.isRest[i]==1)
                {
                    rendered[atFrame]=noteFr[ns];
                }
                else
                {
                    rendered[atFrame]=0;
                }
                atFrame++;
            }
        }
        stk::FileWrite finals;
        finals.setRawwavePath(op);
        finals.open(op);
        finals.setSampleRate(44100);
        finals.write(rendered);
        cout << "Done!" << endl;
        return 0;
    }
}

