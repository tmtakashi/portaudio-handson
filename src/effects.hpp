#include <stdexcept>
#include <cmath>

constexpr int floorMod(int a, int b)
{
    return (a % b + b) % b;
}

namespace Effects {

class DistortionProcessor
{
private:
    double gain;
    double ampMax;
    double ampMin;
    int frameLength;
    int numberOfPoints;
    double *lookupTable;
    double getTableValue(double ampX)
    {
        if (ampX > ampMax)
            return 1.0;
        if (ampX < ampMin)
            return -1.0;

        double ratio = (ampX - ampMin) / (ampMax - ampMin);
        double doubleIndex = ratio * (numberOfPoints - 1);
        int indexLeft = (int)std::floor(doubleIndex);
        int indexRight = indexLeft + 1;
        double fraction = doubleIndex - indexLeft;
        return fraction * lookupTable[indexRight] + fraction * lookupTable[indexLeft];
    }

public:
    DistortionProcessor(
        double gain,
        double ampMax,
        double ampMin,
        int frameLength,
        int numberOfPoints) : gain(gain),
                              ampMax(ampMax),
                              ampMin(ampMin),
                              numberOfPoints(numberOfPoints),
                              frameLength(frameLength)
    {
        if (numberOfPoints < 2)
        {
            throw std::runtime_error("numberOfPoints must be 2 or more");
        }
        lookupTable = new double[numberOfPoints];

        double delta = (ampMax - ampMin) / (numberOfPoints - 1);
        lookupTable[0] = -1.0;
        for (int i = 1; i < numberOfPoints - 1; i++)
        {
            lookupTable[i] = std::tanh(ampMin + i * delta);
        }
        // eliminating for floating point error for last point
        lookupTable[numberOfPoints - 1] = 1.0;
    }

    ~DistortionProcessor()
    {
        delete[] lookupTable;
    }

    void process(double *monoIn, double *monoOut)
    {
        for (int i = 0; i < frameLength; i++)
        {
            monoOut[i] = getTableValue(gain * monoIn[i]);
        }
    }
};

class DelayProcessor {
private:
    double* buffer;

    int frameLength;
    int buffersizeInFrames;
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
    feedbackGain(feedbackGain),
    frameLength(frameLength),
    numberOfFeedbacks(numberOfFeedbacks),
    pointer(0)
    {
        if (feedbackGain < 0.0 || feedbackGain > 1.0)
        {
            std::cout << "WARNING: feedback gain must be less than 1.0" << std::endl;
        }
        delayTimeInSamples = (int)std::round(delayTimeInSecs * sampleRate);
        int necessaryBufferSizeInSamples = numberOfFeedbacks * delayTimeInSamples + 1;
        buffersizeInFrames = (int)std::ceil((double)necessaryBufferSizeInSamples / frameLength);
        buffer = new double[buffersizeInFrames * frameLength]();
    }

    ~DelayProcessor()
    {
        delete[] buffer;
    }

    void process(double *monoIn, double *monoOut)
    {
        memcpy(&buffer[pointer * frameLength], monoIn, sizeof(double) * frameLength);
        memset(monoOut, 0, sizeof(double) * frameLength);
        double gain = 1.0;
        for (int time = 0; time < numberOfFeedbacks + 1; time++)
        {
            for (int i = 0; i < frameLength; i++)
            {
                double read = buffer[floorMod(pointer * frameLength - time * delayTimeInSamples + i, buffersizeInFrames * frameLength)];
                double out_i = read * gain;
                monoOut[i] += out_i;
            }
            gain *= feedbackGain;
        }
        pointer = (pointer + 1) % buffersizeInFrames;
    }
};
}
