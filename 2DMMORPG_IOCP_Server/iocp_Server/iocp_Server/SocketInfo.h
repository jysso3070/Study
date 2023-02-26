#pragma once
#include"globals.h"

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

class SocketInfo
{
public:
	SocketInfo();
	~SocketInfo();
public:
	void set_over(const OVER_EX& over) { m_recv_over = over; }
	void set_socket(const SOCKET& socket) { m_socket = socket; }
	void set_id(const int& id) { m_id = id; }
	void set_DBkey(const int& DBkey) { m_DBkey = DBkey; }
	void set_obj_type(const int& type) { m_obj_type = type; }
	void set_xPos(const short& x) { m_x = x; }
	void set_yPos(const short& y) { m_y = y; }
	void set_hp(const short& hp) { m_hp = hp; }
	void set_att(const short& att) { m_att = att; }
	void set_exp(const short& exp) { m_exp = exp; }
	void set_level(const short& level) { m_level = level; }
	void set_moveCooltime(const bool& flag) { m_move_cooltime = flag; }
	void set_isDead(const bool flag) { m_is_dead = flag; }
	void set_isLive(const bool flag) { m_is_live = flag; }
	void set_isActive(const bool flag) { m_is_active = flag; }
	void insert_viewList(const int& obj_id) { m_view_list.insert(obj_id); }
	int viewList_count(const int& id) { return m_view_list.count(id); }
	void delete_viewList(const int& id) { m_view_list.erase(id); }
	void add_xPos(const int& num) { m_x += num; }
	void add_yPos(const int& num) { m_y += num; }
	void set_luaL_newState() { m_L = luaL_newstate(); }

	void set_targetID(const short& obj_id) { m_target_id = obj_id; }

	OVER_EX& get_over() { return m_recv_over; }
	SOCKET& get_socket() { return m_socket; }
	int get_id() const { return m_id; }
	int get_DBkey() const { return m_DBkey; }
	char get_obj_type() const { return m_obj_type; }
	short get_xPos() const { return m_x; }
	short get_yPos() const { return m_y; }
	short get_hp() const { return m_hp; }
	short get_att() const { return m_att; }
	short get_exp() const { return m_exp; }
	short get_level() const { return m_level; }
	bool get_moveCooltime() const { return m_move_cooltime; }
	bool get_isDead() const { return m_is_dead; }
	bool get_isLive() const { return m_is_live; }
	bool get_isActive() const { return m_is_active; }
	set<int> get_viewList() { return m_view_list; }
	mutex& get_viewList_lock() { return m_view_list_lock; }
	lua_State* get_L() { return m_L; }
	mutex& get_vm_lock() { return m_vm_lock; }
	short get_targetID() const { return m_target_id; }


private:
	OVER_EX m_recv_over;
	SOCKET m_socket;
	int m_id;
	int m_DBkey;
	char m_obj_type;
	short m_x, m_y;
	short m_hp;
	short m_att;
	short m_exp;
	short m_level;
	bool m_move_cooltime;
	bool m_is_dead = false;
	bool m_is_live = false;
	bool m_is_active = false;
	set<int> m_view_list; // 시야범위안에있는 다른 플레이어 목록
	mutex m_view_list_lock;
	lua_State* m_L;
	mutex m_vm_lock;
	short m_target_id;
};

