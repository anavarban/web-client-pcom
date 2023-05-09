#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

char *compute_get_request(char *host, char *url, char *query_params, char *auth_tok,
                            char **cookies, int cookies_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    strcpy(line, "Host: ");
    strcat(line, host);
    compute_message(message, line);

    if (auth_tok != NULL) {
        sprintf(line, "Authorization: Bearer %s", auth_tok);
        compute_message(message, line);
    }

    // Step 3 (optional): add headers and/or cookies, according to the protocol format
    if (cookies != NULL) {
        strcpy(line, "Cookie: ");
       for (int i = 0; i < cookies_count; i++) {
           strcat(line, cookies[i]);
           if (i != cookies_count - 1) {
               strcat(line, "; ");
           }
       }
       compute_message(message, line);
    }
    // Step 4: add final new line
    compute_message(message, "");
    return message;
}

char *compute_post_request(char *host, char *url, char* content_type, char *body_data_json_str,
                            char **cookies, int cookies_count, char* auth_tok)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body_data_buffer = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    // Step 2: add the host
    strcpy(line, "Host: ");
    strcat(line, host);
    compute_message(message, line);
    
    
    if (auth_tok != NULL) {
        sprintf(line, "Authorization: Bearer %s", auth_tok);
        compute_message(message, line);
    }

    if (content_type != NULL) {
        strcpy(line, "Content-Type: ");
        if (!strcmp(content_type, "application/json")){
            strcat(line, content_type);
            compute_message(message, line);
            // for(int i = 0; i < body_data_fields_count; i++) {
            //     strcat(body_data_buffer, body_data[i]);
            //     if ( i != body_data_fields_count - 1) {
            //         strcat(body_data_buffer, "&");
            //     }
            
            // }

            if(body_data_json_str) {
                strcpy(body_data_buffer, body_data_json_str);
               
            }

            int bodylen = strlen(body_data_json_str);
            sprintf(line, "Content-Length: %d", bodylen);
            compute_message(message, line);
        }
    }

    // Step 4 (optional): add cookies
    if (cookies != NULL) {
       strcpy(line, "Cookie: ");
       for (int i = 0; i < cookies_count; i++) {
           strcat(line, cookies[i]);
           if (i != cookies_count - 1) {
               strcat(line, "; ");
           }
       }
       compute_message(message, line);
    }
    // Step 5: add new line at end of header
    compute_message(message, "");
    // Step 6: add the actual payload data
    memset(line, 0, LINELEN);
    compute_message(message, body_data_buffer);

    free(line);
    return message;
}

char* compute_delete_request(char *host, char *url, char *query_params, char *auth_tok,
                            char **cookies, int cookies_count)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL) {
        sprintf(line, "DELETE %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "DELETE %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    strcpy(line, "Host: ");
    strcat(line, host);
    compute_message(message, line);

    if (auth_tok != NULL) {
        sprintf(line, "Authorization: Bearer %s", auth_tok);
        compute_message(message, line);
    }

    // Step 3 (optional): add headers and/or cookies, according to the protocol format
    if (cookies != NULL) {
        strcpy(line, "Cookie: ");
       for (int i = 0; i < cookies_count; i++) {
           strcat(line, cookies[i]);
           if (i != cookies_count - 1) {
               strcat(line, "; ");
           }
       }
       compute_message(message, line);
    }
    // Step 4: add final new line
    compute_message(message, "");
    return message;
}