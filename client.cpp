#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "helpers.h"
#include "buffer.h"
#include "requests.h"

/*used for parsing JSON*/
#include "json.hpp"
using JSON = nlohmann::json;

#define HOST "34.254.242.81"
#define PORT 8080
#define ROUTE_REGISTER "/api/v1/tema/auth/register"
#define ROUTE_LOGIN "/api/v1/tema/auth/login"
#define ROUTE_LIBRARY_ACCESS "/api/v1/tema/library/access"
#define ROUTE_BOOKS_ACCESS "/api/v1/tema/library/books"
#define ROUTE_BOOK_ACCESS "/api/v1/tema/library/books/:bookId."
#define PAYLOAD_TYPE "application/json"

#define GET_COMMAND 1
#define NO_COOKIE ""
#define TRUE 1
#define FALSE 0

/*function to register*/
void start_register();

/*function to login*/
void start_login(int &authenticated, std::string &cookie,
                std::string &token);

/*function to enter library*/
void enter_library(int &authenticated, std::string &cookie,
                  std::string &token, int &library_in);


int main() {
    char command[105];
    int authenticated;
    int library_in;

    authenticated = FALSE;
    library_in = FALSE;

    std::string cookie;
    std::string token;

    while (GET_COMMAND) {
        std::cin >> command;
        if (strcmp(command, "exit") == 0) {
            break;
        } else if (strcmp(command, "register") == 0) {
            start_register();
        } else if (strcmp(command, "login") == 0) {
            start_login(authenticated, cookie, token);
        } else if (strcmp(command, "enter_library") == 0) {
            enter_library(authenticated, cookie, token, library_in);
        }

    }
}

void start_register() {
    std::string username;
    std::string password;

    std::cout << "username=";
    std::cin >> username;

    std::cout << "password=";
    std::cin >> password;

    JSON jmsg;
    jmsg["username"] = username;
    jmsg["password"] = password;

    int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);

    char *message;
    std::string info = jmsg.dump();
    char *infos = (char *) malloc(info.size() + 1);
    char *credentials[1];
    strcpy(infos, info.c_str());
    credentials[0] = (char *) malloc(strlen(infos) + 1);
    strcpy(credentials[0], infos);
    message = compute_post_request((char *)HOST, (char *)ROUTE_REGISTER,
                                   (char *)PAYLOAD_TYPE,
                                   credentials, 1, NULL, 0);
    
    send_to_server(sockfd, message);
    std::string response = receive_from_server(sockfd);
    std::string resp = response.substr(9, 3);

    if (resp == "429") {
        std::cout << "Too many requests, try again later.\n";
    } else if (resp == "400") {
        std::cout << "ID taken, try again.\n";
    } else if (resp == "201") {
        std::cout << "Register succesfully.\n";
    }

    free(infos);
    free(credentials[0]);
    close(sockfd);
}

void start_login(int &authenticated, std::string &cookie,
                std::string &token) {
    std::string username;
    std::string password;

    std::cout << "username=";
    std::cin >> username;

    std::cout << "password=";
    std::cin >> password;

    JSON jmsg;
    jmsg["username"] = username;
    jmsg["password"] = password;

    int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *message;
    std::string info = jmsg.dump();
    char *infos = (char *) malloc(info.size() + 1);
    char *credentials[1];
    strcpy(infos, info.c_str());
    credentials[0] = (char *) malloc(strlen(infos) + 1);
    strcpy(credentials[0], infos);
    message = compute_post_request((char *)HOST, (char *)ROUTE_LOGIN,
                                   (char *)PAYLOAD_TYPE,
                                   credentials, 1, NULL, 0);
    send_to_server(sockfd, message);
    std::string response = receive_from_server(sockfd);
    std::string resp = response.substr(9, 3);

    if (resp == "400") {
        std::cout << "Credentials are not good.\n";
    } else if (resp == "200") {
        std::cout << "Succesfully logged in.\n";

        /*getting cookie*/
        int first;
        int last;
        first = response.find("Set-Cookie: ") + 12;
        last = response.find("; Path=/;");
        cookie = response.substr(first, last - first);
        authenticated = TRUE;
        token.clear();
    }
    close(sockfd);
}

void enter_library(int &authenticated, std::string &cookie,
                  std::string &token, int &library_in) {
    int sockfd = open_connection((char *)HOST, PORT,
                                 AF_INET, SOCK_STREAM, 0);
    char *message;
    /*converting std::string to char** */
    char **cookies = new char*[1];
    cookies[0] = new char[cookie.length() + 1];
    std::strcpy(cookies[0], cookie.c_str());

    message = compute_get_request((char *)HOST, (char *)ROUTE_LIBRARY_ACCESS,
                                   NULL, cookies, 1);

    delete[] cookies[0];
    delete[] cookies;
    
    send_to_server(sockfd, message);
    std::string response = receive_from_server(sockfd);
    std::string resp = response.substr(9, 3);


    if (resp == "401" || !authenticated) {
        std::cout << "Unauthenticated or unauthorized to enter library.\n";
    } else if (resp == "200") {
        std::cout << "Succesfully entered library.\n";
        /*get token*/
        int first;
        int last;
        first = response.find("token") + 8;
        last = response.find("}") - 1;
        token = response.substr(first, last - first);
        library_in = TRUE;
    }
    close(sockfd);
}