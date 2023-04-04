#pragma once
//连接池头文件
#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H
#include<queue>
#include "MySQLConnect.h"
#include <mutex> //C++独占的互斥锁
#include <condition_variable> //引用C++条件变量
using namespace std;
//连接池
class ConnectionPool
{
public:
	//静态实例,通过静态方法获得唯一的单例对象
	static ConnectionPool* getConnectPool();
	//删除掉构造函数
	ConnectionPool(const ConnectionPool& obj) = delete;
	//移动赋值函数重载，删除掉，防止对象的复制
	ConnectionPool& operator =(const ConnectionPool& obj)= delete;
	//获取连接时返回一个可用的连接,返回共享的智能指针
	shared_ptr<MySqlConnect> getConnection();
	//析构函数
	~ConnectionPool();
private:
	ConnectionPool();
	//解析JSON文件的函数
	bool paraseJsonFile();
	//用来生产数据库连接
	void produceConnection();
	
	//用来销毁数据库连接 回收数据库连接
	void recycleConnection();
	//添加连接
	void addConnection();

	//数据库相关信息
	// 通过加载配置文件Json，访问用户指定的数据库
	//数据库ip
	string m_ip; 
	//数据库用户
	string m_user;
	//数据库密码
	string m_passwd;
	//数据库名称
	string m_dbName;
	//数据库访问端口
	unsigned short m_port;
	//设置连接上限
	int m_minSize;
	//设置连接的上限
	int m_maxSize;

	//设置线程等待最大时长,单位毫秒
	//超时时长
	int m_timeout; 
	//最大空闲时长单位毫秒
	int m_maxIdleTime;

	//存储若干数据库连接队列
	queue<MySqlConnect*> m_connectionQ;
	//设置互斥锁
	mutex m_mutexQ;
	//设置条件变量
	condition_variable m_cond;

};






#endif // !CONNECTIONPOOL_H

