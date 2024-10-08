#include "requests.h"
#include <iostream>
#include <string>
#include "helpers.h"
#include <arpa/inet.h>
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <stdio.h>
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <unistd.h>     /* read, write, close */

char *compute_get_request(char *host, char *url, char *query_params,
                          char **cookies, int cookies_count) {
  char *message = (char *) calloc(BUFLEN, sizeof(char));
  char *line = (char *) calloc(LINELEN, sizeof(char));

  // Step 1: write the method name, URL, request params (if any) and protocol
  // type
  if (query_params != NULL) {
    sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
  } else {
    sprintf(line, "GET %s HTTP/1.1", url);
  }

  compute_message(message, line);

  // Step 2: add the host
  sprintf(line, "Host: %s", host);
  compute_message(message, line);

  // Step 3 (optional): add headers and/or cookies, according to the protocol
  // format
  if (cookies != NULL) {
    memset(line, 0, LINELEN);
    strcat(line, "Cookie: ");
    for (int i = 0; i < cookies_count - 1; i++) {
      strcat(line, cookies[i]);
      strcat(line, "; ");
    }

    strcat(line, cookies[cookies_count - 1]);
    compute_message(message, line);
  }

  // Step 4: add final new line
  compute_message(message, "");
  free(line);
  return message;
}

char *compute_post_request(char *host, char *url, char *content_type,
                           char **body_data, int body_data_fields_count,
                           char **cookies, int cookies_count) {
  char *message = (char *)calloc(BUFLEN, sizeof(char));
  char *line = (char *)calloc(LINELEN, sizeof(char));
  char *body_data_buffer = (char *)calloc(LINELEN, sizeof(char));

  // Step 1: write the method name, URL and protocol type
  sprintf(line, "POST %s HTTP/1.1", url);
  compute_message(message, line);

  // Step 2: add the host
  sprintf(line, "Host: %s", host);
  compute_message(message, line);

  memset(body_data_buffer, 0, LINELEN);
  for (int i = 0; i < body_data_fields_count; ++i) {
    strcat(body_data_buffer, body_data[i]);
    if (i != body_data_fields_count - 1) {
      strcat(body_data_buffer, "&");
    }
  }

  sprintf(line, "Content-Type: %s", content_type);
  compute_message(message, line);
  
  sprintf(line, "Content-Length: %lu", strlen(body_data_buffer));
  compute_message(message, line);


  /* Step 3: add necessary headers (Content-Type and Content-Length are
     mandatory) in order to write Content-Length you must first compute the
     message size
  */
  // Step 4 (optional): add cookies
  if (cookies != NULL) {
    memset(line, 0, LINELEN);
    strcat(line, "Cookie: ");
    for (int i = 0; i < cookies_count - 1; i++) {
      strcat(line, cookies[i]);
      strcat(line, ";");
    }

    strcat(line, cookies[cookies_count - 1]);
    compute_message(message, line);
  }

  // Step 5: add new line at end of header
  compute_message(message, "");

  // Step 6: add the actual payload data
  memset(line, 0, LINELEN);
  strcat(message, body_data_buffer);

  free(line);
  free(body_data_buffer);
  return message;
}

char* compute_get_request_aux(const char* host, const char* url, const char* query_params,
                            char** cookies, int cookie_type)
{
    char* message = (char*) calloc(BUFLEN, sizeof(char));
    char* line = (char*) calloc(LINELEN, sizeof(char));

    if (query_params != nullptr) {
        snprintf(line, LINELEN, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        snprintf(line, LINELEN, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    snprintf(line, LINELEN, "HOST: %s", host);
    compute_message(message, line);

    if (cookies != nullptr) {
        if (cookie_type == 1) {
            // we have cookie
            snprintf(line, LINELEN, "Cookie: %s;", *cookies);
        } else {
            // we have token
            snprintf(line, LINELEN, "Authorization: Bearer %s", *cookies);
        }

        compute_message(message, line);
    }

    compute_message(message, "");
    return message;
}

char *compute_post_request_aux(char *host, char *url, char* content_type, std::string body_data,
                            std::string cookies)
{
    char* message = new char[BUFLEN];
    char* line = new char[LINELEN];

    sprintf(line, "POST %s HTTP/1.1\r\n", url);
    strcat(message, line);

    sprintf(line, "HOST: %s\r\n", host);
    strcat(message, line);

    if (!cookies.empty()) {
        sprintf(line, "Authorization: Bearer %s\r\n", cookies.c_str());
        strcat(message, line);
    }

    std::string content = "Content-Type: ";
    content += content_type;
    sprintf(line, "%s\r\n", content.c_str());
    strcat(message, line);

    sprintf(line, "Content-Length: %zu\r\n", body_data.size());
    strcat(message, line);

    strcat(message, "\r\n");
    strcat(message, body_data.c_str());

    delete[] line;
    return message;
}

char *compute_delete_request(char *host, char *url, char *query_params,
                            std::string cookies, int cookie_type)
{
    char* message = (char*)calloc(BUFLEN, sizeof(char));
    char* line = (char*)calloc(LINELEN, sizeof(char));

    if (query_params != NULL) {
        snprintf(line, LINELEN, "DELETE %s?%s HTTP/1.1", url, query_params);
    }
    else {
        snprintf(line, LINELEN, "DELETE %s HTTP/1.1", url);
    }
    compute_message(message, line);

    snprintf(line, LINELEN, "HOST: %s", host);
    compute_message(message, line);

    if (!cookies.empty()) {
        if (cookie_type == 1) {
            snprintf(line, LINELEN, "Cookie: %s;", cookies.c_str());
        }
        else {
            snprintf(line, LINELEN, "Authorization: Bearer %s", cookies.c_str());
        }
        compute_message(message, line);
    }

    compute_message(message, "");

    return message;
}

