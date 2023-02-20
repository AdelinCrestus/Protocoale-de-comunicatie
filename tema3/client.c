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

#define HOST "34.241.4.235"
#define PORT 8080

int main()
{
    int sockfd;

    char buffread[BUFLEN];
    char *response;
    char *payload = calloc(BUFLEN, sizeof(char));
    char *ses_cookie = calloc(BUFLEN, sizeof(char));
    char *token = calloc(BUFLEN, sizeof(char));
    char *url = calloc(LINELEN, sizeof(char));
    while (1)
    {

        fgets(buffread, BUFLEN, stdin);
        sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
        if (strcmp(buffread, "register\n") == 0)
        {
            char username[LINELEN];
            char password[LINELEN];
            printf("username=");
            fgets(username, LINELEN, stdin);
            printf("password=");
            fgets(password, LINELEN, stdin);
            char *username_str = strtok(username, "\n");
            char *password_str = strtok(password, "\n");
            memset(payload, 0, BUFLEN);
            strcat(payload, "{\n\t\"username\":\"");
            strcat(payload, username_str);
            strcat(payload, "\",\n\t\"password\":\"");
            strcat(payload, password_str);
            strcat(payload, "\"\n}");

            char *ret = compute_post_request(HOST, "/api/v1/tema/auth/register", "application/json", &payload, 1, NULL, 0, NULL);
            send_to_server(sockfd, ret);
            response = receive_from_server(sockfd);
            char *p = strtok(response, "\n\r");
            if (p[9] != '2')
            {
                printf("Username already used\n");
            }
            else
            {
                printf("%s\n", p + 9); // am sarit pentru a ajunge la mesajul care indica daca este OK sau avem eroare
            }
        }
        else if (strcmp(buffread, "login\n") == 0)
        {
            if(strlen(ses_cookie) > 0)
            {
                printf("Error. You are already connected\n");
                continue;
            }
            char username[LINELEN];
            char password[LINELEN];
            printf("username=");
            fgets(username, LINELEN, stdin);
            printf("password=");
            fgets(password, LINELEN, stdin);
            char *username_str = strtok(username, "\n");
            char *password_str = strtok(password, "\n");
            memset(payload, 0, BUFLEN);

            strcat(payload, "{\n\t\"username\":\"");
            strcat(payload, username_str);
            strcat(payload, "\",\n\t\"password\":\"");
            strcat(payload, password_str);
            strcat(payload, "\"\n}");

            char *ret = compute_post_request(HOST, "/api/v1/tema/auth/login", "application/json", &payload, 1, NULL, 0, NULL);
            send_to_server(sockfd, ret);
            response = receive_from_server(sockfd);
            char *copy_response = strdup(response);
            copy_response = strtok(copy_response, "\n");
            printf("%s\n", copy_response + 9);
            if (copy_response[9] != '2')
            {
                printf("Wrong credentials\n");
            }
            char *p = strstr(response, "Cookie");
            if (p == NULL)
            {
                continue;
            }
            p = strtok(p, ";");
            memset(ses_cookie, 0, BUFLEN);
            strcpy(ses_cookie, p + 8); // sarim peste "Cookie"
        }
        else if (strcmp(buffread, "enter_library\n") == 0)
        {
            if (strlen(ses_cookie) == 0)
            {
                printf("You are not logged in\n");
                continue;
            }
            char *ret = compute_get_request(HOST, "/api/v1/tema/library/access", NULL, &ses_cookie, 1, NULL);
            send_to_server(sockfd, ret);
            response = receive_from_server(sockfd);
            char *p = strstr(response, "token");
            if (p == NULL)
            {
                printf("Error\n");
                continue;
            }
            p = p + 8;
            p = strtok(p, "\"");
            memset(token, 0, BUFLEN);
            strcpy(token, p);
            if (strlen(token) > 0)
            {
                printf("You have access\n");
            }
            else
            {
                printf("Error\n");
            }
        }
        else if (strcmp(buffread, "get_books\n") == 0)
        {
            if (strlen(token) == 0)
            {
                printf("You don't have access\n");
                continue;
            }
            char *ret = compute_get_request(HOST, "/api/v1/tema/library/books", NULL, NULL, 0, token);
            send_to_server(sockfd, ret);
            response = receive_from_server(sockfd);
            char *p = strstr(response, "[");
            if (p == NULL)
            {
                printf("Error\n");
                continue;
            }
            printf("%s\n", p);
        }

        else if (strcmp(buffread, "add_book\n") == 0)
        {
            if (strlen(token) == 0)
            {
                printf("You don't have access\n");
                continue;
            }
            char title[LINELEN], author[LINELEN], genre[LINELEN], page_count[LINELEN], publisher[LINELEN];
            printf("title=");
            fgets(title, LINELEN, stdin);
            printf("author=");
            fgets(author, LINELEN, stdin);
            printf("genre=");
            fgets(genre, LINELEN, stdin);
            printf("publisher=");
            fgets(publisher, LINELEN, stdin);
            printf("page_count=");
            fgets(page_count, LINELEN, stdin);

            char *title_str = strtok(title, "\n");
            char *author_str = strtok(author, "\n");
            char *genre_str = strtok(genre, "\n");
            char *page_count_str = strtok(page_count, "\n");
            char *publisher_str = strtok(publisher, "\n");
            int valid = 1;
            for (int i = 0; i < strlen(page_count_str); i++)
            {
                if (page_count_str[i] < '0' || page_count_str[i] > '9')
                {
                    printf("Invalid page count!!!\n");
                    valid = 0;
                    break;
                }
            }
            if (valid == 0)
            {
                continue;
            }
            memset(payload, 0, BUFLEN);

            strcat(payload, "{\n\t\"title\":\"");
            strcat(payload, title_str);
            strcat(payload, "\",\n\t\"author\":\"");
            strcat(payload, author_str);
            strcat(payload, "\",\n\t\"genre\":\"");
            strcat(payload, genre_str);
            strcat(payload, "\",\n\t\"page_count\":\"");
            strcat(payload, page_count_str);
            strcat(payload, "\",\n\t\"publisher\":\"");
            strcat(payload, publisher_str);
            strcat(payload, "\"\n}");

            char *ret = compute_post_request(HOST, "/api/v1/tema/library/books", "application/json", &payload, 1, &ses_cookie, 1, token);
            send_to_server(sockfd, ret);
            response = receive_from_server(sockfd);
            char *p = strtok(response, "\r\n");
            printf("%s\n", p + 9);
        }
        else if (strcmp(buffread, "get_book\n") == 0)
        {
            printf("id=");
            char id[LINELEN];
            fgets(id, LINELEN, stdin);
            memset(url, 0, LINELEN);
            strcpy(url, "/api/v1/tema/library/books/");
            strcat(url, id);
            url = strtok(url, "\n");
            if (strlen(token) == 0)
            {
                printf("You don't have access\n");
                continue;
            }
            char *ret = compute_get_request(HOST, url, NULL, &ses_cookie, 1, token);
            send_to_server(sockfd, ret);
            response = receive_from_server(sockfd);
            char *p = strrchr(response, '{');
            p = strtok(p, "]");
            if (p != NULL)
            {
                printf("%s\n", p);
            }
            else
            {
                printf("Error!\n");
            }
        }
        else if (strcmp(buffread, "delete_book\n") == 0)
        {
            printf("id=");
            char id[LINELEN];
            fgets(id, LINELEN, stdin);
            memset(url, 0, LINELEN);
            strcpy(url, "/api/v1/tema/library/books/");
            strcat(url, id);
            url = strtok(url, "\n");
            if (strlen(token) == 0)
            {
                printf("You don't have access\n");
                continue;
            }
            char *ret = compute_get_request(HOST, url, NULL, &ses_cookie, 1, token);
            // schimbam metoda din get in delete , in rest erau identice si nu avea rost sa mai facem functie separat
            ret += 3;
            char *aux = calloc(BUFLEN, sizeof(char));
            strcpy(aux,"DELETE");
            strcat(aux, ret);
            send_to_server(sockfd,aux);
            response = receive_from_server(sockfd);
            free(aux);
            if(response[9] == '2' )
            {
                printf("Book deleted\n");
            }
            else if(response[9] == '4')
            {
                printf("Id invalid\n");
            }
            else
            {
                printf("Error\n");
            }
        }
        else if(strcmp(buffread, "logout\n") == 0)
        {
            char *ret = compute_get_request(HOST, "/api/v1/tema/auth/logout", NULL, &ses_cookie, 1, NULL);
            send_to_server(sockfd, ret);
            response = receive_from_server(sockfd);
            if(response[9] == '2')
            {
                printf("You logged out\n");
                memset(ses_cookie, 0, BUFLEN);
                memset(token, 0, BUFLEN);
            }
            else if(response[9] == '4')
            {
                printf("You are not logged in\n");
            }
            else 
            {
                printf("Error\n");
            }

        }
        else if (strcmp(buffread, "exit\n") == 0)
        {
            break;
        }
        close_connection(sockfd);
    }
    free(url);
    free(token);
    free(ses_cookie);
    free(payload);
}
