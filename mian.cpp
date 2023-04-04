#define _CRT_SECURE_NO_WARNINGS
#include <json/json.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <mysql.h>
#include <memory>
#include "MySQLConnect.h" //MySQL
#include "ConnectionPool.h" //连接池
using namespace std;
//单线程: 使用/不使用连接池
//多线程：使用/不使用连接池
//循环插入操作，从begin到end
void op1(int begin, int end)
{
	for (int i = begin; i < end; ++i)
	{
		//建立MySQL连接类
		MySqlConnect conn;
		//调用connect方法连接
		if (conn.connect("root", "5211314", "cpp", "localhost"))
		{
			//建立sql数组
			char sql[1024] = { 0 };
			//sprintf用于将格式化的字符串写入到一个字符数组中
			// 第一个参数str用于存储格式化后的字符串
			// 第二个format是一个格式化字符串，用于指定输出的格式，
			// 第三个为参数
			//执行插入语句
			sprintf(sql, "insert into person values(%d,25,'man','tom')", i);
			//执行插入操作
			
			conn.update(sql);
		}
		else
		{
			cout << "请检查数据库连接，数据库连接失败" << endl;
		}
		
	}
}
//使用数据库连接池
void op2(ConnectionPool* pool, int begin, int end)
{
	for (int i = begin; i < end; ++i)
	{
		//返回共享的智能指针
		shared_ptr<MySqlConnect> conn = pool->getConnection();
		
		char sql[1024] = { 0 };
		//执行插入语句
		sprintf(sql, "insert into person values(%d,25,'man','tom')", i);
		//执行
		conn->update(sql);
	}
}

//单线程测试函数
void test1()
{
#if 0
	//非连接池, 单线程，用时:59071963900纳秒, 59071毫秒 ，59秒
	cout << "非连接池，单线程测试:" << endl;
	//绝对时钟
	//now：表示当前时间的时间点
	steady_clock::time_point begin = steady_clock::now();
	op1(0, 5000); //单线程每次插入数据都要建立数据库连接，而且析构数据库连接TCP连接浪费较多时间
	steady_clock::time_point end = steady_clock::now();
	auto length = end - begin;
	cout << "非连接池,单线程，用时:" << length.count() << "纳秒,"
		<< length.count() / 1000000 << "毫秒 ，" 
		<< length.count() / 1000000000 << "秒" << endl;
#else
	//连接池,单  线程，用时:10798039100纳秒,10798毫秒 ，10秒
	cout << "连接池，单线程测试:" << endl;
	ConnectionPool* pool = ConnectionPool::getConnectPool();
	//绝对时钟
	steady_clock::time_point begin = steady_clock::now();
	op2(pool,0,5000);
	steady_clock::time_point end = steady_clock::now();
	auto length = end - begin;
	cout << "连接池,单  线程，用时:" << length.count() << "纳秒,"
		<< length.count() / 1000000 << "毫秒 ，"
		<< length.count() / 1000000000 << "秒" << endl;

#endif
}

//
void test2()
{
#if 0
	//多线程非连接池，用时:23374580700纳秒,23374毫秒 ，23秒
	cout << "多线程非连接池:" << endl;
	//额外的数据库连接
	MySqlConnect conn;
	conn.connect("root", "5211314", "cpp", "localhost");
	steady_clock::time_point begin = steady_clock::now();
	//创建线程
	thread t1(op1, 0, 1000);
	thread t2(op1, 1000, 2000);
	thread t3(op1, 2000, 3000);
	thread t4(op1, 3000, 4000);
	thread t5(op1, 4000, 5000);
	//使用join方法，等待线程执行完毕
	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
	steady_clock::time_point end = steady_clock::now();
	auto length = end - begin;
	cout << "多线程非连接池，用时:" << length.count() << "纳秒,"
		<< length.count() / 1000000 << "毫秒 ，"
		<< length.count() / 1000000000 << "秒" << endl;
#else
	//连接池,单  线程，用时:5333870000纳秒,5333毫秒 ，5秒
	cout << "多线程连接池:" << endl;
	ConnectionPool* pool = ConnectionPool::getConnectPool();
	steady_clock::time_point begin = steady_clock::now();
	thread t1(op2,pool, 0, 1000);
	thread t2(op2, pool, 1000, 2000);
	thread t3(op2, pool, 2000, 3000);
	thread t4(op2, pool, 3000, 4000);
	thread t5(op2, pool, 4000, 5000);
	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
	steady_clock::time_point end = steady_clock::now();
	auto length = end - begin;
	cout << "连接池,单  线程，用时:" << length.count() << "纳秒,"
		<< length.count() / 1000000 << "毫秒 ，"
		<< length.count() / 1000000000 << "秒" << endl;
#endif
}



int query()
{
	//建立MySQL连接
	MySqlConnect conn;
	//调用connect方法建立数据库
	conn.connect("root", "5211314", "cpp", "localhost");
	//执行插入语句
	string sql="insert into person values(5,25,'man','tom')";
	//执行插入，更新，删除操作
	
		
	bool flag = conn.update(sql);
	//boolalpha以true和false形式输出
	cout << boolalpha<<  "flag value: " << flag << endl;
	
		
	
	sql = "select * from person";
	//查询数据库

	
	conn.query(sql);
	//返回结果集记录
	while (conn.next())
	{
		cout << conn.value(0) << ", "
			<< conn.value(1) << ", "
			<< conn.value(2) << ", "
			<< conn.value(3) << endl;
	}
	
	return 0;
}



int main(void)
{
	//检测MySQL的环境以及SQL的环境
	printf("MySQL Environment Successful\n");
	//直接操控MySQL进行管理
	//query();
	//test1();
	test2();
	return 0;
}