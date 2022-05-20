#include <string>
// #include <function>

#include "MockAudioStream.h"

class MadDecoder: public AudioStream {
public:
    MadDecoder() : AudioStream(0, nullptr) {}
    void update(void) override {};
    bool decode(std::string mp3_file_path);

private:
  unsigned char const *start_;
  unsigned long length_;

  void handleAudioBlock(int16_t data[AUDIO_BLOCK_SAMPLES]){
    for(int i = 0; i < AUDIO_BLOCK_SAMPLES; i++){
        int16_t sample = data[i];
        putchar((sample >> 0) & 0xff);
        putchar((sample >> 8) & 0xff);
    }
  }

  void inputCallback();
  static void input(){
    
  }

  void ouputCallback();

  void errorCallback();


};

int mad_decode(std::string mp3_file_path, std::function<void(int16_t data[AUDIO_BLOCK_SAMPLES])>);