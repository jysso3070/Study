
//  *������� ��Ƽ����Ʈ�μ��� �����ڵ�� �ҷ��� SQLWCHAR�� �����ϰ� ��罺Ʈ���տ� �빮��L�ٿ�����Ѵ�
// SQLBindCol_ref.cpp  
// compile with: odbc32.lib  
#include <windows.h>  //64��Ʈ ���������� �����ͺ��̽� 64��Ʈ�� �������Ѵ�  
#include <stdio.h>  
#include <iostream>
#include <set>
#include <vector>
using namespace std;

#define UNICODE  
#include <sqlext.h>  

constexpr auto NAME_LEN = 11;

struct DATABASE
{
	int id;
	int level;
	char name[10];
};

vector<DATABASE> set_database;




void show_error() {
	printf("error\n");
}

/************************************************************************
/* HandleDiagnosticRecord : display error/warning information
/*
/* Parameters:
/*      hHandle     ODBC handle
/*      hType       Type of handle (SQL_HANDLE_STMT, SQL_HANDLE_ENV, SQL_HANDLE_DBC)
/*      RetCode     Return code of failing command
/************************************************************************/
void HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode) {
	SQLSMALLINT iRec = 0;
	SQLINTEGER  iError;
	WCHAR       wszMessage[1000];
	WCHAR       wszState[SQL_SQLSTATE_SIZE + 1];
	if (RetCode == SQL_INVALID_HANDLE) {
		fwprintf(stderr, L"Invalid handle!\n");
		return;
	} while (SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage,
		(SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT *)NULL) == SQL_SUCCESS) {
		// Hide data truncated.. 
		if (wcsncmp(wszState, L"01004", 5)) {
			fwprintf(stderr, L"[%5.5s] %s (%d)\n", wszState, wszMessage, iError);
		}
	}
}

void sql_update_database(int keyid, int x, int y)
{
	SQLHENV henv;		// �����ͺ��̽��� �����Ҷ� ����ϴ� �ڵ�
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0; // sql��ɾ �����ϴ� �ڵ�
	SQLRETURN retcode;  // sql��ɾ ������ ���������� ��������
	SQLWCHAR query[1024];
	wsprintf(query, L"UPDATE player_table SET c_px = %d, c_py = %d WHERE c_key = %d", x, y, keyid);


	setlocale(LC_ALL, "korean"); // �����ڵ� �ѱ۷� ��ȯ
	//std::wcout.imbue(std::locale("korean"));

	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0); // ODBC�� ����

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0); // 5�ʰ� ���� 5�ʳѾ�� Ÿ�Ӿƿ�

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"jys_gameserver", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
				//retcode = SQLConnect(hdbc, (SQLWCHAR*)L"jys_gameserver", SQL_NTS, (SQLWCHAR*)NULL, SQL_NTS, NULL, SQL_NTS);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt); // SQL��ɾ� ������ �ѵ�

					retcode = SQLExecDirect(hstmt, (SQLWCHAR *)query, SQL_NTS); // ��� ���� �� ��������
					//retcode = SQLExecDirect(hstmt, (SQLWCHAR *)L"EXEC select_highlevel 90", SQL_NTS); // 90���� �̻� ��������

					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

					}
					else {
						HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
					}

					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLCancel(hstmt); // �ڵ�ĵ��
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}

					SQLDisconnect(hdbc);
				}

				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}
}

int main() {
	sql_update_database(1, 10, 10);


	SQLHENV henv;		// �����ͺ��̽��� �����Ҷ� ����ϴ� �ڵ�
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0; // sql��ɾ �����ϴ� �ڵ�
	SQLRETURN retcode;  // sql��ɾ ������ ���������� ��������
	SQLINTEGER nID, nLEVEL;	// ��Ƽ��
	SQLWCHAR szName[NAME_LEN];	// ���ڿ�
	SQLLEN cbName = 0, cbID = 0, cbLEVEL = 0;
	


	setlocale(LC_ALL, "korean"); // �����ڵ� �ѱ۷� ��ȯ
	//std::wcout.imbue(std::locale("korean"));

	// Allocate environment handle  
	retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0); // ODBC�� ����

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0); // 5�ʰ� ���� 5�ʳѾ�� Ÿ�Ӿƿ�

				// Connect to data source  
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"jys_gameserver", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
				//retcode = SQLConnect(hdbc, (SQLWCHAR*)L"jys_gameserver", SQL_NTS, (SQLWCHAR*)NULL, SQL_NTS, NULL, SQL_NTS);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt); // SQL��ɾ� ������ �ڵ�

					retcode = SQLExecDirect(hstmt, (SQLWCHAR *)L"SELECT c_Name, c_ID, c_Level FROM user_table ORDER BY 2, 1, 3", SQL_NTS); // ��� ���� �� ��������
					//retcode = SQLExecDirect(hstmt, (SQLWCHAR *)L"EXEC select_highlevel 90", SQL_NTS); // 90���� �̻� ��������

					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

						// Bind columns 1, 2, and 3  
						retcode = SQLBindCol(hstmt, 1, SQL_UNICODE_CHAR, szName, 11, &cbName); // �̸� �����ڵ��� SQL_UNICODE_CHAR ���
						retcode = SQLBindCol(hstmt, 2, SQL_C_LONG, &nID, 10, &cbID);	// ���̵�
						retcode = SQLBindCol(hstmt, 3, SQL_C_LONG, &nLEVEL, 10, &cbLEVEL);	// ����ġ


						// Fetch and print each row of data. On an error, display a message and exit.  
						for (int i = 0; ; i++) {
							retcode = SQLFetch(hstmt);  // hstmt ���� �����͸� �������°�
							if (retcode == SQL_ERROR)
								show_error();
							if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
								wprintf(L"%d: %d %lS %d\n", i + 1, nID, szName, nLEVEL);
								DATABASE data;
								data.id = nID;
								data.level = nLEVEL;
								char *temp;
								int strSize = WideCharToMultiByte(CP_ACP, 0, szName, -1, NULL, 0, NULL, NULL);
								temp = new char[NAME_LEN];
								WideCharToMultiByte(CP_ACP, 0, szName, -1, temp, strSize, 0, 0);
								temp[strlen(temp) - 1] = 0; // �������������� �𸣰����� �ǵڿ� �����ϳ��� �߰���
								memcpy(data.name, temp, 10);
								cout << data.name << "]\n";
								cout << data.id << "\n";
								cout << data.level << "\n";
								set_database.emplace_back(data);
							}
							else
								break;
						}
					}
					else {
						HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
					}

					// Process data  
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						SQLCancel(hstmt); // �ڵ�ĵ��
						SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
					}

					SQLDisconnect(hdbc);
				}

				SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
			}
		}
		SQLFreeHandle(SQL_HANDLE_ENV, henv);
	}

	system("pause");

}