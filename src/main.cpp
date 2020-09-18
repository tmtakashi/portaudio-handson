#include <iostream>
#include "portaudio.h"
#include "utils.hpp"

int myCallback(
    const void *inputBuffer,
    void *outputBuffer,
    unsigned long frameLength,
    const PaStreamCallbackTimeInfo *timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData)
{
    int numChannels = 1;
    memcpy(outputBuffer, inputBuffer, sizeof(short) * numChannels * frameLength);
    return PaStreamCallbackResult::paContinue;
}

int main()
{
    printDeviceInfos();
    int frameLength = 64;
    int numberOfOutputChannels = 1;
    int inputDeviceNumber = 0;
    int outputDeviceNumber = 3;

    int *inputChannelSelector = new int[1];
    inputChannelSelector[0] = 0;

    int *outputChannelSelector = new int[numberOfOutputChannels];
    outputChannelSelector[0] = 0;

    for (int i = 0; i < numberOfOutputChannels; i++)
    {
        outputChannelSelector[i] = i;
    }
    int sampleRate = 48000;

    PaStream *stream = createNewPaStream(
        inputDeviceNumber,
        outputDeviceNumber,
        inputChannelSelector,
        numberOfOutputChannels,
        outputChannelSelector,
        sampleRate,
        frameLength,
        myCallback,
        nullptr);
    startStream(stream);

    int consoleInput = -1;
    while (consoleInput != 0)
    {
        std::cout << "type 0 to quit >> ";
        std::cin >> consoleInput;
    }
    closeStream(stream);

    delete[] inputChannelSelector;
    delete[] outputChannelSelector;

    return 0;
}