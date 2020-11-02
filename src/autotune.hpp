#include "portaudio.h"
#include "pa_mac_core.h"
#include <stdexcept>
#include <iostream>
#include <cmath>
#include "fft.hpp"
#include <vector>
#include <bitset>



class AutoTuneProcessor{
private:
    int frameLength;
    int processBlockSizeInFrames; //frameLength*これでブロックサイズ
    double thresholdRatioOfPeriodEstimation;
    bool getsLastIndex;
    double maxDetectingFrequency;
    double minDetectingFrequency;
    int sampleRate;
    int maxDetectingPeriod;
    int minDetectingPeriod;
    const char* usedKeys = "101010110101"; //右がド,左がシ。使うKeyを1にする。(Cメジャー101010110101)
    int period1;
    int period2;
public:
    AutoTuneProcessor(
        int frameLength,
        int processBlockSizeInFrames,
        double thresholdRatioOfPeriodEstimation,
        bool getsLastIndex,
        double maxDetectingFrequency,
        double minDetectingFrequency,
        int sampleRate
    ) : 
        frameLength(frameLength),
        processBlockSizeInFrames(processBlockSizeInFrames),
        thresholdRatioOfPeriodEstimation(thresholdRatioOfPeriodEstimation),
        getsLastIndex(getsLastIndex),
        maxDetectingFrequency(maxDetectingFrequency),
        minDetectingFrequency(minDetectingFrequency),
        sampleRate(sampleRate),
        maxDetectingPeriod((int)std::round(sampleRate / minDetectingFrequency)), //周波数が最小の時は周期は最大
        minDetectingPeriod((int)std::round(sampleRate / maxDetectingFrequency))
    {
        period1 = minDetectingPeriod;
    }
    void process(double* monoIn, double* monoOut){
    //     取得した周波数を表示する
        // int period = estimatePeriod(monoIn);
        // if(period == -1){
        //     std::cout << "couldn't detect frequency" << std::endl;
        // }else{
        //     double freq = (double)sampleRate / period;
        //     std::cout << freq << std::endl;
        // }
        memset(monoOut, 0, sizeof(double) * frameLength);

        int estimated = estimatePeriod(monoIn);
        if(estimated != -1) period1 = estimated;

        double inputFreq = (double)sampleRate / period1;
        double correctedFreq = correctFrequency(inputFreq);
        double shiftRatio = correctedFreq / inputFreq;
        simplePitchShift(monoIn, frameLength, shiftRatio, monoOut);
        
    }
    int estimatePeriod(double* processBlock){ //processBlock:フレーム数の整数倍のサイズ
        int blockLen = frameLength * processBlockSizeInFrames;
        double* correlation = new double[blockLen * 2 - 1];
        crossCorrelation(processBlock, blockLen, processBlock, blockLen, correlation); //自己相関
        double centerMaxValue = correlation[blockLen - 1]; //最大(=周期が一致)となるのは中心なので
        std::vector<int> overThrIndices;
        overThrIndices.reserve(blockLen - 1);
        double threshold = thresholdRatioOfPeriodEstimation * centerMaxValue; //相関が低いときのため
        for(int i=0; i<blockLen - 1; i++){
            double temp = correlation[blockLen + 1]; //貯めておく
            if(temp > threshold) overThrIndices.emplace_back(i);
        }
        if(overThrIndices.empty()){
            delete[] correlation;
            return -1;
        }
        double max = -std::numeric_limits<double>::infinity(); //最初にmaxに最も小さいだろう-∞を入れておく
        int argMax = -1; //最大の時のインデックスを表示するので，インデックスに存在しない-1を入れておく
        for(int i=0; i < overThrIndices.size(); i++){
            double temp = correlation[blockLen + overThrIndices[i]]; //貯めておいたインデックスを取り出す
            int period = overThrIndices[i] + 1;
            bool isPeriodValid = period >= minDetectingPeriod && period <= maxDetectingPeriod;//最小≦周期≦最大と周期を指定
            bool tempFlag;
            if(getsLastIndex){
                tempFlag = temp >= max && isPeriodValid;
            }else {
                tempFlag = temp > max && isPeriodValid;
            }
            if(tempFlag){
                max = temp;
                argMax = i;
            }
        }
        delete[] correlation;
        //if(argMax == -1) return -1; //これはありえん条件だけど念のため書いてもいい
        return overThrIndices[argMax] + 1; //真ん中の一個右が0だから，そのときの周期は1サンプル
       }

    static void crossCorrelation(double* x, int len_x, double* y, int len_y, double* ans){ //相互相関関数
        double* flipped_y = new double[len_y];
        for(int i=0; i<len_y; i++){ //時間反転
        flipped_y[i] = y[len_y - 1 - i];
        }
        convolutionFFT(x, len_x, flipped_y, len_y, ans);
        delete[] flipped_y;
    }
    double correctFrequency(double freq){//どの周波数に補正するか計算する
        std::bitset<12> bs(usedKeys);
        double standardFrequency = 440.0; //A4の音
        double ratio = freq / standardFrequency;
        double octave = std::log2f(ratio);
        double semitone = octave * 12;
        int round = (int)std::round(semitone);
        if(round - semitone > 0.0){ //上に丸めたとき
            int i = 0;
            while(true){
                if(bs[floorMod(round + 9 - i,12)] == 1){
                    round = round - i;
                    break;
                }
                if(bs[floorMod(round + 9 + i,12)] == 1){
                    round = round + i;
                    break;
                }
                i++;
            }
        }else{ //下に丸めたとき
            int i = 0;
            while(true){
                if(bs[floorMod(round + 9 + i,12)] == 1){
                    round = round + i;
                    break;
                }
                if(bs[floorMod(round + 9 - i,12)] == 1){
                    round = round - i;
                    break;
                }
                i++;
            }  
        }
        octave = round / 12.0;
        ratio = std::pow(2,octave);
        return ratio * standardFrequency;
    }

    //絶対に真似してはいけないピッチシフト
    static void simplePitchShift(
        double* original,
        int len_original,
        double ratio, //ratio : frequency_ratio_from_original_to_objective
        double* ans
        ){
        for(int i=0; i<len_original; i++){
            double readoutPositionDouble = i * ratio;
            int readoutPositionInt = (int)std::round(readoutPositionDouble);
            ans[i] = original[readoutPositionInt % len_original];
        }
    }
};
