#include "portaudio.h"
#include "pa_mac_core.h"
#include <stdexcept>
#include <iostream>

void checkError(PaError err){
    if(err != paNoError){
        Pa_Terminate();//それまでのプログラムを閉じてから終了する
        throw std::runtime_error(Pa_GetErrorText(err));//エラーメッセージ(GetErrorText)を表示
    }
}

PaStream* createNewPaStream(
    int inputDeviceID,
    int outputDeviceID,
    int* inputChannelSelector,  
    int outputNumberOfChannels, //出力チャンネル数
    int* outputChannelSelector, 
    int sampleRate,
    int framesPerBuffer,
    PaStreamCallback paStreamCallback,
    void* userData
){
    PaError err = Pa_Initialize();  //初期化
    checkError(err);
    PaMacCoreStreamInfo inputCoreInfo; //インプットのコアの情報
    inputCoreInfo.size = sizeof(PaMacCoreStreamInfo);
    inputCoreInfo.hostApiType = paCoreAudio;  //ホストApiはCoreAudioを使うよ！って言ってる
    inputCoreInfo.version = 1;
    inputCoreInfo.flags = paMacCorePro;
    inputCoreInfo.channelMap = inputChannelSelector;
    inputCoreInfo.channelMapSize = 1;  //インプットのチャンネル数。今回は1チャンネル
    PaStreamParameters inputParameters; //インプットのパラメータの情報
    inputParameters.device = inputDeviceID; //使うデバイスの指定
    inputParameters.channelCount = 1;
    inputParameters.sampleFormat = paInt16;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputDeviceID)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = &inputCoreInfo;  //作った情報の参照
    PaMacCoreStreamInfo outputCoreInfo;  //アウトプットのコアの情報-インプットとほぼ同じ
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
    outputParameters.hostApiSpecificStreamInfo = &outputCoreInfo;  //CoreAudioというホストApiにSpecific情報を
    PaStream* answer; //ここで返すanswerを作る
    err = Pa_OpenStream(&answer, &inputParameters, &outputParameters, sampleRate, framesPerBuffer, paNoFlag, paStreamCallback, userData);
    checkError(err);
    return answer;
}

void printDeviceInfos(){
    PaError err = Pa_Initialize();
    checkError(err);
    int numberOfDevices = Pa_GetDeviceCount(); //この関数でデバイスの数がわかる
    for(int i = 0; i < numberOfDevices ; i++){
        const PaDeviceInfo* info = Pa_GetDeviceInfo(i);//i番目のデバイスの情報(const:値を変更できない設定)
        std::cout << "[" << i << "]" << std::endl;
        std::cout << info->name << std::endl;
        std::cout << "I/O max channels : " << info->maxInputChannels << "/" << info->maxOutputChannels << "\n" << std::endl;
    }
    Pa_Terminate();//PortAudioを終了
}

void startStream(PaStream* stream){ //ストリームを開始する
    PaError err = Pa_StartStream(stream);
    checkError(err);
}

void closeStream(PaStream* stream){ //ストリームを終了する手続き
    PaError err = Pa_StopStream(stream);//ストリームを停止(閉じなければ再スタートできる？)
    checkError(err);
    err = Pa_CloseStream(stream);//ストリームを閉じる
    checkError(err);
    Pa_Terminate();
}