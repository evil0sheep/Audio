#include <iostream>
#include <stdio.h>
#include "analyze_rhythm.h"
#include "mad_decoder.h"



int main(void){
    MadDecoder decoder;
    AudioAnalyzeRhythmDetector detector;
    AudioConnection connection(decoder, detector);

    bool result decoder.decode("/Users/evil0sheep/workspace/train/Fort Knox Five - Mission to the Sitars.mp3");
    if(result){
        printf("success!\n");
    }else{
        printf("Error: decode failed!\n");
        return -1;
    }

    return 0;
}