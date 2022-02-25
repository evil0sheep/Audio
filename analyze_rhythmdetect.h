#ifndef rhythm_analyzer_h_
#define rhythm_analyzer_h_


#include "AudioStream.h"
#include "analyze_fft1024.h"
#include "beat_tracker.h"
#include "novelty_function.h"
#include "tempo_estimator.h"
#include "parameters.h"


static const size_t FFT_RESOLUTION = SAMPLE_RATE/FFT_HOP_LENGTH; 

class AudioAnalyzeRhythmDetector : public AudioStream {

public:
  AudioAnalyzeRhythmDetector(float mock_bpm = 0) : AudioStream(1, inputQueueArray), fft_connection(*this, 0, fft, 0), mock_bpm_(mock_bpm){
    fft.windowFunction(AudioWindowHanning1024);
  }

  
  void compute(){
    if(mock_bpm_ != 0){
      elapsed = 5000;
      delay(elapsed/1000);
      mock_micros_per_beat_ = (60000000.f / mock_bpm_);
      is_available = true;
      return;
    }
    
    if(fft.available()){
      auto start = micros();  
      novelty_function_.compute(fft);
      tempo_estimator_.compute(novelty_function_);
      bpm_ = (1- BPM_FILTER_PARAM) * bpm_ + BPM_FILTER_PARAM * tempo_estimator_.bpm(115 * BPM_MULTIPLIER);

      // first send the full set of candidate peaks to the beat tracker
      novelty_function_.getPeaks(candidate_beats_, MAX_BEATS, NoveltyFunction::PeakSortOrder::INDEX);
      beat_tracker_.trackBeats(novelty_function_, candidate_beats_, MAX_BEATS, bpm_, FFT_RESOLUTION, TIGHTNESS);

      size_t num_beats = beat_tracker_.getBeats(candidate_beats_, MAX_BEATS);
      downbeat_estimator_.trackBeats(novelty_function_, candidate_beats_, num_beats, bpm_/4.0, FFT_RESOLUTION, TIGHTNESS);

      
      is_available = true;
      elapsed = micros() - start;
    }
  }
  
  const NoveltyFunction &noveltyFunction() { return novelty_function_;}
  const TempoEstimator &tempoEstimator() { return tempo_estimator_;}
    

  float bpm() { return mock_bpm_ == 0.f ? bpm_: mock_bpm_;}
  float beatPhase(){ return mock_bpm_ == 0.f ? beat_tracker_.beatPhase() : (micros() % ((uint32_t) mock_micros_per_beat_)) / mock_micros_per_beat_; }
  float barPhase(){ return mock_bpm_ == 0.f ? downbeat_estimator_.beatPhase() : (micros() % ((uint32_t) mock_micros_per_beat_ * 4)) / (mock_micros_per_beat_ * 4);}


  bool available(){
    if(is_available){
      is_available = false;
      return true;
    }
    return false;
  }

  BeatTracker beat_tracker_;
  BeatTracker downbeat_estimator_;



 uint32_t elapsed=0;

 virtual void update(void){
   	audio_block_t *block;

    block = receiveReadOnly();
    if (!block) return;

    transmit(block, 0);
    release(block);
 }

private:
  AudioAnalyzeFFT1024 fft;
  AudioConnection fft_connection;
	audio_block_t *inputQueueArray[1];
  NoveltyFunction novelty_function_;
  TempoEstimator tempo_estimator_;
  NoveltyFunction::Peak candidate_beats_[MAX_BEATS];
  bool is_available = false;
  float bpm_ = 120 * BPM_MULTIPLIER;
  float mock_bpm_ = 0.0;
  float mock_micros_per_beat_ = 0;

};

#endif // rhythm_analyzer_h_
