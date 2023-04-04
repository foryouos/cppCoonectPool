#pragma once
#ifndef MYSQLCONNECT_H
#define MYSQLCONNECT_H
#include <iostream>
#include <mysql.h>
#include <chrono>
using namespace std;
using namespace chrono;
//mysql连接类
class MySqlConnect
{
private:
	//什么时候调用释放结果集
	//1,析构函数2，可能会对数据库进行多次查询，每次查询一次都会得到结果集，查询是清空掉上次的结果集
	void freeResult(); //释放结果集
	MYSQL* m_conn = nullptr; //保存MySQL初始化的私有成员
	MYSQL_RES* m_result = nullptr; //保存结果集
	MYSQL_ROW m_row = nullptr; //保存着当前字段的所有列的数值

	steady_clock::time_point m_alivetime;

public:
	//初始化数据库连接
	MySqlConnect();
	//释放数据库连接
	~MySqlConnect();
	//连接数据库,使用默认端口可省略端口书写
	bool connect(string user, string passwd, string dbName, string ip, unsigned short port = 3306);
	//更新数据库(插入，更新，删除)，传递字符串
	//执行sql语句
	bool update(string sql);
	//查询数据库,单词query:查询
	bool query(string sql);

	//遍历查询得到的结果集,每调一次，从结果集中取出一条数据
	bool next();
	//得到结果集中的字段值，取记录里面字段方法
	string value(int index);
	//事务操作，关闭自动提交
	bool transaction();
	//提交事务
	bool commit();
	//事务回滚;
	bool rollback();
	//刷起始的空闲时间点
	void refreshAliveTime();
	// 计算连接存活的总时长
	long long getAliveTime();


};
#endif // !MYSQLCONNECT_H
