#include "tcp_sock.h"

#include "log.h"

#include <unistd.h>
#define FILE_TRANS
char *send_str = "0123456789abcdefghijklmnopqrstuvwxyz"\
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
// tcp server application, listens to port (specified by arg) and serves only one
// connection request
void *tcp_server(void *arg)
{
	u16 port = *(u16 *)arg;
	struct tcp_sock *tsk = alloc_tcp_sock();

	struct sock_addr addr;
	addr.ip = htonl(0);
	addr.port = port;
	if (tcp_sock_bind(tsk, &addr) < 0) {
		log(ERROR, "tcp_sock bind to port %hu failed", ntohs(port));
		exit(1);
	}

	if (tcp_sock_listen(tsk, 3) < 0) {
		log(ERROR, "tcp_sock listen failed");
		exit(1);
	}

	log(DEBUG, "listen to port %hu.", ntohs(port));

	struct tcp_sock *csk = tcp_sock_accept(tsk);

	log(DEBUG, "accept a connection.");

	sleep(5);
	#ifdef ECHO
	int pkt_size = ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + \
		TCP_BASE_HDR_SIZE + strlen(csk->rcv_buf->buf) + 15;
	do{
		
		char *packet = malloc(pkt_size);
		memset(packet, 0, pkt_size);
		sleep_on(csk->wait_recv);
		if(ring_buffer_empty(csk->rcv_buf))
			break;
		
		strncpy(packet + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + TCP_BASE_HDR_SIZE, \
		"server echoes: ", 15);
		// strncpy(packet + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + TCP_BASE_HDR_SIZE\
		//  + 15, csk->rcv_buf->buf, strlen(csk->rcv_buf->buf));
		read_ring_buffer(csk->rcv_buf, packet + ETHER_HDR_SIZE + \
		IP_BASE_HDR_SIZE + TCP_BASE_HDR_SIZE + 15, ring_buffer_used(csk->rcv_buf));
		log(DEBUG, "send: %s", packet + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + TCP_BASE_HDR_SIZE);
		tcp_send_packet(csk, packet, pkt_size);
		sleep_on(csk->wait_send);
	}while(1);
	#endif
	
	#ifdef FILE_TRANS
	int pkt_size = 1000;
	char *filename = "server-output.dat";
	if(access(filename, F_OK) == 0){
		if (remove(filename) != 0) {
            perror("File removal failed");
            exit(EXIT_FAILURE);
        }
	}
	FILE *file = fopen(filename, "ab");
	if(file == NULL){
		log(DEBUG, "open file failed");
		exit(EXIT_FAILURE);
	}
	int n;
	do{	
		
		sleep_on(csk->wait_recv);
		n = ring_buffer_used(csk->rcv_buf);
		char *packet = malloc(n);
		memset(packet, 0, n);
		if(ring_buffer_empty(csk->rcv_buf))
			break;
		
		read_ring_buffer(csk->rcv_buf, packet, n);
		fwrite(packet, 1, n, file);
	}while(1);
	fclose(file);
	#endif
	tcp_sock_close(csk);
	
	return NULL;
}

// tcp client application, connects to server (ip:port specified by arg), each
// time sends one bulk of data and receives one bulk of data 
void *tcp_client(void *arg)
{
	struct sock_addr *skaddr = arg;

	struct tcp_sock *tsk = alloc_tcp_sock();

	if (tcp_sock_connect(tsk, skaddr) < 0) {
		log(ERROR, "tcp_sock connect to server ("IP_FMT":%hu)failed.", \
				NET_IP_FMT_STR(skaddr->ip), ntohs(skaddr->port));
		exit(1);
	}

	sleep(1);
	#ifdef ECHO
	int pkt_size = ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + TCP_BASE_HDR_SIZE + strlen(send_str);

	for(int i = 0; i < 5; i++){
		char *packet = malloc(pkt_size);
		memset(packet, 0, pkt_size);	
		strncpy(packet + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + TCP_BASE_HDR_SIZE,\
			send_str + i, strlen(send_str) - i);
		strncpy(packet + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + TCP_BASE_HDR_SIZE + strlen(send_str) - i,\
			send_str, i);
		tcp_send_packet(tsk, packet, pkt_size);
		// sleep_on(tsk->wait_send);
		sleep_on(tsk->wait_recv);
	}
	#endif

	#ifdef FILE_TRANS
	char *filename = "client-input.dat";
	FILE *file = fopen(filename, "rb");
	if(file == NULL){
		log(DEBUG, "open file failed");
		exit(EXIT_FAILURE);
	}
	int pkt_size = ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + TCP_BASE_HDR_SIZE + 1000;
	char* packet = malloc(pkt_size);
	while(fread(packet + ETHER_HDR_SIZE + IP_BASE_HDR_SIZE + TCP_BASE_HDR_SIZE\
	,1, 1000, file) > 0){
		tcp_send_packet(tsk, packet, pkt_size);
		sleep_on(tsk->wait_send);
		packet = malloc(pkt_size);
	}
	#endif

	tcp_sock_close(tsk);

	return NULL;
}
