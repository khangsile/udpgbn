CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lpthread

OBJS = DieWithError.o UDPEchoClient-Timeout.o                       

all: tcpserver

tcpserver: $(OBJS)
	$(CC) $(OBJS) -o udpclient $(LDFLAGS)

#TCPEchoServer.o: UDPEchoServer.c
#        $(CC) $(CFLAGS) -c UDPEchoServer.c

HandleTCPClient.o: UDPEchoClient-Timeout.c                                                 
	$(CC) $(CFLAGS) -c UDPEchoClient-Timeout.c                                          
DieWithError.o: DieWithError.c
	$(CC) $(CFLAGS) -c DieWithError.c

clean:
	rm -f *~ *.o udpclient core