#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string>

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
#define ROUTE_BOOK_ACCESS "/api/v1/tema/library/books/"
#define PAYLOAD_TYPE "application/json"

#define GET_COMMAND 1
#define NO_COOKIE ""
#define TRUE 1
#define FALSE 0

/*function to register*/
void start_register();

/*function to login*/
void start_login(int &authenticated, std::string &cookie,
                std::string &token, int &library_in);

/*function to enter library*/
void enter_library(int &authenticated, std::string &cookie,
                  std::string &token, int &library_in);

/*function to get books in library*/
void get_books(std::string &token, int &library_in);

/*function to get book in library*/
void get_book(std::string &token, int &library_in);

/*function to add a book in library*/
void add_book(std::string &token, int &library_in);

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
            start_login(authenticated, cookie, token, library_in);
        } else if (strcmp(command, "enter_library") == 0) {
            enter_library(authenticated, cookie, token, library_in);
        } else if (strcmp(command, "get_books") == 0) {
            get_books(token, library_in);
        } else if (strcmp(command, "get_book") == 0) {
            get_book(token, library_in);
        } else if (strcmp(command, "add_book") == 0) {
            add_book(token, library_in);
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
                std::string &token, int &library_in) {
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
        library_in = FALSE;
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

void get_books(std::string &token, int &library_in) {
    int sockfd = open_connection((char *)HOST, PORT,
                                 AF_INET, SOCK_STREAM, 0);
    char **TOKEN = new char*[1];
    TOKEN[0] = new char[token.length() + 1];
    std::strcpy(TOKEN[0], token.c_str());
    char *message = compute_get_request_aux((char *)HOST,
                                        (char *)ROUTE_BOOKS_ACCESS,
                                        NULL, TOKEN, 2);
    delete[] TOKEN[0];
    delete[] TOKEN;
    

    send_to_server(sockfd, message);
    std::string response = receive_from_server(sockfd);
    std::string resp = response.substr(9, 3);
    std::string books;
    if (resp == "200") {
        /*list of books*/
        int first, last;
        first = response.find("[");
        last = response.find("]") + 1;
        books = response.substr(first, last - first);
        std::cout << books;
    } else if (!library_in) {
        std::cout << "Library not accessed yet. Enter library first.\n";
    } else if (resp == "403") {
        std::cout << "Missing authorization header.\n";
    }
    std::cout << "\n";
    close(sockfd);
}

void get_book(std::string &token, int &library_in) {
    std::cout << "id=";
    std::string id;
    std::cin >> id;
    int sockfd = open_connection((char *)HOST, PORT,
                                 AF_INET, SOCK_STREAM, 0);
    /* getting complete route of book access*/
    std::string route = ROUTE_BOOK_ACCESS + id;

    char **TOKEN = new char*[1];
    TOKEN[0] = new char[token.length() + 1];
    std::strcpy(TOKEN[0], token.c_str());

    char **ROUTE = new char*[1];
    ROUTE[0] = new char[route.length() + 1];
    std::strcpy(ROUTE[0], route.c_str());

    char *message = compute_get_request_aux((char *)HOST,
                                        ROUTE[0],
                                        NULL, TOKEN, 2);
    delete[] ROUTE[0];
    delete[] ROUTE;
    delete[] TOKEN[0];
    delete[] TOKEN;

    send_to_server(sockfd, message);
    std::string response = receive_from_server(sockfd);
    std::string resp = response.substr(9, 3);
    std::string book;

    if (resp == "200") {
        int first;
        int last;
        first = response.find("{");
        last = response.find("}") + 1;
        book = response.substr(first, last - first);
        
        std:: cout << book << "\n";
    } else if (!library_in) {
        std:: cout << "Library not accessed yet. Enter library first.\n";
    } else if (resp == "403") {
        std:: cout << "Missing authorization header.\n";
    } else if (resp == "404") {
        std:: cout << "No book with " << id << " id was found.\n";
    }
    close(sockfd);

}

void add_book(std::string &token, int &library_in) {
    std::cout << "title=";
    std::string title;
    std::cin >> title;

    std::cout << "author=";
    std::string author;
    std::cin >> author;

	std::cout << "genre=";
    std::string genre;
    std::cin >> genre;

	std::cout << "publisher=";
    std::string publisher;
    std::cin >> publisher;

    std::cout << "page_count=";
    std::string page_count;
    std::cin >> page_count;

    int valid_page_count = TRUE;
    if (page_count[0] == '-') {
        valid_page_count = FALSE;
    }

    int size = page_count.length();
    for (int i = 0; i < size; i++) {
        if ((int) page_count[i] < 48 || (int) page_count[i] > 57) {
            valid_page_count = FALSE;
            break;
        }
    }

    if (!valid_page_count) {
        std::cout << "Page count should be an unsigned integer.\n";
        return;
    }
    JSON jmsg;
    jmsg["title"] = title;
    jmsg["author"] = author;
    jmsg["genre"] = genre;
    jmsg["publisher"] = publisher;
    jmsg["page_count"] = page_count;

    int sockfd = open_connection((char *)HOST, PORT,
                                 AF_INET, SOCK_STREAM, 0);

    std::string info = jmsg.dump();

    char *message = compute_post_request_aux((char*) HOST, (char*) ROUTE_BOOKS_ACCESS,
    			(char*) PAYLOAD_TYPE, info, token);


    send_to_server(sockfd, message);
    std::string response;
    std::string resp;

    response = receive_from_server(sockfd);
    resp = response.substr(9, 3);

    if (resp == "200") {
        std::cout << "Book " << title << " added to library.\n";
    }else if (!library_in) {
        std:: cout << "Library not accessed yet. Enter library first.\n";
    } else if (resp == "403") {
        std::cout << "Missing authorization header.\n";
    } else if (resp == "500") {
        std:: cout << "Token not decoded.\n";
    } else if (resp == "429") {
        std::cout << "Too many requests, try again.\n";
    }
    close(sockfd);
}