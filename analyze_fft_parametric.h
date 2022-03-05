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

#include "Arduino.h"
#include "AudioStream.h"
#include "arm_math.h"
#include "utility/queue.h"

// math from https://en.wikipedia.org/wiki/Hann_function
void computeHanningWindow(int16_t *window, size_t N){
	for(size_t n=0; n < N; n++){
		float val = 0.5 * (1 - cosf(2 * PI * n / (float) N));
		arm_float_to_q15(&val, window[i], 1);
	}
}

class AudioAnalyzeFFTParametric : public AudioStream
{
public:
  struct Parameters{
		size_t window_length = 1024;
		// this gives us about 100 ms of buffered audio at 44kHz
		size_t max_buffered_blocks = 32;

		bool validate(){
			bool valid = true;

			valid = valid && (window_length > 128) && (window_length <= 4096);
			valid = valid && ((window_length & (window_length - 1)) == 0) && window_length % 128 == 0;
			valid = valid && max_buffered_blocks > 0 && max_buffered_blocks <= 2048;

			return valid;
		}
	};

	AudioAnalyzeFFTParametric(const Parameters params = Parameters()) : AudioStream(1, inputQueueArray), state(0), outputflag(false), params_(params) {
		arm_cfft_radix4_init_q15(&fft_inst, 1024, 0, 1);

		valid_ = params_.validate() && allocateBuffers();
		if(!valid_) return;

		computeHanningWindow(window_, params_.window_length);
	}

	virtual ~AudioAnalyzeFFTParametric(){
		free(window_);
		free(fft_buffer_);
		free(output_);
	}

	bool valid() { return valid_};
	size_t dynamic_memory() const {
		return dynamic_memory_;
	}


	void update(void) override;
	uint16_t *output_ __attribute__ ((aligned (4)));
private:
	void init(void);
	const int16_t *window;
	int16_t *fft_buffer_ __attribute__ ((aligned (4)));
	volatile bool can_fft_;
	audio_block_t *inputQueueArray[1];
	arm_cfft_radix4_instance_q15 fft_inst;

	size_t dynamic_memory_ = 0;
	int16_t *window_ = nullptr;
	bool valid_ = false;
	Parameters params_;



	bool allocateBuffers(){
		window_ = (int16_t *) malloc(params_.window_length * sizeof(int16_t));
		if(!window_) return false;
		dynamic_memory_ += params_.window_length * sizeof(int16_t);

		fft_buffer_ = (int16_t *) malloc( 2 * params_.window_length * sizeof(int16_t));
		if(!fft_buffer_) return false;
		dynamic_memory_ += 2 * parsams_.window_length * sizeof(int16_t);

		output_ = (int16_t *) malloc( params_.window_length * sizeof(int16_t) / 2);
		if(!output_) return false;
		dynamic_memory_ += params_.window_length * sizeof(int16_t) / 2;



	}
};

#endif // analyze_fft_parametric_h_
