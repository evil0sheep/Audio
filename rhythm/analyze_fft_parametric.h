/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef analyze_fft_parametric_h_
#define analyze_fft_parametric_h_


#if defined(TRAIN_PARAMETERS)
#define DATA_TYPE float
#include "MockAudioStream.h"
#include "kiss_fft.h"
#define PI 3.141592653589793238
#else
#define DATA_TYPE int16_t
#include "AudioStream.h"
#include "arm_math.h"
#endif

void computeHanningWindow(DATA_TYPE *window, size_t N);

class AudioAnalyzeFFTParametric : public AudioStream
{
public:
  struct Parameters{

		size_t window_length = 1024;
		size_t window_step = 512;
		// this gives us about 100 ms of buffered audio at 44kHz
		size_t max_buffered_blocks = 32;

		Parameters(){}

		bool validate(){
			bool valid = true;

			valid = valid && (window_length > 128) && (window_length <= 4096);
			valid = valid && ((window_length & (window_length - 1)) == 0) && window_length % 128 == 0;

			valid = valid && (window_step > 128) && (window_step <= window_length);

			valid = valid && max_buffered_blocks > 0 && max_buffered_blocks <= 2048;

			return valid;
		}
	};

	AudioAnalyzeFFTParametric(const Parameters params = Parameters()) : AudioStream(1, inputQueueArray), state(0), outputflag(false), params_(params) {

    #if defined(TRAIN_PARAMETERS)
      fft_inst = kiss_fft_alloc( 1024 ,0 ,0,0);
    #else
	    arm_cfft_radix4_init_q15(&fft_inst, 1024, 0, 1);
	  #endif

		valid_ = params_.validate() && allocateBuffers();
		if(!valid_) return;

		computeHanningWindow(window_, params_.window_length);
	}

	virtual ~AudioAnalyzeFFTParametric(){
		free(window_);
		free(fft_buffer_);
		free(output_);
		kiss_fft_free(fft_inst);
	}

	bool valid() { return valid_;};
	size_t dynamic_memory() const {
		return dynamic_memory_;
	}

	bool available() {
		if (outputflag == true) {
			outputflag = false;
			return true;
		}
		return false;
	}

  float read(unsigned int binNumber);

	void update(void) override;
	DATA_TYPE *output_ __attribute__ ((aligned (4)));
private:
	void init(void);
	const DATA_TYPE *window;
	DATA_TYPE *fft_buffer_ __attribute__ ((aligned (4)));
	volatile bool outputflag;
	audio_block_t *inputQueueArray[1];
	#if defined(TRAIN_PARAMETERS)
		kiss_fft_cfg fft_inst;
	#else
		arm_cfft_radix4_instance_q15 fft_inst;
	#endif

	size_t dynamic_memory_ = 0;
	DATA_TYPE *window_ = nullptr;
	bool valid_ = false;
	Parameters params_;
	uint8_t state;



	bool allocateBuffers(){
		window_ = (DATA_TYPE *) malloc(params_.window_length * sizeof(DATA_TYPE));
		if(!window_) return false;
		dynamic_memory_ += params_.window_length * sizeof(DATA_TYPE);

		fft_buffer_ = (DATA_TYPE *) malloc( 2 * params_.window_length * sizeof(DATA_TYPE));
		if(!fft_buffer_) return false;
		dynamic_memory_ += 2 * params_.window_length * sizeof(DATA_TYPE);

		output_ = (DATA_TYPE *) malloc( params_.window_length * sizeof(DATA_TYPE) / 2);
		if(!output_) return false;
		dynamic_memory_ += params_.window_length * sizeof(DATA_TYPE) / 2;

		return true;

	}
};

#endif // analyze_fft_parametric_h_
