#include "utils.h"

#define Backlog 10
#define N 100
int main(int argc, char *argv[])
{
    int nr_client = 0;
    int clients_capacity = 10;
    Tclient **clients = calloc(clients_capacity, sizeof(Tclient *));
    int topic_capacity = 30;
    int topic_elements = 0;
    Ttopic **topics = calloc(topic_capacity, sizeof(Ttopic *));
    int ok = 1;
    int sockfdudp = socket(PF_INET, SOCK_DGRAM, 0);
    int sockfdtcp = socket(PF_INET, SOCK_STREAM, 0);
    DIE(sockfdudp < 0, "Socket udp err");
    DIE(sockfdtcp < 0, "Socket tcp err");
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int yes = 1;
    int result = setsockopt(sockfdtcp,
                            IPPROTO_TCP,
                            TCP_NODELAY,
                            (char *)&yes,
                            sizeof(int)); // Dezactivam Nagle

    struct sockaddr_in myaddr;
    memset(&myaddr, 0, sizeof(struct sockaddr_in));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = INADDR_ANY;
    DIE(argv[1] == NULL, "invalid input");
    
    myaddr.sin_port = htons(atoi(argv[1]));
    
    
    int bind_udp = bind(sockfdudp, (struct sockaddr *)&myaddr, sizeof(struct sockaddr));

    int bind_tcp = bind(sockfdtcp, (struct sockaddr *)&myaddr, sizeof(struct sockaddr));

    DIE(bind_tcp < 0, "Bind tcp err");
    DIE(bind_udp < 0, "Bind udp err");
    int ret = listen(sockfdtcp, Backlog);
    
    DIE(ret < 0, "Listen error");
    fd_set readfds, cpyreadfds;
    FD_ZERO(&readfds);
    FD_ZERO(&cpyreadfds);

    FD_SET(sockfdudp, &readfds);
    FD_SET(sockfdtcp, &readfds);
    FD_SET(STDIN_FILENO, &readfds);

    int fdmax = sockfdudp;
    if (sockfdtcp > fdmax)
    {
        fdmax = sockfdtcp;
        // STDIN_FILENO e 0 oricum
    }
    while (ok > 0)
    {
        cpyreadfds = readfds;
        int selected = select(fdmax + 1, &cpyreadfds, NULL, NULL, NULL);
        if (selected < 0)
        {
            ok = 0;
            break;
        }
        for (int i = 0; i <= fdmax; i++)
        {
            if (FD_ISSET(i, &cpyreadfds) > 0)
            {
                if (i == sockfdtcp)
                { // am primit o cerere de abonare

                    struct sockaddr_in *new_client = calloc(1, sizeof(struct sockaddr_in));
                    socklen_t len = sizeof(struct sockaddr_in);
                    int connfd = accept(sockfdtcp, (struct sockaddr *)new_client, &len);
                    if(connfd < 0)
                    {
                        ok = 0;
                        break;
                    }
                    int result = setsockopt(connfd,
                                            IPPROTO_TCP,
                                            TCP_NODELAY,
                                            (char *)&yes,
                                            sizeof(int));
                    FD_SET(connfd, &readfds);
                    if (connfd > fdmax)
                    {
                        fdmax = connfd;
                    }
                    if (nr_client >= clients_capacity - 1)
                    {
                        clients_capacity *= 2;
                        Tclient **real = NULL;
                        real = realloc(clients, clients_capacity * sizeof(Tclient *));
                        if (real != NULL)
                        {
                            clients = real;
                        }
                    }

                    Tclient *cli = calloc(1, sizeof(Tclient));
                    cli->addr = calloc(1, sizeof(struct sockaddr_in));
                    memcpy(cli->addr, new_client, sizeof(struct sockaddr_in));
                    cli->connfd = connfd;
                    cli->enable = 1;
                    char text[] = "Give me your id!";
                    int ret = send(connfd, text, sizeof(text), 0);
                    char *id = calloc(11, sizeof(char));
                    ret = recv(connfd, id, 11, 0);
                    strcpy(cli->id, id);
                    int exist = 0, poz_cl = 0;

                    for (int cntr = 0; cntr < nr_client && exist == 0; cntr++)
                    {
                        if (strcmp(id, clients[cntr]->id) == 0)
                        {
                            exist = 1;
                            poz_cl = cntr;
                        }
                    }

                    if (exist == 1 && clients[poz_cl]->enable == 1)
                    {
                        {
                            strcpy(text, "Id exists");
                            send(connfd, text, strlen(text) + 1, 0);
                            printf("Client ");
                            printf("%s", clients[poz_cl]->id);
                            printf(" already connected.\n");
                            FD_CLR(connfd, &readfds);
                            close(connfd);
                        }
                    }
                    else
                    {
                        if (exist == 1)
                        {
                            cli->sf = clients[poz_cl]->sf;
                            cli->q_data = clients[poz_cl]->q_data;
                            cli->last = clients[poz_cl]->last;
                            clients[poz_cl] = cli;
                        }
                        else
                        {
                            cli->enable = 1;
                            clients[nr_client++] = cli;
                        }

                        strcpy(cli->id, id);
                        printf("New client ");
                        printf("%s", id);
                        printf(" connected from ");
            
                        char *ip_address_hr = inet_ntoa(cli->addr->sin_addr);
                        printf("%s", ip_address_hr);
                        printf(": %d.\n", ntohs(cli->addr->sin_port));
                        while (cli->q_data != NULL)
                        {
                            send(cli->connfd, cli->q_data->msg, cli->q_data->len, 0);
                            cli->q_data = cli->q_data->next;
                        }
                        cli->last = NULL;
                    }
                }
                else if (i == STDIN_FILENO)
                { // primim input de la tastarura
                    char buffer[N];
                    fgets(buffer, N, stdin);
                    char *p = strtok(buffer, "\n ");
                    if (strstr(p, "exit") != NULL)
                    {
                        ok = 0;
                        char *text2 = strdup("server down");
                        char *recvt = calloc(N, sizeof(char));
                        int nr2;
                        for (int ctr = 0; ctr < nr_client; ctr++)
                        {
                            if (clients[ctr]->enable == 1)
                            {

                                send(clients[ctr]->connfd, text2, strlen(text2) + 1, 0);
                                jumpthere:
                                nr2 = recv(clients[ctr]->connfd, recvt, N, 0);
                                if(strcmp(recvt, "I'm out") != 0 && nr2 != 0)
                                {
                                    goto jumpthere;
                                }
                                FD_CLR(clients[ctr]->connfd, &readfds);
                                close(clients[ctr]->connfd);
                                clients[ctr]->enable = 0;
                            }
                        }
                        free(text2);
                        free(recvt);
                        close(sockfdtcp);
                        close(sockfdudp);
                        break;
                    }
                }
                else if (i == sockfdudp)
                {
                    // am primit o datagrama
                    char buffer[MAX_UDP_MSG];
                    memset(buffer, 0, MAX_UDP_MSG);
                    struct sockaddr addr;
                    socklen_t len = sizeof(addr);
                    int bytes_read = recvfrom(i, buffer, MAX_UDP_MSG, 0, &addr, &len);
                    T_udp_data *udpData = (T_udp_data *)(buffer);
                    struct sockaddr_in *adresa = (struct sockaddr_in *)&addr;
                    int poz = exist_topic(topics, udpData->topic, topic_elements);
                    char *string_to = calloc(MAX_UDP_MSG, sizeof(char));

                    if (poz < 0)
                    {
                        Ttopic *top = calloc(1, sizeof(Ttopic));
                        strcpy(top->topic, udpData->topic);

                        if (topic_elements >= topic_capacity - 1)
                        {
                            topic_capacity *= 2;
                            topics = realloc(topics, topic_capacity * sizeof(Ttopic *));
                        }
                        topics[topic_elements++] = top;
                        poz = topic_elements - 1;
                    }
                    int offset = sprintf(string_to, "%s", inet_ntoa(adresa->sin_addr));
                    offset += sprintf(string_to + offset, ":%d", ntohs(adresa->sin_port));
                    offset += sprintf(string_to + offset, " - %s", udpData->topic);
                    switch (udpData->tip_date)
                    {
                    case 0:
                    {
                        char *buf = udpData->continut;
                        uint8_t semn = (uint8_t)*buf;
                        uint32_t *val_continut = calloc(1, sizeof(uint32_t));
                        memcpy(val_continut, buf + 1, sizeof(uint32_t));

                        uint32_t val = *val_continut;
                        int val_de_trimis = 0;
                        if (semn == 0)
                        {
                            val_de_trimis = ntohl(val);
                        }
                        else
                        {
                            val_de_trimis = (-1) * ntohl(val);
                        }
                        offset += sprintf(string_to + offset, " - INT - %d\n", val_de_trimis);
                        break;
                    }
                    case 1:
                    {
                        char *buf = udpData->continut;
                        uint16_t modul = 0;
                        memcpy(&modul, buf, sizeof(uint16_t));
                        uint16_t modul_host_endian = ntohs(modul);
                        float nr = modul_host_endian / 100.00;
                        offset += sprintf(string_to + offset, " - SHORT_REAL - %.2f\n", nr);
                        break;
                    }
                    case 2:
                    {
                        char *buf = udpData->continut;
                        int semn = 0;
                        if (*buf == 1)
                        {
                            semn = 1;
                        }
                        uint32_t modul = 0;
                        memcpy(&modul, buf + 1, sizeof(uint32_t));
                        int modul_host_endian = ntohl(modul);
                        float nr = 1.0 * modul_host_endian;
                        char exp = 0;
                        memcpy(&exp, buf + 5, sizeof(char));
                        for (int p = 0; p < exp; p++)
                        {
                            nr = nr / 10.0;
                        }
                        if (semn == 1)
                        {
                            nr *= -1;
                        }
                        offset += sprintf(string_to + offset, " - FLOAT - %.4f\n", nr);
                        break;
                    }

                    case 3:
                    {
                        char *buf = udpData->continut;
                        offset += sprintf(string_to + offset, " - STRING - %s\n", buf);
                        break;
                    }
                    }
                    Tclient *parc_client = NULL;
                    if (topics[poz]->head != NULL)
                    {
                        parc_client = *(topics[poz]->head);
                    }
                    while (parc_client != NULL)
                    {

                        if (parc_client->enable == 1)
                        {
                            send(parc_client->connfd, string_to, offset, 0);
                        }
                        else if (parc_client->sf[poz] == 1)
                        {
                            TQmsg *msg = calloc(1, sizeof(TQmsg));
                            msg->msg = strdup(string_to);
                            if (parc_client->q_data == NULL)
                            {
                                parc_client->q_data = msg;
                            }
                            else
                            {
                                parc_client->last->next = msg;
                            }
                            parc_client->last = msg;
                            msg->len = offset;
                        }
                        if (parc_client->next != NULL)
                        {
                            parc_client = *(parc_client->next);
                        }
                        else
                        {
                            parc_client = NULL;
                        }
                    }
                }
                else
                {
                    // am primit comanda de la unul din clientii tcp
                    int gasit = 0;
                    for (int k = 0; k < nr_client && gasit == 0; k++)
                    {
                        if (i == clients[k]->connfd)
                        {
                            char buffer[N];
                            gasit = 1;
                            int ret = recv(i, buffer, N, 0);
                            char *copy = strdup(buffer);
                            char *p = strtok(buffer, " \n");
                            if (strcmp(p, "subscribe") == 0)
                            {

                                p = strtok(NULL, " \n");
                                DIE(p == NULL, "invalid input\n");
                                char *name_topic = strdup(p);
                                int poz = exist_topic(topics, p, topic_elements);
                                if (poz > -1 && topics[poz]->head == NULL)
                                {
                                    topics[poz]->head = &(clients[k]);
                                    topics[poz]->last = &(clients[k]);
                                    (*(topics[poz]->last))->next = NULL;
                                }
                                else if (poz > -1)
                                {
                                    (*(topics[poz]->last))->next = &clients[k];
                                    topics[poz]->last = &(clients[k]);
                                    clients[k]->next = NULL;
                                }
                                else
                                {
                                    Ttopic *tpc = calloc(1, sizeof(Ttopic));
                                    strcpy(tpc->topic, name_topic);
                                    tpc->head = &clients[k];
                                    tpc->last = &clients[k];
                                    if (topic_elements + 1 >= topic_capacity)
                                    {
                                        topic_capacity *= 2;
                                        topics = realloc(topics, topic_capacity * sizeof(Ttopic *));
                                        clients[k]->sf = realloc(clients[k]->sf, sizeof(int) * topic_capacity);
                                    }
                                    poz = topic_elements;
                                    topics[topic_elements++] = tpc;
                                }
                                p = strtok(NULL, " \n");
                                DIE(p == NULL, "invalid input");
                                if (poz == 0)
                                {
                                    clients[k]->sf = calloc(topic_capacity, sizeof(int));
                                }
                                clients[k]->sf[poz] = atoi(p);
                            }

                            if (strstr(copy, "unsubscribe") != NULL)
                            {
                                p = strtok(copy, " \n");
                                DIE(p == NULL, "invalid input");
                                p = strtok(NULL, " \n");
                                DIE(p == NULL, "invalid input");
                                for (int kt = 0; kt < topic_elements; kt++)
                                {
                                    if (strcmp(topics[kt]->topic, p) == 0)
                                    {
                                        Tclient *aux = *(topics[kt]->head);
                                        Tclient *prev = NULL;
                                        while (aux != NULL && strcmp(aux->id, clients[k]->id) != 0)
                                        {
                                            prev = aux;
                                            aux = *(aux->next);
                                        }

                                        if (aux != NULL)
                                        {
                                            prev->next = aux->next;
                                        }
                                    }
                                }
                            }

                            if (strcmp(copy, "I'm out") == 0)
                            {
                                clients[k]->enable = 0;
                                FD_CLR(clients[k]->connfd, &readfds);
                                FD_CLR(clients[k]->connfd, &cpyreadfds);
                                close(clients[k]->connfd);
                                printf("Client ");
                                printf("%s", clients[k]->id);
                                printf(" disconnected.\n");
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}