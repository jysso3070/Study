#pragma once
#define UNICODE  
#include "globals.h"

class DatabaseManager
{
public:
	DatabaseManager();
	~DatabaseManager();
public:
	void sql_HandleDiagnosticRecord(SQLHANDLE hHandle, SQLSMALLINT hType, RETCODE RetCode);
	void sql_load_database();
	void sql_update_database(int keyid, short x, short y, short hp, short exp, short level);
	void sql_insert_database(int key, char id[], char name[], short level, short x, short y, short hp, short exp, short att);

	void sql_show_error();

	vector<DATABASE> vec_database;
};

