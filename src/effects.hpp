#include <stdexcept>
#include <cmath>

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

}
