#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_CONNEXION  200 

typedef struct  s_connexions {
    int         nb;
    int         connexions_fd[MAX_CONNEXION];
    int         connexions_available[MAX_CONNEXION];
    int         sock_fd;
    int         tmp_fd;
}               t_connexions;

int     send_to_all(t_connexions *connexions, void *data, int data_len) {
    int ret, total_send = 0;
    int i;

    i = 0;
    while (i < MAX_CONNEXION && total_send < connexions->nb) {
        if (connexions->connexions_available[i]) {
            if ((ret = send(connexions->connexions_fd[i], data, data_len, 0)) > 0) {
                total_send++;
            }
        }
        i++;
    }
    return total_send;
}

void *reader_connexion(void *arg) {
    t_connexions *connexions = (t_connexions *)arg;
    int         conn_fd = connexions->tmp_fd;
    int         thread_nb = connexions->nb;
    int         size_buf = 2048;
    char        buf[size_buf];
    int         ret, i = 0;

    while (i < MAX_CONNEXION) {
        if (!connexions->connexions_available[i]) {
            connexions->connexions_available[i] = 1;
            connexions->connexions_fd[i] = conn_fd;
            break ;
        }
        i++;
    }
    while (1) {
        if ((ret = read(conn_fd, buf, size_buf)) > 0) {
            send_to_all(connexions, buf, ret);
        }
        else
            break ;
    }
    connexions->connexions_available[i] = 0;
    printf("Opended connexion: %d\n", --(connexions->nb));
    return (NULL);
}

void *accepter(void *arg) {
    t_connexions *connexions = (t_connexions *)arg;
    pthread_t   thread_reader;

    while (1) {
        if ((connexions->tmp_fd = accept(connexions->sock_fd, NULL, NULL)) < 0) {
            printf("Accept failed\n");
            exit(0);
        }

        connexions->nb++;
        if (connexions->nb >= MAX_CONNEXION) {
            connexions->nb--;
            printf("Max connexions has been reached\n");
            close(connexions->tmp_fd);
        } else {
            printf("Opended connexion: %d\n", connexions->nb);
            if (pthread_create(&thread_reader, NULL, reader_connexion, (void *)connexions) < 0) {
                printf("Create pthread reader failed\n");
                exit(0);
            }
        }
    }
    printf("ACCEPTER THREAD END HIS JOB");
    return (NULL);
}

void    *reader_input(void *arg) {
    t_connexions *connexions = (t_connexions *)arg;
    int     buf_size = 2048;
    char    buf[buf_size];
    int     ret;

    while (1) {
        if ((ret = read(0, buf, buf_size)) > 0) {
            buf[ret] = 0;
            //send_to_all(connexions, buf, ret);
        } else {
            printf("Read from stdin failed\n");
            exit(0);
        }
    }
}

int main(int ac, char **av) {
    struct      sockaddr_in serv_addr;
    struct      sockaddr client_addr;
    t_connexions connexions;
    char        recvBuff[1024];
    pthread_t   thread_accepter;
    pthread_t   thread_reader_input;

    // Create socket
    connexions.sock_fd = socket(PF_INET, SOCK_STREAM, 0);
    connexions.nb = 0;

    // Initialise datas
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(4242);

    // Bind socket to server
    int b_ret = bind(connexions.sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    // Make server listening
    int l_ret = listen(connexions.sock_fd, 10);

    if (pthread_create(&thread_reader_input, NULL, reader_input, (void *)&connexions)) {
        printf("Create pthread accepter failed\n");
	    exit(0);
    }

    if (pthread_create(&thread_accepter, NULL, accepter, (void *)&connexions) < 0) {
        printf("Create pthread accepter failed\n");
	    exit(0);
    }

    printf("Server perfectly launched, cllients can now be connected\n");
    if (pthread_join(thread_accepter, NULL)) {
        printf("Join pthread failed\n");
    }

    printf("End of programs\n");
    close(connexions.sock_fd);
    return (0);
}