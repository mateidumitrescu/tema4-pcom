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
#define ROUTE_LOGOUT "/api/v1/tema/auth/logout"
#define PAYLOAD_TYPE "application/json"


#define MAX_LEN 125
#define GET_COMMAND 1
#define NO_COOKIE ""
#define TRUE 1
#define FALSE 0

using namespace std;

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

/*function to delete a book from library*/
void delete_book(std::string &token, int &library_in);

/*function to logout user from server*/
void logout(std::string &cookie, std::string &token,
            int &authenticated, int &library_in);

int main() {
    char command[MAX_LEN];
    int authenticated;
    int library_in;

    authenticated = FALSE;
    library_in = FALSE;

    std::string cookie;
    std::string token;

    while (GET_COMMAND) {
        fgets(command, MAX_LEN, stdin);
        command[strlen(command) - 1] = '\0';
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
        } else if (strcmp(command, "delete_book") == 0) {
            delete_book(token, library_in);
        } else if (strcmp(command, "logout") == 0) {
            logout(cookie, token, authenticated, library_in);
        } else if (strcmp(command, "exit") == 0) {
            return 0;
        } else {
            std::cout << "Command is unknown, try other command.\n";
        }
    }

    return 0;
}

void start_register() {
    char username[MAX_LEN];
    char password[MAX_LEN];

    std::cout << "username=";
    fgets(username, MAX_LEN, stdin);

    std::cout << "password=";
    fgets(password, MAX_LEN, stdin);

    for (long unsigned int i = 0; i < strlen(username); i++) {
        if (username[i] == ' ') {
            std::cout << "Username and password should not contain spaces.\n";
            return;
        }
    }

    for (long unsigned int i = 0; i < strlen(password); i++) {
        if (password[i] == ' ') {
            std::cout << "Username and password should not contain spaces.\n";
            return;
        }
    }

    JSON jmsg;
    jmsg["username"] = username;
    jmsg["password"] = password;

    int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);

    char *message = NULL;
    std::string info = jmsg.dump();
    char *infos = new char[info.size() + 1];
    char *credentials[1];

    std::strcpy(infos, info.c_str());
    credentials[0] = new char[std::strlen(infos) + 1];
    std::strcpy(credentials[0], infos);

    message = credentials[0];

    message = compute_post_request((char *)HOST, (char *)ROUTE_REGISTER,
                                   (char *)PAYLOAD_TYPE,
                                   credentials, 1, NULL, 0);
    
    delete[] infos;
    delete[] credentials[0];

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

    close(sockfd);
    std::cout << std::flush;
}

void start_login(int &authenticated, std::string &cookie,
                std::string &token, int &library_in) {
    char username[MAX_LEN];
    char password[MAX_LEN];

    std::cout << "username=";
    fgets(username, MAX_LEN, stdin);

    std::cout << "password=";
    fgets(password, MAX_LEN, stdin);

    for (long unsigned int i = 0; i < strlen(username); i++) {
        if (username[i] == ' ') {
            std::cout << "Username and password should not contain spaces.\n";
            return;
        }
    }

    for (long unsigned int i = 0; i < strlen(password); i++) {
        if (password[i] == ' ') {
            std::cout << "Username and password should not contain spaces.\n";
            return;
        }
    }

    JSON jmsg;
    jmsg["username"] = username;
    jmsg["password"] = password;

    int sockfd = open_connection((char *)HOST, PORT, AF_INET, SOCK_STREAM, 0);
    char *message = NULL;
    std::string info = jmsg.dump();
    char *infos = new char[info.size() + 1];
    char *credentials[1];

    std::strcpy(infos, info.c_str());
    credentials[0] = new char[std::strlen(infos) + 1];
    std::strcpy(credentials[0], infos);

    message = credentials[0];
    message = compute_post_request((char *)HOST, (char *)ROUTE_LOGIN,
                                   (char *)PAYLOAD_TYPE,
                                   credentials, 1, NULL, 0);
    
    delete[] infos;

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
    std::cout << std::flush;
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
    std::cout << std::flush;
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
        std::cout << "\n";

    } else if (!library_in) {
        std::cout << "Library not accessed yet. Enter library first.\n";
    } else if (resp == "403") {
        std::cout << "Missing authorization header.\n";
    }
    close(sockfd);
    std::cout << std::flush;
}

void get_book(std::string &token, int &library_in) {
    char id[MAX_LEN];
    char route[MAX_LEN];
    strcpy(route, ROUTE_BOOK_ACCESS);
    std::cout << "id=";
    fgets(id, MAX_LEN, stdin);

    id[strlen(id) - 1] = '\0';


    /*getting full route of book*/
    strcat(route, id);
    int sockfd = open_connection((char *)HOST, PORT,
                                 AF_INET, SOCK_STREAM, 0);

    char **TOKEN = new char*[1];
    TOKEN[0] = new char[token.length() + 1];
    std::strcpy(TOKEN[0], token.c_str());

    char *message = compute_get_request_aux((char *)HOST,
                                        route,
                                        NULL, TOKEN, 2);

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
    std::cout << std::flush;

}

void add_book(std::string &token, int &library_in) {
    std::cout << "title=";
    char title[MAX_LEN];
    fgets(title, MAX_LEN, stdin);

    std::cout << "author=";
    char author[MAX_LEN];
    fgets(author, MAX_LEN, stdin);

	std::cout << "genre=";
    char genre[MAX_LEN];
    fgets(genre, MAX_LEN, stdin);

	std::cout << "publisher=";
    char publisher[MAX_LEN];
    fgets(publisher, MAX_LEN, stdin);

    std::cout << "page_count=";
    char page_count[MAX_LEN];
    fgets(page_count, MAX_LEN, stdin);

    if (strlen(title) == 1 ||
        strlen(author) == 1 ||
        strlen(genre) == 1 ||
        strlen(publisher) == 1 ||
        strlen(page_count) == 1) {
            std::cout << "All fields must be completed to add book.\n";
            return;
        }

    title[strlen(title) - 1] = '\0';
    author[strlen(author) - 1] = '\0';
    genre[strlen(genre) - 1] = '\0';
    publisher[strlen(publisher) - 1] = '\0';
    page_count[strlen(page_count) - 1] = '\0';


    int valid_page_count = TRUE;
    if (page_count[0] == '-') {
        valid_page_count = FALSE;
    }

    
    for (long unsigned int i = 0; i < strlen(page_count); i++) {
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
    string title_string = string(title);

    jmsg["publisher"] = publisher;
    jmsg["author"] = author;
    jmsg["genre"] = genre;
    jmsg["page_count"] = page_count;
    jmsg["title"] = title_string;

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
    std::cout << std::flush;
}

void delete_book(std::string &token, int &library_in) {
    char id[MAX_LEN];
    char route[MAX_LEN];
    strcpy(route, ROUTE_BOOK_ACCESS);
    std::cout << "id=";
    fgets(id, MAX_LEN, stdin);

    id[strlen(id) - 1] = '\0';


    /*getting full route of book*/
    strcat(route, id);

    int sockfd;
    sockfd = open_connection((char *)HOST, PORT,
                                 AF_INET, SOCK_STREAM, 0);
    char *message;
    message = compute_delete_request((char *)HOST, route,
                                     NULL, token, 2);

    send_to_server(sockfd, message);
    std::string response;
    std::string resp;

    response = receive_from_server(sockfd);
    resp = response.substr(9, 3);

    if (resp == "200") {
        std:: cout << "Book was deleted.\n";
    } else if (!library_in) {
        std:: cout << "Library not accessed yet. Enter library first.\n";
    } else if (resp == "403") {
        std:: cout << "Missing authorization header.\n";
    } else if (resp == "404") {
        std:: cout << "No book with id " << id << " was found.\n";
    }
    close(sockfd);
    std::cout << std::flush;
}

void logout(std::string &cookie, std::string &token,
            int &authenticated, int &library_in) {
    int sockfd;
    char *message;
    sockfd = open_connection((char *)HOST, PORT,
                            AF_INET, SOCK_STREAM, 0);
    
    /*converting std::string to char** */
    char **cookies = new char*[1];
    cookies[0] = new char[cookie.length() + 1];
    std::strcpy(cookies[0], cookie.c_str());

    message = compute_get_request((char *)HOST, (char *)ROUTE_LOGOUT,
                                   NULL, cookies, 1);

    delete[] cookies[0];
    delete[] cookies;

    send_to_server(sockfd, message);
    std::string response;
    std::string resp;

    response = receive_from_server(sockfd);
    resp = response.substr(9, 3);

    if (resp == "200") {
        authenticated = FALSE;
        library_in = FALSE;
        cookie.clear();
        token.clear();
        std::cout << "Succesfully logged out from server.\n";
    } else if (!authenticated) {
        std::cout << "No user is logged in.\n";
    } else if (resp == "403") {
        std:: cout << "Missing authorization header.\n";
    } else if (resp == "429") {
        std:: cout << "Too many requests, try again later.\n";
    }
    close(sockfd);
    std::cout << std::flush;
}