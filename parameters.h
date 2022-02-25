// waveform FFT params
#define FFT_HOP_LENGTH 512
#define SAMPLE_RATE 44000

// Novelty curve params
#define TIME_BINS 1024
#define LOCAL_AVG_SAMPLES 16
#define C 10
#define EPSILON 0.0000001

// Tempo estimation params
#define DEFAULT_CENTER_BPM 140.0
#define WEIGHTING_WIDTH 0.4

//Be0at Tracker Params
#define TIGHTNESS 100
#define MAX_BEATS  256
#define ALPHA 1
#define BETA 10000
#define GAMMA 1
#define SIGMA 1
#define TEMPO_ERROR_THRESHOLD -0.05
#define BPM_FILTER_PARAM 0.01
#define BPM_MULTIPLIER 1

// Debug params
#define DEBUG 1
#define PLOT_BINS  512
