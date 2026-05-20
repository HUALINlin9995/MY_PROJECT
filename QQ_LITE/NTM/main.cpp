#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <QByteArray>
#include "windivert.h"
#include <windows.h>
#include <iphlpapi.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <QThreadPool>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>

// 链接依赖库
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")

//同步锁
QQueue<QPair<char*, UINT>> packetQueue;
QMutex queueMutex;
QWaitCondition queueCond;

// 网络流统计结构
struct FlowStats
{
    QString srcIp;
    QString dstIp;
    UINT16 srcPort;
    UINT16 dstPort;
    UINT64 bytesSent = 0;
    UINT64 bytesReceived = 0;
};

//流统计容器
QMap<QString, FlowStats> flowMap;

//统计各进程的带宽
QMap<QString,int> processBand;

bool g_isCaptureRunning = true;

// 全局句柄，方便信号处理函数关闭
HANDLE g_wdHandle = INVALID_HANDLE_VALUE;

void handleSigInt(int sig)
{
    Q_UNUSED(sig);
    qInfo() << "\n【捕获Ctrl+C快捷键】正在停止抓包...";
    g_isCaptureRunning = false; // 修改标志位，让循环退出
    // 提前关闭WinDivert句柄
    if (g_wdHandle != INVALID_HANDLE_VALUE)
    {
        WinDivertClose(g_wdHandle);
    }
}

// 根据本地端口+协议查PID
DWORD GetPidByLocalPort(UINT16 port, const QString& protocol)
{
    if (protocol == "TCP")
    {
        MIB_TCPTABLE2* tcpTable = nullptr;
        DWORD size = 0;
        // 先获取表大小，失败则直接返回
        if (GetExtendedTcpTable(tcpTable, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) != ERROR_INSUFFICIENT_BUFFER)
        {
            return 0;
        }
        // 动态分配内存，加判空
        tcpTable = (MIB_TCPTABLE2*)malloc(size);
        if (!tcpTable) return 0;
        //调用api，获取完整的tcp端口表
        if (GetExtendedTcpTable(tcpTable, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) == NO_ERROR)
        {
            for (DWORD i = 0; i < tcpTable->dwNumEntries; ++i)
            {
                if (ntohs((u_short)tcpTable->table[i].dwLocalPort) == port)
                {
                    DWORD pid = tcpTable->table[i].dwOwningPid;
                    free(tcpTable); // 释放内存，避免泄漏
                    return pid;
                }
            }
        }
        free(tcpTable); // 无论是否找到，都释放内存
    }
    else if (protocol == "UDP")
    {
        MIB_UDPTABLE_OWNER_PID* udpTable = nullptr;
        DWORD size = 0;
        if (GetExtendedUdpTable(udpTable, &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0) != ERROR_INSUFFICIENT_BUFFER)
        {
            return 0;
        }
        udpTable = (MIB_UDPTABLE_OWNER_PID*)malloc(size);
        if (!udpTable) return 0;

        if (GetExtendedUdpTable(udpTable, &size, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0) == NO_ERROR)
        {
            for (DWORD i = 0; i < udpTable->dwNumEntries; ++i)
            {
                if (ntohs((u_short)udpTable->table[i].dwLocalPort) == port)
                {
                    DWORD pid = udpTable->table[i].dwOwningPid;
                    free(udpTable);
                    return pid;
                }
            }
        }
        free(udpTable);
    }
    return 0; // 未找到对应进程
}

// 根据PID获取进程名
QString GetProcessNameByPid(DWORD pid)
{
    if (pid == 0 || pid == 4 || pid == 8000)
    {
        if (pid == 0) return "未知进程";
        else return "System（内核进程）";
    }

    // 打开进程：增加PROCESS_TERMINATE避免部分进程拒绝访问
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, FALSE, pid);
    if (!hProcess)
    {
        return "权限不足-无法获取进程名";
    }
    //存储进程完整路径
    wchar_t path[MAX_PATH] = {0};
    QString processName = "未知进程";
    // 调用宽字符版API，确保中文路径正常
    if (GetModuleFileNameExW(hProcess, nullptr, path, MAX_PATH))
    {
        processName = QString::fromWCharArray(path).split("\\").last();
    }

    CloseHandle(hProcess); // 必须释放进程句柄，核心修复点
    return processName;
}

// 数据包处理函数
void processPacket(char* packet, UINT packetLen, WINDIVERT_ADDRESS* addr)
{
    if (packet == nullptr || packetLen == 0 || addr == nullptr)
    {
        return; // 空包直接返回，避免崩溃
    }

    WINDIVERT_IPHDR *ipHdr = nullptr;
    WINDIVERT_TCPHDR *tcpHdr = nullptr;
    WINDIVERT_UDPHDR *udpHdr = nullptr;

    // 解析数据包，增加返回值判断
    BOOL parseOk = WinDivertHelperParsePacket(
        packet, packetLen,
        &ipHdr, nullptr, nullptr, nullptr, nullptr,
        &tcpHdr, &udpHdr, nullptr, nullptr, nullptr, nullptr);
    if (!parseOk || !ipHdr)
    {
        return; // 解析失败直接返回
    }

    // 转换IP地址为字符串
    char srcIpStr[46] = {0}, dstIpStr[46] = {0};
    WinDivertHelperFormatIPv4Address(ipHdr->SrcAddr, srcIpStr, sizeof(srcIpStr));
    WinDivertHelperFormatIPv4Address(ipHdr->DstAddr, dstIpStr, sizeof(dstIpStr));
    QString srcIp = srcIpStr;
    QString dstIp = dstIpStr;
    UINT16 srcPort = 0, dstPort = 0;
    QString protocol = "Unknown";

    // 解析TCP/UDP/ICMP协议
    if (tcpHdr != nullptr)
    {
        protocol = "TCP";
        srcPort = ntohs(tcpHdr->SrcPort);
        dstPort = ntohs(tcpHdr->DstPort);
    }
    else if (udpHdr != nullptr)
    {
        protocol = "UDP";
        srcPort = ntohs(udpHdr->SrcPort);
        dstPort = ntohs(udpHdr->DstPort);
    }
    else if (ipHdr->Protocol == IPPROTO_ICMP)
    {
        protocol = "ICMP";
        // qDebug() << "[ICMP] " << srcIp << " -> " << dstIp << " | 长度: " << packetLen << " bytes";
        return; // ICMP无端口，直接打印返回
    }

    // 查找PID：优先源端口，失败再试目的端口
    DWORD pid = GetPidByLocalPort(srcPort, protocol);
    if (pid == 0)
    {
        pid = GetPidByLocalPort(dstPort, protocol);
    }
    if (pid == 0)
    {
        return;
    }

    // 获取进程名并打印抓包信息
    QString processName = GetProcessNameByPid(pid);
    if(processName=="权限不足-无法获取进程名"||processName=="System（内核进程）")return;
    processBand[processName]=processBand.value(processName,0)+packetLen;
    system("cls");
    qDebug() << "┌─────────────────────────────────────────";
    qDebug() << "进程: " << processName << "(PID: " << pid << ")";
    qDebug() << QString("[%1] 源: %2:%3 | 目的: %4:%5 | 包长度: %6 bytes")
                    .arg(protocol, 3)
                    .arg(srcIp, 15)
                    .arg(srcPort, 5)
                    .arg(dstIp, 15)
                    .arg(dstPort, 5)
                    .arg(packetLen, 6);
    qDebug() << "└─────────────────────────────────────────\n";
    for(auto it = processBand.begin(); it != processBand.end(); ++it)
    {
        qDebug()<<"进程："<<it.key()<<"数据"<<it.value()/1024.0<<"KB";
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QObject::connect(&a, &QCoreApplication::aboutToQuit, []() {
        g_isCaptureRunning = false;
        if (g_wdHandle != INVALID_HANDLE_VALUE)
        {
            WinDivertClose(g_wdHandle);
        }
    });
    signal(SIGINT, handleSigInt);
    //打开WinDivert设备，过滤所有网络包
    g_wdHandle = WinDivertOpen("tcp", WINDIVERT_LAYER_NETWORK, 0, 0);
    if (g_wdHandle == INVALID_HANDLE_VALUE)
    {
        qCritical() << "WinDivert打开失败，错误码：" << GetLastError();
        return -1;
    }
    qInfo() << "WinDivert配置成功！设备打开正常～";
    qInfo() << "正在抓包...\n";

    // 数据包缓冲区
    char packet[WINDIVERT_MTU_MAX] = {0};
    WINDIVERT_ADDRESS addr = {0};
    UINT packetLen = 0;
    // 主抓包循环
    while (g_isCaptureRunning)
    {
        // 捕获数据包：阻塞式接收，直到有包或句柄关闭
        BOOL recvOk = WinDivertRecv(g_wdHandle, packet, sizeof(packet), &packetLen, &addr);
        if (recvOk)
        {
            // 1. 处理数据包
            processPacket(packet, packetLen, &addr);
            //把捕获的包重新注入网络栈，恢复正常网络
            WinDivertSend(g_wdHandle, packet, packetLen, &packetLen, &addr);
        }
        else
        {
            DWORD err = GetLastError();
            if (err != ERROR_OPERATION_ABORTED) // 排除句柄关闭导致的正常错误
            {
                qWarning() << "WinDivertRecv失败，错误码：" << err;
            }
            break;
        }
    }
    for(auto it = processBand.begin(); it != processBand.end(); ++it)
    {
        qDebug()<<"进程："<<it.key()<<"数据"<<it.value()/1024.0<<"KB";
    }

    // 程序退出时，确保关闭句柄
    if (g_wdHandle != INVALID_HANDLE_VALUE)
    {
        WinDivertClose(g_wdHandle);
    }
    qInfo() << "\n抓包程序已正常退出，资源已释放！";

    return a.exec();
}
