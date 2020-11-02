#include <iostream>
#include <cmath>
#include "portaudio.h"
#include "utils.hpp" //ここにPaStreamを作った
#include "distortion.hpp"
#include "delay.hpp"
#include "autotune.hpp"


typedef enum{
    DELAY,
    AUTOTUNE,
    DISTORTION,
} Effect;

#define EFFECT_TYPE AUTOTUNE //ここを書き換えれば,ディレイやディストーションなどを切り替えられる

typedef struct{
    double* inputTempArray;
    double** outputTempArray;
    int numberOfOutputChannels;
    DistortionProcessor* distortionProcessor;
    DelayProcessor* delayProcessor;
    AutoTuneProcessor* autouneProcessor;
}UserData;


void process(
    double* in,
    double** out,
    int numberOfOutputChannels,
    int frameLength,
    void* userData
){ 
    switch(EFFECT_TYPE){
        case DISTORTION:
            ((UserData*)userData)->distortionProcessor->process(in, out[0]);
            break;
        case DELAY:
            ((UserData*)userData)->delayProcessor->process(in, out[0]);
            break;
        case AUTOTUNE:
            ((UserData*)userData)->autouneProcessor->process(in, out[0]);
            break;
        default:
            std::cout << "Warning: unknown effect type" << std::endl;
            break;
    }
    for(int c=1; c<numberOfOutputChannels; c++){
        memcpy(out[c], out[0], frameLength * sizeof(double));
    }

    // for(int c=0; c<numberOfOutputChannels; c++){
    //     memcpy(out[c], in, frameLength * sizeof(double));
    // }
}

int myCallback( //PaStreamCallback型の関数。これはintを返す型だからこの関数もintを返す
    const void* inputBuffer,//マイク.インプットは変わらないのでconst  void*でどんな型でも入れられるが，その後ちゃんと自分でした型で取り出す必要がある
    void* outputBuffer,//スピーカ
    unsigned long frameLength,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData
){  
    for(int i=0; i<frameLength ; i++){
        char low = ((char*)inputBuffer)[i * 2];
        char upp = ((char*)inputBuffer)[i * 2 + 1];
        short i16 = (low & 0xFF) | (upp << 8); //上位バイトを8桁シフトして下位を空けて下位バイトを入れる
        double ans = (double) i16 / 32768.0; //負の最大値の絶対値で割る 片方doubleだから実際はキャストしなくても勝手にdoubleにアップキャストされる
        ((UserData*)userData)->inputTempArray[i] = ans;
    }
    //この時点ではdouble
    //エフェクト処理
    int numberOfOutputChannels = ((UserData*)userData)->numberOfOutputChannels;
    process(
        ((UserData*)userData)->inputTempArray,
        ((UserData*)userData)->outputTempArray,
        numberOfOutputChannels,
        frameLength,
        userData
        );

    //shortに戻し，分割し，outに書き込む
    for(int c = 0; c < numberOfOutputChannels; c++){
        for(int i=0; i<frameLength; i++){
            double f64 = ((UserData*)userData)->outputTempArray[c][i];
            short i16 = (short)round(f64 * 32767.0);
            char low = (char)(i16 & 0xFF);
            char upp = (char)((i16 >> 8) & 0xFF); //シフトしてからマスク取り出し
            ((char*)outputBuffer)[i * sizeof(short) * numberOfOutputChannels + c * sizeof(short)] = low;
            ((char*)outputBuffer)[i * sizeof(short) * numberOfOutputChannels + c * sizeof(short) + 1] = upp;
        }
    }
    return PaStreamCallbackResult::paContinue; //「処理を続ける」を返す
}

int main(){
    printDeviceInfos();
    int frameLength = 4096; //オートチューンでは4096
    int numberOfOutputChannels = 2;
    int inputDeviceNumber = 0; //デバイスの[番号]を選択
    int outputDeviceNumber = 1;

    int* inputChannelSelector = new int[1];
    inputChannelSelector[0] = 0;//インプットの使用するチャンネルを選ぶ...

    int* outputChannelSelector = new int[numberOfOutputChannels];
    for(int i = 0; i < numberOfOutputChannels; i++){
        outputChannelSelector[i] = i;
    }

    int sampleRate = 48000;

    UserData userData;

    userData.numberOfOutputChannels = numberOfOutputChannels;

    userData.inputTempArray = new double[frameLength];
    userData.outputTempArray = new double*[numberOfOutputChannels];
    for(int i=0; i < numberOfOutputChannels; i++){
        userData.outputTempArray[i] = new double[frameLength];
    }
 
    double gain = 40.0; //これを変えて歪み度合いを変える
    double ampMax = 100.0;
    double ampMin = -100.0;
    int numberOfPoints = 100000;
    DistortionProcessor distp(
        gain,
        ampMax,
        ampMin,
        frameLength,
        numberOfPoints
    );
    userData.distortionProcessor = &distp;

    double feedbackGain = 0.7;
    int numberOfFeedbacks = 10;
    double delayTimeInSecs = 0.3;
    DelayProcessor delyp(
        frameLength,
        sampleRate,
        feedbackGain,
        numberOfFeedbacks,
        delayTimeInSecs
    );
    userData.delayProcessor = &delyp;

    
    int processBlockSizeInFrames = 1;
    double thresholdRatioOfPeriodEstimation = 0.5;
    bool getsLastIndex = true;
    double maxDetectingFrequency = 440.0;
    double minDetectingFrequency = 440.0 / 8.0;
    AutoTuneProcessor ap(
        frameLength,
        processBlockSizeInFrames,
        thresholdRatioOfPeriodEstimation,
        getsLastIndex,
        maxDetectingFrequency,
        minDetectingFrequency,
        sampleRate
    );
    userData.autouneProcessor = &ap;

    PaStream* stream = createNewPaStream(
        inputDeviceNumber,
        outputDeviceNumber,
        inputChannelSelector,
        numberOfOutputChannels,
        outputChannelSelector,
        sampleRate,
        frameLength,
        myCallback,//関数ポインタ
        &userData
        );
    startStream(stream);
    int consoleInput = -1;
    while(consoleInput != 0){
        std::cout << "type 0 to quit >>";
        std::cin >> consoleInput; 
    }
    closeStream(stream);
    for(int i=0; i<numberOfOutputChannels; i++){
        delete[] userData.outputTempArray[i];
    }
    delete[] userData.outputTempArray;
    delete[] userData.inputTempArray;
    delete[] inputChannelSelector;
    delete[] outputChannelSelector;

    return 0;
}