#include <iostream>
#include "portaudio.h"
#include "pa_mac_core.h"
#include <stdexcept>
#include <cmath>

void checkError(PaError err)
{
    if (err != paNoError)
    {
        Pa_Terminate();
        throw std::runtime_error(Pa_GetErrorText(err));
    }
}

PaStream *createNewPaStream(
    int inputDeviceID,
    int outputDeviceID,
    int *inputChannelSelector,
    int outputNumberOfChannels,
    int *outputChannelSelector,
    int sampleRate,
    int framesPerBuffer,
    PaStreamCallback paStreamCallback,
    void *userData)
{
    PaError err = Pa_Initialize();
    checkError(err);
    PaMacCoreStreamInfo inputCoreInfo;
    inputCoreInfo.size = sizeof(PaMacCoreStreamInfo);
    inputCoreInfo.hostApiType = paCoreAudio;
    inputCoreInfo.version = 1;
    inputCoreInfo.flags = paMacCorePro;
    inputCoreInfo.channelMap = inputChannelSelector;
    inputCoreInfo.channelMapSize = 1;
    PaStreamParameters inputParameters;
    inputParameters.device = inputDeviceID;
    inputParameters.channelCount = 1;
    inputParameters.sampleFormat = paInt16;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputDeviceID)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = &inputCoreInfo;
    PaMacCoreStreamInfo outputCoreInfo;
    outputCoreInfo.size = sizeof(PaMacCoreStreamInfo);
    outputCoreInfo.hostApiType = paCoreAudio;
    outputCoreInfo.version = 1;
    outputCoreInfo.flags = paMacCorePro;
    outputCoreInfo.channelMap = outputChannelSelector;
    outputCoreInfo.channelMapSize = outputNumberOfChannels;
    PaStreamParameters outputParameters;
    outputParameters.device = outputDeviceID;
    outputParameters.channelCount = outputNumberOfChannels;
    outputParameters.sampleFormat = paInt16;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputDeviceID)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = &outputCoreInfo;
    PaStream *answer;
    err = Pa_OpenStream(&answer, &inputParameters, &outputParameters, sampleRate, framesPerBuffer, paNoFlag, paStreamCallback, userData);
    checkError(err);
    return answer;
}

void printDeviceInfos()
{
    PaError err = Pa_Initialize();
    checkError(err);
    int numberOfDevices = Pa_GetDeviceCount();
    for (int i = 0; i < numberOfDevices; i++)
    {
        const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
        std::cout << "[" << i << "]" << std::endl;
        std::cout << info->name << std::endl;
        std::cout << "I/O max channels: " << info->maxInputChannels << "/" << info->maxOutputChannels << "\n"
                  << std::endl;
    }
    Pa_Terminate();
}

void startStream(PaStream *stream)
{
    PaError err = Pa_StartStream(stream);
    checkError(err);
}

void closeStream(PaStream *stream)
{
    PaError err = Pa_StopStream(stream);
    checkError(err);
    err = Pa_CloseStream(stream);
    checkError(err);
    Pa_Terminate();
}
