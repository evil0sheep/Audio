
#ifndef novelty_function_h_
#define novelty_function_h_

#include <Audio.h>
#include "parameters.h"

static const size_t PLOT_STEP = TIME_BINS/PLOT_BINS;

class NoveltyFunction {
public:
  NoveltyFunction(){
    memset(novelty_curve, 0, TIME_BINS * sizeof(float));
    memset(peak_curve, 0, TIME_BINS * sizeof(float));
    
    memset(novelty_plot, 0, PLOT_BINS * sizeof(float));

    for(size_t i = 0; i < 2; i++){ 
      memset(spectrums, 0, FFT_HOP_LENGTH * sizeof(float));
    }
  }

void compute(const AudioAnalyzeFFT1024 &fft){
    size_t curr_spectrum = (prev_spectrum+1)%2;
    
    novelty_curve[novelty_index] = 0;
    for(size_t i = 0; i < FFT_HOP_LENGTH; i++){
      // Log compress the current FFT and store
      spectrums[curr_spectrum][i] = log2f(1 + C * fabs(fft.read(i)));

      // Take the per frequency derivative and sum into the novelty curve 
//       novelty_curve[novelty_index] += fabs(spectrums[curr_spectrum][i] - spectrums[prev_spectrum][i]);
       novelty_curve[novelty_index] += fmax(spectrums[curr_spectrum][i] - spectrums[prev_spectrum][i], 0);

    }

    // subtract the local average so only peaks remain
    float local_average = 0;
    for(size_t i = 0; i < LOCAL_AVG_SAMPLES; i++){
      local_average += novelty_curve[(novelty_index + TIME_BINS - i) % TIME_BINS];
    }
    local_average /= LOCAL_AVG_SAMPLES;

    peak_curve[novelty_index] = fmax(novelty_curve[novelty_index] - local_average, 0.f);

    //find the max peak so the curve can be normalized
    max_peak = EPSILON; // we divide by max_peak to normalize so dont set to zero
    for(size_t i = 0; i < TIME_BINS; i++){
      size_t i0 = (novelty_index + i - 1)  % TIME_BINS;
      size_t i1 = (novelty_index + i)  % TIME_BINS;
      size_t i2 = (novelty_index + i + 1)  % TIME_BINS;

      float value = peak_curve[i1];
      max_peak = fmax(max_peak, value);
      
      if(value >= peak_curve[i0] && value > peak_curve[i2]){
        peaks[i1] = {i, value};
      }else{
        peaks[i1] = {i, 0};
      }
    }
    

#ifdef DEBUG
    if(novelty_index % PLOT_STEP == 0){
      novelty_plot[ novelty_index / PLOT_STEP] = 0.f;
    }
    novelty_plot[ novelty_index / PLOT_STEP] = max(peak_curve[novelty_index], novelty_plot[ novelty_index / PLOT_STEP]) ;
#endif // DEBUG

    prev_spectrum = curr_spectrum;
    novelty_index = (novelty_index +1) % TIME_BINS;

  }

  float read(size_t i) const {

    if(i > TIME_BINS) return 0.f;
    return peak_curve[(novelty_index + i) % TIME_BINS] / max_peak;
  }

  float readPeak(size_t i) const {

    if(i > TIME_BINS) return 0.f;
    return peaks[(novelty_index + i) % TIME_BINS].value / max_peak;
  }

  // |out| must point to an array at least TIME_BINS long
  void readAll(float * out) const {
    for(size_t i = 0; i < TIME_BINS; i++){
       out[i] = peak_curve[(novelty_index + i) % TIME_BINS] / max_peak;
    }
  }

  size_t length() { return TIME_BINS;}


  struct Peak{
    size_t index;
    float value;
  };

  enum PeakSortOrder{
    VALUE,
    INDEX
  };

  // writes the |n| highest peaks into |out| in order of |order|
  void  getPeaks(Peak* out, size_t n, PeakSortOrder order = PeakSortOrder::INDEX){
    // first sort the peaks by value to find the N highest
    qsort(peaks, TIME_BINS, sizeof(Peak), comparePeakValue );
    // copy those peaks into out buffer
    memcpy(out, peaks, n * sizeof(Peak));

    for(size_t i = 0; i< n; i++){
      out[i].value /= max_peak;
    }
    
    if(order == PeakSortOrder::INDEX){
      qsort(out, n, sizeof(Peak), comparePeakIndex );
    }
  }

  
#ifdef DEBUG
  // if you Serial.print this buffer one bin per line the Arduino Serial Plotter can render the novelty curve
  float novelty_plot[PLOT_BINS];
  float readPlot(size_t i) const {
    if(i > PLOT_BINS) return 0.f;
    return novelty_plot[(novelty_index/PLOT_STEP + i) % PLOT_BINS] / max_peak;
  }
#endif // DEBUG

  
private:

   float spectrums[2][FFT_HOP_LENGTH];
   size_t prev_spectrum=0;

   //These are all circular buffers representing the novelty curve in some state
   float novelty_curve[TIME_BINS];
   // think of this as the index of the current columnn in the spectrogram
   size_t novelty_index = 0; 

   float peak_curve[TIME_BINS];
   float max_peak = EPSILON;
   // Sorted list of peaks in peak curve. 
   Peak peaks[TIME_BINS];
   
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

#endif // novelty_function_h_
