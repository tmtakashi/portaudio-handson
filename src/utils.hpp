#include "portaudio.h"
#include "pa_mac_core.h"
#include <stdexcept>

void checkError(PaError err)
{
    if (err != paNoError)
    {
        Pa_Terminate();
        throw std::runtime_error(Pa_GetErrorText(err));
    }
}

PaStream *createNewPaStream(
    int deviceID,
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
    inputParameters.device = deviceID;
    inputParameters.channelCount = 1;
    inputParameters.sampleFormat = paFloat32;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(deviceID)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = &inputCoreInfo;
    PaMacCoreStreamInfo outputCoreInfo;
    outputCoreInfo.size = sizeof(PaMacCoreStreamInfo);
    outputCoreInfo.hostApiType = paCoreAudio;
    outputCoreInfo.version = 1;
    outputCoreInfo.flags = paMacCorePro;
    outputCoreInfo.channelMap = outputChannelSelector;
    outputCoreInfo.channelMapSize = outputNumberOfChannels;
    PaStreamParameters outputParameters;
    outputParameters.device = deviceID;
    outputParameters.channelCount = outputNumberOfChannels;
    outputParameters.sampleFormat = paFloat32;
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(deviceID)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = &outputCoreInfo;
    PaStream *answer;
    err = Pa_OpenStream(&answer, &inputParameters, &outputParameters, sampleRate, framesPerBuffer, paNoFlag, paStreamCallback, userData);
    checkError(err);
    return answer;
}