#include<iostream>
#include <string.h>
#include <vector>
#include <unordered_map>
#include <algorithm>
using namespace std;

#define TRUE 1
#define FALSE 0
#define BUFLEN 1600

struct udp_message {
	char  topic[50];
	uint8_t  data_type;
	char content[1501];
};
struct from_subscriber {
	char act;
	char  topic[50];
	int sf;
};
struct tcp_message {
	char ip[13];
	int port;
	char  topic[50];
	char  data_type[20];
	char content[1501];
};
struct client {
	char id[17];
	char* ip;
	int port;
	int fd;
};