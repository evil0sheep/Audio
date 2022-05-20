# include <stdio.h>
# include <unistd.h>
# include <sys/stat.h>
# include <sys/mman.h>
# include <fcntl.h>

# include "mad.h"
# include "mad_decoder.h"




/*
 * This is a private message structure. A generic pointer to this structure
 * is passed to each of the callback functions. Put here any data you need
 * to access from within the callbacks.
 */

struct buffer {
  unsigned char const *start;
  unsigned long length;
};

/*
 * This is the input callback. The purpose of this callback is to (re)fill
 * the stream buffer which is to be decoded. In this example, an entire file
 * has been mapped into memory, so we just call mad_stream_buffer() with the
 * address and length of the mapping. When this callback is called a second
 * time, we are finished decoding.
 */

static
enum mad_flow input(void *data,
		    struct mad_stream *stream)
{
  struct buffer *buffer = (struct buffer *) data;

  if (!buffer->length)
    return MAD_FLOW_STOP;

  mad_stream_buffer(stream, buffer->start, buffer->length);

  buffer->length = 0;

  return MAD_FLOW_CONTINUE;
}

/*
 * The following utility routine performs simple rounding, clipping, and
 * scaling of MAD's high-resolution samples down to 16 bits. It does not
 * perform any dithering or noise shaping, which would be recommended to
 * obtain any exceptional audio quality. It is therefore not recommended to
 * use this routine if high-quality output is desired.
 */

static inline
int16_t scale(mad_fixed_t sample)
{
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  int16_t quantized = sample >> (MAD_F_FRACBITS + 1 - 16);
  return quantized; // + INT16_MAX + 1;
}

static inline
uint16_t average(uint16_t a, uint16_t b)
{
    return (a / 2) + (b / 2) + (a & b & 1);
}

/*
 * This is the output callback function. It is called after each frame of
 * MPEG audio data has been completely decoded. The purpose of this callback
 * is to output (or play) the decoded PCM audio.
 */

static
enum mad_flow output(void *data,
		     struct mad_header const *header,
		     struct mad_pcm *pcm)
{
  unsigned int nchannels, nsamples;
  mad_fixed_t const *left_ch, *right_ch;

  /* pcm->samplerate contains the sampling frequency */

  nchannels = pcm->channels;
  nsamples  = pcm->length;
  left_ch   = pcm->samples[0];
  right_ch  = pcm->samples[1];

  if(nsamples % AUDIO_BLOCK_SAMPLES != 0){
    fprintf(stderr, "Error: AUDIO_BLOCK_SAMPLES does not divide evenly into nsamples\n");
    return MAD_FLOW_BREAK;
  };

  int16_t audio_block[AUDIO_BLOCK_SAMPLES];

  for(int i = 0; i < nsamples / AUDIO_BLOCK_SAMPLES; i++){
    for(int j = 0; j < AUDIO_BLOCK_SAMPLES; j++){
      audio_block[j] = scale(*left_ch++);
    }
    g_callback(audio_block);
  }

  // while (nsamples--) {
  //   int16_t sample;

  //   // uint16_t avg_sample = average(scale(*left_ch++), scale(*right_ch++));
  //   // putchar((avg_sample >> 0) & 0xff);
  //   // putchar((avg_sample >> 8) & 0xff);

  //   /* output sample(s) in 16-bit signed little-endian PCM */

  //   sample = scale(*left_ch++);
  //   putchar((sample >> 0) & 0xff);
  //   putchar((sample >> 8) & 0xff);

  //   // if (nchannels == 2) {
  //   //   sample = scale(*right_ch++);
  //   //   putchar((sample >> 0) & 0xff);
  //   //   putchar((sample >> 8) & 0xff);
  //   // }
  // }

  return MAD_FLOW_CONTINUE;
}

/*
 * This is the error callback function. It is called whenever a decoding
 * error occurs. The error is indicated by stream->error; the list of
 * possible MAD_ERROR_* errors can be found in the mad.h (or stream.h)
 * header file.
 */

static
enum mad_flow error(void *data,
		    struct mad_stream *stream,
		    struct mad_frame *frame)
{
  struct buffer *buffer = (struct buffer *) data;

  fprintf(stderr, "decoding error 0x%04x (%s) at byte offset %lu\n",
	  stream->error, mad_stream_errorstr(stream),
	  stream->this_frame - buffer->start);

  /* return MAD_FLOW_BREAK here to stop decoding (and propagate an error) */

  return MAD_FLOW_CONTINUE;
}

/*
 * This is the function called by main() above to perform all the decoding.
 * It instantiates a decoder object and configures it with the input,
 * output, and error callback functions above. A single call to
 * mad_decoder_run() continues until a callback function returns
 * MAD_FLOW_STOP (to stop decoding) or MAD_FLOW_BREAK (to stop decoding and
 * signal an error).
 */

bool MadDecoder::decode(std::string mp3_file_path)
{
  struct stat stat;
  void *fdm;

  int fd = open(mp3_file_path.c_str(), O_RDONLY);
  if(fd < 0){
    printf("Error: %s\n", strerror(errno));
    return false;
  }

  if (fstat(fd, &stat) == -1 || stat.st_size == 0){
    printf("Error: stat() failed");
    return false;
  }

  fdm = mmap(0, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (fdm == MAP_FAILED){
    printf("Error: failed to map mp3");
    return false;
  }

  struct mad_decoder decoder;
  int result;

  /* initialize our private message structure */

  start_ = (unsigned char *) fdm;
  length_ = stat.st_size;

  /* configure input, output, and error functions */

  mad_decoder_init(&decoder, this,
		   input, 0 /* header */, 0 /* filter */, output,
		   error, 0 /* message */);

  /* start decoding */

  result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);

  /* release the decoder */

  mad_decoder_finish(&decoder);

  if(result != 0){
    printf("Error: mad_decoder_run() returned %d\n", result);
    return false;
  }

  if (munmap(fdm, stat.st_size) == -1){
    printf("Error: munmap() failed");
    return false;
  }

  return 0;
}