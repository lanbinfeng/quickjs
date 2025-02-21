#include "quickjs-debugger.h"

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <winsock2.h>

struct js_transport_data {
    int handle;
} js_transport_data;

static size_t js_transport_read(void *udata, char *buffer, size_t length) {
    struct js_transport_data* data = (struct js_transport_data *)udata;
    if (data == NULL || data->handle <= 0)
        return -1;

    if (length == 0)
        return -2;

    if (buffer == NULL)
        return -3;

    //ssize_t ret = read(data->handle, (void *)buffer, length);
	ssize_t ret = recv( data->handle, (void*)buffer, length, 0);

    if (ret == SOCKET_ERROR )
        return -4;

    if (ret == 0)
        return -5;

    if (ret > length)
        return -6;

    return ret;
}

static size_t js_transport_write(void *udata, const char *buffer, size_t length) {
    struct js_transport_data* data = (struct js_transport_data *)udata;
    if (data == NULL || data->handle <= 0)
        return -1;

    if (length == 0)
        return -2;

    if (buffer == NULL) {
        return -3;
	}

    //size_t ret = write(data->handle, (const void *) buffer, length);
	size_t ret = send( data->handle, (const void *) buffer, length, 0);
    if (ret <= 0 || ret > (ssize_t) length)
        return -4;

    return ret;
}

static size_t js_transport_peek(void *udata) {

    struct js_transport_data* data = (struct js_transport_data *)udata;
    if (data == NULL || data->handle <= 0)
        return -1;

    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(data->handle, &readSet);

    struct timeval timeout;
    timeout.tv_sec = 0;    // 超时时间设置为1秒
    timeout.tv_usec = 0;
    // 调用 select() 函数来检测可读性和断开
    int result = select(1, &readSet, NULL, NULL, &timeout);
    //printf("select fd=%d, ret=%d\n",data->handle, result);

    if (result < 0) {
        printf("Select error.\n");
        return -2;
    }
    // no data
    if (result == 0)
        return 0;
    // has data
    return 1;
}

static void js_transport_close(JSContext* ctx, void *udata) {
    struct js_transport_data* data = (struct js_transport_data *)udata;
    if (data == NULL || data->handle <= 0)
        return;

    close(data->handle);
	data->handle = 0;

    free(udata);

	WSACleanup();
}

void js_debugger_connect(JSContext *ctx, const char *address) {
    printf("js_debugger_connect address = %s\n", address);

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

    char* port_string = strstr(address, ":");
    assert(port_string);

    int port = atoi(port_string + 1);
    assert(port);

    int client = socket(AF_INET, SOCK_STREAM, 0);
    assert(client > 0);
    char host_string[256];
    strcpy(host_string, address);
    host_string[port_string - address] = 0;

    struct hostent *host = gethostbyname(host_string);
    assert(host);
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    memcpy((char *)&addr.sin_addr.s_addr, (char *)host->h_addr, host->h_length);
    addr.sin_port = htons(port);

	//__asm__ volatile("int $0x03");
	assert(!connect(client, (const struct sockaddr *)&addr, sizeof(addr)));
	    
    struct js_transport_data *data = (struct js_transport_data *)malloc(sizeof(struct js_transport_data));
    data->handle = client;
    js_debugger_attach(ctx, js_transport_read, js_transport_write, js_transport_peek, js_transport_close, data);
}


void js_debugger_wait_connection(JSContext *ctx, const char* address) {
    printf("js_debugger_wait_connection address = %s\n", address);

	// 初始化 Winsock 库
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Failed to initialize Winsock\n");
        return;
    }

    // 拆分地址字符串
    char* ipAddress = _strdup(address);
    unsigned short port;

    char* colonPos = strchr(ipAddress, ':');
    if (colonPos != NULL) {
        *colonPos = '\0';
        ++colonPos;

        // 提取端口号
        port = atoi(colonPos);
    } else {
        printf("Invalid address format!\n");
        free(ipAddress);
        WSACleanup();
        return;
    }

    // 创建监听套接字，使用 IPv4 和 TCP 协议
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        printf("Failed to create listen socket: %d\n", WSAGetLastError());
        free(ipAddress);
        WSACleanup();
        return;
    }

    // 设置服务器地址结构
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ipAddress);

    // 绑定监听套接字到指定地址和端口
    if (bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Failed to bind the listen socket: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        free(ipAddress);
        WSACleanup();
        return;
    }

    // 开始监听连接
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Failed to listen for connections: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        free(ipAddress);
        WSACleanup();
        return;
    }

    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    SOCKET clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
    if (clientSocket == INVALID_SOCKET) 
    {
        printf("Failed to accept for connections: %d\n", WSAGetLastError());
        closesocket(listenSocket);
        free(ipAddress);
        WSACleanup();
        return;
    }
    // 释放内存
    free(ipAddress);
	closesocket(listenSocket);

    struct js_transport_data *data = (struct js_transport_data *)malloc(sizeof(struct js_transport_data));
    data->handle = clientSocket;
    js_debugger_attach(ctx, js_transport_read, js_transport_write, js_transport_peek, js_transport_close, data);
}
