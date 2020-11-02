#include "portaudio.h"
#include "pa_mac_core.h"
#include <stdexcept>
#include <iostream>
#include <cmath>

class DistortionProcessor{
private:
    double gain;
    double ampMax;
    double ampMin;
    int frameLength;
    int numberOfPoints;
    double* lookupTable;
    double getTableValue(double ampX){
        if(ampX > ampMax) return 1.0; //ampX=ampMaxではそのまま通過してampMaxを読む
        if(ampX < ampMin) return -1.0; 
        double wariai = (ampX - ampMin) / (ampMax - ampMin);
        double doubleIndex = wariai * (numberOfPoints - 1);
        int indexLeft = (int)std::floor(doubleIndex);
        int indexRight = indexLeft + 1;
        double fraction = doubleIndex - indexLeft; //小数部
        return (1.0 - fraction) * lookupTable[indexLeft] + fraction * lookupTable[indexRight];
    }
public:
    DistortionProcessor(
        double gain, //入力を何倍するか
        double ampMax,
        double ampMin,
        int frameLength,
        int numberOfPoints //MinからMaxの間の補間点数

    ):  //[:]を入力すれば，this->gain = gain と同じことをできる
        gain(gain),
        ampMax(ampMax),
        ampMin(ampMin),
        frameLength(frameLength),
        numberOfPoints(numberOfPoints)
    {
        if(numberOfPoints<2) throw std::runtime_error("numberOfPoints must be 2 or more.");
        lookupTable = new double[numberOfPoints]();
        double delta = (ampMax - ampMin)/(numberOfPoints - 1);
        lookupTable[0] = -1.0; //両端はハードクリッピング
        for(int i=1; i<numberOfPoints - 1; i++){ //浮動小数点誤差対策by皆川 [ampMax]と[ampMin＋delata*i]が誤差で一致しなくなる
            lookupTable[i] = std::tanh(ampMin + delta * i);
        }
        lookupTable[numberOfPoints - 1] = 1.0; //両端はハードクリッピング
    }
    ~DistortionProcessor() {
        delete[] lookupTable;
    }
    void process(double* monoIn, double* monoOut){
        for(int i=0; i<frameLength; i++){
            monoOut[i] = getTableValue(monoIn[i] * gain);
        }
    }
};
