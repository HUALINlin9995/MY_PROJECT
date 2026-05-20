#include "master.h"
#include "ui_master.h"

master::master(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::master)
{
    ui->setupUi(this);
    QPixmap pixmap(":/images/512x512.png");
    ui->log->setPixmap(pixmap);
    ui->passwdEdit->setEchoMode(QLineEdit::Password);
    ui->stateno->setVisible(false);
    ui->Reconnect->setVisible(false);
    ui->label_4->setVisible(false);
    socket=new QTcpSocket;
    //服务器域名
    serverDomain = "1nd2256xr2130.vicp.fun";
    //服务器端口
    serverPort =35951;
    // 自动解析域名并连接
    QHostInfo::lookupHost(serverDomain, this, &master::onHostResolved);
    //监听逻辑
    connect(socket, &QTcpSocket::readyRead, this, &master::onReadyRead);
}

//连接事件
void master::onHostResolved(const QHostInfo &hostInfo)
{
    if (hostInfo.error() != QHostInfo::NoError)
    {
        QMessageBox::warning(this, "解析失败", "域名解析错误：" + hostInfo.errorString());
        return;
    }
    //获取自己的ip
    QString serverIp = hostInfo.addresses().first().toString();
    // 尝试连接服务器
    socket->connectToHost(QHostAddress(serverIp), serverPort);
    // 连接信号
    connect(socket, &QTcpSocket::connected, [this]()
            {
                isconnect=true;
                ui->stateyes->setVisible(true);
                ui->stateno->setVisible(false);
                ui->Reconnect->setVisible(false);
            });
    connect(socket, &QTcpSocket::disconnected, [this]()
            {
                isconnect=false;
                ui->stateyes->setVisible(false);
                ui->stateno->setVisible(true);
                ui->Reconnect->setVisible(true);
            });
}

//监听服务器的回应
void master::onReadyRead()
{
    QByteArray data = socket->readAll();
    QString serverReply = QString::fromUtf8(data);
    qDebug()<<"收到消息"<<serverReply;
    if(serverReply.startsWith("REGISTER_ACK:OK"))
        QMessageBox::information(this,"提示","注册成功!");
    else if(serverReply.startsWith("REGISTER_ACK:NO"))
        QMessageBox::warning(this,"警告","账号或令牌重复！!");
    if(serverReply.startsWith("SIGN-IN PROMPTS:OK"))
    {
        User=ui->accountEdit->text();
        qDebug()<<"账号"<<User;
        //QMessageBox::information(this,"提示","登录成功！");
        //登录成功，跳转至聊天主界面
        MainWindow *mainWindow = new MainWindow(User,socket);
        connect(this, &master::newMessageReceived, mainWindow, &MainWindow::handleNewMessage);
        //关闭登录窗口
        this->close();
        mainWindow->show();
    }
    //else if(serverReply.startsWith("SIGN-IN PROMPTS:NO"))
    //QMessageBox::warning(this,"登录失败","账号或密码错误！");
    if(serverReply.startsWith("PASSWORD_RESET:OK"))
        QMessageBox::information(this,"提示","修改成功！");
    else if(serverReply.startsWith("PASSWORD_RESET:NO"))
        QMessageBox::warning(this,"警告","未找到该用户！");
    if(serverReply.startsWith("MESSAGE_FORWARDING:OK"))
    {
        //QMessageBox::information(this,"提示","发送成功");
    }
    else if(serverReply.startsWith("MESSAGE_FORWARDING:NO"))
        QMessageBox::warning(this,"警告","用户不在线已发送离线消息");
    //if(serverReply.startsWith("FROM:"))

    //QMessageBox::information(this,"提示","收到消息成功");
    qDebug()<<"收到消息啦:";
    // 发射信号将消息转发给MainWindow
    emit newMessageReceived(data);

}

master::~master()
{
    if(socket)
    {
        socket->disconnectFromHost();
        socket->waitForDisconnected();
        delete socket;
        qDebug()<<"已经断开连接";
    }
    delete ui;
}

//关闭按钮
void master::on_cancelButton_clicked()
{
    this->close();
}

//注册按钮
void master::on_registerButton_clicked()
{
    if(!isconnect)
    {
        QMessageBox::warning(this, "警告", "网络断开");
        return;
    }
    //打开注册窗口
    Register m(socket);
    m.exec();
}

//重新连接事件
void master::on_Reconnect_linkActivated()
{
    QHostInfo::lookupHost(serverDomain, this, &master::onHostResolved);
}

//登录事件
void master::on_loginButton_clicked()
{
    if(!isconnect)
    {
        QMessageBox::warning(this,"警告","网络未连接！");
        return;
    }
    QString account=ui->accountEdit->text();
    QString passwd=ui->passwdEdit->text();
    QString data = QString("MY_LOGIN:%1|%2")
                       .arg(account)
                       .arg(passwd);
    //发送事件
    socket->write(data.toUtf8());
    socket->flush();
    QTimer *timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, [=]()
            {
                ui->label_4->setVisible(true);
                timer->deleteLater();
            });
    timer->start(1000);
}

//忘记密码事件
void master::on_RTpasswd_linkActivated()
{
    if(!isconnect)
    {
        QMessageBox::warning(this,"警告","网络未连接！");
        return;
    }
    ChangePasswd m(socket);
    m.exec();
}

void master::keyPressEvent(QKeyEvent *k)
{
    if(k->key()==Qt::Key_Enter||k->key()==Qt::Key_Return)
        on_loginButton_clicked();
}

