# TcpSessionMgr
 * 一个简单高效的TCP会话管理器。
 * 用session id 区分每一个tcp连接会话, 底层网络事件读写基于高性能 libswevent 库.
 * 几分钟的移植时间就可让你的程序拥有高效率处理网络会话功能，包括侦听客户端连接、主动连接其他服务器，并将tcp会话统一管理。

# 依赖库
  libswevent

# 使用
  安装完依赖库后，将TcpSession.h 和 TcpSession.cpp 拷贝至自己的工程目录，编译选项用 -std=c++11 。

  需要你自己补全的内容：
  
  1. 网络协议解码程序，其实大多数情况下TcpSessionMgr自带的DefaultMsgDecoder已经够用，如需定制自己的协议解码程序，从MsgDecoderBase继承一个实现自己的解码逻辑即可。
  2. 在子类中重写以下三个函数
 
    TcpSessionManager::OnSessionHasBegin(TcpSession *); // 在这里增加sessionid和用户对象的map
    TcpSessionManager::OnSessionWillEnd(TcpSession *);  // 在这里解除sessionid和用户对象的map
    TcpSessionManager::ProcessMessage(TcpSession *, const char *, unsigned);  // 处理各个请求消息，并做应答

  
