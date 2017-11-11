# TcpSessionMgr
 * 一个简单高效的TCP会话管理器。
 * 用session id 区分每一个tcp连接会话, 底层网络事件读写基于高性能 libswevent 库.
 * 几分钟的移植时间就可让你的程序拥有高效率处理网络会话功能，包括侦听客户端连接、主动连接其他服务器，并将tcp会话统一管理。

# 依赖库
  libswevent

# 使用
  安装完依赖库后，将TcpSession.h 和 TcpSession.cpp 拷贝至自己的工程目录，编译选项用 -std=c++11

