#include<jni.h>
extern "C"{
#include "include/libavcodec/avcodec.h"
}
#include <pthread.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;


class PakcetQueue
{

public:
	typedef struct MyAVPacketList {
		AVPacket pkt;
		struct MyAVPacketList *next;
		int serial;
	} MyAVPacketList;
	MyAVPacketList *first_pkt, *last_pkt;
	int nb_packets;
	int size;
	int abort_request;
	int serial;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	AVPacket flush_pkt;
public:
	int packet_queue_put_private(AVPacket *pkt);
	int packet_queue_put(AVPacket *pkt);
	int packet_queue_put_nullpacket( int stream_index);
	void packet_queue_init();
	void packet_queue_flush();
	void packet_queue_destroy();
	void packet_queue_abort();
	void packet_queue_start();
	int packet_queue_get( AVPacket *pkt, int block, int *serial);
};
