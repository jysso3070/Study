/***************************************
5장 두번째 연습

파일 전송후 종료 - Client module
***************************************/

#pragma comment(lib, "ws2_32")
#pragma warning(disable:4996)
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000

// 소켓 함수 오류 출력 후 종료
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

// 소켓 함수 오류 출력
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

	// 윈속 초기화
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

	// 파일 열기
	FILE *fp = fopen(argv[1], "rb");
	if(fp == NULL){
		perror("fopen()");
		return 1;
	}

	// 파일 이름 보내기(고정 길이+가변 길이)
	char filename[1024];
	sprintf(filename, argv[1]);
	int length = strlen(filename) + 1;
	retval = send(sock, (char *)&length, sizeof(int), 0); /* 고정 길이 */
	if(retval == SOCKET_ERROR) err_quit("send()");
	retval = send(sock, filename, length, 0);             /* 가변 길이 */
	if(retval == SOCKET_ERROR) err_quit("send()");

	// 파일 크기와 데이터 보내기(고정 길이+가변 길이)

	/*--- 파일 크기: 고정 길이 ---*/
	fseek(fp, 0, SEEK_END);			// 파일 포인터를 파일의 끝으로 이동시킴
	length = ftell(fp);				// 파일 포인터의 현재 위치를 얻음
	retval = send(sock, (char *)&length, sizeof(int), 0);
	if(retval == SOCKET_ERROR) err_quit("send()");
	rewind(fp);

	/*--- 파일 데이터: 가변 길이 ---*/
	char *buf = (char *)malloc(length);
	if(buf == NULL){
		fprintf(stderr, "malloc() error!\n");
		exit(1);
	}

	int nbytes = fread(buf, 1, length, fp);			// 1바이트씩 length만큼 읽기
	if(nbytes == length){
		retval = send(sock, buf, nbytes, 0);
		if(retval == SOCKET_ERROR) err_quit("send()");
		printf("-> 파일 전송 완료! (%d바이트)\n", nbytes);
	}
	else{
		perror("fread()");
	}
	fclose(fp);

	// closesocket()
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}