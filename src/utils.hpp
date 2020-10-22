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
    int outputNumberOfChannels,//出力チャンネル数
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