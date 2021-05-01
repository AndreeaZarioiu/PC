
#include "requests.h"

void send_to_server(int sockfd, const char *message) {
  int bytes, sent = 0;
  int total = strlen(message);

  do {
    bytes = write(sockfd, message + sent, total - sent);
    if (bytes < 0) {
      perror("ERROR writing message to socket");
    }

    if (bytes == 0) {
      break;
    }

    sent += bytes;
  } while (sent < total);
}

string compute_get_request(string host, string url, string query_params,
                           vector<string> cookies, int cookies_count,
                           string token) {
  string message;
  string line;

  if (query_params != "") {
    line = "GET " + url + "?" + query_params + " HTTP/1.1";
  } else {
    line = "GET " + url + " HTTP/1.1";
  }
  message = line + "\r\n";
  line = "Host: " + host;
  message += line + "\r\n";
  if (token.size() != 0) {
    line = "Authorization: Bearer " + token;
    message += line + "\r\n";
  }
  if (!cookies.empty()) {
    int i;
    for (i = 0; i < cookies_count; i++) {
      line = "Authorization: Bearer ";
      message += line + "\r\n";
      line = "Cookie: " + cookies[i];
      message += line + "\r\n";
    }
  }

  message += "\r\n";
  return message;
}

string compute_delete_request(string host, string url, string content_type,
                              vector<string> cookies, int cookies_count,
                              string token) {
  string message;
  string line;

  line = "DELETE " + url + " HTTP/1.1";
  message = line + "\r\n";
  if (token.size() != 0) {
    line = "Authorization: Bearer " + token;
    message += line + "\r\n";
  }
  line = "Accept: " + content_type;
  message += line + "\r\n";
  line = "Content-Type: " + content_type;
  message += line + "\r\n";
  line = "Host: " + host;
  message += line + "\r\n";

  if (!cookies.empty()) {
    int i;
    for (i = 0; i < cookies_count; i++) {
      line = "Cookie: " + cookies[i];
      message += line + "\r\n";
    }
  }

  message += "\r\n";
  return message;
}

string compute_post_request(string host, string url, string content_type,
                            vector<string> body_data,
                            int body_data_fields_count, vector<string> cookies,
                            int cookies_count, string token) {
  string message;
  string line;
  string body;

  line = "POST " + url + " HTTP/1.1";
  message = line + "\r\n";

  line = "HOST: " + host;
  message = message + line + "\r\n";
  line = "Content-Type: " + content_type + " ";
  message = message + line + "\r\n";
  for (int i = 0; i < body_data_fields_count; i++) body += body_data[i];
  line = "Content-Length: " + to_string(body.length());
  message = message + line + "\r\n";
  if (token.size() != 0) {
    line = "Authorization: Bearer " + token;
    message += line + "\r\n";
  }
  if (!cookies.empty()) {
    int i;
    for (i = 0; i < cookies_count; i++) {
      line = "Cookie: " + cookies[i];
      message = message + line + "\r\n";
    }
  }
  message += "\r\n";
  message += body + "\r\n";

  return message;
}