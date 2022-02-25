#ifndef tempo_estimator_h_
#define tempo_estimator_h_

#include "arm_math.h"
#include "novelty_function.h"
#include "parameters.h"

static const size_t NOV_SPECTRUM_BINS = TIME_BINS/2;
static const size_t NOV_SPECTRUM_PLOT_STEP = NOV_SPECTRUM_BINS/2/PLOT_BINS;

static const float bin_resolution_hz = ((float) SAMPLE_RATE) / FFT_HOP_LENGTH / TIME_BINS /4;
static const float bin_resolution_bpm = bin_resolution_hz * 60;

class TempoEstimator{
public:
  TempoEstimator()  {
     arm_cfft_radix2_init_f32(&novelty_fft, TIME_BINS, 0, 1);
  }
  
  void compute(const NoveltyFunction &novelty_function){
    memset(novelty_fft_buffer, 0, 2 * TIME_BINS * sizeof(float)); 
    
    for(size_t i = 0; i < TIME_BINS; i++){
      novelty_fft_buffer[ i * 2] = novelty_function.read(i);
    }
    
    arm_cfft_radix2_f32 (&novelty_fft, novelty_fft_buffer);
    for (size_t i=0; i < NOV_SPECTRUM_BINS; i++) {
      float real = novelty_fft_buffer[i * 2];
      float imag = novelty_fft_buffer[i * 2 + 1];
      
      novelty_spectrum[i] = sqrtf(real * real + imag * imag);
    }

    for (size_t i=0; i < NOV_SPECTRUM_BINS/2; i++) {
      float real = novelty_fft_buffer[i * 2];
      float imag = novelty_fft_buffer[i * 2 + 1];
      
      novelty_spectrum[i] = sqrtf(real * real + imag * imag);

#ifdef DEBUG   
      if(i % NOV_SPECTRUM_PLOT_STEP == 0){
        novelty_spectrum_plot[ i / NOV_SPECTRUM_PLOT_STEP] = 0;
      }
      if(novelty_spectrum[i] > novelty_spectrum_plot[ i / NOV_SPECTRUM_PLOT_STEP]){
         novelty_spectrum_plot[ i / NOV_SPECTRUM_PLOT_STEP] = novelty_spectrum[i];
      }
#endif // DEBUG

    }
  }

//  float bpm(float center_frequency_bpm = DEFAULT_CENTER_BPM){
//
//    return getPeakForBpmRange(center_frequency_bpm * 2.f/3.f, center_frequency_bpm * 4.f/3.f) * bin_resolution_bpm;
//  }

  float bpm(float center_frequency_bpm = DEFAULT_CENTER_BPM){
    return interpolatePeak(index(center_frequency_bpm)) * bin_resolution_bpm;
  }
  size_t index(float center_frequency_bpm){

      size_t bottom_bin = (center_frequency_bpm/2) / bin_resolution_bpm;
      size_t top_bin = fmin((center_frequency_bpm * 2) / bin_resolution_bpm, NOV_SPECTRUM_BINS);
      size_t window_width = top_bin -  bottom_bin;

      float max_value = 0;
      size_t max_index = 0;

//      size_t center_index =  2* center_frequency_bpm/bin_resolution_bpm;
      
      for (size_t i=bottom_bin; i < top_bin; i++) {
          float period = 1/(i * bin_resolution_bpm);
          float weighting = logf(period/(1/center_frequency_bpm))/WEIGHTING_WIDTH;
          weighting = expf(-0.5 * weighting * weighting);
//          float weighting = fabs(i - center_index)/(window_width);
          
          float weighted_value = weighting * novelty_spectrum[i];
          if(weighted_value > max_value){
            max_value = weighted_value;
            max_index= i;
          }
      }

      return max_index ;
  }

  float readNoveltySpectrum(size_t i) const{
    if(i > PLOT_BINS) return 0.f;
    return novelty_spectrum_plot[i % PLOT_BINS];
  }
  
#ifdef DEBUG
  // if you Serial.print this buffer one bin per line the Arduino Serial Plotter can render the novelty spectrum
  float novelty_spectrum_plot[PLOT_BINS];
  float readNoveltySpectrumPlot(size_t i) const{
    if(i > PLOT_BINS) return 0.f;
    return novelty_spectrum_plot[i % PLOT_BINS];
  }

#endif // DEBUG

private:

  arm_cfft_radix2_instance_f32 novelty_fft;
  
  float novelty_fft_buffer[ 2 * TIME_BINS];

  float novelty_spectrum[NOV_SPECTRUM_BINS];

  size_t getPeakForBpmRange(float low_bpm, float high_bpm){

     size_t bottom_bin = (size_t) (low_bpm/bin_resolution_bpm);
     size_t top_bin = (size_t) (high_bpm/bin_resolution_bpm);


      float max_value = 0;
      size_t max_index = bottom_bin;
      for (size_t i=bottom_bin; i < top_bin; i++) {
          if(novelty_spectrum[i] > max_value){
            max_value = novelty_spectrum[i];
            max_index= i;
          }
      }

      return max_index;
  }


  // Returns a float representing the nudged peak index
  float interpolatePeak(size_t peak_index){
    
      // Parbolic interpolation 
      // https://dspguru.com/dsp/howtos/how-to-interpolate-fft-peak/
      size_t k = peak_index;
      size_t k1 = ( k + NOV_SPECTRUM_BINS -1) % NOV_SPECTRUM_BINS;
      size_t k2 = k ;
      size_t k3 = (k +1) % NOV_SPECTRUM_BINS;
      
      float y1  =  novelty_spectrum[k1];
      float y2  =  novelty_spectrum[k2];
      float y3  =  novelty_spectrum[k3];
      float delta = (y3 - y1) / (2 * (2 * y2 - y1 - y3));

      return peak_index + delta;


//      size_t index[2];
//      if(delta > 0){
//        index[0] = k;
//        index[1] = k+1;
//      }else{
//        index[0] = k - 1;
//        index[1] = k;
//      }
//      float phases[2];
//      for(size_t i = 0; i < 2; i++){
//        float real = novelty_fft_buffer[index[i] * 2];
//        float imag = novelty_fft_buffer[index[i] * 2 + 1];
//        
//        phases[i] = atan2f(imag, real);
//      }
//
//      float weight = delta > 0 ? delta : 1.f + delta;
//      float phase = (1 - weight) * phases[0] +  weight * phases[1];
//      return (phase  / PI / 2) + 0.5;
  }
  


};

#endif // tempo_estimator_h_
