/********************************
�ǽ� 3-1 

����Ʈ ���� �Լ� 
*********************************/


#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	WSADATA wsa;
	if(WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	if(s == SOCKET_ERROR)
		return 1;

	u_short x1 = 0x1234;
	u_long  y1 = 0x12345678;
	u_short x2, x3;
	u_long  y2, y3;

	// ȣ��Ʈ ����Ʈ -> ��Ʈ��ũ ����Ʈ
	printf("[ȣ��Ʈ ����Ʈ -> ��Ʈ��ũ ����Ʈ]\n");
	WSAHtons(s, x1, &x2);
	printf("0x%x -> 0x%x\n", x1, x2);
	WSAHtonl(s, y1, &y2);
	printf("0x%x -> 0x%x\n", y1, y2);

	// ��Ʈ��ũ ����Ʈ -> ȣ��Ʈ ����Ʈ
	printf("\n[��Ʈ��ũ ����Ʈ -> ȣ��Ʈ ����Ʈ]\n");
	WSANtohs(s, x2, &x3);
	printf("0x%x -> 0x%x\n", x2, x3);
	WSANtohl(s, y2, &y3);
	printf("0x%x -> 0x%x\n", y2, y3);

	// �߸��� ��� ��
	printf("\n[�߸��� ��� ��]\n");
	WSAHtonl(s, x1, &y3);
	printf("0x%x -> 0x%x\n", x1, y3);

	WSACleanup();
	return 0;
}