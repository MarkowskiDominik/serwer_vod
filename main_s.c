/* 
 * File:   main.c
 * Author: dominik
 *
 * Created on 1 lipiec 2015, 08:57
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define MYPORT 1501
#define BACKLOG 10 // dlugosc kolejki

void *connection_handler(void *);

void sigchld_handler(int s) {
    while (wait(NULL) > 0);
}

int main(int argc, char * argv[]) {
    int socket_desc, client_sock, sock_size, *new_sock, true = 1;
    struct sockaddr_in server, client;
    struct sigaction sa;

    //create socket
    if ((socket_desc = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return EXIT_FAILURE;
    }

    //set socket options
    if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &true, sizeof ( int)) == -1) {
        perror("setsockopt");
        return EXIT_FAILURE;
    }

    //Prepare structure sockaddr_in
    server.sin_family = AF_INET; // rodzaj gniazda z ktorego kozysta TCP/IP
    server.sin_port = htons(MYPORT); // numer portu
    server.sin_addr.s_addr = INADDR_ANY;
    //server.sin_addr.s_addr = inet_addr( "127.0.0.1" );
    //bzero( &( server.sin_zero ), 8 ); 

    //bind
    if (bind(socket_desc, (struct sockaddr *) & server, sizeof ( struct sockaddr)) == -1) {
        perror("bind");
        return EXIT_FAILURE;
    }

    //listen
    if (listen(socket_desc, BACKLOG) == -1) {
        perror("listen");
        return EXIT_FAILURE;
    }

    sa.sa_handler = sigchld_handler; // zbieranie martwych procesow
    sigemptyset(& sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    sock_size = sizeof ( struct sockaddr_in);
    while (true) {

        if ((client_sock = accept(socket_desc, (struct sockaddr *) &client, (socklen_t*) & sock_size)) == -1) {
            perror("accept");
            continue;
        }

        printf("server link %s: %d\n", inet_ntoa(client.sin_addr), (int)client.sin_port);

        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = client_sock;

        if (pthread_create(&sniffer_thread, NULL, connection_handler, (void*) new_sock) < 0) {
            perror("create thread");
        }
    }
    return 0;
}

void *connection_handler(void *socket_desc) {
    //Get the socket descriptor
    int sock = *(int*) socket_desc;
    int read_size;
    char *message, client_message[128];

    //Receive a message from client
    while ((read_size = recv(sock, client_message, 128, 0)) > 0) {
        printf("%s\n", client_message);
        write(sock, client_message, strlen(client_message));
    }

    int i;
    FILE * plik;
    plik = fopen(client_message, "r");
    if (!plik) {
        perror("file opening");
    } else {
        while (feof(plik) == 0) {
            fscanf(plik, "%c", & message[ i ]);
            i++;
        }

        if (send(sock, message, strlen(message), 0 == -1))
            perror("send");
    }
    fclose(plik);
    close(sock);

    if (read_size == 0) {
        puts("Client disconnected");
        fflush(stdout);
    } else if (read_size == -1) {
        perror("recv failed");
    }

    //Free the socket pointer
    free(socket_desc);

    return EXIT_SUCCESS;
}