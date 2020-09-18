#include <iostream>
#include "portaudio.h"
#include "utils.hpp" //ここにPaStreamを作った

int myCallback( //PaStreamCallback型の関数。これはintを返す型だからこの関数もintを返す
    const void* inputBuffer,//マイク.インプットは変わらないのでconst
    void* outputBuffer,//スピーカ
    unsigned long frameLength,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData
){  //Outは1chとして書いていく。
    int sizeOfInt16 = 2;
    memcpy(outputBuffer, inputBuffer, sizeOfInt16 * 1 * frameLength);//memcpy(アウトプット,インプット,バイト数 * ch * フレーム長)
    //((short*)outputBuffer)[0] = 30000; アウトプットの確認
    std::cout << ((short*)inputBuffer)[0] << std::endl; //インプットの確認
    return PaStreamCallbackResult::paContinue; //「処理を続ける」を返す
}

int main(){
    printDeviceInfos();
    int frameLength = 64;
    int numberOfOutputChannels = 1;
    int inputDeviceNumber = 2; //デバイスの[番号]を選択
    int outputDeviceNumber = 1;

    int* inputChannelSelector = new int[1];
    inputChannelSelector[0] = 1;//インプットの使用するチャンネルを選ぶ...
    
    int* outputChannelSelector = new int[numberOfOutputChannels];
    for(int i = 0; i < numberOfOutputChannels; i++){
        outputChannelSelector[i] = i;
    }
    int sampleRate = 48000;

    PaStream* stream = createNewPaStream(
        inputDeviceNumber,
        outputDeviceNumber,
        inputChannelSelector,
        numberOfOutputChannels,
        outputChannelSelector,
        sampleRate,
        frameLength,
        myCallback,//関数ポインタ
        nullptr//ユーザーデータが無い
        );
    startStream(stream);
    int consoleInput = -1;
    while(consoleInput != 0){
        std::cout << "type 0 to quit >>";
        std::cin >> consoleInput; 
    }
    closeStream(stream);
    delete[] inputChannelSelector;
    delete[] outputChannelSelector;

    return 0;
}