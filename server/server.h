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

/* server返回信息 */
const char *msg150 = "150 Opening BINARY mode data connection\r\n";
const char *msg2001 = "200 Type set to I.\r\n";
const char *msg2002 = "200 PORT command successful.\r\n";
const char *msg215 = "215 UNIX Type: L8\r\n";
const char *msg220 = "220 Anonymous FTP server ready.\r\n";
const char *msg221 = "221-Thank you for using the FTP service on Anonymous.\r\n221 Goodbye.\r\n";
const char *msg226 = "226 Transfer complete.\r\n";
const char *msg227 = "227 Entering Passive Mode (";
const char *msg230 = "230-\r\n230-Welcome to\r\n230-School of Software\r\n230-FTP Archives at Anonymous\r\n230-\r\n230-This site is provided as a public service by School of\r\n230-Software. Use in violation of any applicable laws is strictly\r\n230-prohibited. We make no guarantees, explicit or implicit, about the\r\n230-contents of this site. Use at your own risk.\r\n230-\r\n230 Guest login ok, access restrictions apply.\r\n";
const char *msg250 = "250 Okay\r\n";
const char *msg331 = "331 Guest login ok, set your complete e-mail address as password.\r\n";
const char *msg425 = "425 No TCP connection was established.\r\n";
const char *msg426 = "426 connection was broken or network failure.\r\n";
const char *msg451 = "451 Error when reading the file from disk.\r\n";
const char *msg500 = "500 error command.\r\n";
const char *msg503 = "503 No mode is estabished(PORT/PASV).\r\n";
const char *msg504 = "504 Wrong TYPE command.\r\n";
const char *msg5301 = "530 the command is invalid, try to use <USER>.\r\n";
const char *msg5302 = "530 All users other than anonymous are not supported, please input again.\r\n";
const char *msg5303 = "530 the command is invalid, try to use <PASS>.\r\n";
const char *msg5304 = "530 password is invalid, set your complete e-mail address as password.\r\n";
const char *msg5305 = "530 Error save file\r\n";
const char *msg5306 = "530 Error transfer file\r\n";
const char *msg550 = "550 error.\r\n";
const char *msg5501 = "550 the file has existed.\r\n";
const char *msg551 = "551 No such file or dictionary.\r\n";
const char *msg552 = "552 the server had trouble saving the file to disk.\r\n";

/* 
    set_path: 默认server工作路径 格式：-root set_path
    set_port: 默认server端口    格式： -port: set_port
*/
char set_path[80] = "/tmp";
unsigned int set_port = 21;

/* 
    server_listen_pasv_socket： PASV模式监听socket 
    server_data_socket： PASV模式accept客户端后建立的数据连接socket
    server_port_socket：PORT模式socket
*/
int server_listen_pasv_socket = -1;
int server_data_socket = -1;
int server_port_socket = -1;

/* 
    listenfd: server最初建立的监听socket
    connfd: server最初accept客户端后建立的连接socket
*/
int listenfd = -1;
int connfd = -1;

/* PORT模式server的通信地址 */
struct sockaddr_in portaddr;

/* 
    MODE模式：默认MDEFAULT
    MDEFAULT：默认模式，最初建立的通信连接
    MPORT：PORT模式
    MPASV：PASV模式
*/
typedef enum { MDEFAULT, MPORT, MPASV } MODE;
MODE mode = MDEFAULT;

/* PASV模式中随机端口的初始值 */
unsigned int rand_port = 20001;

/***************************************************/

/* server的所有操作汇总 */
int serverOperation(int connfd, char *sentence);

/* server验证用户登录 */
void verifyLogin(int connfd, char *sentence);

/* server关闭所有socket */
void closeAllSocket();

/* server接受客户端消息 */
int ServerRecv(int connfd, char *sentence);

/* server给客户端发送消息 */
int ServerSend(int connfd, const char *sentence);

/* 判断server接受的命令 */
int judgeCommand(char *source, const char *target, int len );

/* int转为char数组 */
void intToChar(int num, char *str);

/* 解析带参数的命令中的参数部分 */
void parseCommand(char *sentence, int len, char *cmd);

/* 解析得到参数中的文件路径 */
void parseFilepath(int cmdlen, char *filepath, char *sentence);

/* 解析PORT参数中的ip和port */
void parseIpPort(char *sentence, int len, char *ip, int *port);

/* server接受PASV命令，在server端进行socket绑定，建立监听 */
int server_PASV(int connfd, char *sentence);

/* server接受PORT命令，建立socket，等待与客户端建立连接 */
int server_PORT(char *sentence);

/* server在建立PASV模式后当客户端发起connect时进行accept连接 */
int serverAccept();

/* server在建立PORT模式时向客户端发起connect请求 */
int serverConnect();

/* server端接受文件 */
int serverReceiveFile(int sock, char *sentence);

/* server端传输文件给客户端 */
int serverTransferFile(int sock, char *sentence);

/* server建立PORT/PASV模式后接受RETR命令进行文件下载 */
int server_RETR(char *sentence);

/* server建立PORT/PASV模式后接受STOR命令进行文件上传 */
int server_STOR(char *sentence);

/* server接收LIST命令 */
int server_LIST(int connfd, char *sentence);

/* server接收PWD命令 */
void server_PWD(int connfd, char *sentence);

/* server接收MKD命令 */
int server_MKD(int connfd, char *sentence);

/* server接收RMD命令 */
int server_RMD(int connfd, char *sentence);

/* server接收CWD命令 */
int server_CWD(int connfd, char *sentence);
