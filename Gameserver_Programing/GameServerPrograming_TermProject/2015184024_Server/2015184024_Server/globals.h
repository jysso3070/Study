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
#include <fstream>
#include <chrono>
#include <queue>
#include <random>
#include <array>
using namespace std;

//sql
#define UNICODE  
#include <sqlext.h>  


struct DATABASE
{
	int idKey;
	char idName[10];
	short x, y;
	short hp;
	short exp;
	short level;
};