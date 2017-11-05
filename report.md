#**REPORT**

​                                                                                                                                               *name*: 侯建国    *student_id*: 2015013212

##**一. FTP介绍**

### 一. 开发/运行环境：

开发系统：Ubuntu 16.04 LTS       开发语言：C语言

### 二. 框架：

基本框架见下图（可能存在不完善或者表述不清的地方）：

最上面为最初建立的client和server的连接模型（当然也可以作为PORT或PASV模式的正向或反向的连接模型）；紧接着是client和server的交互循环模型；最下面是每个命令的实现（橙色块代表命令的实现函数）；左右两侧：server会解析cmd，client会判断返回码（retrcode），而只有在客户端登录（UserLogin），服务器成功验证（verifyLogin）之后才能进行所有命令的使用。

![框架](C:\Users\hjg\Desktop\2017年秋季学期\计算机与网络体系结构(1)\框架.png)

##**二. FTP COMMANDS介绍**

###1. **USER/PASS** COMMANDS

FTP客户端在登录之后，会发送给用户"220 Anonymous FTP server ready. "，告诉用户已经成功进入到FTP，用户需要用"USER anonymous"来进行登录，如果不正确则循环该命令直到正确（即为登录状态不可访问FTP服务器内任何内容）；"USER"命令成功后要求用户继续输入"PASS password"（password中必须包含'@'）后才能成功登录。

###2. **SYST** COMMANDS

用户输入"SYST"后，FTP服务器返回一个"215 UNIX Type: L8"信息给用户。

###3. **TYPE** COMMANDS

当用户输入"TYPE I"之后，系统返回"200 Type set to I."；当用户输入其他"TYPE others"则会返回一个错误码，告知没有此条命令。

###4. **QUIT/ABOR** COMMANDS

当用户输入QUIT/ABOR命令后，将会输出"221-Thank you forusing the FTP service on Anonymous.\r\n221 Goodbye."告知用户已退出FTP服务器；输入"ABOR"同理。

###5. **MKD** COMMANDS:

用户输入MKD filepath之后，系统给用户返回FTP当前所处的工作路径；当系统成功创建用户所指定目录后，系统返回250，否则返回550。

###6. **CWD** COMMANDS:

用户输入CWD filepath后，系统根据指定目录改变当前工作路径，成功返回250，否则返回550。

###7. **LIST**  COMMANDS:

用户输入LIST后，系统会创建一条数据端口与用户连接，之后系统检索当前工作路径下的文件或目录，并依次通过数据端口返回给用户，显示在client，最后系统会自动将数据端口关闭。

###8. **RMD** COMMANDS:

用户输入RWD filepath后，系统会根据所给参数删除指定内容，成功返回250，否则返回550。

###9. **PORT** COMMANDS:

用户输入PORT ipaddr+port（以","分隔）后系统将命令解析为ip和port，并在系统上建立socket，等待进一步RETR/STOR命令进入，之后关闭连接。

###10. **PASV** COMMANDS:

用户输入PASV后，系统会以自身服务器的ip地址和一个随机（20000 – 65536）的port建立socket，并建立起监听，然后返回给用户"227 Entering Passive Mode (ip+port)"（仍然以","分隔），之后等待客户端的connect进行连接。

###11. **RETR** COMMANDS:

在建立起PORT或者PASV模式后，用户输入RETR filename后，系统和客户端尝试建立连接，连接成功后系统在其指定目录下打开文件，成功打开后返回给客户端"150 Opening BINARY mode data connection for filename (n bytes)"，然后系统将文件内容读取发送给客户端，同时客户端在本地建立文件，写入系统发送来的文件内容。这一切完成之后系统关闭此数据连接。当在非PORT/PASV模式下输入RETR命令，系统会返回"503 No mode isestabished(PORT/PASV). "；其它非成功情况详见文档叙述。

###12. **STOR** COMMANDS:

STOR与RETR命令类似，在PORT/PASV模式下输入STOR filename，系统和客户端建立连接后打开二进制数据传输模式，将客户端的文件传输到FTP服务器端并进行保存，然后关闭连接。

###13. **MULTI-CLIENT** SUPPORT:

该FTP是通过使用fork建立的多线程来进行多客户端登录。

##**三. 困难/感想**

在差不多ddl前一周开始思考这次的FTP该如何去写，刚开始翻看了文档，然后看课本第二章关于udp和tcp的讲解，最后翻看助教上课讲的socket课件，结果是我只把udp粗糙地实现了，tcp看了很长时间还是没搞懂。之后通过网上各种讲解以及ftp源码实现去进行理解才逐渐有了自己的感悟，我想说的是一个本身未接触过的事物切勿急躁，要细嚼慢咽、循序渐进地理解，才能获得更有效的成果。

第二个困难点事理解PORT/PASV和RETR/STOR这几组的关系上，刚开始上网查资料竟然有各种误导，和文档的理解是相反的，我当时一筹莫展，不知道PORT/PASV到底是怎样实现的，不得已我请教了大神后，才理解了这两种模式是谁主动连接谁，之后的RETR/STOR理解就快的多了，实际上就是读写文件的操作，只不过是披上了“连接”的外皮。

在写那几条简略命令（MKD/CWD等）时遇到了缓冲区的问题，当时这个问题围绕了我一整天，处于一种莫名其妙的挖坑埋坑的状态，之后上网查清楚是缓冲区未刷新导致的缓冲区输出错位，然后尝试用"\r\n"解决了问题，但是曾经遇到过加上printf("invalidinformation")就会输出正确，去掉客户端就不会更新"ftp> "导致无法输入命令的错误（之后莫名其妙不见了），我猜测可能还是缓冲区的问题吧。

还有一个困难就是测试的时候，写好makefile，运行autograde直接报错，0 credit，然后各种调试代码消除问题，最开始的时连接不上server，和搞定一个vsftpd问题后，又发现了原代码在设定端口时没有用htons导致连接端口一直错位；另一个是linux的文件权限问题，利用sudo创建的文件夹或者文件都是有权限的，用c的fopen打不开，不断报错"551 No such file ordictionary."，不得已我只能注释测试代码，自创文件一个一个测试，经过测试后发现是正确的。

我觉得这次实验对我来说较难的是对于文档的理解，我通读n次文档，每次都有新的“理解与体会”，比如在写客户端的时候，每次因为理解的不同不得不重新改代码，以后还需要加强语文理解的能力……

最后感谢老师和助教留下的FTP作业，让我对命令行以及文件传输系统和FTP协议有了更深一层的理解。