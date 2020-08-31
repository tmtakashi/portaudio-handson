#include <iostream>
#include "portaudio.h"

int main()
{
    auto err = Pa_Initialize();
    std::cout << err << "\n";
}