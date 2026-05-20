#include "mainwindow.h"
#include "qevent.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QString user,QTcpSocket *socket,QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    ,socket(socket)
    ,user(user)
{
    ui->setupUi(this);
    RcomboBox=ui->comboBox;
    //RcomboBox->addItem("全体好友");
    initFriendTable();
    //设置发送缓冲区
    socket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 10 * 1024 * 1024);
    //设置接收缓冲区
    socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, 10 * 1024 * 1024);
    std::ifstream ifs;
    ifs.open(FILENAME,std::ios::in);
    if(!ifs.is_open())
        qDebug()<<"文件不存在";
    else
    {
        QFile file(FILENAME);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "文件不存在";
        }
        //读取本地当前用户历史聊天记录
        else
        {
            QTextStream in(&file);
            QString line;
            while(!(line=in.readLine()).isNull())
            {
                qDebug() << "读取到的一行内容：" << line;
                QStringList parts = line.split('|');
                if(parts.size()==4)
                {
                    QString userd=parts[0];
                    QString Receiver=parts[1];
                    QString content=parts[2];
                    QString times=parts[3];
                    //判断是否为当前用户的聊天记录
                    if(userd==user)
                        ui->chatRecordEdit->append(QString("%1对%2说%3").arg(times).arg(Receiver).arg(content));
                }
            }
            qDebug()<<user;
            file.close();
        }
    }
    ui->chatRecordEdit->append(QString("当前登录用户:%1").arg(user));
}

MainWindow::~MainWindow()
{
    if (socket)
    {
        socket->disconnectFromHost();
        socket->waitForDisconnected(1000);
        delete socket;
    }
    delete ui;
}

//关闭窗口事件
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (socket)
    {
        socket->disconnectFromHost();
        socket->waitForDisconnected(1000);
    }
    // 允许窗口关闭
    event->accept();
}

//发送消息事件
void MainWindow::on_sendButton_clicked()
{
    //获取接收方
    QString User=RcomboBox->currentText();
    //获取发送的消息
    QString lnformation=ui->messageInput->text();
    //获取当前时间
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    //检查是否选择了接收者
    if (User.isEmpty())
    {
        QMessageBox::warning(this, "警告", "请选择接收者账号");
        qDebug() << "未选择接收者，无法发送";
        return;
    }
    //检查消息是否为空
    if (lnformation.isEmpty())
    {
        QMessageBox::warning(this, "警告", "消息内容不能为空");
        qDebug() << "消息内容为空，无法发送";
        return;
    }
    QString senddata;
    if (User == "全体好友")
    {
        // 群发消息格式
        senddata = QString("SEND_BROADCAST_MESSAGE:%1|%2")
                       .arg(user).arg(lnformation);
    }
    else
    {
        // 单聊消息格式
        senddata = QString("SEND_A_MESSAGE_TO:%1|%2|%3")
                       .arg(user).arg(lnformation).arg(User);
    }
    ui->chatRecordEdit->append(QString("[%1]对用户%2说:%3").arg(timestamp).arg(User).arg(lnformation));
    QString data=QString("%1|%2|%3|[%4]").arg(user).arg(User).arg(lnformation).arg(timestamp);
    //调用保存聊天记录函数
    Save_Informain(data);
    socket->write(senddata.toUtf8());
    socket->flush();
    ui->messageInput->clear();
}

//将发送函数链接到键盘enter
void MainWindow::keyPressEvent(QKeyEvent *k)
{
    if(k->key()==Qt::Key_Enter||k->key()==Qt::Key_Return)
        on_sendButton_clicked();
}

//读取消息函数
void MainWindow::handleNewMessage(const QByteArray &message)
{
    qDebug()<<"新窗口收到消息了"<<message;

    //监听收到的文本消息
    if(message.startsWith("FROM:"))
    {
        QString msg = message.mid(5);
        int firstSeparatorIndex = msg.indexOf('|');
        if(firstSeparatorIndex !=-1)
        {
            QString time = msg.left(firstSeparatorIndex);
            int secondSeparatorIndex = msg.indexOf('|', firstSeparatorIndex + 1);
            if (secondSeparatorIndex != -1)
            {
                QString sender = msg.mid(firstSeparatorIndex + 1, secondSeparatorIndex - firstSeparatorIndex - 1);
                QString content = msg.mid(secondSeparatorIndex + 1);
                // 格式化并追加到聊天记录，注意arg的顺序与%1、%2、%3对应
                QString data=QString("%1|%2|%3|%4").arg(user).arg(sender).arg(content).arg(time);
                Save_Informain(data);
                ui->chatRecordEdit->append(QString("%1 %2:%3").arg(time).arg(sender).arg(content));
            }
        }
        return;
    }

    //服务器回复收到好友请求
    if(message.startsWith("ADD_FRIEND:OK"))
    {
        QMessageBox::information(this,"提示","发送成功");
        return;
    }
    else if(message.startsWith("ADD_FRIEND:NO"))
    {
        QMessageBox::warning(this,"警告","该用户不存在");
        return;
    }

    //服务器回复已经是好友，不允许重复添加
    if(message.startsWith("ADD_FRIEND_:OK"))
    {
        QMessageBox::information(this,"提示","已经是好友了!");
        return;
    }

    //确认添加好友（发送好友请求方）
    if(message.startsWith("ADD_FRIEND_R:"))
    {
        QString content = message.mid(13);
        qDebug()<<"添加用户"<<content<<"好友成功";
        QMessageBox::information(this,"提示",QString("添加 %1 好友成功").arg(content));
        RcomboBox->addItem(content);
        friendAccount.append(content);
        qDebug()<<friendAccount;
        qDebug() << "成功 friendAccount 添加" << content;
        QString contents=friendAccount.join("|");
        QString informationyy=QString("ONLINE_USERS:%1").arg(contents);
        qDebug()<<informationyy;
        online_users(informationyy);
        return;
    }

    //从服务器接收我的好友信息
    if(message.startsWith("YOUR_FRIEND:"))
    {
        QString friendsInfo = message.mid(12);
        QStringList friendAccounts = friendsInfo.split("|");
        friendAccount.clear();
        //将好友信息存入容器中
        friendAccount=friendAccounts;
        qDebug()<<"从服务器获取到好友信息"<<friendAccount;
        ui->comboBox->clear();
        RcomboBox=ui->comboBox;
        RcomboBox->addItem("全体好友");
        for (const QString& friendAccountes : friendAccounts)
        {
            ui->comboBox->addItem(friendAccountes);
        }
        return;
    }

    //接收到好友请求通知
    if(message.startsWith("ADD_FRIEND_REQUEST:"))
    {
        QString content = message.mid(19);
        QStringList parts = content.split('|');
        if(parts.size()==2)
        {
            QString friends=parts[0];
            qDebug()<<friends;
            QString user1=parts[1];
            //接收到好友请求后，弹出好友请求操作窗口
            FriendNotice m(user1,friends,socket);
            m.exec();
        }
        return;
    }

    //确认添加好友（接收好友请求方）
    if(message.startsWith("ADD_FRIEND_AGREE:"))
    {
        QString content = message.mid(17);
        qDebug()<<content;
        if(RcomboBox->findText(content)!=-1)
        {
            QMessageBox::information(this, "提示", "该好友已存在！");
            return;
        }
        //将新好友添加到下拉框中
        RcomboBox->addItem(content);
        //将好友账号存入容器中
        friendAccount.append(content);
        qDebug()<<friendAccount;
            qDebug() << "成功 friendAccount 添加" << content;
            QString contents=friendAccount.join("|");
            QString informationyy=QString("ONLINE_USERS:%1").arg(contents);
            qDebug()<<informationyy;
            online_users(informationyy);
        return;
    }

    //监听在线好友
    if(message.startsWith("ONLINE_USERS:"))
    {
        online_users(message);
    }

    //接收好友删除回复
    if(message.startsWith("DELETE_FRIEND_R:OK"))
    {
        QString content = message.mid(18);
        QMessageBox::information(this,"提示","删除成功");
        bool isRemoved = friendAccount.removeOne(content);
        if (isRemoved)
        {
            qDebug() << "成功从 friendAccount 中删除" << content;
            QString contents=friendAccount.join("|");
            QString informationyy=QString("ONLINE_USERS:%1").arg(contents);
            qDebug()<<informationyy;
            online_users(informationyy);
        }
        else
            qDebug() << "在 friendAccount 中未找到" << content << "，删除失败";
    }

    //接收文件
    if (m_expectedFileSize == -1)
    {
        // 找到头部结束位置,头部格式：FILE_SEND:发送者|接收者|时间|文件名|大小|
        int headerStart = message.indexOf("FILE_SEND:");
        qDebug()<<headerStart;
        if (headerStart == -1)
        {
            qDebug() << "未找到文件头部，等待后续数据...";
            return;
        }
        int headerEnd = message.indexOf("|", headerStart + 10);
        qDebug()<<headerEnd;
        if (headerEnd == -1)
        {
            qDebug() << "文件头部格式错误，等待后续数据...";
            return;
        }
        // 提取头部字节并转码为字符串解析
        QString content = message.mid(10);
        QStringList parts = content.split("|");
        QString sender;
        QString receiver;
        QString time;
        qDebug()<<parts;
        if (parts.size() >= 5 )
        {
            sender = parts[0];
            receiver = parts[1];
            time = parts[2];
            m_currentFileName = parts[3];
            m_expectedFileSize = parts[4].toLongLong();
            float sizes=m_expectedFileSize/1024.0;
            ui->chatRecordEdit->append(QString("[%1]收到来自[%2]的文件[%3]大小[%4kb]").arg(time).arg(sender).arg(m_currentFileName).arg(sizes));
            qDebug() << "开始接收文件：" << m_currentFileName
                     << "，预期大小：" << m_expectedFileSize << "字节";
        }
        else
        {
            qDebug() << "文件头部解析失败";
            return;
        }
    }
    // 持续累加文件数据（非第一次收到数据时）
    else
    {
        m_fileDataBuffer.append(message);
        qDebug()<<m_fileDataBuffer;
    }
    //检查是否接收完成
    if (m_fileDataBuffer.size() >= m_expectedFileSize)
    {
        // 截取刚好预期大小的原始字节数据
        QByteArray finalFileData = m_fileDataBuffer.left(m_expectedFileSize);
        // 保存文件
        QDir dir("./linshiwenjian");
        if (!dir.exists() && !dir.mkpath("."))
        {
            qDebug() << "无法创建文件夹";
            return;
        }
        QString savePath = QString("./linshiwenjian/received_%1").arg(m_currentFileName);
        QFile saveFile(savePath);
        if (saveFile.open(QIODevice::WriteOnly))
        {
            saveFile.write(finalFileData);
            saveFile.close();
            qDebug() << "文件保存成功：" << savePath
                     << "，实际大小：" << finalFileData.size() << "字节";
        }
        else
            qDebug() << "文件保存失败：" << saveFile.errorString();
        // 保存后重置所有状态变量，为下一次文件传输做准备
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

//保存聊天记录事件
void MainWindow::Save_Informain(const QString &data)
{
    QFile file(FILENAME);
    if (file.open(QIODevice::Append | QIODevice::Text))
    {
        QTextStream out(&file);
        out << data;
        out<<"\n";
        file.close();
    }
}

//删除当前用户历史记录事件
void MainWindow::on_pushButton_clicked()
{
    ui->chatRecordEdit->clear();
    QFile files(FILENAME);
    QStringList fileContent;
    //读取每一行内容
    if (files.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&files);
        QString line;
        while (!(line = in.readLine()).isNull())
        {
            fileContent.append(line);
        }
        files.close();
    }
    else
    {
        qDebug() << "文件打开失败，无法读取内容";
        return;
    }
    QStringList newFileContent;
    // 筛选出 userd 不等于 user 的行
    for (const QString &line : fileContent)
    {
        QStringList parts = line.split('|');
        if (parts.size() >= 1)
        {
            QString userd = parts[0];
            if (userd != user)
                newFileContent.append(line);
        }
        else
            newFileContent.append(line);
    }
    // 将筛选后的内容写回文件
    if (files.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        QTextStream out(&files);
        for (const QString &line : newFileContent)
            out << line << Qt::endl;
        files.close();
        qDebug() << "删除符合条件的行成功";
    }
    else
        qDebug() << "文件打开失败，无法写入内容";
}

//清空所有用户历史消息事件
void MainWindow::on_purgeButton_clicked()
{
    ui->chatRecordEdit->clear();
    QFile file(FILENAME);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        file.close();
        qDebug()<<"清空文件内容";
    }
}

//发送文件事件
void MainWindow::on_sendfileButton_clicked()
{
    // 打开文件选择对话框
    QString filePath = QFileDialog::getOpenFileName(this, tr("选择文件"), QDir::homePath());
    if (filePath.isEmpty())
        return;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::warning(this, tr("错误"), tr("无法打开文件：%1").arg(file.errorString()));
        return;
    }
    QString fileName = QFileInfo(filePath).fileName();
    QString Receiver = RcomboBox->currentText();
    if (Receiver == "全体好友")
    {
        QMessageBox::warning(this, "警告", "文件不允许群发");
        file.close();
        return;
    }
    if (Receiver.isEmpty())
    {
        QMessageBox::warning(this, "警告", "无效的收件人");
        qDebug() << "收件人为空，无法发送文件";
        file.close();
        return;
    }
    // 获取文件信息
    qint64 fileSize = file.size();
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    // 发送文件头部信息
    // 格式：FILE_HEADER:发送者|接收者|时间|文件名|总大小
    QString header = QString("FILE_SEND:%1|%2|%3|%4|%5|")
                         .arg(user).arg(Receiver).arg(timestamp).arg(fileName).arg(fileSize);
    socket->write(header.toUtf8());
    socket->flush();
    // 等待头部发送完成
    socket->waitForBytesWritten();
    // 分块发送文件数据
    const qint64 CHUNK_SIZE = 4 * 1024 * 1024;
    qint64 totalSent = 0;
    QByteArray chunk;
    // 循环读取并发送每一块数据
    while (totalSent < fileSize)
    {
        // 计算当前块大小（最后一块可能小于CHUNK_SIZE）
        qint64 chunkSize = qMin(CHUNK_SIZE, fileSize - totalSent);
        // 读取一块数据
        chunk = file.read(chunkSize);
        if (chunk.size() != chunkSize)
        {
            QMessageBox::warning(this, "错误", "文件读取失败");
            qDebug() << "文件读取错误，预期" << chunkSize << "字节，实际" << chunk.size() << "字节";
            file.close();
            return;
        }
        // 发送当前块
        socket->write(chunk);
        socket->flush();
        // 更新已发送大小
        totalSent += chunkSize;
        // 显示进度
        int progress = static_cast<int>((totalSent * 100) / fileSize);
        qDebug() << "文件发送进度：" << progress << "%";
    }
    file.close();
    QMessageBox::information(this, "成功", "文件发送完成");
    ui->chatRecordEdit->append(QString("[%1]发送文件[%2]给用户[%3]").arg(timestamp).arg(fileName).arg(Receiver));
    qDebug() << "文件发送成功，总大小：" << fileSize << "字节";
}

//打开接收文件的文件夹事件
void MainWindow::on_openfileButton_clicked()
{
    QString currentDir = QDir::currentPath();
    // 拼接 linshiwenjian 文件夹的路径
    QString folderPath = currentDir + "/linshiwenjian";
    // 将路径转换为 QUrl
    QDir folderDir(folderPath);
    if (!folderDir.exists())
    {
        // 文件夹不存在，尝试创建
        bool isCreated = folderDir.mkpath(".");
        if (isCreated)
            qDebug() << "文件夹创建成功：" << folderPath;
        else
        {
            qDebug() << "文件夹创建失败：" << folderPath;
            QMessageBox::warning(this, "错误", "无法创建文件夹！");
            return;
        }
    }
    // 打开文件夹
    bool isOpened = QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
    if (!isOpened)
    {
        qDebug() << "文件夹打开失败：" << folderPath;
        QMessageBox::warning(this, "错误", "无法打开文件夹！");
    }
}

// 初始化好友表格事件
void MainWindow::initFriendTable()
{
    ui->friendTableWidget->setColumnCount(1);
    ui->friendTableWidget->setHorizontalHeaderLabels(QStringList() << "在线好友");
    ui->friendTableWidget->horizontalHeader()->setStretchLastSection(true);
}

//添加好友事件
void MainWindow::on_AddFriendButton_clicked()
{
    AddFriend m(user,socket);
    m.exec();
}

//删除好友事件
void MainWindow::on_DeleteFriendButton_clicked()
{
    DeleteFriend m(user,socket);
    m.exec();
}

//显示在线好友列表
void MainWindow::online_users(const QString &message)
{
    qDebug()<<"收到了在线好友";
    // 清除表格现有内容
    ui->friendTableWidget->clearContents();
    ui->friendTableWidget->setRowCount(0); // 重置行数
    QString content = message.mid(13);
    QStringList onlineUsers = content.split('|');
    // 循环添加用户到表格
    int row = 0;
    for (const QString& onlineUser : onlineUsers)
    {
        // 跳过空字符串
        if (onlineUser.isEmpty())
            continue;
        // 判断该在线用户是否是好友
        if (friendAccount.contains(onlineUser))
        {
            // 添加新行
            ui->friendTableWidget->insertRow(row);
            // 创建单元格并设置内容
            QTableWidgetItem *userItem = new QTableWidgetItem(onlineUser);
            // 设置单元格不可编辑
            userItem->setFlags(userItem->flags() & ~Qt::ItemIsEditable);
            // 将单元格添加到表格
            ui->friendTableWidget->setItem(row, 0, userItem);
            row++;
        }
    }
    return;
}



