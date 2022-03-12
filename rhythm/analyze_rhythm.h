#ifndef analyze_rhythm_h_
#define analyze_rhythm_h_

#include "analyze_fft_parametric.h"
#include "analyze_beats.h"
#include "analyze_spectral_novelty.h"
#include "analyze_tempo.h"
#include "parameters.h"

#if defined(TRAIN_PARAMETERS)
#include "MockAudioStream.h"
#else
#include "AudioStream.h"
#endif

static const size_t FFT_RESOLUTION = SAMPLE_RATE/FFT_HOP_LENGTH;


class AudioAnalyzeRhythmDetector : public AudioStream {

public:
  AudioAnalyzeRhythmDetector(float mock_bpm = 0) : AudioStream(1, inputQueueArray), fft_connection(*this, 0, fft, 0), mock_bpm_(mock_bpm){
    // fft.windowFunction(AudioWindowHanning1024);
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
      spectral_novelty_.compute(fft);
      tempo_detector_.compute(spectral_novelty_);
      bpm_ = (1- BPM_FILTER_PARAM) * bpm_ + BPM_FILTER_PARAM * tempo_detector_.bpm();

      // first send the full set of candidate peaks to the beat tracker
      spectral_novelty_.getPeaks(candidate_beats_, MAX_BEATS, AudioAnalyzeSpectralNovelty::PeakSortOrder::INDEX);
      beat_tracker_.trackBeats(spectral_novelty_, candidate_beats_, MAX_BEATS, bpm_, FFT_RESOLUTION, TIGHTNESS);

      // size_t num_beats = beat_tracker_.getBeats(candidate_beats_, MAX_BEATS);
      // downbeat_estimator_.trackBeats(spectral_novelty_, candidate_beats_, num_beats, bpm_/4.0, FFT_RESOLUTION, TIGHTNESS);


      is_available = true;
      elapsed = micros() - start;
    }
  }

  const AudioAnalyzeSpectralNovelty &spectralNovelty() { return spectral_novelty_;}
  const AudioAnalyzeTempoDetector &tempoDetector() { return tempo_detector_;}


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

  AudioAnalyzeBeatTracker beat_tracker_;
  AudioAnalyzeBeatTracker downbeat_estimator_;



 uint32_t elapsed=0;

 virtual void update(void){
   	audio_block_t *block;

    block = receiveReadOnly();
    if (!block) return;

    transmit(block, 0);
    release(block);
 }

private:
  AudioAnalyzeFFTParametric fft;
  AudioConnection fft_connection;
	audio_block_t *inputQueueArray[1];
  AudioAnalyzeSpectralNovelty spectral_novelty_;
  AudioAnalyzeTempoDetector tempo_detector_;
  AudioAnalyzeSpectralNovelty::Peak candidate_beats_[MAX_BEATS];
  bool is_available = false;
  float bpm_ = 120 * BPM_MULTIPLIER;
  float mock_bpm_ = 0.0;
  float mock_micros_per_beat_ = 0;

};

#endif // analyze_rhythm_h_
