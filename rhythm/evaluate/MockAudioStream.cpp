/* Teensyduino Core Library
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2017 PJRC.COM, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * 2. If the Software is incorporated into a build system that allows
 * selection among a list of target devices, then similar target
 * devices manufactured by PJRC.COM must be included in the list of
 * target devices and selectable in the same manner.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <string.h>

#include "MockAudioStream.h"


// Allocate 1 audio data block.  If successful
// the caller is the only owner of this new block
audio_block_t * AudioStream::allocate(void)
{
	audio_block_t *block =  new audio_block_t;

  memset(block, 0, sizeof(audio_block_t));
	block->ref_count = 1;
	return block;
}

// Release ownership of a data block.  If no
// other streams have ownership, the block is
// returned to the free pool
void AudioStream::release(audio_block_t *block)
{
	if (block->ref_count > 1) {
		block->ref_count--;
	} else {
		delete block;
	}
}

// Transmit an audio data block
// to all streams that connect to an output.  The block
// becomes owned by all the recepients, but also is still
// owned by this object.  Normally, a block must be released
// by the caller after it's transmitted.  This allows the
// caller to transmit to same block to more than 1 output,
// and then release it once after all transmit calls.
void AudioStream::transmit(audio_block_t *block, unsigned char index)
{
	for (AudioConnection *c = destination_list; c != NULL; c = c->next_dest) {
		if (c->src_index == index) {
			if (c->dst.inputQueue[c->dest_index] == NULL) {
				c->dst.inputQueue[c->dest_index] = block;
				block->ref_count++;
			}
		}
	}
}


// Receive block from an input.  The block's data
// may be shared with other streams, so it must not be written
audio_block_t * AudioStream::receiveReadOnly(unsigned int index)
{
	audio_block_t *in;

	if (index >= num_inputs) return NULL;
	in = inputQueue[index];
	inputQueue[index] = NULL;
	return in;
}

// Receive block from an input.  The block will not
// be shared, so its contents may be changed.
audio_block_t * AudioStream::receiveWritable(unsigned int index)
{
	audio_block_t *in, *p;

	if (index >= num_inputs) return NULL;
	in = inputQueue[index];
	inputQueue[index] = NULL;
	if (in && in->ref_count > 1) {
		p = allocate();
		if (p) memcpy(p->data, in->data, sizeof(p->data));
		in->ref_count--;
		in = p;
	}
	return in;
}


void AudioConnection::connect(void)
{
	AudioConnection *p;

	if (isConnected) return;
	if (dest_index > dst.num_inputs) return;
	p = src.destination_list;
	if (p == NULL) {
		src.destination_list = this;
	} else {
		while (p->next_dest) {
			if (&p->src == &this->src && &p->dst == &this->dst
				&& p->src_index == this->src_index && p->dest_index == this->dest_index) {
				//Source and destination already connected through another connection, abort
				return;
			}
			p = p->next_dest;
		}
		p->next_dest = this;
	}
	this->next_dest = NULL;
	src.numConnections++;
	src.active = true;

	dst.numConnections++;
	dst.active = true;

	isConnected = true;

}

void AudioConnection::disconnect(void)
{
	AudioConnection *p;

	if (!isConnected) return;
	if (dest_index > dst.num_inputs) return;
	// Remove destination from source list
	p = src.destination_list;
	if (p == NULL) {
//>>> PAH re-enable the IRQ
		return;
	} else if (p == this) {
		if (p->next_dest) {
			src.destination_list = next_dest;
		} else {
			src.destination_list = NULL;
		}
	} else {
		while (p) {
			if (p == this) {
				if (p->next_dest) {
					p = next_dest;
					break;
				} else {
					p = NULL;
					break;
				}
			}
			p = p->next_dest;
		}
	}
//>>> PAH release the audio buffer properly
	//Remove possible pending src block from destination
	if(dst.inputQueue[dest_index] != NULL) {
		AudioStream::release(dst.inputQueue[dest_index]);
		// release() re-enables the IRQ. Need it to be disabled a little longer
		dst.inputQueue[dest_index] = NULL;
	}

	//Check if the disconnected AudioStream objects should still be active
	src.numConnections--;
	if (src.numConnections == 0) {
		src.active = false;
	}

	dst.numConnections--;
	if (dst.numConnections == 0) {
		dst.active = false;
	}

	isConnected = false;

}


// When an object has taken responsibility for calling update_all()
// at each block interval (approx 2.9ms), this variable is set to
// true.  Objects that are capable of calling update_all(), typically
// input and output based on interrupts, must check this variable in
// their constructors.

AudioStream * AudioStream::first_update = NULL;

void AudioStream::update_all(void) // AudioStream::update_all()
{
	AudioStream *p;

	//digitalWriteFast(2, HIGH);
	for (p = AudioStream::first_update; p; p = p->next_update) {
		if (p->active) {
			p->update();
		}
	}
}

