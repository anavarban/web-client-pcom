#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

//function to extract the session cookie from the 
//server response
char* get_cookie(char* serv_rsp) {
    char* cookie = strstr(serv_rsp, "Cookie:");
    char* tok = strtok(cookie, " ");
    tok = strtok(NULL, ";");
    return tok;
}

//function to extract the JWT token
char* get_token(char* tok_to_parse) {
    char* tok = strtok(tok_to_parse, ":\"");
    tok = strtok(NULL, "\"");
    tok = strtok(NULL, "\"");
    tok = strtok(NULL, "\"");
    return tok;
}


int main(int argc, char *argv[])
{
    char *message;
    char *response;
    int sockfd;
    
    //allocate memory for command, session cookie and
    //jwt token strings
    char* cmd = calloc(LINELEN, sizeof(char));
    char* session_cookie = calloc(BUFLEN, sizeof(char));
    char* auth_token = calloc(BUFLEN, sizeof(char));
    //a loop to read commands from stdin 
    while(1) {
        //connect to the server before every command to make sure
        //the connection is not closed before exit command
        sockfd = open_connection(SERVERADDR, PORT, AF_INET, SOCK_STREAM, 0);
        //read current command from stdin
        printf("Enter a command:\n");
        fgets(cmd, LINELEN, stdin);
        //extract the command
        char* command = strtok(cmd, "\n");
        if (command == NULL) {
            printf("{\"error\":\"Enter a valid string!\"}\n");
            continue;
        }
        //check if exit command was read
        if (!strcmp(command, "exit")) {
            //close the connection and exit reading loop
            close(sockfd);
            break;
        }
        //check if register command was read
        if (!strcmp(command, "register")) {
            //guard for registering while logged in
            if (strcmp(session_cookie, "")) {
                printf("{\"error\":\"Cannot register while logged in!\"}\n");
            } else {
                char user[LINELEN];
                char passw[LINELEN];

                char* uname;
                char* pass;

                //get the username and password from stdin
                printf("username=");
                fgets(user, LINELEN, stdin);
                uname = strtok(user, "\n");
                if (uname == NULL) {
                    printf("{\"error\":\"Enter a valid string!\"}\n");
                    continue;
                }
                printf("password=");
                fgets(passw, LINELEN, stdin);
                pass = strtok(passw, "\n");
                if (pass == NULL) {
                    printf("{\"error\":\"Enter a valid string!\"}\n");
                    continue;
                }
                //check if there are spaces and print error message if true
                if (strchr(uname, ' ') != NULL || strchr(pass, ' ') != NULL) {
                    printf("{\"error\":\"Username and password cannot contain spaces!\"}\n");
                } else {
                    //build the json for payload data
                    JSON_Value *root_value = json_value_init_object();
                    JSON_Object *root_object = json_value_get_object(root_value);
                    char *json_content = NULL;
                    json_object_set_string(root_object, "username", uname);
                    json_object_set_string(root_object, "password", pass);
                    json_content = json_serialize_to_string_pretty(root_value);
                    //compute the request message
                    message = compute_post_request(CONNECTADDR, "/api/v1/tema/auth/register",
                            "application/json", json_content, NULL, 0, NULL);
                    //send it to the server
                    send_to_server(sockfd, message);
                    response = receive_from_server(sockfd);
                    //extract the information from the server response
                    char* rsp_info = basic_extract_json_response(response);
                    if (rsp_info == NULL) {
                        printf("OK - Register successful!\n");
                    } else {
                        printf("%s\n", rsp_info);
                    }
                    //free what was allocated in various functions
                    free(message);
                    free(response);
                    json_free_serialized_string(json_content);
                    json_value_free(root_value);
                }
            }
            
        }
        //check if login command was read
        else if (!strcmp(command, "login")) {
            //proceed similarly to register command
            //guard for logging in while already logged in
            if (strcmp(session_cookie, "")) {
                printf("{\"error\":\"Already logged in!\"}\n");
            } else {
                char user[LINELEN];
                char passw[LINELEN];

                char* uname;
                char* pass;

                printf("username=");
                fgets(user, LINELEN, stdin);
                uname = strtok(user, "\n");
                if (uname == NULL) {
                    printf("{\"error\":\"Enter a valid string!\"}\n");
                    continue;
                }
                printf("password=");
                fgets(passw, LINELEN, stdin);
                pass = strtok(passw, "\n");
                if (pass == NULL) {
                    printf("{\"error\":\"Enter a valid string!\"}\n");
                    continue;
                }
                //no need to check for spaces because there won't be
                //any registered accounts with spaces in their credentials
                //so any spaces in the username or password will
                //generate an "invalid credentials" error
                JSON_Value *root_value = json_value_init_object();
                JSON_Object *root_object = json_value_get_object(root_value);
                char *json_content = NULL;
                json_object_set_string(root_object, "username", uname);
                json_object_set_string(root_object, "password", pass);
                json_content = json_serialize_to_string_pretty(root_value);

                message = compute_post_request(CONNECTADDR, "/api/v1/tema/auth/login",
                        "application/json", json_content, NULL, 0, NULL);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);
                char* rsp_info = basic_extract_json_response(response);
                if (rsp_info == NULL) {
                    printf("OK - Login successful!\n");
                    //extract the session cookie
                    strcpy(session_cookie, get_cookie(response));
                    //also print it to make sure it's okay
                    printf("Current session cookie:\n%s\n", session_cookie);
                } else {
                    printf("%s\n", rsp_info);
                }

                free(message);
                free(response);
                json_free_serialized_string(json_content);
                json_value_free(root_value);

            }
                    }
        //check if enter_library command was read
        else if (!strcmp(command, "enter_library")) {
            //assign the session cookie in an array to
            //pass to compute_get_request function
            char** cookies = calloc(1, sizeof(char*));
            cookies[0] = session_cookie;
            //server knows to respond with "not logged in"
            //so there's no guard against it
            message = compute_get_request(CONNECTADDR, "/api/v1/tema/library/access", NULL,
                    NULL, cookies, 1);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            //extract the payload from the response
            char* rsp_info = basic_extract_json_response(response);
            char* tok = strstr(rsp_info, "{\"token\"");
            if (tok != NULL) {
                //extract the JWT token
                strcpy(auth_token, get_token(tok));
                printf("OK - Entered library!\n");
                //print it
                printf("Current JWT Token:\n%s\n", auth_token);
            } else {
                printf("%s\n", rsp_info);
            }
            free(message);
            free(response);
            free(cookies);
        }
        //check if get_books command was read
        else if (!strcmp(command, "get_books")) {
            char** cookies = calloc(1, sizeof(char*));
            cookies[0] = session_cookie;
            //preliminary guards
            //first for when user isn't logged in
            if (!strcmp(session_cookie, "")) {
                printf("{\"error\":\"No session cookie, please login!\"}\n");
            } else {
                //second for when user hasn't entered library
                if (!strcmp(auth_token, "")) {
                    printf("{\"error\":\"No JWT Token, please enter library before accessing books!\"}\n");
                } else {
                    message = compute_get_request(CONNECTADDR, "/api/v1/tema/library/books", NULL,
                        auth_token, cookies, 1);
                    send_to_server(sockfd, message);
                    response = receive_from_server(sockfd);
                    char* rsp_info = basic_extract_json_response(response);
                    if (rsp_info == NULL) {
                        //extracting the book list
                        char* book_list = strstr(response, "[");
                        printf("%s\n", book_list);
                    } else {
                        printf("%s\n", rsp_info);
                    }
                    free(message);
                    free(response);
                }
            }
            free(cookies);
        }
        //check if get_book command was read
        else if (!strcmp(command, "get_book")) {
            char* id_s = calloc(LINELEN, sizeof(char));
            //prompt for id
            printf("id=");
            fgets(id_s, LINELEN, stdin);
            char* id_str = strtok(id_s, "\n");
            if (id_str == NULL) {
                printf("{\"error\":\"Enter a valid string!\"}\n");
                continue;
            }
            //no guard for non-number id because 
            //server can respond with an adequate error message
            char** cookies = calloc(1, sizeof(char*));
            cookies[0] = session_cookie;
            //preliminary guards
            //for when user is not logged in
            if (!strcmp(session_cookie, "")) {
                printf("{\"error\":\"No session cookie, please login!\"}\n");
            } else {
                //for when user hasn't entered library
                if (!strcmp(auth_token, "")) {
                    printf("{\"error\":\"No JWT Token, please enter library before accessing a book!\"}\n");
                } else {
                    char* url = strdup("/api/v1/tema/library/books/");
                    strcat(url, id_str);
                    message = compute_get_request(CONNECTADDR, url, NULL, auth_token, 
                            cookies, 1);
                    send_to_server(sockfd, message);
                    response = receive_from_server(sockfd);
                    char* rsp_info = basic_extract_json_response(response);
                    printf("%s\n", rsp_info);
                    free(response);
                    free(message);
                    free(url);
                }
            }
            free(cookies);
            free(id_s);
        }
        //check if add_book command was read
        else if (!strcmp(command, "add_book")) {
            char* title_s = calloc(LINELEN, sizeof(char));
            char* author_s = calloc(LINELEN, sizeof(char));
            char* publisher_s = calloc(LINELEN, sizeof(char));
            char* genre_s = calloc(LINELEN, sizeof(char));
            char* pg_count_s = calloc(LINELEN, sizeof(char));
            //read the book info from stdin
            //along the lines check for void input
            printf("title=");
            fgets(title_s, LINELEN, stdin);
            char* title = strtok(title_s, "\n");
            if (title == NULL) {
                printf("{\"error\":\"Enter a valid string!\"}\n");
                continue;
            }
            printf("author=");
            fgets(author_s, LINELEN, stdin);
            char* author = strtok(author_s, "\n");
            if (author == NULL) {
                printf("{\"error\":\"Enter a valid string!\"}\n");
                continue;
            }
            printf("genre=");
            fgets(genre_s, LINELEN, stdin);
            char* genre = strtok(genre_s, "\n");
            if (genre == NULL) {
                printf("{\"error\":\"Enter a valid string!\"}\n");
                continue;
            }
            printf("publisher=");
            fgets(publisher_s, LINELEN, stdin);
            char* publisher = strtok(publisher_s, "\n");
            if (publisher == NULL) {
                printf("{\"error\":\"Enter a valid string!\"}\n");
                continue;
            }
            printf("page_count=");
            fgets(pg_count_s, LINELEN, stdin);
            char* pg_count_str = strtok(pg_count_s, "\n");
            if (pg_count_str == NULL) {
                printf("{\"error\":\"Enter a valid string!\"}\n");
                continue;
            }
            //extract the number of pages from the string that was read
            int pg_count = atoi(pg_count_str);
            //validate the input
            if(pg_count == 0) {
                printf("{\"error\":\"Page count is not a number!\"}\n");
            } else {
                JSON_Value *root_value = json_value_init_object();
                JSON_Object *root_object = json_value_get_object(root_value);
                char *json_content = NULL;
                json_object_set_string(root_object, "title", title);
                json_object_set_string(root_object, "author", author);
                json_object_set_string(root_object, "genre", genre);
                json_object_set_number(root_object, "page_count", pg_count);
                json_object_set_string(root_object, "publisher", publisher);
                json_content = json_serialize_to_string_pretty(root_value);

                char** cookies = calloc(1, sizeof(char*));
                cookies[0] = session_cookie;
                //preliminary guards
                if (!strcmp(session_cookie, "")) {
                    printf("{\"error\":\"No session cookie, please login!\"}\n");
                } else {
                    if (!strcmp(auth_token, "")) {
                        printf("{\"error\":\"No JWT Token, please enter library!\"}\n");
                    } else {
                        message = compute_post_request(CONNECTADDR, "/api/v1/tema/library/books", 
                                "application/json", json_content, cookies, 1, auth_token);
                        send_to_server(sockfd, message);
                        response = receive_from_server(sockfd);
                        char* rsp_info = basic_extract_json_response(response);
                        if (rsp_info == NULL) {
                            printf("OK - Book added!\n");
                        } else {
                            printf("%s\n", rsp_info);
                        }
                        free(message);
                        free(response);
                    }
                }
                json_free_serialized_string(json_content);
                json_value_free(root_value);
                free(cookies);
            }
            free(title_s);
            free(author_s);
            free(genre_s);
            free(publisher_s);
            free(pg_count_s);
        }
        //check if delete_book command was read
        else if (!strcmp(command, "delete_book")) {
            char* id_s = calloc(LINELEN, sizeof(char));
            //read the id from stdin
            printf("id=");
            fgets(id_s, LINELEN, stdin);
            char* id_str = strtok(id_s, "\n");
            if (id_str == NULL) {
                printf("{\"error\":\"Enter a valid string!\"}\n");
                continue;
            }
            if (atoi(id_str) == 0) {
                printf("{\"error\":\"Id is not a number!\"}\n");
                continue;
            }
            char** cookies = calloc(1, sizeof(char*));
            cookies[0] = session_cookie;
            //preliminary guards
            if (!strcmp(session_cookie, "")) {
                printf("{\"error\":\"No session cookie, please login!\"}\n");
            } else {
                if (!strcmp(auth_token, "")) {
                    printf("{\"error\":\"No JWT Token, please enter library before deleteing a book!\"}\n");
                } else {
                    //build the url with the id
                    char* url = strdup("/api/v1/tema/library/books/");
                    strcat(url, id_str);
                    message = compute_delete_request(CONNECTADDR, url, NULL, auth_token, 
                            cookies, 1);
                    send_to_server(sockfd, message);
                    response = receive_from_server(sockfd);
                    char* rsp_info = basic_extract_json_response(response);
                    if (rsp_info == NULL) {
                        printf("OK - Book deleted!\n");
                    } else {
                        printf("%s\n", rsp_info);
                    }
                    free(url);
                    free(response);
                    free(message);
                }
            }
            free(cookies);
            free(id_s);
        }
        //check if logout command was read
        else if (!strcmp(command, "logout")) {
            char** cookies = calloc(1, sizeof(char*));
            cookies[0] = session_cookie;
            //preliminary guards
            if (!strcmp(session_cookie, "")) {
                printf("{\"error\":\"No session cookie, please login!\"}\n");
            } else {
                message = compute_get_request(CONNECTADDR, "/api/v1/tema/auth/logout", NULL,
                    auth_token, cookies, 1);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);
                char* rsp_info = basic_extract_json_response(response);
                if (rsp_info == NULL) {
                    printf("OK - Successfully logged out!\n");
                } else {
                    printf("%s\n", rsp_info);
                }
                free(message);
                free(response);
                //free the jwt token for logout
                free(auth_token);
                //reallocate the memory for auth_token with calloc
                //to represent that there's no token
                //(auth_token will be a void string in this case)
                auth_token = calloc(BUFLEN, sizeof(char));
                //same for session cookie
                free(session_cookie);
                session_cookie = calloc(BUFLEN, sizeof(char));

            }    
        }
        //the connection is closed 
        close_connection(sockfd);
        //but it will be reopened at the beginning of the loop
    }
    // free the allocated data
    free(auth_token);
    free(session_cookie);
    free(cmd);
    return 0;
}
