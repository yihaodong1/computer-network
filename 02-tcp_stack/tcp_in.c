#include "tcp.h"
#include "tcp_sock.h"
#include "tcp_timer.h"

#include "log.h"
#include "ring_buffer.h"

#include <stdlib.h>
// update the snd_wnd of tcp_sock
//
// if the snd_wnd before updating is zero, notify tcp_sock_send (wait_send)
static inline void tcp_update_window(struct tcp_sock *tsk, struct tcp_cb *cb)
{
	u16 old_snd_wnd = tsk->snd_wnd;
	tsk->snd_wnd = cb->rwnd;
	if (old_snd_wnd == 0)
		wake_up(tsk->wait_send);
}

// update the snd_wnd safely: cb->ack should be between snd_una and snd_nxt
static inline void tcp_update_window_safe(struct tcp_sock *tsk, struct tcp_cb *cb)
{
	if (less_or_equal_32b(tsk->snd_una, cb->ack) && less_or_equal_32b(cb->ack, tsk->snd_nxt))
		tcp_update_window(tsk, cb);
}

#ifndef max
#	define max(x,y) ((x)>(y) ? (x) : (y))
#endif

// check whether the sequence number of the incoming packet is in the receiving
// window
static inline int is_tcp_seq_valid(struct tcp_sock *tsk, struct tcp_cb *cb)
{
	u32 rcv_end = tsk->rcv_nxt + max(tsk->rcv_wnd, 1);
	if (less_than_32b(cb->seq, rcv_end) && less_or_equal_32b(tsk->rcv_nxt, cb->seq_end)) {
		return 1;
	}
	else {
		log(ERROR, "received packet with invalid seq, drop it.");
		return 0;
	}
}

// Process the incoming packet according to TCP state machine. 
void tcp_process(struct tcp_sock *tsk, struct tcp_cb *cb, char *packet)
{
	assert(tsk != NULL);
	// assert(tsk->snd_nxt == cb->seq);
	log(DEBUG, "cb->flags: 0x%x, cb->seq: %d, cb->ack: %d", \
	cb->flags, cb->seq, cb->ack);	
	if(tsk->state == TCP_CLOSED){//not realise
		if(cb->flags ==  (TCP_FIN | TCP_ACK)){

		}else
			assert(0);
		
	}else if(tsk->state == TCP_LISTEN){
		if(cb->flags == TCP_SYN){

			
			struct tcp_sock *new_tsk = alloc_tcp_sock();
			new_tsk->parent = tsk;
			new_tsk->sk_sip = cb->daddr;
			new_tsk->sk_dip = cb->saddr;
			new_tsk->sk_sport = cb->dport;
			new_tsk->sk_dport = cb->sport;
			new_tsk->snd_nxt = tcp_new_iss();
			new_tsk->rcv_nxt = cb->seq + 1;
			if(!tcp_sock_accept_queue_full(tsk))
				tcp_sock_accept_enqueue(new_tsk);
			tcp_set_state(new_tsk, TCP_SYN_RECV);
			tcp_hash(new_tsk);
			tcp_send_control_packet(new_tsk, TCP_SYN | TCP_ACK);
		}else if(cb->flags == (TCP_FIN | TCP_ACK)){
			
		}else{
			assert(0);
		}
	}else if(tsk->state == TCP_SYN_SENT){
		if(cb->flags == (TCP_SYN | TCP_ACK)){
			// tsk->snd_nxt = cb->ack + 1;
			tsk->rcv_nxt = cb->seq + 1;
			wake_up(tsk->wait_connect);
			
		}else if(cb->flags == TCP_SYN){
			tcp_set_state(tsk, TCP_SYN_RECV);
			tcp_send_control_packet(tsk, TCP_SYN | TCP_ACK);
		}else
		{
			assert(0);
		}
		
	}else if(tsk->state == TCP_SYN_RECV){
		if(cb->flags == TCP_ACK){
			tcp_set_state(tsk, TCP_ESTABLISHED);
			wake_up(tsk->parent->wait_accept);
		}else//syn_recv --> fin_wait1 not realise
		{
			assert(0);
		}
		
	}else if(tsk->state == TCP_ESTABLISHED){
		if(cb->flags & TCP_FIN){
			tcp_set_state(tsk, TCP_CLOSE_WAIT);
			tcp_send_control_packet(tsk, TCP_ACK);
			// if(tsk->wait_recv->sleep)
				wake_up(tsk->wait_recv);
				wake_up(tsk->wait_send);
		}else if(cb->flags == (TCP_PSH | TCP_ACK)){
			printf("%s\n", cb->payload);
			// tcp_sock_write(tsk, cb->payload, strlen(cb->payload));
			write_ring_buffer(tsk->rcv_buf, cb->payload, strlen(cb->payload));
			while(ring_buffer_empty(tsk->rcv_buf));
			// tsk->snd_nxt = cb->ack + 1;
			tsk->rcv_nxt = cb->seq + strlen(cb->payload);
			tcp_send_control_packet(tsk, TCP_ACK);
			// if(tsk->wait_recv->sleep)
			wake_up(tsk->wait_recv);
		}else if(cb->flags == TCP_ACK){
			// tsk->snd_nxt = cb->ack + 1;
			if(tsk->wait_send->sleep)
				wake_up(tsk->wait_send);
			//file send
			// if(cb->payload != NULL){
			// 	// printf("%s\n", cb->payload);
			// 	write_ring_buffer(tsk->rcv_buf, cb->payload, strlen(cb->payload));
			// 	while(ring_buffer_empty(tsk->rcv_buf));
			// 	tsk->rcv_nxt = cb->seq + strlen(cb->payload);
			// 	tcp_send_control_packet(tsk, TCP_ACK);
			// 	wake_up(tsk->wait_recv);
			// }
		}else
			assert(0);
		
	}else if(tsk->state == TCP_FIN_WAIT_1){
		if(cb->flags & TCP_ACK){
			tcp_set_state(tsk, TCP_FIN_WAIT_2);
		}else if (cb->flags == TCP_FIN){
			tcp_set_state(tsk, TCP_CLOSING);
			tcp_send_control_packet(tsk, TCP_ACK);
		}else if (cb->flags == (TCP_FIN | TCP_ACK)){

		}
		else
		{
			assert(0);
		}
		
	}else if(tsk->state == TCP_FIN_WAIT_2){
		if(cb->flags & TCP_FIN){
			tcp_set_state(tsk, TCP_TIME_WAIT);
			tcp_send_control_packet(tsk, TCP_ACK);
			tcp_set_timewait_timer(tsk);
			while(!list_empty(&tsk->timewait.list)){
				
			};
			tcp_set_state(tsk, TCP_CLOSED);
			free_tcp_sock(tsk);
		}else 
			assert(0);
	}else if(tsk->state == TCP_CLOSING){
		if(cb->flags == TCP_ACK){
			tcp_set_state(tsk, TCP_TIME_WAIT);
			
		}else
			assert(0);
	}else if(tsk->state == TCP_TIME_WAIT){
		assert(0);

	}else if(tsk->state == TCP_CLOSE_WAIT){
		if(cb->flags & (TCP_FIN | TCP_ACK)){}
		else assert(0);
	}else if(tsk->state == TCP_LAST_ACK){
		if(cb->flags & TCP_ACK){
			tcp_set_state(tsk, TCP_CLOSED);
			free_tcp_sock(tsk);
		}else
		{
			assert(0);
		}
		
	}
	else{
		assert(0);
	}
}
