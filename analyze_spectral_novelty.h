
#ifndef analyze_spectral_novelty_h_
#define analyze_spectral_novelty_h_

#include "analyze_fft1024.h"
#include "parameters.h"

static const size_t PLOT_STEP = TIME_BINS/PLOT_BINS;

class AudioAnalyzeSpectralNovelty {
public:
  AudioAnalyzeSpectralNovelty(){
    memset(novelty_curve_, 0, TIME_BINS * sizeof(float));
    memset(peak_curve_, 0, TIME_BINS * sizeof(float));
    memset(novelty_plot_, 0, PLOT_BINS * sizeof(float));

    for(size_t i = 0; i < 2; i++){
      memset(spectrums_, 0, FFT_HOP_LENGTH * sizeof(float));
    }
  }

void compute(AudioAnalyzeFFT1024 &fft){
    size_t curr_spectrum = (prev_spectrum_+1)%2;
    novelty_curve_[novelty_index_] = 0;
    for(size_t i = 0; i < FFT_HOP_LENGTH; i++){
      // Log compress the current FFT and store
      spectrums_[curr_spectrum][i] = log2f(1 + C * fabs(fft.read(i)));

      // Take the per frequency derivative and sum into the novelty curve
      // novelty_curve_[novelty_index_] += fabs(spectrums_[curr_spectrum][i] - spectrums_[prev_spectrum_][i]);
       novelty_curve_[novelty_index_] += fmax(spectrums_[curr_spectrum][i] - spectrums_[prev_spectrum_][i], 0);

    }

    // subtract the local average so only peaks_ remain
    float local_average = 0;
    for(size_t i = 0; i < LOCAL_AVG_SAMPLES; i++){
      local_average += novelty_curve_[(novelty_index_ + TIME_BINS - i) % TIME_BINS];
    }
    local_average /= LOCAL_AVG_SAMPLES;

    peak_curve_[novelty_index_] = fmax(novelty_curve_[novelty_index_] - local_average, 0.f);

    //find the max peak so the curve can be normalized
    max_peak_ = EPSILON; // we divide by max_peak_ to normalize so dont set to zero
    for(size_t i = 0; i < TIME_BINS; i++){
      size_t i0 = (novelty_index_ + i - 1)  % TIME_BINS;
      size_t i1 = (novelty_index_ + i)  % TIME_BINS;
      size_t i2 = (novelty_index_ + i + 1)  % TIME_BINS;

      float value = peak_curve_[i1];
      max_peak_ = fmax(max_peak_, value);
      if(value >= peak_curve_[i0] && value > peak_curve_[i2]){
        peaks_[i1] = {i, value};
      }else{
        peaks_[i1] = {i, 0};
      }
    }

#if DEBUG
    // if(novelty_index_ % PLOT_STEP == 0){
    //   novelty_plot[ novelty_index_ / PLOT_STEP] = 0.f;
    // }
    // novelty_plot[ novelty_index_ / PLOT_STEP] = max(peak_curve_[novelty_index_], novelty_plot[ novelty_index_ / PLOT_STEP]) ;
#endif // DEBUG

    prev_spectrum_ = curr_spectrum;
    novelty_index_ = (novelty_index_ +1) % TIME_BINS;

  }

  float read(size_t i) const {

    if(i > TIME_BINS) return 0.f;
    return peak_curve_[(novelty_index_ + i) % TIME_BINS] / max_peak_;
  }

  float readPeak(size_t i) const {

    if(i > TIME_BINS) return 0.f;
    return peaks_[(novelty_index_ + i) % TIME_BINS].value / max_peak_;
  }

  // |out| must point to an array at least TIME_BINS long
  void readAll(float * out) const {
    for(size_t i = 0; i < TIME_BINS; i++){
       out[i] = peak_curve_[(novelty_index_ + i) % TIME_BINS] / max_peak_;
    }
  }

  size_t length() const { return TIME_BINS;}


  struct Peak{
    size_t index;
    float value;
  };

  enum PeakSortOrder{
    VALUE,
    INDEX
  };

  // writes the |n| highest peaks_ into |out| in order of |order|
  void  getPeaks(Peak* out, size_t n, PeakSortOrder order = PeakSortOrder::INDEX){
    // first sort the peaks_ by value to find the N highest
    qsort(peaks_, TIME_BINS, sizeof(Peak), comparePeakValue );
    // copy those peaks_ into out buffer
    memcpy(out, peaks_, n * sizeof(Peak));

    for(size_t i = 0; i< n; i++){
      out[i].value /= max_peak_;
    }

    if(order == PeakSortOrder::INDEX){
      qsort(out, n, sizeof(Peak), comparePeakIndex );
    }
  }


#if DEBUG
  // if you Serial.print this buffer one bin per line the Arduino Serial Plotter can render the novelty curve
  float novelty_plot_[PLOT_BINS];
  float readPlot(size_t i) const {
    if(i > PLOT_BINS) return 0.f;
    return novelty_plot_[(novelty_index_/PLOT_STEP + i) % PLOT_BINS] / max_peak_;
  }
#endif // DEBUG


private:

   float spectrums_[2][FFT_HOP_LENGTH];
   size_t prev_spectrum_=0;

   //These are all circular buffers representing the novelty curve in some state
   float novelty_curve_[TIME_BINS];
   // think of this as the index of the current columnn in the spectrogram
   size_t novelty_index_ = 0;

   float peak_curve_[TIME_BINS];
   float max_peak_ = EPSILON;
   // Sorted list of peaks_ in peak curve.
   Peak peaks_[TIME_BINS];

   // peak comparators for sorting
   static int comparePeakValue( const void* a, const void* b)
   {
       Peak *peak_a =  (Peak*) a ;
       Peak *peak_b =  (Peak*) b ;

       if ( peak_a->value  == peak_b->value ) return 0;
       else if ( peak_a->value < peak_b->value ) return 1;
       else return -1;
   }

   static int comparePeakIndex( const void* a, const void* b)
   {
       Peak *peak_a =  (Peak*) a ;
       Peak *peak_b =  (Peak*) b ;

       return (int) peak_a->index  - (int) peak_b->index;
   }

};

#endif // analyze_spectral_novelty_h_
