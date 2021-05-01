
#include <iostream>
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <string>
#include "nlohmann/json.hpp"
#include "requests.h"

using namespace std;
using json = nlohmann::json;

#define MAXBUFF 4096
#define LEN 50
bool checkResponse(string response) {
  if (response.find("error") != std::string::npos) {
    response.erase(0, response.find("error") + 8);
    response.erase(response.find("\"}"), response.find("}"));
    cout << response << endl;
    ;
    return false;
  }
  return true;
}
void printBookData(string data) {
  data.erase(0, data.find("[{"));
  auto book = json::parse(data.c_str());
  cout << "### " << book[0]["title"] << " ###" << endl;
  cout << "author -> " << book[0]["author"] << endl;
  cout << "publisher -> " << book[0]["publisher"] << endl;
  cout << "page_count -> " << book[0]["page_count"] << endl;
}
void printLibrary(string response) {
  string list;
  list = response;
  list.erase(0, list.find("["));
  if (list == "[]") {
    cout << "Your list is empty!";
    return;
  }
  cout << "Your list : " << endl;

  auto datas = json::parse(list.c_str());

  for (int i = 0; i < datas.size(); i++) {
    cout << "### " << datas[i]["id"] << " - " << datas[i]["title"] << " ###"
         << endl;
  }
}
string getResponse(int tcp_socket){
  char buffer[MAXBUFF];
  string response;
  int n;
  memset(buffer, 0, MAXBUFF);
      
      while (strstr(buffer, "\r\n\r\n") == nullptr) {
       
        memset(buffer, 0, MAXBUFF);
        n = recv(tcp_socket, buffer, sizeof(buffer), 0);
        if (n < 0) perror("recv in enter_library failed!");
        response += buffer;
        
      }
      
      char *p = strstr(buffer, "Content-Length:");
      char *pEnd;
      long size = strtol(p + 16, &pEnd, 10);

      size = strlen(strstr(buffer, "\r\n\r\n")) - size - 2;
      if(size == 0) return response;
      while(size > 0){
        memset(buffer, 0, MAXBUFF);
        n = recv(tcp_socket, buffer, sizeof(buffer), 0);
        if (n < 0) perror("recv in enter_library failed!");
        if(n <= 0) return response;
        response += buffer;
        size -= n;
      }
      return response;
}

int main(int argc, char *argv[]) {
  string message;
  string username;
  string password;
  string response;
  int tcp_socket, n;
  int portno = 8080;
  char buffer[MAXBUFF];
  struct sockaddr_in tcp_serv_addr;
  socklen_t socket_len = sizeof(struct sockaddr_in);

  tcp_socket = socket(PF_INET, SOCK_STREAM, 0);
  if (tcp_socket < 0) perror("Socket tcp failed!");

  memset((char *)&tcp_serv_addr, 0, sizeof(tcp_serv_addr));
  tcp_serv_addr.sin_family = AF_INET;
  tcp_serv_addr.sin_port = htons(portno);
  inet_aton("3.8.116.10", &tcp_serv_addr.sin_addr);
  string token;
  string cookiel;
  char command[LEN];
  int conn = connect(tcp_socket, (struct sockaddr *)&tcp_serv_addr, socket_len);
  if (conn < 0) perror("Connect failed!");
  while (1) {
    memset(buffer, 0, MAXBUFF);
    n = recv(tcp_socket, buffer, 1, MSG_DONTWAIT | MSG_PEEK);
    if (sizeof(buffer) != 1) {
      tcp_socket = socket(PF_INET, SOCK_STREAM, 0);
      int conn =
          connect(tcp_socket, (struct sockaddr *)&tcp_serv_addr, socket_len);
      if (conn < 0) perror("Connect failed!");
    }
    response.clear();
    username.clear();
    password.clear();
    memset(command, 0, LEN);
    fgets(command, LEN, stdin);
    if (strcmp(command, "exit\n") == 0) break;
    if (strcmp(command, "register\n") == 0) {
      cout<<"username=";
      getline(cin, username);
      cout<<"password=";
      getline(cin, password);
      username = "{\"username\": \"" + username + "\", ";
      password = "\"password\": \"" + password + "\"}";
      vector<string> info;
      vector<string> cookies;
      string token;
      info.push_back(username);
      info.push_back(password);
      message =
          compute_post_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com",
                               "/api/v1/tema/auth/register", "application/json",
                               info, 2, cookies, 0, token);

      send_to_server(tcp_socket, message.c_str());
      memset(buffer, 0, MAXBUFF);
      n = recv(tcp_socket, buffer, sizeof(buffer), 0);
      response = buffer;
      if (n < 0) perror("recv in register failed!");
      if (!checkResponse(response)) continue;
      cout << "Registration successful" << endl;
      continue;
    } else if (strcmp(command, "login\n") == 0) {
      cout<<"username=";
      getline(cin, username);
      cout<<"password=";
      getline(cin, password);

      username = "{\"username\": \"" + username + "\", ";
      password = "\"password\": \"" + password + "\"}";
      vector<string> info;
      vector<string> cookies;

      info.push_back(username);
      info.push_back(password);
      message =
          compute_post_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com",
                               "/api/v1/tema/auth/login", "application/json",
                               info, 2, cookies, 0, token);
      send_to_server(tcp_socket, message.c_str());

      memset(buffer, 0, MAXBUFF);
      n = recv(tcp_socket, buffer, sizeof(buffer), 0);
      response = buffer;
      if (n < 0) perror("recv in login failed!");
      if (!checkResponse(response)) continue;
      cookiel = response;
      cookiel.erase(0, response.find("Cookie") + 8);
      cookiel.erase(cookiel.begin() + cookiel.find("Path") - 2, cookiel.end());
      cout << "Welcome!" << endl;
      memset(command, 0, LEN);
      continue;
    } else if (strcmp(command, "enter_library\n") == 0) {
      vector<string> cookies;
      cout<<"Please wait 5 seconds.."<<endl;
      if (cookiel.size() != 0) cookies.push_back(cookiel);
      message = compute_get_request(
          "ec2-3-8-116-10.eu-west-2.compute.amazonaws.com",
          "/api/v1/tema/library/access", "", cookies, cookies.size(), token);
      send_to_server(tcp_socket, message.c_str());

      memset(buffer, 0, MAXBUFF);
      n = recv(tcp_socket, buffer, sizeof(buffer), 0);
      while(n){
        response+=buffer;
        memset(buffer, 0, MAXBUFF);
        n = recv(tcp_socket, buffer, sizeof(buffer), 0);
        if(n < 0) perror("recv in get_books failed");
      }
      if (!checkResponse(response)) continue;
      token = response;
      token.erase(0, token.find("token") - 2);
      auto j1 = json::parse(token);
      token = j1.at("token");
      cout << "Welcome to our library!" << endl;
      continue;
    } else if (strcmp(command, "get_books\n") == 0) {
      vector<string> cookies;
      cout<<"Please wait 5 seconds.."<<endl;
      message = compute_get_request(
          "ec2-3-8-116-10.eu-west-2.compute.amazonaws.com",
          "/api/v1/tema/library/books", "", cookies, cookies.size(), token);
      send_to_server(tcp_socket, message.c_str());
      memset(buffer, 0, MAXBUFF);
      n = recv(tcp_socket, buffer, sizeof(buffer), 0);
      while(n){
        response+=buffer;
        memset(buffer, 0, MAXBUFF);
        n = recv(tcp_socket, buffer, sizeof(buffer), 0);
        if(n < 0) perror("recv in get_books failed");
      }
      
      if (!checkResponse(response)) continue;
      printLibrary(response);
      continue;
    } else if (strcmp(command, "add_book\n") == 0) {
      vector<string> info;
      string aux;
      cout<<"title=";
      getline(cin, aux);
      aux = "{\"title\": \"" + aux + "\", ";
      info.push_back(aux);
      cout<<"author=";
      getline(cin, aux);
      aux = "\"author\": \"" + aux + "\", ";
      info.push_back(aux);
      cout<<"genre=";
      getline(cin, aux);
      aux = "\"genre\": \"" + aux + "\", ";
      info.push_back(aux);
      cout<<"page_count=";
      getline(cin, aux);
      aux = "\"page_count\": \"" + aux + "\", ";
      info.push_back(aux);
      cout<<"publisher=";
      getline(cin, aux);
      aux = "\"publisher\": \"" + aux + "\"}";
      info.push_back(aux);
      vector<string> cookies;

      message =
          compute_post_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com",
                               "/api/v1/tema/library/books", "application/json",
                               info, 5, cookies, 0, token);
      send_to_server(tcp_socket, message.c_str());
      memset(buffer, 0, MAXBUFF);
      n = recv(tcp_socket, buffer, sizeof(buffer), 0);
      if (n < 0) perror("recv in add_book failed!");

      response = buffer;
      if (!checkResponse(response)) continue;
      cout << "Book added!" << endl;
      continue;
    } else if (strcmp(command, "delete_book\n") == 0) {
      string id;
      cout<<"id=";
      getline(cin, id);
      string url = "/api/v1/tema/library/books/" + id;
      vector<string> cookies;
      message = compute_delete_request(
          "ec2-3-8-116-10.eu-west-2.compute.amazonaws.com", url,
          "application/json", cookies, cookies.size(), token);

      send_to_server(tcp_socket, message.c_str());

      memset(buffer, 0, MAXBUFF);
      n = recv(tcp_socket, buffer, sizeof(buffer), 0);

      response = buffer;
      if (!checkResponse(response)) continue;
      cout << "Book deleted!" << endl;

    } else if (strcmp(command, "logout\n") == 0) {
      vector<string> cookies;
      if (cookiel.size() != 0) cookies.push_back(cookiel);
      token.clear();
      message = compute_get_request(
          "ec2-3-8-116-10.eu-west-2.compute.amazonaws.com",
          "/api/v1/tema/auth/logout", "", cookies, cookies.size(), token);
      send_to_server(tcp_socket, message.c_str());

      memset(buffer, 0, MAXBUFF);
      n = recv(tcp_socket, buffer, sizeof(buffer), 0);
      response = buffer;
      if (!checkResponse(response)) continue;
      cout << "You successfully logged out!" << endl;
      continue;
    } else if (strcmp(command, "get_book\n") == 0) {
      string id;
      cout<<"id=";
      getline(cin, id);
      cout<<"Please wait 5 seconds.."<<endl;
      string url = "/api/v1/tema/library/books/" + id;
      vector<string> cookies;
      message =
          compute_get_request("ec2-3-8-116-10.eu-west-2.compute.amazonaws.com",
                              url, "", cookies, cookies.size(), token);
      send_to_server(tcp_socket, message.c_str());
      memset(buffer, 0, MAXBUFF);
      n = recv(tcp_socket, buffer, sizeof(buffer), 0);
      while(n){
        response+=buffer;
        memset(buffer, 0, MAXBUFF);
        n = recv(tcp_socket, buffer, sizeof(buffer), 0);
        if(n < 0) perror("recv in get_books failed");
      }
      if (!checkResponse(response)) continue;
      printBookData(response);
      continue;

    } else if (strlen(command) != 0 && strcmp(command, "\n") != 0) {
      cout << "Unknown command: " << command << " !" << endl;
      continue;
    }
  }

  close(tcp_socket);

  return 0;
}