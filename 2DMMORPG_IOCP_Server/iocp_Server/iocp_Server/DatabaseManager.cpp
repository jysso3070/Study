#include "DatabaseManager.h"

DatabaseManager::DatabaseManager()
{

}

DatabaseManager::~DatabaseManager()
{
}

void DatabaseManager::sql_HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode)
{
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

void DatabaseManager::sql_load_database()
{
	SQLHENV henv;		// �����ͺ��̽��� �����Ҷ� ����ϴ� �ڵ�
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0; // sql��ɾ �����ϴ� �ڵ�
	SQLRETURN retcode;  // sql��ɾ ������ ���������� ��������
	SQLINTEGER nKey, nLevel, nX, nY, nHp, nExp;	// ��Ƽ��
	SQLWCHAR szID[11];	// ���ڿ�
	SQLLEN cbID = 0, cbKey = 0, cbLevel = 0, cbX = 0, cbY = 0, cbHp = 0, cbExp = 0;

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
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2015184024_gameserver", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
				//retcode = SQLConnect(hdbc, (SQLWCHAR*)L"jys_gameserver", SQL_NTS, (SQLWCHAR*)NULL, SQL_NTS, NULL, SQL_NTS);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt); // SQL��ɾ� ������ �ѵ�

					retcode = SQLExecDirect(hstmt, (SQLWCHAR *)L"SELECT c_id, c_key, c_level, c_px, c_py, c_hp, c_exp FROM player_table ORDER BY 2, 1, 3", SQL_NTS); // ��� ���� �� ��������
					//retcode = SQLExecDirect(hstmt, (SQLWCHAR *)L"EXEC select_highlevel 90", SQL_NTS); // 90���� �̻� ��������

					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {

						// Bind columns 1, 2, and 3  
						retcode = SQLBindCol(hstmt, 1, SQL_UNICODE_CHAR, szID, 11, &cbID); // �̸� �����ڵ��� SQL_UNICODE_CHAR ���
						retcode = SQLBindCol(hstmt, 2, SQL_C_LONG, &nKey, 4, &cbKey);	// ���̵�
						retcode = SQLBindCol(hstmt, 3, SQL_C_LONG, &nLevel, 4, &cbLevel);	// ����ġ
						retcode = SQLBindCol(hstmt, 4, SQL_C_LONG, &nX, 4, &cbX);
						retcode = SQLBindCol(hstmt, 5, SQL_C_LONG, &nY, 4, &cbY);
						retcode = SQLBindCol(hstmt, 6, SQL_C_LONG, &nHp, 4, &cbHp);
						retcode = SQLBindCol(hstmt, 7, SQL_C_LONG, &nExp, 4, &cbExp);

						// Fetch and print each row of data. On an error, display a message and exit.  
						for (int i = 0; ; i++) {
							retcode = SQLFetch(hstmt);  // hstmt ���� �����͸� �������°�
							if (retcode == SQL_ERROR)
								sql_show_error();
							if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
								wprintf(L"%d: %d %lS %d %d %d %d %d \n", i + 1, nKey, szID, nLevel, nX, nY, nHp, nExp);

								// wchar char�� ��ȯ
								char *temp;
								int strSize = WideCharToMultiByte(CP_ACP, 0, szID, -1, NULL, 0, NULL, NULL);
								temp = new char[11];
								WideCharToMultiByte(CP_ACP, 0, szID, -1, temp, strSize, 0, 0);
								if (isdigit(temp[strlen(temp) - 1]) == 0) {
									//cout << "���ڿ���������\n";
									temp[strlen(temp) - 1] = 0; // �������������� �𸣰����� ���̵��������ڰ� ������ ��� �ǵڿ� �����ϳ��� �߰���
								}
								DATABASE data;
								data.idKey = nKey;
								memcpy(data.idName, temp, 10);
								data.level = nLevel;
								data.x = (short)nX;
								data.y = (short)nY;
								data.hp = (short)nHp;
								data.exp = (short)nExp;
								cout << "[" << data.idName << "]  ";
								cout << data.idKey << " " << data.level << " " << data.x << " " << data.y << " "
									<< data.hp << " " << data.exp << "\n";
								vec_database.emplace_back(data);
							}
							else
								break;
						}
					}
					else {
						sql_HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
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
	cout << "database access complete. \n";
}

void DatabaseManager::sql_update_database(int keyid, short x, short y, short hp, short exp, short level)
{
	SQLHENV henv;		// �����ͺ��̽��� �����Ҷ� ����ϴ� �ڵ�
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0; // sql��ɾ �����ϴ� �ڵ�
	SQLRETURN retcode;  // sql��ɾ ������ ���������� ��������
	SQLWCHAR query[1024];
	wsprintf(query, L"UPDATE player_table SET c_px = %d, c_py = %d, c_hp = %d, c_exp = %d, c_level = %d WHERE c_key = %d", x, y, hp, exp, level, keyid);


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
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2015184024_gameserver", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
				//retcode = SQLConnect(hdbc, (SQLWCHAR*)L"jys_gameserver", SQL_NTS, (SQLWCHAR*)NULL, SQL_NTS, NULL, SQL_NTS);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt); // SQL��ɾ� ������ �ѵ�

					retcode = SQLExecDirect(hstmt, (SQLWCHAR *)query, SQL_NTS); // ������
					//retcode = SQLExecDirect(hstmt, (SQLWCHAR *)L"EXEC select_highlevel 90", SQL_NTS); // 90���� �̻� ��������

					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						cout << "DataBase update success";
					}
					else {
						sql_HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
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

void DatabaseManager::sql_insert_database(int key, char id[], char name[], short level, short x, short y, short hp, short exp, short att)
{
	SQLHENV henv;		// �����ͺ��̽��� �����Ҷ� ����ϴ� �ڵ�
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0; // sql��ɾ �����ϴ� �ڵ�
	SQLRETURN retcode;  // sql��ɾ ������ ���������� ��������
	SQLWCHAR query[1024];
	SQLWCHAR tempid[10];
	SQLWCHAR tempname[10];

	MultiByteToWideChar(CP_ACP, 0, id, strlen(id), tempid, lstrlen(tempid));
	//MultiByteToWideChar(CP_ACP, 0, name, strlen(name), tempname, lstrlen(tempname));

	wsprintf(query, L"INSERT INTO player_table (c_key, c_id, c_name, c_level, c_px, c_py, c_hp, c_exp, c_att) VALUES (%d, '%s', '%s', %d, %d, %d, %d, %d, %d)"
		, key, tempid, tempid, level, x, y, hp, exp, att);


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
				retcode = SQLConnect(hdbc, (SQLWCHAR*)L"2015184024_gameserver", SQL_NTS, (SQLWCHAR*)NULL, 0, NULL, 0);
				//retcode = SQLConnect(hdbc, (SQLWCHAR*)L"jys_gameserver", SQL_NTS, (SQLWCHAR*)NULL, SQL_NTS, NULL, SQL_NTS);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt); // SQL��ɾ� ������ �ѵ�

					retcode = SQLExecDirect(hstmt, (SQLWCHAR *)query, SQL_NTS); // ������
					//retcode = SQLExecDirect(hstmt, (SQLWCHAR *)L"EXEC select_highlevel 90", SQL_NTS); // 90���� �̻� ��������

					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO) {
						cout << "insert success";
					}
					else {
						sql_HandleDiagnosticRecord(hstmt, SQL_HANDLE_STMT, retcode);
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

void DatabaseManager::sql_show_error()
{
	printf("error\n");
}
