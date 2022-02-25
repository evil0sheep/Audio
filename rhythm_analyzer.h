#ifndef rhythm_analyzer_h_
#define rhythm_analyzer_h_

#include <Audio.h>
#include "beat_tracker.h"
#include "novelty_function.h"
#include "tempo_estimator.h"
#include "parameters.h"

static const size_t FFT_RESOLUTION = SAMPLE_RATE/FFT_HOP_LENGTH; 

class RhythmAnalyzer {
public:
  RhythmAnalyzer(AudioAnalyzeFFT1024 &fft) : fft(fft) {}

  
  void compute(){
    
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
    
  float bpm() { return bpm_;}
  float beatPhase(){ return beat_tracker_.beatPhase();}
  float barPhase(){ return downbeat_estimator_.beatPhase();}


  bool available(){
    if(is_available){
      is_available = false;
      return true;
    }
    return false;
  }

  BeatTracker beat_tracker_;
  BeatTracker downbeat_estimator_;



 uint32_t elapsed;

private:
  AudioAnalyzeFFT1024 &fft;
  NoveltyFunction novelty_function_;
  TempoEstimator tempo_estimator_;
  NoveltyFunction::Peak candidate_beats_[MAX_BEATS];
  bool is_available = false;
  float bpm_ = 120 * BPM_MULTIPLIER;

};

#endif // rhythm_analyzer_h_
