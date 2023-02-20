#include "utils.h"
#define N 100
int main(int argc, char *argv[])
{
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    int ok = 1;
    if (sockfd < 0)
    {
        ok = 0;
    }
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int yes = 1;
    int result = setsockopt(sockfd,
                            IPPROTO_TCP,
                            TCP_NODELAY,
                            (char *)&yes,
                            sizeof(int));

    DIE(argv[1] == NULL, "invalid input");
    char *my_id = strdup(argv[1]);
    struct sockaddr_in addr;
    socklen_t len = sizeof(struct sockaddr_in);
    memset(&addr, 0, len);
    addr.sin_family = AF_INET;
    DIE(argv[2] == NULL, "invalid input");
    int ret = inet_aton(argv[2], &addr.sin_addr);
    if (ret < 0)
    {
        ok = 0;
    }
    DIE(argv[3] == NULL, "invalid input");
    addr.sin_port = htons(atoi(argv[3]));
    int rez = connect(sockfd, (const struct sockaddr *)(&addr), len);
    DIE(rez < 0, "Not connected");
    fd_set readfds, cpyreadfds;
    FD_ZERO(&readfds);
    FD_ZERO(&cpyreadfds);
    FD_SET(sockfd, &readfds);
    FD_SET(STDIN_FILENO, &readfds);
    int fdmax = sockfd;
    while (ok > 0)
    {
        cpyreadfds = readfds;
        int selected = select(fdmax + 1, &cpyreadfds, NULL, NULL, NULL);
        if (selected < 0)
        {
            ok = 0;
        }
        if (FD_ISSET(STDIN_FILENO, &cpyreadfds) > 0)
        {
            // primim input de la tastatura
            char buffer[N];
            fgets(buffer, N, stdin);
            char *copy = strdup(buffer);
            copy = strtok(copy, "\n");
            char *p = strtok(buffer, " \n");
            if (strcmp(p, "subscribe") == 0)
            {
                p = strtok(NULL, " \n");
                DIE(p == NULL, "invalid input\n");
                p = strtok(NULL, " \n");
                DIE(p == NULL, "invalid input\n");
                printf("Subscribed to topic.\n");
                send(sockfd, copy, strlen(copy) + 1, 0);
            }
            else if (strcmp(p, "unsubscribe") == 0)
            {
                p = strtok(NULL, " \n");
                DIE(p == NULL, "invalid input\n");
                send(sockfd, copy, strlen(copy) + 1, 0);
            }
            else if (strcmp(p, "exit") == 0)
            {
                char text[] = "I'm out";
                int ret = send(sockfd, text, strlen(text) + 1, 0);
                ok = 1;
                break;
            }
        }
        if (FD_ISSET(sockfd, &cpyreadfds) > 0)
        {
            // primim mesaje de la server
            char buf[MAX_UDP_MSG];
            memset(buf, 0, MAX_UDP_MSG);
            int ret = recv(sockfd, buf, MAX_LEN_CONTENT, 0);

            if (strcmp(buf, "Give me your id!") == 0)
            {
                send(sockfd, my_id, 11, 0);
            }
            else if (strcmp(buf, "Id exists") == 0)
            {
                ok = 1;
                break;
            }
            else if (strcmp(buf, "server down") == 0)
            {
                char text[] = "I'm out";
                send(sockfd, text, strlen(text) + 1, 0);
                ok = 1;
                break;
            }
            else
            {
                printf("%s", buf);
            }
        }
    }

    close(sockfd);
}