#include "client.h"

int main(int argc, char **argv) {
    /* 格式：./client -ip set_ip(点分格式，例如127.0.0.1) -port set_port */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-ip") == 0 && argv[i+1] && strcmp(argv[i+1], "-port") != 0) {
            memset(set_ip, 0, 20);
            strcpy(set_ip, argv[i+1]);
        }
        else if (strcmp(argv[i], "-port") == 0 && argv[i+1] && strcmp(argv[i+1], "-ip") != 0) {
            set_port = atoi(argv[i+1]);
        }
    }

    int sockfd;
    struct sockaddr_in addr;
    char sentence[MAXSIZE];
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }
    // 助教服务器: 166.111.80.127
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;    
    addr.sin_port = htons(set_port);   //127.0.0.1
    if (inet_pton(AF_INET, set_ip, &addr.sin_addr) <= 0) {
        printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    printf("connecting...\n");
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("Error connect(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }
    clientOperation(sockfd, sentence);
    closeAllSocket();
    return 0;
}

int clientOperation(int sockfd, char *sentence) {
    char cmd[MAXSIZE];
    ClientRecv(sockfd, sentence);
    if (judgeSubStr(sentence, "220") == 0) {
        printf("fail to connect.\n");
        closeAllSocket();
        exit(1);
    }
    printf("%s", sentence);
    UserLogin(sockfd, sentence);
    while(1){
        multireceive = -1;
        readFtpCmd(sentence);
        memset(cmd, 0, MAXSIZE);
        strcpy(cmd, sentence);
        if (judgeSubStr(sentence, "LIST")) {
            char str[10] = "PASV";
            for (int i = 0; i < 4; i++) {
                sentence[i] = str[i];
            }
        }
        if (ClientSend(sockfd, sentence) < 0) {
            closeAllSocket();
            exit(1);
        }
        if (ClientRecv(sockfd, sentence) < 0) {
            closeAllSocket();
            exit(1);
        }
        printf("%s", sentence);
        /* 判是否接收了多条server消息 */
        for (unsigned int i = 0; i < strlen(sentence); i++) {
            if (sentence[i] == '\r' || sentence[i] == '\n') {
                multireceive++;
                if (sentence[i+1] == '\r' || sentence[i+1] == '\n') {
                    i++;
                }
            }
        }
        if (judgeSubStr(sentence, "200") && judgeSubStr(cmd, "PORT")) {
            if (client_PORT(cmd) < 0) {
                printf("Error opening socket for data connection\r\n");
            }
            else {
                mode = MPORT;
            }
        }
        else if (judgeSubStr(sentence, "227") && judgeSubStr(cmd, "PASV")) {
            if (client_PASV(sockfd, sentence) < 0) {
                printf("Error opening socket for data connection\r\n");
            }
            else {
                mode = MPASV;
            }
        }
        else if (judgeSubStr(sentence, "150") && judgeSubStr(cmd, "RETR")) {
            if (client_RETR(cmd) < 0) {
                printf("Error download file\r\n");
            }
            if (multireceive == 0) {
                if (ClientRecv(sockfd, sentence) < 0) {
                    closeAllSocket();
                    exit(1);
                }
                printf("%s", sentence);
            }
            mode = MDEFAULT;
        }
        else if (judgeSubStr(sentence, "150") && judgeSubStr(cmd, "STOR")) {
            
            if (client_STOR(cmd) < 0) {
                printf("Error upload file\r\n");
            }
            if (multireceive == 0) {
                if (ClientRecv(sockfd, sentence) < 0) {
                    closeAllSocket();
                    exit(1);
                }
                printf("%s", sentence);
            }
            mode = MDEFAULT;
        }
        else if (judgeSubStr(sentence, "257") && judgeSubStr(cmd, "MKD")) {
            if (client_MKD(sockfd, cmd) < 0) {
                printf("fail to create directory\n");
            }
        }
        else if (judgeSubStr(sentence, "227") && judgeSubStr(cmd, "LIST")) {
            if (client_LIST(sockfd, cmd, sentence) < 0) {
                printf("fail to get list.\n");
            }
            if (multireceive == 0) {
                if (ClientRecv(sockfd, sentence) < 0) {
                    closeAllSocket();
                    exit(1);
                }
                printf("%s", sentence);
            }
        }
        else if (judgeSubStr(sentence, "221") && judgeSubStr(cmd, "QUIT")) {
            break;
        }
        else if (judgeSubStr(sentence, "221") && judgeSubStr(cmd, "ABOR")) {
            break;
        }
    }
    return 0;
}

void UserLogin(int sockfd, char *sentence) {
    /* 输入用户名循环 */
    while(1) {
        readCommand(sentence);
        ClientSend(sockfd, sentence);
        ClientRecv(sockfd, sentence);
        printf("%s", sentence);
        if (judgeSubStr(sentence, "331")) {
            break;
        }
    }
    /* 输入密码循环 */
    while(1) {
        readCommand(sentence);
        ClientSend(sockfd, sentence);
        ClientRecv(sockfd, sentence);
        printf("%s", sentence);
        if (judgeSubStr(sentence, "230")) {
            break;
        }
    }
}

void closeAllSocket() {
    if (client_data_socket > 0) {
        close(client_data_socket);
        client_data_socket = -1;
    }
    if (client_pasv_socket > 0) {
        close(client_pasv_socket);
        client_pasv_socket = -1;
    }
    if (client_listen_port_socket > 0) {
        close(client_listen_port_socket);
        client_listen_port_socket = -1;
    }
    close(sockfd);
}

void readCommand(char *sentence) {
    memset(sentence, 0, MAXSIZE);
    fgets(sentence,4096,stdin);
    //int len = strlen(sentence);
    //sentence[len - 1] = '\0';
    return;
}

void readFtpCmd(char *sentence) {
    printf("ftp> ");
    readCommand(sentence);
    return;
}

int ClientSend(int sockfd, const char *sentence) {
    if (write(sockfd, sentence, strlen(sentence)) < 0) {
        printf("Error write(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }
    return 0;
}

int ClientRecv(int sockfd, char *sentence) {
    memset(sentence, 0, MAXSIZE);
    int len = 0;
    if ((len = read(sockfd, sentence, MAXSIZE)) < 0) {
        printf("Error read(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }

    return len;
}

int judgeSubStr(char *source, const char *target) {
    int flag = 1;
    for (unsigned int i = 0; i < strlen(target); i++) {
        if (source[i] != target[i]) {
            flag = 0;
        }
    }
    return flag;
}

void parseCommand(char *sentence, int len, char *cmd) {
    unsigned int index = len;
    while (sentence[index] == ' ' && index < strlen(sentence)) {
        index++;
    }
    if (index == strlen(sentence)) {
        printf("command is invalid.\r\n");
        return;
    }
    int i = 0;
    for (; index < strlen(sentence); index++) {
        cmd[i] = sentence[index];
        i++;
    }
    cmd[i] = '\0';
}

void parseIpPort(char *sentence, int len, char *ip, int *port) {
    int digit = 0;
    unsigned int index = len;
    int symbol = 0;
    for (int i = 0; index < strlen(sentence); index++) {
        if (sentence[index] >= '0' && sentence[index] <= '9') {
            ip[i++] = sentence[index];
            digit = digit * 10 + (sentence[index] - '0');
        }
        else if (sentence[index] == ','){
            symbol++;
            if (symbol == 4) {
                ip[i++] = '\0';
                break;
            }
            ip[i++] = '.';
            if (digit < 0 || digit > 255) {
                printf("ip address is invalid.\r\n");
                return;
            }
            digit = 0;
            
        }
    }
    digit = 0;
    for (; index < strlen(sentence); index++) {
        if (sentence[index] >= '0' && sentence[index] <= '9') {
            *port = *port * 10 + (sentence[index] - '0');
        }
        else if (sentence[index] == ','){
            digit = *port;
            *port = 0;
        }
    }
    *port += digit * 256;
}

int client_PASV(int sockfd, char *sentence) {
    char ip[20];
    int port = 0;
    int index = 0;
    /* 客户端解析获取ip和port */
    sentence[strlen(sentence) - 1] = '\0';
    while (sentence[index] != '(') {
        index++;
    }
    parseIpPort(sentence, index + 1, ip, &port);
    memset(&pasvaddr, 0, sizeof(pasvaddr));
    /* 本地建立socket */
    pasvaddr.sin_family = AF_INET;
    pasvaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &pasvaddr.sin_addr) <= 0) {
        printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }
    /* 若client的PASV处于打开状态，则关闭 */
    if (client_pasv_socket >= 0) {
        close(client_pasv_socket);
        client_pasv_socket = -1;
    }
    if ((client_pasv_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }
    if (clientConnect() < 0) {
        printf("Error connect(): %s(%d)\n", strerror(errno), errno);
        close(client_pasv_socket);
        client_pasv_socket = -1;
        return -1;
    }
    return 0;
}

int client_PORT(char *sentence) {
    char ip[20];
    int port = 0;
    struct sockaddr_in portaddr;
    parseIpPort(sentence, 5, ip, &port);
    /* PORT模式建立socket并设立监听 */
    memset(&portaddr, 0, sizeof(portaddr));
    portaddr.sin_family = AF_INET;
    portaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &portaddr.sin_addr) <= 0) {
        printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }
    /* 若client的PORT模式处于打开状态，则关闭 */
    if (client_listen_port_socket >= 0) {
        close(client_listen_port_socket);
        client_listen_port_socket = -1;
    }
    if ((client_listen_port_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }

    if (bind(client_listen_port_socket, (struct sockaddr*)&portaddr, sizeof(portaddr)) == -1) {
        printf("Error bind(): %s(%d)\n", strerror(errno), errno);
        if (client_listen_port_socket >= 0) {
            close(client_listen_port_socket);
            client_listen_port_socket = -1;
        }
        return -1;
    }
    if (listen(client_listen_port_socket, 10) == -1) {
        printf("Error listen(): %s(%d)\n", strerror(errno), errno);
        if (client_listen_port_socket >= 0) {
            close(client_listen_port_socket);
            client_listen_port_socket = -1;
        }
        return -1;
    }
    return 0;
}

int clientConnect() {
    if (connect(client_pasv_socket, (struct sockaddr*)&pasvaddr, sizeof(pasvaddr)) < 0) {
        printf("Error connect(): %s(%d)\n", strerror(errno), errno);
        if (client_pasv_socket >= 0) {
            close(client_pasv_socket);
            client_pasv_socket = -1;
        }
        return -1;
    }
    return 0;
}

int clientAccept() {
    if (client_data_socket >= 0) {
        close(client_data_socket);
        client_data_socket = -1;
    }
    if ((client_data_socket = accept(client_listen_port_socket, NULL, NULL)) == -1) {
        printf("Error accept(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }
    return 0;
}

int clientReceiveFile(int sock, char *sentence) {
    char filepath[MAXFILEPATH];
    parseCommand(sentence, 4, filepath);
    for (unsigned int i = 0; i < strlen(filepath); i++) {
        if (filepath[i] == '\r' || filepath[i] == '\n') {
            filepath[i] = '\0';
            break;
        }
    }
    FILE *dataFile = fopen(filepath, "wb");
    if (!dataFile) {
        ClientSend(sockfd, "No such file or dictionary.");
        return -1;
    }
    else {
        int datasize = 0;
        while (1) {
            datasize = ClientRecv(sock, sentence);
            if (datasize > 0) {
                fwrite(sentence, 1, datasize, dataFile);
            }
            else if (datasize == 0) {
                break;
            }
            else {
                fclose(dataFile);
                return -1;
            }
        }
    }
    fclose(dataFile);
    return 0;
}

int clientTransferFile(int sock, char *sentence) {
    char filepath[MAXFILEPATH];
    parseCommand(sentence, 4, filepath);
    for (unsigned int i = 0; i < strlen(filepath); i++) {
        if (filepath[i] == '\r' || filepath[i] == '\n') {
            filepath[i] = '\0';
            break;
        }
    }
    FILE *dataFile = fopen(filepath, "rb");
    if (!dataFile) {
        printf("%s", "No such file or dictionary.\r\n");
        return -1;
    }
    else {
        int datasize = 0;
        memset(sentence, 0, MAXSIZE);
        do {
            if ((datasize = fread(sentence, 1, MAXSIZE, dataFile)) < 0) {
                printf("%s", "Error when reading the file.\r\n");
                close(sock);
                sock = -1;
                if (mode == MPORT) {
                    close(client_listen_port_socket);
                    client_listen_port_socket = -1;
                }
                return -1;
            }
            else if (datasize == 0) {
                break;
            }
            if (write(sock, sentence, datasize) < 0) {
                printf("%s", "connection was broken or network failure.\r\n");
                close(sock);
                sock = -1;
                if (mode == MPORT) {
                    close(client_listen_port_socket);
                    client_listen_port_socket = -1;
                }
                return -1;
            }
        } while (datasize > 0);
    }
    fclose(dataFile);
    return 0;
}

int client_RETR(char *sentence) {
    if (mode == MPORT) {
        if (clientAccept() < 0) {
            printf("Error accept(): %s(%d)\n", strerror(errno), errno);
            close(client_data_socket);
            client_data_socket = -1;
            close(client_listen_port_socket);
            client_listen_port_socket = -1;
            return -1;
        }
        if (clientReceiveFile(client_data_socket, sentence) < 0) {
            return -1;
        }
        close(client_data_socket);
        client_data_socket = -1;
        close(client_listen_port_socket);
        client_listen_port_socket = -1;
    }
    else if (mode == MPASV) {
        if (clientReceiveFile(client_pasv_socket, sentence) < 0) {
            return -1;
        }
        close(client_pasv_socket);
        client_pasv_socket = -1;
    }
    return 0;
}

int client_STOR(char *sentence) {
    if (mode == MPORT) {
        if (clientAccept() < 0) {
            printf("Error accept(): %s(%d)\n", strerror(errno), errno);
            close(client_data_socket);
            client_data_socket = -1;
            close(client_listen_port_socket);
            client_listen_port_socket = -1;
            return -1;
        }
        if (clientTransferFile(client_data_socket, sentence) < 0) {        
            close(client_data_socket);
            client_data_socket = -1;
            close(client_listen_port_socket);
            client_listen_port_socket = -1;
            return -1;
        }
        close(client_data_socket);
        client_data_socket = -1;
        close(client_listen_port_socket);
        client_listen_port_socket = -1;
    }
    else if (mode == MPASV) {
        if (clientTransferFile(client_pasv_socket, sentence) < 0) {
            close(client_pasv_socket);
            client_pasv_socket = -1;
            return -1;
        }
        close(client_pasv_socket);
        client_pasv_socket = -1;
    }
    return 0;
}

int client_LIST(int sockfd, char *cmd, char *sentence) {
    if (client_PASV(sockfd, sentence) < 0) {
        printf("Error opening socket for data connection\r\n");
        return -1;
    }
    ClientSend(sockfd, cmd);
    ClientRecv(sockfd, sentence);
    printf("%s", sentence);
    /* 判是否接收了多条server消息 */
    multireceive = -1;
    for (unsigned int i = 0; i < strlen(sentence); i++) {
        if (sentence[i] == '\r' || sentence[i] == '\n') {
            multireceive++;
            if (sentence[i+1] == '\r' || sentence[i+1] == '\n') {
                i++;
            }
        }
    }
    while (ClientRecv(client_pasv_socket, sentence) > 0) {
        printf("%s", sentence);
    }
    close(client_pasv_socket);
    client_pasv_socket = -1;
    return 0;
}

int client_MKD(int sockfd, char *sentence) {
    //////ClientRecv(sockfd, sentence);
    //////printf("%s", sentence);
    return 0;
}

int client_PWD(int sockfd, char *sentence) {
    return 0;
}

int client_RMD(int sockfd, char *sentence) {
    return 0;
}

int client_CWD(int sockfd, char *sentence) {
    return 0;
}
