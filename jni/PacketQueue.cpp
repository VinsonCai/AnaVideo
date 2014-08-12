

#include "PacketQueue.h"

int PakcetQueue::packet_queue_put_private( AVPacket *pkt)
{
	MyAVPacketList *pkt1;

	if (abort_request)
		return -1;

	pkt1 = (MyAVPacketList*)av_malloc(sizeof(MyAVPacketList));
	if (!pkt1)
		return -1;
	pkt1->pkt = *pkt;
	pkt1->next = NULL;
	if (pkt == &flush_pkt)
		serial++;
	pkt1->serial = serial;

	if (!last_pkt)
		first_pkt = pkt1;
	else
		last_pkt->next = pkt1;
	last_pkt = pkt1;
	nb_packets++;
	size += pkt1->pkt.size + sizeof(*pkt1);
	/* XXX: should duplicate packet data in DV case */
	pthread_cond_signal(&cond);
	return 0;
}

int PakcetQueue::packet_queue_put( AVPacket *pkt) {
	int ret;

	/* duplicate the packet */
	if (pkt != &flush_pkt && av_dup_packet(pkt) < 0)
		return -1;

	pthread_mutex_lock(&mutex);
	ret = packet_queue_put_private(pkt);
	pthread_mutex_unlock(&mutex);
	if (pkt != &flush_pkt && ret < 0)
		av_free_packet(pkt);
	while(serial>20) {
			usleep(20000);
		}
	return ret;
}

int PakcetQueue::packet_queue_put_nullpacket(int stream_index) {
	AVPacket pkt1, *pkt = &pkt1;
	av_init_packet(pkt);
	pkt->data = NULL;
	pkt->size = 0;
	pkt->stream_index = stream_index;
	return packet_queue_put(pkt);
}

/* packet queue handling */
void PakcetQueue::packet_queue_init()
{
//	memset(q, 0, sizeof(PacketQueue));
	mutex = PTHREAD_MUTEX_INITIALIZER;
	cond = PTHREAD_COND_INITIALIZER;
	abort_request = 1;
}

void PakcetQueue::packet_queue_flush()
{
	MyAVPacketList *pkt, *pkt1;

	pthread_mutex_lock(&mutex);
	for (pkt = first_pkt; pkt != NULL; pkt = pkt1) {
		pkt1 = pkt->next;
		av_free_packet(&pkt->pkt);
		av_freep(&pkt);
	}
	last_pkt = NULL;
	first_pkt = NULL;
	nb_packets = 0;
	size = 0;
	pthread_mutex_unlock(&mutex);
}

void PakcetQueue::packet_queue_destroy()
{
	packet_queue_flush();
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
}

void PakcetQueue::packet_queue_abort()
{
	pthread_mutex_lock(&mutex);

	abort_request = 1;
	pthread_cond_signal(&cond);

	pthread_mutex_unlock(&mutex);
}

void PakcetQueue::packet_queue_start() {
	pthread_mutex_lock(&mutex);
	abort_request = 0;
	av_init_packet(&flush_pkt);
	flush_pkt.data = (uint8_t *)&flush_pkt;
	packet_queue_put_private( &flush_pkt);
	pthread_mutex_unlock(&mutex);
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
int PakcetQueue::packet_queue_get( AVPacket *pkt, int block, int *serial) {
	MyAVPacketList *pkt1;
	int ret;
	pthread_mutex_lock(&mutex);
	for (;;) {
		if (abort_request) {
			ret = -1;
			break;
		}
		pkt1 = first_pkt;
		if (pkt1) {
			first_pkt = pkt1->next;
			if (!first_pkt)
				last_pkt = NULL;
			nb_packets--;
			size -= pkt1->pkt.size + sizeof(*pkt1);
			*pkt = pkt1->pkt;
			if (serial)
				*serial = pkt1->serial;
			av_free(pkt1);
			ret = 1;
			break;
		} else if (!block) {
			ret = 0;
			break;
		} else {
			pthread_cond_wait(&cond, &mutex);
			//            SDL_CondWait(cond, mutex);
		}
	}
	pthread_mutex_unlock(&mutex);
	return ret;
}

