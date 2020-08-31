#include <iostream>
#include "portaudio.h"

int main()
{
    int err = Pa_Initialize();
    std::cout << err << "\n";
}