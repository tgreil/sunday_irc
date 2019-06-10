#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct  s_msg {
    char        msg[2048 - 64];
    char        from[64];
}               t_msg;

typedef struct  s_data {
    char        name[64];
    int         sock_fd;
}               t_data;

void    *listen_server(void *arg) {
    t_data  *data = (t_data *)arg;
    int     buf_size = 2048;
    char    buf[buf_size];
    t_msg   *msg;
    int     ret;

    while (1) {
        bzero(buf, buf_size);
        if ((ret = read(data->sock_fd, buf, buf_size)) > 0) {
            buf[ret] = 0;
            msg = (t_msg *)buf;
            if (strcmp(msg->from, data->name)) {
                printf("[%s]: %s\n", msg->from, msg->msg);
            }
        } else {
            printf("Read from server failed\n");
            exit(0);
        }
    }
    return (NULL);
}

int main(int argc, char **argv) {
    char                server_ip[] = "127.0.0.1";
    struct sockaddr_in  serv_addr;
    int                 buf_size = 2048;
    char                buf[buf_size];
    int                 ret, con;
    pthread_t           thread_listen_server;
    t_data              data;
    t_msg               msg;

    if (argc != 2) {
        printf("You must specify an user name\n");
        exit(0);
    } else
        strncpy(data.name, argv[1], 64);

    // Create socket
    data.sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Initialise datas
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(4242);
    inet_pton(AF_INET, server_ip, &serv_addr.sin_addr);

    if ((con = connect(data.sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0){
        printf("Connection to server failed\n");
        exit(0);
    }
    else
        printf("Hi [%s], you successfully connect to server\n\n", data.name);

    if (pthread_create(&thread_listen_server, NULL, listen_server, (void *)&data) < 0) {
        printf("Create pthread listen server failed\n");
        exit(0);
    }

    while (1) {
        if ((ret = read(0, buf, buf_size)) > 0) {
            if (ret > 1) {
                buf[ret - 1] = 0;
                strncpy(msg.from, data.name, 64);
                strncpy(msg.msg, buf, 2048 - 64);
                if ((ret = send(data.sock_fd, &msg, sizeof(t_msg), 0)) < 0) {
                    printf("Send to server failed\n");
                    exit(0);
                }
            }
        } else {
            printf("Read from stdin failed\n");
            exit(0);
        }
    }
    close(data.sock_fd);
    return (0);
}