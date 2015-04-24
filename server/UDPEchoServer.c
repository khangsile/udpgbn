#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include "UDPPacket.c"  /* UDP Packet */
#include "UDPAck.c"     /* UDP Ack */
#include <errno.h>      /* for errno and EINTR */
#include <signal.h>     /* for sigaction() */

#define ECHOMAX 8192     /* Longest string to echo */

typedef struct UDPPacket UDPPacket;
typedef struct UDPAck UDPAck;

void DieWithError(char *errorMessage);  /* External error handling function */
int is_lost(float loss_rate); /* Loss rate */
void CatchAlarm(int ignored); /* Handler for sig alarm */

int main(int argc, char *argv[])
{
    int sock;                        /* Socket */
    struct sockaddr_in echoServAddr; /* Local address */
    struct sockaddr_in echoClntAddr; /* Client address */
    unsigned int cliAddrLen;         /* Length of incoming message */
    char buffer[ECHOMAX];        /* Buffer for echo string */
    struct sigaction myAction;       /* For setting signal handler */
    unsigned short servPort;     /* Server port */
    int recvMsgSize;                 /* Size of received message */
    double lossRate;                 /* Loss rate */
    int packet_rcvd  = -1;      /* Last packet received */
    int bits_rcvd = 0;           /* Number of bits received */

    if (argc < 2 || argc > 3)         /* Test for correct number of parameters */
    {
        fprintf(stderr,"Usage:  %s <UDP SERVER PORT> [UDP Loss Rate]\n", argv[0]);
        exit(1);
    }

    servPort = atoi(argv[1]);  /* First arg:  local port */
    lossRate = (argc == 4) ? atof(argv[2]) : 0.0; /* Second arg (optional) : loss rate */

    srand48(1523); /* Seed for number generator */


    /* Set signal handler for alarm signal */
    myAction.sa_handler = CatchAlarm;
    if (sigfillset(&myAction.sa_mask) < 0) /* block everything in handler */
        DieWithError("sigfillset() failed");
    myAction.sa_flags = 0;

    if (sigaction(SIGALRM, &myAction, 0) < 0)
        DieWithError("sigaction() failed for SIGALRM");

    /* Create socket for sending/receiving datagrams */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    /* Construct local address structure */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Zero out structure */
    echoServAddr.sin_family = AF_INET;                /* Internet address family */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    echoServAddr.sin_port = htons(servPort);      /* Local port */

    /* Bind to the local address */
    if (bind(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("bind() failed");
  
    for (;;) /* Run forever */
    {
        /* Set the size of the in-out parameter */
        cliAddrLen = sizeof(echoClntAddr);
        UDPPacket packet;

        /* Block until receive message from a client */
        if ((recvMsgSize = recvfrom(sock, &packet, sizeof(packet), 0,
            (struct sockaddr *) &echoClntAddr, &cliAddrLen)) < 0)
            DieWithError("recvfrom() failed");
        packet.type = ntohl(packet.type);
        packet.length = ntohl(packet.length);
        packet.seqno = ntohl(packet.seqno);

        /* Check if tear down */
        if (packet.type == 4) {
            /* Tear down */
            UDPAck ack;
            ack.type = htonl(8);
            ack.ack_no = htonl(-1);
            if (sendto(sock, &ack, sizeof(ack), 0, (struct sockaddr*) &echoClntAddr,
                cliAddrLen) != sizeof(ack))
                DieWithError("Error sending tear down ack");

            // Wait to receive ack
            alarm(7);

            // Wait for 7 seconds or until receive ack
            while(1) {
                while (recvfrom(sock, &packet, sizeof(int)*3, 0,
                    (struct sockaddr *) &echoClntAddr, &cliAddrLen) < 0) {
                    if (errno == EINTR) { /* Never received ack - alarm signaled */
                        exit(0);
                    }
                }
                int type = ntohl(packet.type);
                if (type == 4) {
                    if (sendto(sock, &ack, sizeof(ack), 0, (struct sockaddr*) &echoClntAddr,
                        cliAddrLen) != sizeof(ack))
                        DieWithError("Error sending tear down ack");
                }
            }
        } else {
            if (is_lost(lossRate)) continue;

            if (packet.seqno == packet_rcvd + 1) {
                packet_rcvd++;
                memcpy(&buffer[bits_rcvd], packet.data, packet.length);
                bits_rcvd += packet.length;
            }

            UDPAck ack;
            ack.type = htonl(2);
            ack.ack_no = htonl(packet_rcvd);

            if (sendto(sock, &ack, sizeof(ack), 0, (struct sockaddr*) &echoClntAddr,
                cliAddrLen) != sizeof(ack))
                DieWithError("sendto() sent a different number of bytes than expected");
        }
    }
    /* NOT REACHED */
}

int is_lost(float loss_rate) {
    double rv;
    rv = drand48();
    if (rv < loss_rate) return 1;
    else return 0;
}

void CatchAlarm(int ignored)     /* Handler for SIGALRM */
{
    exit(0);
}
