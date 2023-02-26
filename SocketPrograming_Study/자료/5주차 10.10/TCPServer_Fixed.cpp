/****************************************
�ǽ� 5�� 1��°


���� ���� ������ ���� - ���� ���
*****************************************/

#define _WINSOCK_DEPRECATED_NO_WARNINGS // �ֽ� VC++ ������ �� ��� ����
#define _CRT_SECURE_NO_WARNINGS         // �ֽ� VC++ ������ �� ��� ����
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVERPORT 9000			// ���� ��Ʈ ����: 9000�� ��Ʈ ���
#define BUFSIZE    50			// 50����Ʈ ���� ���� ���۸� ����� �����͸� �д´�. Server�� Client buffer size ����

// ���� �Լ� ���� ��� �� ����
void err_quit(char *msg)
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
    LocalFree(lpMsgBuf);
    exit(1);
}

// ���� �Լ� ���� ���
void err_display(char *msg)
{
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    printf("[%s] %s", msg, (char *)lpMsgBuf);
    LocalFree(lpMsgBuf);
}

// ����� ���� ������ ���� �Լ�
int recvn(SOCKET s, char *buf, int len, int flags)      // �������� �����͸� �б� ���� ����� �����Լ� recvn() �Լ� ��� ~63���α��� - 4�忡�� ����ߴ� �κ�
{
    int received;						// ���������� ȣ���ϴ� recv() �Լ��� ���ϰ��� ������ ����
    char *ptr = buf;					// ������ ���� ptr�� ���� ���α׷� ������ ���� �ּҸ� ����Ŵ. �����͸� ���������� ptr���� ����
    int left = len;						// left ������ ���� ���� ���� ������ ũ����. �����͸� ���� ������ left ���� ����

    while (left > 0) {					// ���� ���� �����Ͱ� ������ ������ ��� ��
        received = recv(s, ptr, left, flags);
        if (received == SOCKET_ERROR)
            return SOCKET_ERROR;
        else if (received == 0)			// recv() �Լ��� ���� ���� 0�̸� (��������), ��밡 �����͸� ���̻� ������ ���� ���̹Ƿ� ������ ��������
            break;
        left -= received;				// recv() �Լ��� ������ ����̹Ƿ� left�� ptr ������ ����
        ptr += received;
    }

    return (len - left);				// ���� ����Ʈ ���� ����. ������ �߻��ϰų� ��밡 ������ ������ ������ �ƴϸ� left ������ �׻� 0�̹Ƿ� ���ϰ��� len�� ��
}

int main(int argc, char *argv[])
{
    int retval;

    // ���� �ʱ�ȭ
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    // socket()
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) err_quit("socket()");

    // bind()
    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("bind()");

    // listen()
    retval = listen(listen_sock, SOMAXCONN);
    if (retval == SOCKET_ERROR) err_quit("listen()");

    // ������ ��ſ� ����� ����
    SOCKET client_sock;
    SOCKADDR_IN clientaddr;
    int addrlen;
    char buf[BUFSIZE + 1];						// 1����Ʈ�� �� ���ڸ� ����ϱ����� BUFSIZE+1 ũ���� ���۸� ������

    while (1) {
        // accept()
        addrlen = sizeof(clientaddr);
        client_sock = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
        if (client_sock == INVALID_SOCKET) {
            err_display("accept()");
            break;
        }

        // ������ Ŭ���̾�Ʈ ���� ���
        printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
            inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

        // Ŭ���̾�Ʈ�� ������ ���
        while (1) {
            // ������ �ޱ�
            retval = recvn(client_sock, buf, BUFSIZE, 0);					//recvn() �Լ��� ȣ���� �����͸� �׻� BUFSIZE(=50) ��ŭ �о����
            if (retval == SOCKET_ERROR) {
                err_display("recv()");
                break;
            }
            else if (retval == 0)
                break;

            // ���� ������ ���
            buf[retval] = '\0';
            printf("[TCP/%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr),
                ntohs(clientaddr.sin_port), buf);
        }

        // closesocket()
        closesocket(client_sock);
        printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
            inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
    }

    // closesocket()
    closesocket(listen_sock);

    // ���� ����
    WSACleanup();
    return 0;
}
