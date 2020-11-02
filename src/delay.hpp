#include "portaudio.h"
#include "pa_mac_core.h"
#include <stdexcept>
#include <iostream>
#include <cmath>

constexpr int floorMod(int a, int b){//a % bのaがマイナスの時も行けるやつ
    return(a % b + b) % b;
}

class DelayProcessor{
private:
    double* buffer;
    int frameLength;
    int bufferSizeInFrames;
    int pointer;
    double feedbackGain;
    int numberOfFeedbacks;
    int delayTimeInSamples;
public:
    DelayProcessor(
        int frameLength,
        int sampleRate,
        double feedbackGain,
        int numberOfFeedbacks,
        double delayTimeInSecs
    ) :
        frameLength(frameLength),
        pointer(0),
        feedbackGain(feedbackGain),
        numberOfFeedbacks(numberOfFeedbacks)
    {
        if(feedbackGain < 0.0 || feedbackGain > 1.0) std::cout << "Warning: feedback gain must be less than 1.0" << std::endl;
        delayTimeInSamples = (int)std::round(delayTimeInSecs * sampleRate);
        int necessaryBufferSizeInSamples = numberOfFeedbacks * delayTimeInSamples + 1;
        int necessaryBufferSizeInFrames = (int)std::ceil((double)necessaryBufferSizeInSamples / frameLength);
        bufferSizeInFrames = necessaryBufferSizeInFrames;
        buffer = new double[bufferSizeInFrames * frameLength]();
    }
    ~DelayProcessor() {
        delete[] buffer;
    }
    void process(double* monoIn, double* monoOut){
        memcpy(buffer + pointer * frameLength, monoIn, frameLength * sizeof(double));
        memset(monoOut, 0, sizeof(double) * frameLength);
        double gain = 1.0;
        for(int time = 0; time < numberOfFeedbacks + 1; time++){
            for(int i = 0; i < frameLength; i++){
                //読み出し結果にgainをかけ,outに加える
                double read = buffer[floorMod(pointer * frameLength + i - time * delayTimeInSamples, bufferSizeInFrames * frameLength)];
                double out_i = read * gain;
                monoOut[i]  += out_i;
            }
            gain *= feedbackGain; //ここでgainがfeedbackGain倍され，次の中のfor文ではより小さくなる
        }
        pointer = (pointer + 1) % bufferSizeInFrames;
    }
};