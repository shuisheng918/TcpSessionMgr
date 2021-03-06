# TcpSessionMgr
 * 一个简单高效的TCP会话管理器
 * 用session id 区分每一个tcp连接会话, 底层网络事件读写基于高性能 libswevent 库
 * 全部非阻塞socket操作，拥有高效率处理网络会话功能，包括侦听客户端连接、主动连接其他服务器，并将tcp会话统一管理
 * 支持普通的带头长字段的二进制流数据包会话
 * 支持http服务端和客户端会话
 * 支持ssl加密的tcp会话
 * 支持https服务端和客户端会话

# 依赖库
 * libswevent
 * openssl

# 运行环境
 * 建议linux下使用，windows或其他平台需要自行修改少许代码。

# 使用
 * 安装完依赖库后，把 *h *.cpp文件都拷贝至自己的工程目录，编译选项加上"-std=c++11",连接选项加上"-lswevent"
 * 如果启用安全加密的会话，编译选项加上"-DENABLE_SSL"，连接选项加上"-lssl"即可. 可参考samples目录中的实例代码
 * 多线程环境下使用时请注意libswevent和openssl上下文的线程安全性
  
