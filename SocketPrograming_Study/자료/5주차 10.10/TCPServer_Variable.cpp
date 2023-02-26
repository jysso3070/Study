/*****************************************************
�ǽ� 5�� 2��°


���� ���� ������ ���� - ���� ���
*******************************************************/

#define _WINSOCK_DEPRECATED_NO_WARNINGS // �ֽ� VC++ ������ �� ��� ����
#define _CRT_SECURE_NO_WARNINGS         // �ֽ� VC++ ������ �� ��� ����
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVERPORT 9000			// ������ ����� ��Ʈ ����
#define BUFSIZE    512			// 512 ����Ʈ ���۸� ����� �����͸� ����. ���������͹Ƿ� �̺��ٴ� ���� ���������Ͷ�� ����.

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

// ���� ������ �Լ�
int _recv_ahead(SOCKET s, char *p)				// ���� ���� ���ۿ��� �����͸� �ѹ��� ���� �о� ���� �� 1����Ʈ�� �������ִ� ����� ���� �Լ�.(�����Լ�).recvline() �Լ����� ȣ���Ͽ� ��� 
{
    __declspec(thread) static int nbytes = 0;	// ���� ���� ���ۿ��� ���� �����͸� �� ����Ʈ�� �����ϴµ� �ʿ��� �ٽ� ������. �Լ��� ������ �ص� ���� ��� �����ؾ��ϹǷ� static ������ ���� 
    __declspec(thread) static char buf[1024];	// __declspec(thread)�� ��Ƽ������ ȯ�濡���� �Լ��� �������� �����ϴµ� �ʿ��� �Լ���.
    __declspec(thread) static char *ptr;		// ��Ƽ������� ���� �忡�� ����

    if (nbytes == 0 || nbytes == SOCKET_ERROR) {		// ���� ���� ���ۿ��� �о���� �����Ͱ� ���� ���ų� �����Ͽ� ��� �Ҹ��� ��쿡�� ���� �о� buf[] �迭�� �����صΰ� 
        nbytes = recv(s, buf, sizeof(buf), 0);			// ������ ���� ptr�� �� ���� ����Ʈ�� ����Ű�� ��
        if (nbytes == SOCKET_ERROR) {
            return SOCKET_ERROR;
        }
        else if (nbytes == 0)
            return 0;
        ptr = buf;
    }

    --nbytes;				// ���� ����Ʈ ��(nbytes)�� 1 ���ҽ�Ű��, 
    *p = *ptr++;			// ������ ���� ptr�� ����Ű�� �����͸� ������ ���� p�� ����Ű�� �޸� ������ �־ ������
    return 1;
}

// ����� ���� ������ ���� �Լ�
int recvline(SOCKET s, char *buf, int maxlen)		// \n�� ���ö����� �����͸� �б� ���� recvline() �Լ��� ����.
{													// ���� s���� �����͸� �ѹ���Ʈ�� �о buf�� ����Ű�� �޸� ������ �����ϵ�, \n�� �����ų� �ִ���� maxlen-1�� �����ϸ�
    int n, nbytes;									// \0�� �ٿ��� ������. \0�� �ݵ�� �ʿ������� ������ �����͸� ���ڿ��� ������ ����� �� ������
    char c, *ptr = buf;

    for (n = 1; n < maxlen; n++) {
        nbytes = _recv_ahead(s, &c);
        if (nbytes == 1) {
            *ptr++ = c;
            if (c == '\n')
                break;
        }
        else if (nbytes == 0) {
            *ptr = 0;
            return n - 1;
        }
        else
            return SOCKET_ERROR;
    }

    *ptr = 0;
    return n;
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
    char buf[BUFSIZE + 1];

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
            retval = recvline(client_sock, buf, BUFSIZE + 1);		// recvline() �Լ��� ȣ���� \n�� ���ö����� �����͸� ����. +1�� 1����Ʈ �ι��ڸ� ������ ������
            if (retval == SOCKET_ERROR) {							// ������ �ִ� BUFSIZE(512����Ʈ) ũ�⸸ŭ �����͸� ����
                err_display("recv()");
                break;
            }
            else if (retval == 0)
                break;

            // ���� ������ ���
            printf("[TCP/%s:%d] %s", inet_ntoa(clientaddr.sin_addr),	// buf[] �迭�� �ִ� ������ ��ü�� \n\0�� ���ԵǾ� �����Ƿ� �ٹٲ� ���� ȭ�鿡 ���ڿ��� ��µ�
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
