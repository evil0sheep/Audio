#ifndef analyze_rhythm_h_
#define analyze_rhythm_h_

#include "analyze_fft1024.h"
#include "analyze_beats.h"
#include "analyze_spectral_novelty.h"
#include "analyze_tempo.h"
#include "parameters.h"
#include "AudioStream.h"

static const size_t FFT_RESOLUTION = SAMPLE_RATE/FFT_HOP_LENGTH;


class AudioAnalyzeRhythmDetector : public AudioStream {

public:
  AudioAnalyzeRhythmDetector(float mock_bpm = 0) : AudioStream(1, inputQueueArray), fft_connection(*this, 0, fft, 0), mock_bpm_(mock_bpm){
    fft.windowFunction(AudioWindowHanning1024);
    // We need a global for the timer interrupt so there should only be one object instance.
    singleton = this;
  }

  // This should be run as interrupt to ensure we don't drop any audio blocks
  void update_novelty() {
    if(fft.available()){
      spectral_novelty_.compute(fft);
      is_available = true;
    }
  }

  void compute(){
    if(mock_bpm_ != 0){
      elapsed = 5000;
      delay(elapsed/1000);
      mock_micros_per_beat_ = (60000000.f / mock_bpm_);
      is_available = true;
      return;
    }

    if(is_available){
      auto start = micros();

      tempo_detector_.compute(spectral_novelty_);
      bpm_ = (1- BPM_FILTER_PARAM) * bpm_ + BPM_FILTER_PARAM * tempo_detector_.bpm();

      // first send the full set of candidate peaks to the beat tracker
      int novelty_update_count = spectral_novelty_.getPeaks(
        candidate_beats_, MAX_BEATS, AudioAnalyzeSpectralNovelty::PeakSortOrder::INDEX);

      beat_tracker_.trackBeats(spectral_novelty_, novelty_update_count, candidate_beats_,
        MAX_BEATS, bpm_, FFT_RESOLUTION, TIGHTNESS);

      // size_t num_beats = beat_tracker_.getBeats(candidate_beats_, MAX_BEATS);
      // downbeat_estimator_.trackBeats(spectral_novelty_, candidate_beats_, num_beats, bpm_/4.0, FFT_RESOLUTION, TIGHTNESS);

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
  AudioAnalyzeTempoDetector tempo_detector_;

 uint32_t elapsed=0;

 virtual void update(void){
   	audio_block_t *block;

    block = receiveReadOnly();
    if (!block) return;

    transmit(block, 0);
    release(block);
 }

 float getNoveltyUpdatePeriod() { return 0.1 * 1000000 * FFT_HOP_LENGTH / SAMPLE_RATE; }

 void beginTimerInterrupt() {
    timer_.priority(255); // low priority, don't interrupt low level shit
    timer_.begin(timer_interrupt, getNoveltyUpdatePeriod());
 }

private:
 static void timer_interrupt() {
   singleton->update_novelty();
 }

private:
  AudioAnalyzeFFT1024 fft;
  AudioConnection fft_connection;
	audio_block_t *inputQueueArray[1];
  AudioAnalyzeSpectralNovelty spectral_novelty_;
  AudioAnalyzeSpectralNovelty::Peak candidate_beats_[MAX_BEATS];
  bool is_available = false;
  float bpm_ = 120 * BPM_MULTIPLIER;
  float mock_bpm_ = 0.0;
  float mock_micros_per_beat_ = 0;
  IntervalTimer timer_;
  static AudioAnalyzeRhythmDetector *singleton; // for timer interrupt
};

AudioAnalyzeRhythmDetector *AudioAnalyzeRhythmDetector::singleton;

#endif // analyze_rhythm_h_
