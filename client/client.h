/* 包含的库 */
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>

/* 缓冲区最大值 */
#define MAXSIZE 8192

/* 路径长度最大值 */
#define MAXFILEPATH 80

/*
    set_ip: client默认的连接ip     格式：-ip set_ip
    set_port: client默认的连接端口  格式：-port set_port
*/
char set_ip[20] = "127.0.0.1";
unsigned int set_port = 21;

/* 
    client_listen_port_socket PORT模式监听socket 
    client_data_socket PORT模式接受server的连接请求后建立的数据连接socket
    client_pasv_socket: PASV模式socket
    sockfd: 客户端最初建立的socket
*/
int client_listen_port_socket = -1;
int client_data_socket = -1;
int client_pasv_socket = -1;
int sockfd = -1;

/* PASV模式client的通信地址 */
struct sockaddr_in pasvaddr;

/* 
    MODE模式：默认MDEFAULT
    MDEFAULT：默认模式，最初建立的通信连接
    MPORT：PORT模式
    MPASV：PASV模式
*/
typedef enum { MDEFAULT, MPORT, MPASV } MODE;
MODE mode = MDEFAULT;

/* client是否一条recv接收了server多条消息,防错而设，可忽略 */
int multireceive = -1;    

/***************************************************/

/* client命令操作汇总 */
int clientOperation(int sockfd, char *sentence);

/* 客户端用户登录 */
void UserLogin(int sockfd, char *sentence);

/* client关闭所有socket */
void closeAllSocket();

/* 在登录FTP服务器前从命令行读取命令 */
void readCommand(char *sentence);

/* 在登录FTP服务器后从命令行读取命令 */
void readFtpCmd(char *sentence);

/* 客户端向服务器发送命令 */
int ClientSend(int sockfd, const char *sentence);

/* 客户端接收来自服务器的反馈消息 */
int ClientRecv(int sockfd, char *sentence);

/* 判断source最开始部分是否包含target，是返回1，否则返回0 */
int judgeSubStr(char *source, const char *target);


/* 解析带参数的命令中的参数部分 */
void parseCommand(char *sentence, int len, char *cmd);

/* 解析PORT参数中的ip和port */
void parseIpPort(char *sentence, int len, char *ip, int *port);

/* client进入PASV模式，本地绑定socket，等待与server建立连接*/
int client_PASV(int sockfd, char *sentence);

/* client进入PORT模式，本地绑定socket并建立监听 */
int client_PORT(char *sentence);

/* client在建立PASV模式时向server发起connect请求 */
int clientConnect();

/* client在建立PORT模式后当server发起connect时进行accept连接 */
int clientAccept();

/* client接收来自server的文件 */
int clientReceiveFile(int sock, char *sentence);

/* client向server传输文件 */
int clientTransferFile(int sock, char *sentence);

/* client在PORT/PASV模式中接受RETR命令进行文件下载 */
int client_RETR(char *sentence);

/* client在PORT/PASV模式中接受STOR命令进行文件上传 */
int client_STOR(char *sentence);

/* client接收LIST命令 */
int client_LIST(int sockfd, char *cmd, char *sentence);

/* client接收MKD命令 */
int client_MKD(int sockfd, char *sentence);

/* client接收PWD命令 */
int client_PWD(int sockfd, char *sentence);

/* client接收RMD命令 */
int client_RMD(int sockfd, char *sentence);

/* client接收CWD命令 */
int client_CWD(int sockfd, char *sentence);
