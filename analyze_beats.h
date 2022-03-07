#ifndef analyze_beats_h_
#define analyze_beats_h_

#include "analyze_spectral_novelty.h"
#include "parameters.h"

class AudioAnalyzeBeatTracker{
public:

  void trackBeats(const AudioAnalyzeSpectralNovelty &novelty_function, const AudioAnalyzeSpectralNovelty::Peak *candidate_beats, size_t num_candidates, float bpm, size_t fft_res, size_t tightness){
      memcpy(candidate_beats_,  &candidate_beats[MAX_BEATS - num_candidates], num_candidates * sizeof(AudioAnalyzeSpectralNovelty::Peak));


      size_t beat_size = fft_res * 60.f/bpm;
      size_t last_beat_search_threshold = novelty_function.length() - 8 * beat_size;

      float last_beat_score = 0;
      last_beat_index_ = num_candidates -1;

       uint32_t current_micros = micros();
       auto elapsed_micros = current_micros - prev_last_beat_micros_;
       prev_last_beat_micros_ = current_micros;
       size_t expected_last_beat_index_ = prev_last_beat_index__ - (elapsed_micros/1000000.f) * fft_res / 8;
      for(size_t i = 0; i < num_candidates; i++){
        best_score_[i] = -3.4e8;
        backlink_[i] = 0;
        cumulative_continuity_error_[i] = 0;
        backlink_depth_[i]  = 0;
        for(size_t j = 0; j < i; j++){
          if(candidate_beats_[i].index - candidate_beats_[j].index > 2 * beat_size) continue;
          float tempo_error = tempoError(candidate_beats_[i].index - candidate_beats_[j].index, bpm, fft_res);
          float continuity_error = continuityError(candidate_beats_[i].index, expected_last_beat_index_, bpm, fft_res);

          float score = ALPHA * tempo_error + BETA * continuity_error + GAMMA * candidate_beats_[i].value ; 
          if(score > best_score_[i]){
            best_score_[i] = score;
            backlink_[i] = j;
            backlink_depth_[i]  =  backlink_depth_[j] +1;
          }
        }


        if(best_score_[i]  > last_beat_score && candidate_beats_[i].index > last_beat_search_threshold){
          last_beat_score = best_score_[i];
          last_beat_index_ = i;
        }
      }

      prev_last_beat_index__ = (1-SIGMA) * expected_last_beat_index_ + SIGMA * candidate_beats_[last_beat_index_].index;
      while(prev_last_beat_index__ < TIME_BINS - beat_size){
        prev_last_beat_index__ += beat_size;
      }


      beat_phase_  = ((TIME_BINS - prev_last_beat_index__) / (float) fft_res) / (60.f/bpm);
      beat_phase_ = beat_phase_-floor(beat_phase_);




#if DEBUG
      memset(beats_viz, 0, TIME_BINS * sizeof(float));
      size_t curr_beat = last_beat_index_;
      while(curr_beat != 0){
        beats_viz[candidate_beats_[curr_beat].index] = 1;
        curr_beat=backlink_[curr_beat];
      }

      memset(expectation_viz, 0, TIME_BINS * sizeof(float));
      for(int i = expected_last_beat_index_; i >=0; i -= beat_size){
        expectation_viz[i] = 1.0;
      }

#endif // DEBUG

  }

#if DEBUG
  float beats_viz[TIME_BINS];
  float expectation_viz[TIME_BINS];
  float debug;
#endif // DEBUG


  float beatPhase(){
    return beat_phase_;
  }

  // writes N beats into the last N slots in |beats|, where N is the max of |len| and the number of beats tracked
  size_t getBeats(AudioAnalyzeSpectralNovelty::Peak *beats, size_t len){
      memset(beats, 0, len * sizeof(AudioAnalyzeSpectralNovelty::Peak));
      size_t curr_beat = last_beat_index_;
      size_t backlink_count = 0;
      while(curr_beat != 0 && backlink_count < len){
        backlink_count++;
        beats[len - backlink_count] = candidate_beats_[curr_beat];
        curr_beat=backlink_[curr_beat];
      }
      return backlink_count;
  }

private:
  AudioAnalyzeSpectralNovelty::Peak candidate_beats_[MAX_BEATS];
  float best_score_[MAX_BEATS];
  size_t backlink_[MAX_BEATS];
  size_t backlink_depth_[MAX_BEATS];
  float cumulative_continuity_error_[MAX_BEATS];
  size_t prev_last_beat_index__ =  TIME_BINS -1;
  uint32_t prev_last_beat_micros_ = 0;
  size_t last_beat_index_;
  float beat_phase_ = 0.5;

  float tempoError(size_t index_delta, float bpm, size_t fft_res){
    float tempo_period_seconds = 60/bpm;
    float actual_period_seconds = (float) index_delta  / fft_res;
    float logval = logf(actual_period_seconds/tempo_period_seconds);
    return -1 * logval * logval;
  }

  float continuityError(size_t index, size_t expected_index, float bpm, size_t fft_res){
       size_t beat_size = fft_res * 60.f/bpm;
       float continuity_error = 0;
      if(prev_last_beat_micros_ !=0 ){
         continuity_error =-1 *  (abs((expected_index % beat_size) - (index % beat_size)) / ((float) beat_size));
      }
      return continuity_error;
  }
};

#endif //analyze_beats_h_
