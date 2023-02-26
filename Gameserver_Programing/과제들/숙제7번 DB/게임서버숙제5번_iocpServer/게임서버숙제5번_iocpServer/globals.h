#pragma once
#include <iostream> 
#include <map> 
#include <thread>
#include <set>
#include <vector>
#include <algorithm>
#include <iterator>
#include <list>
#include <mutex>
using namespace std;

//sql
#define UNICODE  
#include <sqlext.h>  

struct DATABASE
{
	int idKey;
	int level;
	char idName[10];
	short x, y;
};