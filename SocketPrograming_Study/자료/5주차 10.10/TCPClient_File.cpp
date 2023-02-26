/***************************************
5�� �ι�° ����

���� ������ ���� - Client module
***************************************/

#pragma comment(lib, "ws2_32")
#pragma warning(disable:4996)
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000

// ���� �Լ� ���� ��� �� ����
void err_quit(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
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
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

int main(int argc, char *argv[])
{
	int retval;

	if(argc != 2){
		fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
		exit(1);
	}

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == INVALID_SOCKET) err_quit("socket()");

	// connect()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if(retval == SOCKET_ERROR) err_quit("connect()");

	// ���� ����
	FILE *fp = fopen(argv[1], "rb");
	if(fp == NULL){
		perror("fopen()");
		return 1;
	}

	// ���� �̸� ������(���� ����+���� ����)
	char filename[1024];
	sprintf(filename, argv[1]);
	int length = strlen(filename) + 1;
	retval = send(sock, (char *)&length, sizeof(int), 0); /* ���� ���� */
	if(retval == SOCKET_ERROR) err_quit("send()");
	retval = send(sock, filename, length, 0);             /* ���� ���� */
	if(retval == SOCKET_ERROR) err_quit("send()");

	// ���� ũ��� ������ ������(���� ����+���� ����)

	/*--- ���� ũ��: ���� ���� ---*/
	fseek(fp, 0, SEEK_END);			// ���� �����͸� ������ ������ �̵���Ŵ
	length = ftell(fp);				// ���� �������� ���� ��ġ�� ����
	retval = send(sock, (char *)&length, sizeof(int), 0);
	if(retval == SOCKET_ERROR) err_quit("send()");
	rewind(fp);

	/*--- ���� ������: ���� ���� ---*/
	char *buf = (char *)malloc(length);
	if(buf == NULL){
		fprintf(stderr, "malloc() error!\n");
		exit(1);
	}

	int nbytes = fread(buf, 1, length, fp);			// 1����Ʈ�� length��ŭ �б�
	if(nbytes == length){
		retval = send(sock, buf, nbytes, 0);
		if(retval == SOCKET_ERROR) err_quit("send()");
		printf("-> ���� ���� �Ϸ�! (%d����Ʈ)\n", nbytes);
	}
	else{
		perror("fread()");
	}
	fclose(fp);

	// closesocket()
	closesocket(sock);

	// ���� ����
	WSACleanup();
	return 0;
}