#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    db = QSqlDatabase::addDatabase("QODBC");
    QString dsn = "DRIVER={SQL Server};"
                  "SERVER=HUASHUO4;"
                  "DATABASE=QQ_LITE;"
                  "Trusted_Connection=Yes;";
    db.setDatabaseName(dsn);
    // 打开数据库
    if (!db.open()) {
        qDebug() << "数据库连接失败:" ;
    } else {
        qDebug() << "数据库已连接";
    }
    server=new QTcpServer;
    //服务器监听
    server->listen(QHostAddress::AnyIPv4,PORT);
    //客户端发起连接，server发出信号
    connect(server,&QTcpServer::newConnection,this,&Widget::newclinethandler);
}

//客户端连接
void Widget::newclinethandler()
{
    QTcpSocket *socket=server->nextPendingConnection();
    socket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption,10 * 1024 * 1024);
    if(!socket)
        return;
    // 保存客户端socket
    clientSockets.insert(socket);
    //监听客户端发送的消息
    connect(socket,&QTcpSocket::readyRead, this, &Widget::receiveMessage);
    //监听客户端状态
    connect(socket, &QTcpSocket::disconnected, this, &Widget::clientDisconnected);
    qDebug() << "新客户端连接:" << socket->peerAddress().toString();
}

//服务端监听
void Widget::receiveMessage()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;
    //读取数据
    QByteArray data = socket->readAll();
    //qDebug()<<"未转码:"<<data;
    //转换编码
    QString message = QString::fromUtf8(data);
    //qDebug()<<"已转码:"<<message;
    //获取当前时间
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    ui->textEdit->append(QString("[%1] 客户端 %2 说：%3")
                             .arg(timestamp)
                             .arg(userid)
                             .arg(message));

    //注册事件
    if(message.startsWith("NEWACCOUNT:"))
    {
        //跳过NEWACCOUNT:
        QString content = message.mid(11);
        QStringList parts = content.split('|');
        QString account=parts[0];
        QString passwd=parts[1];
        QString id=parts[2];
        savedate(account,passwd,id);
        QString response;
        //成功插入，返回响应
        if(issave==true)
        {
            qDebug()<<"发送OK";
            response="REGISTER_ACK:OK";
        }
        //插入失败返回响应
        else
        {
            qDebug()<<"发送NO";
            response="REGISTER_ACK:NO";
        }
        ui->textEdit->append(QString("[%1] 回复客户端 %2：%3")
                                 .arg(timestamp)
                                 .arg(userid)
                                 .arg(response));
        socket->write(response.toUtf8()); // 发送响应
        socket->flush();
        return;
    }

    //客户端发起登录账号事件
    if(message.startsWith("MY_LOGIN:"))
    {
        QString content = message.mid(9);
        QStringList parts = content.split('|');
        QString account=parts[0];
        QString passwd=parts[1];
        qDebug()<<"已经收到客户端发来的账户信息:"<<account<<"|"<<passwd;
        QSqlQuery query(db);
        //从数据库中查看是否有该账号
        QString sql = "SELECT UserID FROM [QQ_LITE].[dbo].[UserInfo] WHERE Account = :account AND Password = :password";
        query.prepare(sql);
        query.bindValue(":account", account);
        query.bindValue(":password", passwd);
        if (query.exec())
        {
            if (query.next())
            {
                QString response;
                QString *msgPtr = pendingMsgPtrs[account];
                int UID = query.value(0).toInt();
                if (UID > 0)
                {
                    // 账号密码匹配
                    qDebug() << "账号密码匹配成功";
                    qDebug()<<"UserID"<<account;
                    response="SIGN-IN PROMPTS:OK";
                    onlineUsers.insert(account);
                    // 建立 socket 与 account 的映射
                    socketToUserID.insert(socket,account);
                    //userIdToSocket[account]=socket;
                    //显示在线的客户端
                    ShowOnlineUser();
                }
                else
                {
                    // 账号密码不匹配
                    qDebug() << "账号密码不匹配";
                    response="SIGN-IN PROMPTS:NO";
                }
                socket->write(response.toUtf8());
                socket->flush();
                QTimer *timer = new QTimer(this);
                timer->setSingleShot(true);
                connect(timer, &QTimer::timeout, [=]()
                        {
                            if (UID > 0)
                            {
                                if(msgPtr!=nullptr&&socket->state()==QAbstractSocket::ConnectedState)
                                {
                                    socket->write((*msgPtr).toUtf8());
                                    socket->flush();
                                    qDebug()<<"发送离线消息"<<*msgPtr;
                                    pendingMsgPtrs.remove(account);
                                    delete msgPtr;
                                }
                                else if (socket->state() != QAbstractSocket::ConnectedState)
                                {
                                    qDebug()<<"Socket 未连接，无法发送离线消息";
                                }
                                else
                                    qDebug()<<"无待发送离线消息";
                            }
                            timer->deleteLater();
                        });
                timer->start(1000);
                QTimer *timers = new QTimer(this);
                timers->setSingleShot(true);
                connect(timers, &QTimer::timeout, [=]()
                        {
                            QStringList friendaccounts=FriendGroup(account);
                            qDebug() << "查询到的FriendAccount列表：" << friendaccounts;
                            QString friendsStr = friendaccounts.join("|");
                            QString yourfriend = QString("YOUR_FRIEND:%1").arg(friendsStr);
                            socket->write(yourfriend.toUtf8());
                            socket->flush();
                            timers->deleteLater();
                        });
                timers->start(2000);
                QTimer *timerss = new QTimer(this);
                timerss->setSingleShot(true);
                connect(timerss, &QTimer::timeout, [=]()
                        {
                            broadcastOnlineUsers();
                            //onlineaccount(socket);
                            timerss->deleteLater();
                        });
                timerss->start(3000);
            }
        }
        else
            qDebug() << "查询执行失败: " ;
        return;
    }

    //群发消息
    if(message.startsWith("SEND_BROADCAST_MESSAGE:"))
    {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        QString content=message.mid(23);
        QStringList parts=content.split('|');
        qDebug()<<parts;
        if(parts.size()==2)
        {
            QString account=parts[0];
            QString information=parts[1];
            QStringList Friendgroup=FriendGroup(account);
            for(int i=0;i<Friendgroup.size();i++)
            {
                QString informations=QString("SEND_A_MESSAGE_TO:%1|%2|%3").arg(account).arg(information).arg(Friendgroup[i]);
                QString messages=informations.toUtf8();
                send_A_Message(messages,socket);
            }
        }
    }

    //修改密码操作
    if(message.startsWith("CHANGEPASSWD:"))
    {
        qDebug()<<"收到修改密码请求";
        //跳过NEWACCOUNT:
        QString content = message.mid(13);
        QStringList parts = content.split('|');
        QString UID=parts[0];
        QString passwd=parts[1];
        qDebug()<<"用户UID"<<UID;
        qDebug()<<"用户密码"<<passwd;
        QSqlQuery query(db);
        QString sql="SELECT COUNT(*) FROM [QQ_LITE].[dbo].[UserInfo] WHERE UserID = :UID";
        query.prepare(sql);
        query.bindValue(":UID",UID);
        if(query.exec())
        {
            if (query.next())
            {
                QString response;
                response="PASSWORD_RESET:NO";
                int count = query.value(0).toInt();
                if (count > 0)
                {
                    qDebug()<<"找到了该用户";
                    QSqlQuery updateQuery(db);
                    QString updateSql = "UPDATE [QQ_LITE].[dbo].[UserInfo] "
                                        "SET Password = :passwd "
                                        "WHERE UserID = :UID";
                    updateQuery.prepare(updateSql);
                    updateQuery.bindValue(":passwd",passwd);
                    updateQuery.bindValue(":UID",UID);
                    response="PASSWORD_RESET:NO";
                    if(updateQuery.exec())
                    {
                        qDebug()<<"密码修改成功";
                        response="PASSWORD_RESET:OK";
                    }
                    else
                        qDebug()<<"修改失败";
                }
                else
                    qDebug()<<"未找到该用户";
                socket->write(response.toUtf8());
                socket->flush();
            }
        }
        return;
    }

    //转发消息
    if(message.startsWith("SEND_A_MESSAGE_TO:"))
    {
        send_A_Message(message,socket);
    }

    //添加好友
    if(message.startsWith("ADD_FRIEND:"))
    {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        QString content = message.mid(11);
        QStringList parts = content.split('|');
        if(parts.size()==2)
        {
            QString sender=parts[0];
            QString friends=parts[1];
            qDebug()<<sender;
            qDebug()<<friends;
            QSqlQuery query(db);
            //从数据库中查看是否有该账号
            QString sql = "SELECT UserID FROM [QQ_LITE].[dbo].[UserInfo] WHERE Account = :account ";
            QString response;
            query.prepare(sql);
            query.bindValue(":account", friends);
            if (query.exec())
            {
                if (query.next())
                {
                    QSqlQuery querys(db);
                    //从数据库中查看是否有该账号
                    QString sql = "SELECT Account FROM [QQ_LITE].[dbo].[Friendships] WHERE Account = :account AND FriendAccount= :friendaccount AND FriendshipStatus=1";
                    querys.prepare(sql);
                    querys.bindValue(":account", sender);
                    querys.bindValue(":friendaccount", friends);
                    if (querys.exec())
                    {
                        if (querys.next())
                        {
                            qDebug()<<"用户"<<sender<<"与用户"<<friends<<"已经是好友了";
                            socket->write("ADD_FRIEND_:OK");
                            return;
                        }
                    }
                    QSqlQuery queryss(db);
                    QString sqll = "SELECT Account FROM [QQ_LITE].[dbo].[Friendships] WHERE Account = :account AND FriendAccount= :friendaccount AND FriendshipStatus=1";
                    queryss.prepare(sql);
                    queryss.bindValue(":account", friends);
                    queryss.bindValue(":friendaccount", sender);
                    if (queryss.exec())
                    {
                        if (queryss.next())
                        {
                            qDebug()<<"用户"<<sender<<"与用户"<<friends<<"已经是好友了";
                            socket->write("ADD_FRIEND_:OK");
                            return;
                        }
                    }
                    qDebug()<<"找到了该用户"<<friends;
                    response="ADD_FRIEND:OK";
                    QSqlQuery insertQuery(db);
                    QString insertSql = "INSERT INTO [QQ_LITE].[dbo].[Friendships] (Account, FriendAccount, FriendshipStatus) VALUES (:account, :friendaccount, 0)";
                    insertQuery.prepare(insertSql);
                    insertQuery.bindValue(":account", sender);
                    insertQuery.bindValue(":friendaccount", friends);
                    if(insertQuery.exec())
                        qDebug()<<"好友关系（待确认）插入成功";
                }
                else
                {
                    qDebug()<<"该用户"<<friends<<"不存在";
                    response="ADD_FRIEND:NO";
                }
            }
            socket->write(response.toUtf8());
            socket->flush();
            QString responses=QString("ADD_FRIEND_REQUEST:%1|%2").arg(sender).arg(friends);
            QTcpSocket *receiverSocket = nullptr;
            for (auto it = socketToUserID.begin(); it != socketToUserID.end(); ++it)
            {
                if (it.value() == friends)
                {
                    receiverSocket = it.key();
                    break;
                }
            }
            if (receiverSocket)
            {
                qDebug()<<"开始转发好友请求";
                receiverSocket->write(responses.toUtf8());
            }
        }
        return;
    }

    //确认添加好友
    if(message.startsWith("ADD_FRIEND_AGREE_TO_THE_REQUEST:"))
    {
        QString content = message.mid(32);
        QStringList parts = content.split('|');
        if(parts.size()==2)
        {
            QString user1=parts[0];
            QString user2=parts[1];
            qDebug()<<"确认用户"<<user1<<"和用户"<<user2<<"添加好友";
            QSqlQuery updateQuery1(db);
            QString updateSql1 = "UPDATE [QQ_LITE].[dbo].[Friendships] SET FriendshipStatus = 1 WHERE Account = :account AND FriendAccount = :friendaccount";
            updateQuery1.prepare(updateSql1);
            updateQuery1.bindValue(":account", user1);
            updateQuery1.bindValue(":friendaccount", user2);
            if (updateQuery1.exec())
            {
                QString yy=QString("ADD_FRIEND_AGREE:%1").arg(user1);
                qDebug()<<yy;
                socket->write(yy.toUtf8());
                socket->flush();
                qDebug() << "user1 与 user2 的好友关系状态更新成功";
                QString responses=QString("ADD_FRIEND_R:%1").arg(user2);
                qDebug()<<responses;
                QTcpSocket *receiverSocket = nullptr;
                for (auto it = socketToUserID.begin(); it != socketToUserID.end(); ++it)
                {
                    if (it.value() == user1)
                    {
                        receiverSocket = it.key();
                        break;
                    }
                }
                if (receiverSocket)
                {
                    qDebug()<<"开始转发好友请求回复";
                    receiverSocket->write(responses.toUtf8());
                }
            }
            else
                qDebug() << "user1 与 user2 的好友关系状态更新失败:" ;

        }
        return;
    }

    //删除好友
    if(message.startsWith("DELETE_FRIEND:"))
    {
        QString content = message.mid(14);
        QStringList parts = content.split('|');
        if(parts.size()==2)
        {
            QString user1=parts[0];
            QString user2=parts[1];
            qDebug()<<user1<<"   "<<user2;
            QString deleteFriend=QString("DELETE FROM QQ_LITE.dbo.Friendships WHERE Account = '%1' AND FriendAccount = '%2'").arg(user1).arg(user2);
            QSqlQuery query;
            // 执行删除操作
            if(query.exec(deleteFriend))
            {
                int rowsDeleted = query.numRowsAffected();
                if(rowsDeleted > 0)
                {
                    qDebug()<<"1好友关系删除成功";
                    QString m=QString("DELETE_FRIEND_R:OK%1").arg(user2);
                    socket->write(m.toUtf8());
                    socket->flush();
                }
                else
                {
                    QString deleteFriends=QString("DELETE FROM QQ_LITE.dbo.Friendships WHERE Account = '%1' AND FriendAccount = '%2'").arg(user2).arg(user1);
                    QSqlQuery queryy;
                    if(queryy.exec(deleteFriends))
                    {
                        int rowsDeleteds = queryy.numRowsAffected();
                        if(rowsDeleteds > 0)
                        {
                            QString m=QString("DELETE_FRIEND_R:OK%1").arg(user2);
                            socket->write(m.toUtf8());
                            socket->flush();
                            qDebug()<<"2好友关系删除成功";
                        }
                        else
                        {
                            qDebug()<<"好友关系删除失败";
                            return;
                        }
                    }
                }
            }
            else
            {
                qDebug()<<"删除操作执行失败";
                return;
            }
            QString m=QString("DELETE_FRIEND_R:OK%1").arg(user1);
            QTcpSocket *receiverSocket = nullptr;
            for (auto it = socketToUserID.begin(); it != socketToUserID.end(); ++it)
            {
                if (it.value() == user2)
                {
                    receiverSocket = it.key();
                    break;
                }
            }
            if (receiverSocket)
            {
                receiverSocket->write(m.toUtf8());
            }

            QTimer *timer = new QTimer(this);
            timer->setSingleShot(true);
            connect(timer, &QTimer::timeout, [=](){
            QStringList friendaccounts=FriendGroup(user1);
            qDebug() << "查询到的FriendAccount列表：" << friendaccounts;
            QString friendsStr = friendaccounts.join("|");
            QString yourfriend = QString("YOUR_FRIEND:%1").arg(friendsStr);
            socket->write(yourfriend.toUtf8());
            socket->flush();
            QString responses=QString("DELETE_FRIEND_TO:%1|%2").arg(user1).arg(user2);
            QTcpSocket *receiverSocket = nullptr;
            for (auto it = socketToUserID.begin(); it != socketToUserID.end(); ++it)
            {
                if (it.value() == user2)
                {
                    receiverSocket = it.key();
                    break;
                }
            }
            if (receiverSocket)
            {
                qDebug()<<"开始转发好友删除消息";
                QStringList friends=FriendGroup(user2);
                qDebug()<<friends;
                QString friendsStrs = friendaccounts.join("|");
                QString yourfriends = QString("YOUR_FRIEND:%1").arg(friendsStrs);
                receiverSocket->write(yourfriends.toUtf8());
            }
            });
            timer->start(2000);
        }
        return;
    }

    //转发文件
    if (m_expectedFileSize == -1)
    {
        // 找到头部结束位置（头部格式：FILE_SEND:发送者|接收者|时间|文件名|大小|）
        int headerStart = data.indexOf("FILE_SEND:");
        //qDebug()<<headerStart;
        if (headerStart == -1)
        {
            qDebug() << "未找到文件头部，等待后续数据...";
            return;
        }
        int headerEnd = data.indexOf("|", headerStart + 10); // 从"FILE_SEND:"后开始找分隔符
        //qDebug()<<headerEnd;
        if (headerEnd == -1)
        {
            qDebug() << "文件头部格式错误，等待后续数据...";
            return;
        }

        // 提取头部字节并转码为字符串解析
        QString content = data.mid(10);
        QStringList parts = content.split("|");
        QString sender;
        QString receiver;
        QString time;
        //qDebug()<<parts;
        if (parts.size() >= 5 )
        {
            sender = parts[0];
            receiver = parts[1];
            time = parts[2];
            receivers=parts[1];
            m_currentFileName = parts[3];
            m_expectedFileSize = parts[4].toLongLong();

            qDebug() << "开始接收文件：" << m_currentFileName
                     << "，预期大小：" << m_expectedFileSize << "字节";
        }

        else
        {
            qDebug() << "文件头部解析失败";
            return;
        }
        QTcpSocket *receiversocket = nullptr;
        //qDebug()<<receiver;
        for (auto it = socketToUserID.begin(); it != socketToUserID.end(); ++it)
        {
            if (it.value() == receiver)
            {
                receiversocket = it.key();
                break;
            }
        }
        if (receiversocket)
        {
            qDebug()<<"开始转发文件";
            receiversocket->write(data);
            //socket->write("MESSAGE_FORWARDING:OK");
        }
        else
        {
            qDebug()<<"用户不在线，转发文件失败";
            return;
        }
    }
    //持续累加文件数据（非第一次收到数据时）
    else
    {
        m_fileDataBuffer.append(data);
        //qDebug()<<m_fileDataBuffer;
    }
    // 检查是否接收完成
    if (m_fileDataBuffer.size() >= m_expectedFileSize)
    {
        // 截取刚好预期大小的原始字节数据
        QByteArray finalFileData = m_fileDataBuffer.left(m_expectedFileSize);
        QTcpSocket *receiversockets = nullptr;
        //qDebug()<<receivers;
        for (auto it = socketToUserID.begin(); it != socketToUserID.end(); ++it)
        {
            if (it.value() == receivers)
            {
                receiversockets = it.key();
                break;
            }
        }
        if (receiversockets)
        {
            qDebug()<<"开始转发文件具体内容";
            forwardFileInChunks(receiversockets, finalFileData);
            //socket->write("MESSAGE_FORWARDING:OK");
        }
        else
        {
            qDebug()<<"用户不在线，转发文件内容失败";
            //return;
        }
        // 保存文件
        QDir dir("./fuwuqidata");
        if (!dir.exists() && !dir.mkpath("."))
        {
            qDebug() << "无法创建文件夹";
            return;
        }
        QString savePath = QString("./fuwuqidata/received_%1").arg(m_currentFileName);
        QFile saveFile(savePath);

        if (saveFile.open(QIODevice::WriteOnly))
        {
            saveFile.write(finalFileData);
            saveFile.close();
            qDebug() << "文件保存成功：" << savePath
                     << "，实际大小：" << finalFileData.size() << "字节";
        }
        else
        {
            qDebug() << "文件保存失败：" << saveFile.errorString();
        }

        // 关键：保存后重置所有状态变量，为下一次文件传输做准备
        m_fileDataBuffer.clear();
        m_expectedFileSize = -1;
        m_currentFileName.clear();
    }
    else
    {
        qDebug() << "文件接收中，已接收：" << m_fileDataBuffer.size()
            << "/" << m_expectedFileSize << "字节";
    }

}

//将新账号密码插入数据库
void Widget::savedate(const QString &account,const QString &passwd,const QString &id)
{
    if (!db.isOpen())
    {
        qDebug() << "数据库未打开，无法插入数据";
        return;
    }
    qDebug()<<"account:"<<account;
    qDebug()<<"passwd:"<<passwd;
    qDebug()<<"ID:"<<id;
    QSqlQuery query;
    QString sql = "INSERT INTO UserInfo (Account, Password,UserID) VALUES (:account, :passwd,:id)";
    query.prepare(sql);
    query.bindValue(":account", account);
    query.bindValue(":passwd", passwd);
    query.bindValue(":id", id);
    if(query.exec())
    {
        qDebug()<<"数据插入成功:";
        issave=true;
    }
    else
    {
        qDebug()<<"插入失败，账号或令牌重复！";
        issave=false;
    }
}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_deleteButton_clicked()
{
    daccount m(db);
    m.exec();
}


void Widget::on_CheckButton_clicked()
{
    caccount m;
    m.exec();
}

//展示当前在线用户
void Widget::ShowOnlineUser()
{
    ui->onlineEdit->clear();
    for (const QString& userId : onlineUsers)
    {
        ui->onlineEdit->append(QString("客户端：%1").arg(userId));
    }
}

//监听当前在线的用户
void Widget::clientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket)
        return;
    // 从集合中移除socket
    //clientSockets.remove(socket);
    // 获取用户ID并从在线用户列表中移除
    QString userID = socketToUserID.value(socket);
    if (!userID.isEmpty())
    {
        onlineUsers.remove(userID);
        socketToUserID.remove(socket); // 从映射中移除
        ShowOnlineUser(); // 更新在线用户显示
        qDebug() << "用户" << userID << "已断开连接";
        broadcastOnlineUsers();
    }
    clientSockets.remove(socket);
    // 释放socket资源
    socket->deleteLater();
}

//分块转发
void Widget::forwardFileInChunks(QTcpSocket* receiverSocket, const QByteArray& fileData)
{
    const qint64 CHUNK_SIZE = 10 * 1024 * 1024;
    // 初始化转发状态
    if (m_forwardedSize[receiverSocket] == 0)
    {
        m_pendingForwardData[receiverSocket] = fileData;
        m_totalForwardSize[receiverSocket] = fileData.size();
        qDebug() << "开始分块转发文件，总大小：" << fileData.size() << "字节";
    }
    while (m_forwardedSize[receiverSocket] < m_totalForwardSize[receiverSocket])
    {
        // 计算当前块大小
        qint64 remaining = m_totalForwardSize[receiverSocket] - m_forwardedSize[receiverSocket];
        qint64 chunkSize = qMin(CHUNK_SIZE, remaining);

        // 提取当前块数据
        QByteArray chunk = m_pendingForwardData[receiverSocket].mid(
            m_forwardedSize[receiverSocket],
            chunkSize
            );


        // 发送当前块
        qint64 bytesWritten = receiverSocket->write(chunk);
        if (bytesWritten == -1)
        {
            qDebug() << "转发失败：" << receiverSocket->errorString();
            return;
        }
        receiverSocket->flush();
        if (!receiverSocket->waitForBytesWritten(30000)) {  // 30秒超时
            qDebug() << "转发超时：" << receiverSocket->errorString();
            return;
        }

        // 更新已转发大小
        m_forwardedSize[receiverSocket] += bytesWritten;
        qDebug() << "已转发：" << m_forwardedSize[receiverSocket]
                 << "/" << m_totalForwardSize[receiverSocket] << "字节";
    }

    // 转发完成，清理状态
    qDebug() << "文件转发部转发完成";
    m_pendingForwardData.remove(receiverSocket);
    m_forwardedSize.remove(receiverSocket);
    m_totalForwardSize.remove(receiverSocket);
}

//从数据库中获取当前用户的好友
QStringList Widget::FriendGroup(const QString &account)
{
    QSqlQuery querys(db);
    QString sql = "SELECT FriendAccount FROM [QQ_LITE].[dbo].[Friendships] WHERE Account = :account AND FriendshipStatus=1";
    querys.prepare(sql);
    querys.bindValue(":account", account);
    QStringList friendaccounts;
    if (querys.exec())
    { // 执行查询
        while (querys.next())
        { // 遍历结果集
            QString friendAccount = querys.value(0).toString(); // 获取FriendAccount列的值，索引0对应第一列
            friendaccounts.append(friendAccount); // 添加到QStringList中
        }
        // 可以在这里使用friendAccounts，比如打印出来看看
        qDebug() << "查询到的FriendAccount列表：" << friendaccounts;
    }
    QSqlQuery quer(db);
    QString sqll = "SELECT Account FROM [QQ_LITE].[dbo].[Friendships] WHERE FriendAccount = :account AND FriendshipStatus=1";
    quer.prepare(sqll);
    quer.bindValue(":account", account);
    if (quer.exec())
    { // 执行查询
        while (quer.next())
        {
            QString friendAccount = quer.value(0).toString();
            friendaccounts.append(friendAccount);
        }
    }
    return friendaccounts;
}

//转发消息函数
void Widget::send_A_Message(const QString &message,QTcpSocket *socket)
{
    //获取当前时间
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    QString content = message.mid(18);
    QStringList parts = content.split('|');
    if(parts.size()==3)
    {
        QString sender=parts[0];
        QString Rinformation=parts[1];
        QString Recipients=parts[2];
        QTcpSocket *receiverSocket = nullptr;
        QString forwardMsg = QString("FROM:[%3]|%1|对你说:%2").arg(sender).arg(Rinformation).arg(timestamp);
        for (auto it = socketToUserID.begin(); it != socketToUserID.end(); ++it)
        {
            if (it.value() == Recipients)
            {
                receiverSocket = it.key();
                break;
            }
        }
        if (receiverSocket)
        {
            qDebug()<<"开始转发消息";
            //QString forwardMsg = QString("FROM:[%3]|%1|对你说:%2").arg(sender).arg(Rinformation).arg(timestamp);
            receiverSocket->write(forwardMsg.toUtf8());
            socket->write("MESSAGE_FORWARDING:OK");
        }
        else
        {
            qDebug()<<"没有找到该用户";
            QString* msgPtr=new QString(forwardMsg);
            pendingMsgPtrs[Recipients]=msgPtr;
            QString* msgPtrs = pendingMsgPtrs[Recipients];
            qDebug()<<"存入成功接收者："<<Recipients<<"消息"<<*msgPtrs;
            socket->write("MESSAGE_FORWARDING:NO");
        }
    }
    return;
}

//发送在线用户表
void Widget::onlineaccount(QTcpSocket *socket)
{
    QStringList onlineUserList;
    foreach (const QString &user, onlineUsers)
    {
        onlineUserList.append(user);
    }
    qDebug()<<onlineUserList;
    QString userStr = onlineUserList.join("|");
    QString messages = QString("ONLINE_USERS:%1").arg(userStr);
    socket->write(messages.toUtf8());
    socket->flush();
    qDebug()<<"发送在线用户列表";
}

//广播函数
void Widget::broadcastOnlineUsers()
{
    QStringList onlineUserList;
    foreach (const QString &user, onlineUsers)
    {
        onlineUserList.append(user);
    }
    QString userStr = onlineUserList.join("|");
    QString message = QString("ONLINE_USERS:%1").arg(userStr);
    QByteArray data = message.toUtf8();
    // 遍历所有在线客户端，逐个发送
    foreach (QTcpSocket *socket, clientSockets)
    {
        if (socket->state() == QAbstractSocket::ConnectedState)
        {
            socket->write(data);
            socket->flush();
        }
    }
    qDebug() << "已向所有在线客户端广播最新在线用户列表";
}

