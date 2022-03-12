#ifndef mock_audio_stream_h_
#define mock_audio_stream_h_

#include <stdint.h>
#include <stddef.h>

#define AUDIO_BLOCK_SAMPLES  128

class AudioStream;
class AudioConnection;

typedef struct audio_block_struct {
	uint8_t  ref_count;
	uint8_t  reserved1;
	uint16_t memory_pool_index;
	int16_t  data[AUDIO_BLOCK_SAMPLES];
} audio_block_t;


class AudioConnection
{
public:
	AudioConnection(AudioStream &source, AudioStream &destination) :
		src(source), dst(destination), src_index(0), dest_index(0),
		next_dest(NULL)
		{ isConnected = false;
		  connect(); }
	AudioConnection(AudioStream &source, unsigned char sourceOutput,
		AudioStream &destination, unsigned char destinationInput) :
		src(source), dst(destination),
		src_index(sourceOutput), dest_index(destinationInput),
		next_dest(NULL)
		{ isConnected = false;
		  connect(); }
	friend class AudioStream;
	~AudioConnection() {
		disconnect();
	}
	void disconnect(void){}
	void connect(void){}
protected:
	AudioStream &src;
	AudioStream &dst;
	unsigned char src_index;
	unsigned char dest_index;
	AudioConnection *next_dest;
	bool isConnected;
};

class AudioStream
{
public:
	AudioStream(unsigned char ninput, audio_block_t **iqueue) :
		num_inputs(ninput), inputQueue(iqueue) {
			// active = false;
			// destination_list = NULL;
			// for (int i=0; i < num_inputs; i++) {
			// 	inputQueue[i] = NULL;
			// }
			// // add to a simple list, for update_all
			// // TODO: replace with a proper data flow analysis in update_all
			// if (first_update == NULL) {
			// 	first_update = this;
			// } else {
			// 	AudioStream *p;
			// 	for (p=first_update; p->next_update; p = p->next_update) ;
			// 	p->next_update = this;
			// }
			// next_update = NULL;
			// cpu_cycles = 0;
			// cpu_cycles_max = 0;
			// numConnections = 0;
		}
protected:
	bool active;
	unsigned char num_inputs;
	static audio_block_t * allocate(void){return nullptr;};
	static void release(audio_block_t * block);
	void transmit(audio_block_t *block, unsigned char index = 0){}
	audio_block_t * receiveReadOnly(unsigned int index = 0){return nullptr;}
	audio_block_t * receiveWritable(unsigned int index = 0){return nullptr;}
	friend class AudioConnection;
	uint8_t numConnections;
private:
	AudioConnection *destination_list;
	audio_block_t **inputQueue;
	static bool update_scheduled;
	virtual void update(void) = 0;
	static AudioStream *first_update; // for update_all
	AudioStream *next_update; // for update_all
};

#endif //mock_audio_stream_h_