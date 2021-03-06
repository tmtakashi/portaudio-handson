#include <iostream>
#include "portaudio.h"
#include "utils.hpp"
#include "effects.hpp"

typedef enum
{
    DELAY,
    AUTOTUNE,
    DISTORTION
} EffectTypes;

#define EFFECT_TYPE DELAY

typedef struct
{
    double *inputTempArray;
    double **outputTempArray;
    int numberOfOutputChannels;
    Effects::DistortionProcessor *distortionProcessor;
    Effects::DelayProcessor *delayProcessor;
} UserData;

void process(double *in,
             double **out,
             int numberOfOutputChannels,
             int frameLength,
             void *userData)
{
    switch (EFFECT_TYPE)
    {
    case DISTORTION:
        ((UserData *)userData)->distortionProcessor->process(in, out[0]);
        break;
    case DELAY:
        ((UserData *)userData)->delayProcessor->process(in, out[0]);
        break;
    case AUTOTUNE:
        break;
    default:
        std::cout << "no such effects" << std::endl;
        break;
    }
    for (int c = 1; c < numberOfOutputChannels; ++c)
    {
        memcpy(out[c], out[0], frameLength * sizeof(double));
    }
}

int myCallback(
    const void *inputBuffer,
    void *outputBuffer,
    unsigned long frameLength,
    const PaStreamCallbackTimeInfo *timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData)
{
    for (int i = 0; i < frameLength; i++)
    {
        // little-endian
        char low = ((char *)inputBuffer)[i * 2];
        char upp = ((char *)inputBuffer)[i * 2 + 1];
        short i16 = (low & 0x00FF) | (upp << 8);
        double ans = (double)i16 / 32768.0;
        ((UserData *)userData)->inputTempArray[i] = ans;
    }

    int numberOfOutputChannels = ((UserData *)userData)->numberOfOutputChannels;
    process(((UserData *)userData)->inputTempArray, ((UserData *)userData)->outputTempArray, numberOfOutputChannels, frameLength, userData);

    for (int c = 0; c < numberOfOutputChannels; c++)
    {
        for (int i = 0; i < frameLength; i++)
        {
            double f64 = ((UserData *)userData)->outputTempArray[c][i];
            short i16 = (short)std::round(f64 * 32767.0);
            char low = (char)(i16 & 0xFF);
            char upp = (char)((i16 >> 8) & 0xFF);
            // little-endian
            ((char *)outputBuffer)[sizeof(short) * i * numberOfOutputChannels + sizeof(short) * c] = low;
            ((char *)outputBuffer)[sizeof(short) * i * numberOfOutputChannels + sizeof(short) * c + 1] = upp;
        }
    }

    return PaStreamCallbackResult::paContinue;
}

int main()
{
    printDeviceInfos();
    int frameLength = 64;
    int numberOfOutputChannels = 2;
    int inputDeviceNumber = 1;
    int outputDeviceNumber = 0;

    int *inputChannelSelector = new int[1];
    inputChannelSelector[0] = 0;

    int *outputChannelSelector = new int[numberOfOutputChannels];
    outputChannelSelector[0] = 0;

    for (int i = 0; i < numberOfOutputChannels; i++)
    {
        outputChannelSelector[i] = i;
    }
    int sampleRate = 48000;

    UserData userData;

    userData.inputTempArray = new double[frameLength];
    userData.outputTempArray = new double *[numberOfOutputChannels];
    for (int c = 0; c < numberOfOutputChannels; c++)
    {
        userData.outputTempArray[c] = new double[frameLength];
    }
    userData.numberOfOutputChannels = numberOfOutputChannels;

    double gain = 2;
    double ampMax = 100.0;
    double ampMin = -100.0;
    int numberOfPoints = 100000;
    Effects::DistortionProcessor distp(
        gain,
        ampMax,
        ampMin,
        frameLength,
        numberOfPoints);

    double feedbackGain = 0.4;
    int numberOfFeedbacks = 100;
    double delayTimeInSecs = 0.3;
    Effects::DelayProcessor delayp(
        frameLength,
        sampleRate,
        feedbackGain,
        numberOfFeedbacks,
        delayTimeInSecs);

    userData.delayProcessor = &delayp;
    userData.distortionProcessor = &distp;

    PaStream *stream = createNewPaStream(
        inputDeviceNumber,
        outputDeviceNumber,
        inputChannelSelector,
        numberOfOutputChannels,
        outputChannelSelector,
        sampleRate,
        frameLength,
        myCallback,
        &userData);
    startStream(stream);

    int consoleInput = -1;
    while (consoleInput != 0)
    {
        std::cout << "type 0 to quit >> ";
        std::cin >> consoleInput;
    }
    closeStream(stream);

    delete[] userData.inputTempArray;
    for (int c = 0; c < numberOfOutputChannels; c++)
    {
        delete[] userData.outputTempArray[c];
    }
    delete[] inputChannelSelector;
    delete[] outputChannelSelector;

    return 0;
}