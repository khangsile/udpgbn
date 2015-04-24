struct UDPPacket {
  int type;
  int seqno;
  int length;
  char data[512];
};
