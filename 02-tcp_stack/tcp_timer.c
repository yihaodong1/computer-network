#include "tcp.h"
#include "tcp_timer.h"
#include "tcp_sock.h"

#include <stdio.h>
#include <unistd.h>
#include "log.h"
static struct list_head timer_list;

// scan the timer_list, find the tcp sock which stays for at 2*MSL, release it
void tcp_scan_timer_list()
{
	struct tcp_sock *tsk, *q;
	list_for_each_entry_safe(tsk, q, &timer_list, timewait.list){
		tsk->timewait.timeout += TCP_TIMER_SCAN_INTERVAL;
		if(tsk->timewait.timeout > TCP_TIMEWAIT_TIMEOUT){
			log(DEBUG, "delete "IP_FMT":%u -->"IP_FMT":%u from timer_list", \
			HOST_IP_FMT_STR(tsk->sk_sip), tsk->sk_sport, \
			HOST_IP_FMT_STR(tsk->sk_dip), tsk->sk_dport);
			list_delete_entry(&tsk->timewait.list);
			init_list_head(&tsk->timewait.list);
		}
	}
}

// set the timewait timer of a tcp sock, by adding the timer into timer_list
void tcp_set_timewait_timer(struct tcp_sock *tsk)
{
	list_add_head(&tsk->timewait.list, &timer_list);
	log(DEBUG, "add "IP_FMT":%u -->"IP_FMT":%u to timer_list", \
			HOST_IP_FMT_STR(tsk->sk_sip), tsk->sk_sport, \
			HOST_IP_FMT_STR(tsk->sk_dip), tsk->sk_dport);
}

void tcp_scan_retrans_timer_list(void){
	struct tcp_sock *tsk, *q;
	list_for_each_entry_safe(tsk, q, &timer_list, retrans_timer.list){
		tsk->timewait.timeout += TCP_TIMER_SCAN_INTERVAL;
		if(tsk->timewait.timeout > TCP_TIMEWAIT_TIMEOUT){
			log(DEBUG, "delete "IP_FMT":%u -->"IP_FMT":%u from timer_list", \
			HOST_IP_FMT_STR(tsk->sk_sip), tsk->sk_sport, \
			HOST_IP_FMT_STR(tsk->sk_dip), tsk->sk_dport);
			list_delete_entry(&tsk->timewait.list);
			init_list_head(&tsk->timewait.list);
		}
	}
}

void tcp_set_retrans_timer(struct tcp_sock *tsk){
	;
}

void tcp_update_retrans_timer(struct tcp_sock *tsk){

}

void tcp_unset_retrans_timer(struct tcp_sock *tsk){

}
// scan the timer_list periodically by calling tcp_scan_timer_list
void *tcp_timer_thread(void *arg)
{
	init_list_head(&timer_list);
	while (1) {
		usleep(TCP_TIMER_SCAN_INTERVAL);
		tcp_scan_timer_list();
	}

	return NULL;
}
