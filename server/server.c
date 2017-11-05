#include "server.h"

int main(int argc, char **argv) {
    /* 格式：./server -root set_path -port set_port */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-root") == 0 && argv[i+1] && strcmp(argv[i+1], "-port") != 0) {
            memset(set_path, 0, 80);
            strcpy(set_path, argv[i+1]);
        }
        else if (strcmp(argv[i], "-port") == 0 && argv[i+1] && strcmp(argv[i+1], "-root") != 0) {
            set_port = atoi(argv[i+1]);
        }
    }

    struct sockaddr_in addr;
    char sentence[MAXSIZE];
    pid_t pid;
    int res = 0;
    if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(set_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        printf("Error bind(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }
    if (listen(listenfd, 10) == -1) {
        printf("Error listen(): %s(%d)\n", strerror(errno), errno);
        return 1;
    }

    while (1) {
        // connect successfully
        if ((connfd = accept(listenfd, NULL, NULL)) == -1) {
            printf("Error accept(): %s(%d)\n", strerror(errno), errno);
            continue;
        }
        switch(pid = fork()) {
            case -1:
                printf("Error fork(): %s(%d)\n", strerror(errno), errno);
                break;
            case 0:
                res = serverOperation(connfd, sentence);
                if (res == -1) {
                    closeAllSocket();
                    exit(-1);
                }
                else if (res == -2) {
                    closeAllSocket();
                    exit(0);
                }
                break;
            default:
                close(connfd);
        }
        close(connfd);
    }
    closeAllSocket();
    return 0;
}

int serverOperation(int connfd, char *sentence) {
    ServerSend(connfd, msg220);
    verifyLogin(connfd, sentence);
    chdir(set_path);
    while(1) {
        ServerRecv(connfd, sentence);
        if (judgeCommand(sentence, "SYST", 4)) {
            ServerSend(connfd, msg215);
        }
        else if (judgeCommand(sentence, "TYPE", 4)) {
            if (judgeCommand(sentence, "TYPE I", 6)) {
                ServerSend(connfd, msg2001);
            }
            else {
                ServerSend(connfd, msg504);
            }
        }
        else if (judgeCommand(sentence, "QUIT", 4)) {
            ServerSend(connfd, msg221);
            return -2;
        }
        else if (judgeCommand(sentence, "ABOR", 4)) {
            ServerSend(connfd, msg221);
            return -2;
        }
        else if (judgeCommand(sentence, "RMD", 3)) {
            if (server_RMD(connfd, sentence) < 0) {
                ServerSend(connfd, msg500);
            }
        }
        else if (judgeCommand(sentence, "PWD", 3)) {
            server_PWD(connfd, sentence);
        }
        else if (judgeCommand(sentence, "CWD", 3)) {
            if (server_CWD(connfd, sentence) < 0) {
                ServerSend(connfd, msg500);
            }
        }
        else if (judgeCommand(sentence, "MKD", 3)) {
            if (server_MKD(connfd, sentence) < 0) {
                ServerSend(connfd, msg500);
            }
        }
        else if (judgeCommand(sentence, "PORT", 4)) {
            if (server_PORT(sentence) < 0) {
                ServerSend(connfd, msg500);
            }
            else {
                mode = MPORT;
                ServerSend(connfd, msg2002);
            }
        }
        else if (judgeCommand(sentence, "LIST", 4)) {
            if (server_LIST(connfd, sentence) < 0) {
                ServerSend(connfd, msg500);
            }
        }
        else if (judgeCommand(sentence, "PASV", 4)) {
            if (server_PASV(connfd, sentence) < 0) {
                ServerSend(connfd, msg500);
            }
            else {
                mode = MPASV;
            }
        }
        else if (judgeCommand(sentence, "STOR", 4)) {
            if (server_STOR(sentence) < 0) {
                ServerSend(connfd, msg5305);
            }
            mode = MDEFAULT;
        }
        else if (judgeCommand(sentence, "RETR", 4)) {
            if (server_RETR(sentence) < 0) {
                ServerSend(connfd, msg5306);
            }
            mode = MDEFAULT;
        }
        else if (judgeCommand(sentence, "null", 4)) {
            ServerSend(connfd, "null");
        }
        else {
            ServerSend(connfd, msg500);
        }
    }
    return 0;
}

void verifyLogin(int connfd, char *sentence) {
    while(1) {
        ServerRecv(connfd, sentence);
        if(judgeCommand(sentence, "USER", 4) == 0) {
            ServerSend(connfd, msg5301);
        }
        else if (judgeCommand(sentence, "USER anonymous", 14) == 0) {
            ServerSend(connfd, msg5302);
        }
        else {
            ServerSend(connfd, msg331);
            break;
        }
    }
    while(1) {
        ServerRecv(connfd, sentence);
        if(judgeCommand(sentence, "PASS", 4) == 0) {
            ServerSend(connfd, msg5303);
        }
        else {
            int flag = 0;
            for (unsigned int i = 4; i < strlen(sentence); i++) {
                if (sentence[i] == '@') {
                    flag = 1;
                    break;
                }
            }
            if (flag) {
                ServerSend(connfd, msg230);
                break;
            }
            else {
                ServerSend(connfd, msg5304);
            }
        }
    }
}

void closeAllSocket() {
    if (server_data_socket > 0) {
        close(server_data_socket);
        server_data_socket = -1;
    }
    if (server_port_socket > 0) {
        close(server_port_socket);
        server_port_socket = -1;
    }
    if (server_listen_pasv_socket > 0) {
        close(server_listen_pasv_socket);
        server_listen_pasv_socket = -1;
    }
    if (connfd > 0) {
        close(connfd);
        connfd = -1;
    }
    if (listenfd > 0) {
        close(listenfd);
        listenfd = -1;
    }
}

int ServerRecv(int connfd, char *sentence) {
    memset(sentence, 0, MAXSIZE);
    int len = read(connfd, sentence, MAXSIZE);
    if (len < 0) {
        printf("Error read(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }
    return len;
}

int ServerSend(int connfd, const char *sentence) {
    if (write(connfd, sentence, strlen(sentence)) < 0) {
        printf("Error write(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }
    return 0;
}

int judgeCommand(char *source, const char *target, int len ) {
    if (len == 0)
        len = strlen(target);
    int flag = 1;
    for (int i = 0; i < len; i++) {
        if (source[i] != target[i]) {
            flag = 0;
        }
    }
    return flag;
}

void intToChar(int num, char *str) {
    int index = 0;
    char temp[10];
    while(num) {
        temp[index ++] = num % 10 + '0';
        num /= 10;
    }
    for (int i = 0; i < index; i++) {
        str[i] = temp[index - 1 - i];
    }
    str[index] = '\0';
}

void parseCommand(char *sentence, int len, char *cmd) {
    unsigned int index = len;
    while (sentence[index] == ' ' && index < strlen(sentence)) {
        index++;
    }
    if (index == strlen(sentence)) {
        //printf("command is invalid.\r\n");
        return;
    }
    int i = 0;
    for (; index < strlen(sentence); index++) {
        cmd[i] = sentence[index];
        i++;
    }
    cmd[i] = '\0';
}

void parseFilepath(int cmdlen, char *filepath, char *sentence) {
    parseCommand(sentence, cmdlen, filepath);
    /* 相对路径时添加工作路径 */
    if (judgeCommand(filepath, "/", 1) == 0) {
        char temp[MAXFILEPATH];
        strcpy(temp, set_path);
        int len = strlen(temp);
        temp[len++] = '/';
        temp[len] = '\0';
        strcat(temp, filepath);
        strcpy(filepath, temp);
    }
    /* 去除client输入的"\r\n" */
    filepath[strlen(filepath)-1] = '\0';
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

int server_PASV(int connfd, char *sentence) {
    struct sockaddr_in pasvaddr;
    srand((unsigned)time(NULL));
    rand_port = rand() % (65535 - 20000 + 1) + 20000;
    /* 如果旧的监听端口处于打开状态，将其关闭，以下同理 */
    if (server_listen_pasv_socket >= 0) {
        close(server_listen_pasv_socket);
        server_listen_pasv_socket = -1;
    }
    if ((server_listen_pasv_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        ServerSend(connfd, "500 Error socket()\r\n");
        return -1;
    }
    socklen_t len = sizeof(pasvaddr);
    getsockname(connfd, (struct sockaddr*)&pasvaddr, &len);
    pasvaddr.sin_port = htons(rand_port);
    if (bind(server_listen_pasv_socket, (struct sockaddr*)&pasvaddr, sizeof(pasvaddr)) == -1) {
        ServerSend(connfd, "500 Error bind()\r\n");
        close(server_listen_pasv_socket);
        server_listen_pasv_socket = -1;
        return -1;
    }
    if (listen(server_listen_pasv_socket, 10) == -1) {
        ServerSend(connfd, "500 Error listen()\r\n");
        close(server_listen_pasv_socket);
        server_listen_pasv_socket = -1;
        return -1;
    }

    len = sizeof(pasvaddr);
    getsockname(server_listen_pasv_socket, (struct sockaddr*)&pasvaddr, &len);
    /* 输出227反馈消息 */
    char msg[50] = "227 Entering Passive Mode (";
    char str[20];
    strcat(msg, inet_ntoa(pasvaddr.sin_addr));
    len = strlen(msg);
    msg[len++] = '.';
    msg[len] = '\0';
    intToChar(rand_port / 256, str);
    strcat(msg, str);
    len = strlen(msg);
    msg[len++] = '.';
    msg[len] = '\0';
    intToChar(rand_port % 256, str);
    strcat(msg, str);
    len = strlen(msg);
    msg[len++] = ')';
    msg[len++] = '\n';
    msg[len] = '\0';
    for (unsigned int i = 0; i < strlen(msg); i++) {
        if (msg[i] == '.') {
            msg[i] = ',';
        }
    }
    ServerSend(connfd, msg);
    return 0;
}

int server_PORT(char *sentence) {
    char ip[20];
    int port = 0;
    parseIpPort(sentence, 5, ip, &port);

    memset(&portaddr, 0, sizeof(portaddr));
    portaddr.sin_family = AF_INET;
    portaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &portaddr.sin_addr) <= 0) {
        printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }
    /* 如果server PORT模式端口处于打开状态，则关闭，以下同理 */
    if (server_port_socket >= 0) {
        close(server_port_socket);
        server_port_socket = -1;
    }
    if ((server_port_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Error socket(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }
    return 0;
}

int serverAccept() {
    if (server_data_socket >= 0) {
        close(server_data_socket);
        server_data_socket = -1;
    }
    if ((server_data_socket = accept(server_listen_pasv_socket, NULL, NULL)) == -1) {
        printf("Error accept(): %s(%d)\n", strerror(errno), errno);
        return -1;
    }
    return 0;
}

int serverConnect() {
    if (connect(server_port_socket, (struct sockaddr*)&portaddr, sizeof(portaddr)) < 0) {
        printf("Error connect(): %s(%d)\n", strerror(errno), errno);
        if (server_port_socket >= 0) {
            close(server_port_socket);
            server_port_socket = -1;
        }
        return -1;
    }
    return 0;
}

int serverReceiveFile(int sock, char *sentence) {
    char filepath[MAXFILEPATH];
    parseCommand(sentence, 4, filepath);
    /* 去除client输入的"\r\n"(未清除干净) */
    for (unsigned int i = 0; i < strlen(filepath); i++) {
        if (filepath[i] == '\r' || filepath[i] == '\n') {
            filepath[i] = '\0';
            break;
        }
    }
    FILE *dataFile = fopen(filepath, "wb");
    if (!dataFile) {
        ServerSend(connfd, msg552);
        return -1;
    }
    else {
        char msg[100] = "150 Opening BINARY mode data connection for ";

        strcat(msg, filepath);
        int len = strlen(msg);
        msg[len] = '\0';
        char bytes[20] = ".\r\n";
        strcat(msg, bytes);
        len = strlen(msg);
        msg[len] = '\0';
        ServerSend(connfd, msg);
        /* 接受客户端的文件消息并保存 */
        memset(sentence, 0, MAXSIZE);
        int datasize = 0;
        while (1) {
            if ((datasize = ServerRecv(sock, sentence)) > 0) {
                fwrite(sentence, 1, datasize, dataFile);
            }
            else if (datasize < 0) {
                return -1;
            }
            else {
                break;
            }
        }
        ServerSend(connfd, msg226);
    }
    fclose(dataFile);
    return 0;
}

int serverTransferFile(int sock, char *sentence) {
    char filepath[MAXFILEPATH];
    parseCommand(sentence, 4, filepath);
    /* 去除client输入的"\r\n"(未清除干净) */
    for (unsigned int i = 0; i < strlen(filepath); i++) {
        if (filepath[i] == '\r' || filepath[i] == '\n') {
            filepath[i] = '\0';
            break;
        }
    }
    FILE *dataFile = fopen(filepath, "rb");
    if (!dataFile) {
        ServerSend(connfd, msg551);
        return -1;
    }
    else {
        /* 发送150反馈消息 */
        fseek(dataFile, 0, SEEK_END); //定位到文件末
        int length = ftell(dataFile); //文件长度
        fseek(dataFile, 0, 0);   //定位到文件首
        char str[20];
        intToChar(length, str);
        char msg[100] = "150 Opening BINARY mode data connection for ";
        strcat(msg, filepath);
        int len = strlen(msg);
        msg[len++] = ' ';
        msg[len++] = '(';
        msg[len] = '\0';
        strcat(msg, str);
        char bytes[20] = " bytes).\r\n";
        strcat(msg, bytes);
        len = strlen(msg);
        msg[len] = '\0';
        ServerSend(connfd, msg);
        /* 读取文件并发送给客户端 */
        int datasize = 0;
        memset(sentence, 0, MAXSIZE);
        do {
            datasize = fread(sentence, 1, MAXSIZE, dataFile);
            if (datasize < 0) {
                ServerSend(connfd, msg451);
                close(sock);
                sock = -1;
                if (mode == MPASV) {
                    close(server_listen_pasv_socket);
                    server_listen_pasv_socket = -1;
                }
                return -1;
            }
            else if (datasize == 0) {
                break;
            }
            if (write(sock, sentence, datasize) < 0) {
                ServerSend(connfd, msg426);
                close(sock);
                sock = -1;
                if (mode == MPASV) {
                    close(server_listen_pasv_socket);
                    server_listen_pasv_socket = -1;
                }
                return -1;
            }
        } while (datasize > 0);
        ServerSend(connfd, msg226);
    }
    fclose(dataFile);
    return 0;
}

int server_RETR(char *sentence) {
    if (mode == MPORT) {
        if (serverConnect() < 0) {
            ServerSend(connfd, msg425);
            close(server_port_socket);
            server_port_socket = -1;
            return -1;
        }
        if (serverTransferFile(server_port_socket, sentence) < 0) {
            close(server_port_socket);
            server_port_socket = -1;
            return -1;
        }
        close(server_port_socket);
        server_port_socket = -1;
    }
    else if (mode == MPASV) {
        if (serverAccept() < 0) {
            ServerSend(connfd, msg425);
            close(server_data_socket);
            server_data_socket = -1;
            close(server_listen_pasv_socket);
            server_listen_pasv_socket = -1;
            return -1;
        }
        if (serverTransferFile(server_data_socket, sentence) < 0) {
            close(server_data_socket);
            server_data_socket = -1;
            close(server_listen_pasv_socket);
            server_listen_pasv_socket = -1;
            return -1;
        }
        close(server_data_socket);
        server_data_socket = -1;
        close(server_listen_pasv_socket);
        server_listen_pasv_socket = -1;
    }
    else {
        ServerSend(connfd, msg503);
    }
    return 0;
}

int server_STOR(char *sentence) {
    if (mode == MPORT) {
        if (serverConnect() < 0) {
            ServerSend(connfd, msg425);
            close(server_port_socket);
            server_port_socket = -1;
            return -1;
        }
        if (serverReceiveFile(server_port_socket, sentence) < 0) {
            close(server_port_socket);
            server_port_socket = -1;
            return -1;
        }
        close(server_port_socket);
        server_port_socket = -1;
    }
    else if (mode == MPASV) {
        if (serverAccept() < 0) {
            ServerSend(connfd, msg425);
            close(server_data_socket);
            server_data_socket = -1;
            close(server_listen_pasv_socket);
            server_listen_pasv_socket = -1;
            return -1;
        }
        if (serverReceiveFile(server_data_socket, sentence) < 0) {
            close(server_data_socket);
            server_data_socket = -1;
            close(server_listen_pasv_socket);
            server_listen_pasv_socket = -1;
            return -1;
        }
        close(server_data_socket);
        server_data_socket = -1;
        close(server_listen_pasv_socket);
        server_listen_pasv_socket = -1;
    }
    else {
        ServerSend(connfd, msg503);
    }
    return 0;
}

int server_LIST(int connfd, char *sentence) {
    /* 建立数据连接 */
    /*if (server_PASV(connfd, sentence) < 0) {
        ServerSend(connfd, msg500);
        return -1;
    }*/
    if (serverAccept() < 0) {
        ServerSend(connfd, msg425);
        close(server_data_socket);
        server_data_socket = -1;
        close(server_listen_pasv_socket);
        server_listen_pasv_socket = -1;
        return -1;
    }
    /* 利用系统ls将目录下列表保存到filelist.txt */
    if(system("ls -l | tail -n+2 > /var/filelist.txt") < 0){
        ServerSend(connfd, msg451);
        return -1;
    }
    FILE* dataFile = fopen("/var/filelist.txt","r");
    if(!dataFile) {
       ServerSend(connfd, msg451);
       return -1;
    }
    else {
        size_t datasize;
        ServerSend(connfd, msg150);
        memset(sentence, 0, MAXSIZE);
        do {
            datasize = fread(sentence, 1, MAXSIZE, dataFile);
            if (write(server_data_socket, sentence, datasize) < 0) {
                ServerSend(connfd, msg426);
                close(server_data_socket);
                server_data_socket = -1;
                close(server_listen_pasv_socket);
                server_listen_pasv_socket = -1;
                return -1;
            }
        } while (datasize > 0);
        ServerSend(connfd, msg226);
    }
    fclose(dataFile);
    system("rm /var/filelist.txt");
    close(server_data_socket);
    server_data_socket = -1;
    close(server_listen_pasv_socket);
    server_listen_pasv_socket = -1;
    return 0;
}

void server_PWD(int connfd, char *sentence) {
    char curdir[MAXFILEPATH] = "257 ";
    strcat(curdir, set_path);
    strcat(curdir, "\r\n");
    ServerSend(connfd, curdir);
}

int server_MKD(int connfd, char *sentence) {
    char filepath[MAXFILEPATH];
    parseFilepath(3, filepath, sentence);
    ///////server_PWD(connfd, sentence);
    if (access(filepath, 0) == 0) {
        ServerSend(connfd, msg5501);
        return -1;
    }
    else if (mkdir(filepath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0) {
        ServerSend(connfd, msg250);
    }
    else {
        ServerSend(connfd, msg550);
        return -1;
    }
    return 0;
}

int server_RMD(int connfd, char *sentence) {
    char filepath[MAXFILEPATH];
    parseFilepath(3, filepath, sentence);

    if (rmdir(filepath) == 0) {
        ServerSend(connfd, msg250);
    }
    else {
        ServerSend(connfd, msg550);
        return -1;
    }
    return 0;
}

int server_CWD(int connfd, char *sentence) {
    char filepath[MAXFILEPATH];
    parseFilepath(3, filepath, sentence);
    if (chdir(filepath) == 0) {
        memset(set_path, 0, MAXFILEPATH);
        strcpy(set_path, filepath);
        ServerSend(connfd, msg250);
    }
    else {
        ServerSend(connfd, msg550);
        return -1;
    }
    return 0;
}
