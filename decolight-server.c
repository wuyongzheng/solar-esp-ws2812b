#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <stdint.h>
#include <errno.h>

#define MAGIC1 0x18ceb591
#define MAGIC2 0xe1697166
#define PORT 12345

struct msg_struct {
	uint32_t magic;
	uint32_t nounce;
	uint32_t vbat;
	uint32_t status;
};
struct ack_struct {
	uint32_t magic;
	uint32_t ack_nounce;
	uint32_t ts;
	uint32_t do_ota;
};

int do_init ()
{
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		fprintf(stderr, "error creating UDP socket\n");
		return -1;
	}

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port        = htons(PORT);
	if (bind(sock, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		fprintf(stderr, "error binding to UDP port %d\n", PORT);
		return -1;
	}

	return sock;
}

int do_service (const char *logfile, const char *otafile, int sock)
{
	struct msg_struct msg;
	struct ack_struct ack;
	struct sockaddr_in client_addr;
	socklen_t client_addr_len;
	char addr_str[INET_ADDRSTRLEN];

	memset(&client_addr, 0, sizeof(client_addr));
	client_addr_len = sizeof(client_addr);
	ssize_t msgsize = recvfrom(sock, &msg, sizeof(msg), 0, (struct sockaddr *)&client_addr, &client_addr_len);
	int saved_errno = errno;

	FILE *logfp = fopen(logfile, "a");
	if (logfp == NULL)
		return 0;

	if (msgsize < 0) {
		fprintf(logfp, "recvfrom failed. errno=%d\n", saved_errno);
		goto ret0;
	}
	if (msgsize != sizeof(msg)) {
		fprintf(logfp, "bad msgsize %ld\n", msgsize);
		goto ret1;
	}
	if (msg.magic != MAGIC1) {
		fprintf(logfp, "bad magic %x\n", msg.magic);
		goto ret1;
	}
	inet_ntop(AF_INET, &client_addr.sin_addr, addr_str, sizeof(addr_str));
	fprintf(logfp, "%lu %s %u %u\n",
			(unsigned long)time(NULL), addr_str, msg.vbat, msg.status);

	ack.magic = MAGIC2;
	ack.ack_nounce = msg.nounce;
	ack.ts = (unsigned long)time(NULL);
	ack.do_ota = access(otafile, F_OK) == 0;
	sendto(sock, &ack, sizeof(ack), 0, (struct sockaddr *)&client_addr, client_addr_len);

ret1:
	fclose(logfp);
	return 1;
ret0:
	fclose(logfp);
	return 0;
}

void daemonize ()
{
	close(0);
	close(1);
	close(2);
	if (fork())
		exit(0);
}

int main (int argc, char *argv[])
{
	if (argc != 3) {
		fprintf(stderr, "Usage: decolight-server <datafile> <otafile>\n");
		return 1;
	}

	int sock = do_init();
	if (sock < 0)
		return 1;

	daemonize();

	while (do_service(argv[1], argv[2], sock))
		;
	return 1;
}
