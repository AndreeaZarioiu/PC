#include <iostream>
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <string>
#include <vector>

using namespace std;
#ifndef _REQUESTS_
#define _REQUESTS_

string compute_get_request(string host, string url, string query_params,
                           vector<string> cookies, int cookies_count,
                           string token);

string compute_post_request(string host, string url, string content_type,
                            vector<string> body_data,
                            int body_data_fields_count, vector<string> cookies,
                            int cookies_count, string token);
string compute_delete_request(string host, string url, string content_type,
                              vector<string> cookies, int cookies_count,
                              string token);
void send_to_server(int sockfd, const char *message);
#endif
