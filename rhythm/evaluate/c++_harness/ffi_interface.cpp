#include <iostream>
#include "analyze_rhythm.h"

extern "C" void* create_rhythm_detector(){
    return new AudioAnalyzeRhythmDetector;
}
extern "C" void delete_rhythm_detector(void *detector){
    delete static_cast<AudioAnalyzeRhythmDetector *>(detector);
}

extern "C" void testcall_cpp(float value)
{
    std::cout << "Hello, world from C++! Value passed: " << value << std::endl;
}