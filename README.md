# C++ 调用 MYSQL API 连接池
## environment：
* VS2022
* MySQL8.0.27
## 高并发下频繁处理瓶颈
* 建立通信：`TCP三次握手`
* 数据库服务器的`连接认证`
* 服务器`关闭连接`的资源回收
* `断开`通信的TCP四次挥手

> 如果客户端和服务端`频繁`进行类似操纵，影响整个`开发效率`

## 数据库连接池
> 为了`提高`数据库(关系型数据库)的访问`瓶颈`，除在服务器端添加缓存服务器缓存常用的数据，还可添加连接池来提高服务器访问效率
## 创建连接池思路
![连接池URML](https://mmbiz.qpic.cn/mmbiz_png/ORog4TEnkbt72TibPibqUOnXvuG7dQ3WYYmP1QVHBJDhYJ9FB3DS6GJVib2ibQKQVIiaiaVfug7kiaOMLRBbE1nNYRzew/0?wx_fmt=png "连接池UML")
>连接池主要用于`网络服务器端`，用于同时接受`多个用户端`请求，数据库与数据库客户端采用`TCP通信`.

* 数据库客户端和服务端先建立起`多个连接`
* 多线程通过`套接字通信`取出连接池中的一个连接，然后和服务器直接进行通信，通信之后再将此连接`还给连接池`(减少数据库连接和断开的次数)
* 数据库连接池对应C++中的一个数据库连接对象，即`单例模式`
* 连接池中包括数据库服务器连接对应的IP，端口，用户，密码等信息
* 对数据库对象存入`STL`当中，需要设置最大值，最小值限制队列
* 多线程从连接池中取出数据库对象若有取出，`没有等待`调用算法
* 对 连接池中的数据库连接(空间时间长的即调度算法)进行`适当`断开连接
* 共享资源的访问，需要`互斥锁`(生产者消费者问题)
## 单例模式
* `懒汉模式`
> 当使用这个类的时候才创建它
> 创建对象时，加锁保证有且仅有一个
> (有线程安全问题)

* `饿汉模式`
> 不管用不用它，只要类被创建，这个实例就有
> 没有线程安全问题

## 连接池算法实现C++
> 换将配置参考`jsoncpp`和`MySQL API`文章
[Jsoncpp配置](https://www.blog.foryouos.cn/computer-science/cpp/course-3/C%E5%92%8CcppMySQL%20API%E8%B0%83%E7%94%A8/)
[MySQl配置](https://www.blog.foryouos.cn/computer-science/cpp/course-3/jsoncpp%E5%A4%84%E7%90%86Json%E6%95%B0%E6%8D%AE/)

![连接池](https://mmbiz.qpic.cn/mmbiz_jpg/ORog4TEnkbvjQPcRPuH2CcuicpSLUXpTeGKvufeV604g8HohMsTqHcXp0ibY0kHjqCOv0ds0LaskbgiaAMiaelLBAA/0?wx_fmt=jpeg "连接池")

### MysqlConnect.h
```cpp
#include "MySQLConnect.h"

//释放结果集空间
void MySqlConnect::freeResult()
{
	if (m_result)
	{
		//释放数据库连接
		mysql_free_result(m_result);
		//将数据库指针置空
		m_result = nullptr;
	}
}
//连接MySQL数据库
MySqlConnect::MySqlConnect()
{
	//初始化MySQL
	m_conn = mysql_init(nullptr);
	//设置MySQL的格式字符utf8
	mysql_set_character_set(m_conn, "utf8");
}
//MySQL的析构函数
MySqlConnect::~MySqlConnect()
{
	//如果连接不为空
	if (m_conn != nullptr)
	{
		//关闭MySQL连接
		mysql_close(m_conn);
	}
	freeResult();
}

bool MySqlConnect::connect(string user, string passwd, string dbName, string ip, unsigned short port)
{
	//ip传入为string，使用.str将ip转为char *类型
	MYSQL* ptr = mysql_real_connect(m_conn, ip.c_str(), user.c_str(), passwd.c_str(), dbName.c_str(), port, nullptr, 0);
	//连接成功返回true
	//如果连接成功返回TRUE，失败返回FALSE
	return ptr != nullptr;
}

bool MySqlConnect::update(string sql)
{
	//query执行成功返回0
	try
	{
		if (mysql_query(m_conn, sql.c_str()))
		{
			throw invalid_argument("执行语句插入/更新/删除失败!请检查数据库");
			return false;
		};
	}
	catch (exception& e)
	{
		cout << e.what() << endl;
	}
	

	return true;
}

bool
MySqlConnect::query(string sql)
{
	freeResult();
	//query执行成功返回0
	try
	{
		if (mysql_query(m_conn, sql.c_str()))
		{
			throw invalid_argument("查询数据库失败!");
			return false;
		};
		m_result = mysql_store_result(m_conn);
	}
	catch (exception& e)
	{
		cout << e.what() << endl;
	}
	
	return true;
}

bool MySqlConnect::next()
{
	//如果结果集为空则没有必要遍历
	if (m_result != nullptr)
	{
		//保存着当前字段的所有列的数值
		m_row = mysql_fetch_row(m_result);
		//如果字段不为空
		if (m_row != nullptr)
		{
			return true;
		}
		
	}
	return false;
}

string MySqlConnect::value(int index)
{
	//表示列的数量
	int row_num = mysql_num_fields(m_result); //函数得到结果集中的列数
	//如果查询的的index列大于总列，或小于0，是错误的
	if (index >= row_num || index < 0)
	{
		return string();
	}
	char* val = m_row[index]; //若为二进制数据，中间是有"\0"的
	unsigned long length = mysql_fetch_lengths(m_result)[index];
	return string(val, length); //传入length就不会以"\0"为结束符，而是通过长度把对应的字符转换为string类型
}

bool MySqlConnect::transaction()
{
	return mysql_autocommit(m_conn, false); //函数返回值本身就是bool类型
}

bool MySqlConnect::commit()
{
	return mysql_commit(m_conn);//提交
}

bool MySqlConnect::rollback()
{
	return mysql_rollback(m_conn);//bool类型，函数成功返回TRUE，失败返回FALSE
}
//刷新起始空闲时间
void MySqlConnect::refreshAliveTime()
{
	m_alivetime = steady_clock::now();
}

long long MySqlConnect::getAliveTime()
{
	nanoseconds res = steady_clock::now() - m_alivetime;
	//将纳秒转换为毫秒
	milliseconds millsec = duration_cast<milliseconds>(res);
	return millsec.count();
}

```
### MysqlConnect.cpp

```cpp
#include "MySQLConnect.h"

//释放结果集空间
void MySqlConnect::freeResult()
{
	if (m_result)
	{
		//释放数据库连接
		mysql_free_result(m_result);
		//将数据库指针置空
		m_result = nullptr;
	}
}
//连接MySQL数据库
MySqlConnect::MySqlConnect()
{
	//初始化MySQL
	m_conn = mysql_init(nullptr);
	//设置MySQL的格式字符utf8
	mysql_set_character_set(m_conn, "utf8");
}
//MySQL的析构函数
MySqlConnect::~MySqlConnect()
{
	//如果连接不为空
	if (m_conn != nullptr)
	{
		//关闭MySQL连接
		mysql_close(m_conn);
	}
	freeResult();
}

bool MySqlConnect::connect(string user, string passwd, string dbName, string ip, unsigned short port)
{
	//ip传入为string，使用.str将ip转为char *类型
	MYSQL* ptr = mysql_real_connect(m_conn, ip.c_str(), user.c_str(), passwd.c_str(), dbName.c_str(), port, nullptr, 0);
	//连接成功返回true
	//如果连接成功返回TRUE，失败返回FALSE
	return ptr != nullptr;
}

bool MySqlConnect::update(string sql)
{
	//query执行成功返回0
	try
	{
		if (mysql_query(m_conn, sql.c_str()))
		{
			throw invalid_argument("执行语句插入/更新/删除失败!请检查数据库");
			return false;
		};
	}
	catch (exception& e)
	{
		cout << e.what() << endl;
	}
	

	return true;
}

bool
MySqlConnect::query(string sql)
{
	freeResult();
	//query执行成功返回0
	try
	{
		if (mysql_query(m_conn, sql.c_str()))
		{
			throw invalid_argument("查询数据库失败!");
			return false;
		};
		m_result = mysql_store_result(m_conn);
	}
	catch (exception& e)
	{
		cout << e.what() << endl;
	}
	
	return true;
}

bool MySqlConnect::next()
{
	//如果结果集为空则没有必要遍历
	if (m_result != nullptr)
	{
		//保存着当前字段的所有列的数值
		m_row = mysql_fetch_row(m_result);
		//如果字段不为空
		if (m_row != nullptr)
		{
			return true;
		}
		
	}
	return false;
}

string MySqlConnect::value(int index)
{
	//表示列的数量
	int row_num = mysql_num_fields(m_result); //函数得到结果集中的列数
	//如果查询的的index列大于总列，或小于0，是错误的
	if (index >= row_num || index < 0)
	{
		return string();
	}
	char* val = m_row[index]; //若为二进制数据，中间是有"\0"的
	unsigned long length = mysql_fetch_lengths(m_result)[index];
	return string(val, length); //传入length就不会以"\0"为结束符，而是通过长度把对应的字符转换为string类型
}

bool MySqlConnect::transaction()
{
	return mysql_autocommit(m_conn, false); //函数返回值本身就是bool类型
}

bool MySqlConnect::commit()
{
	return mysql_commit(m_conn);//提交
}

bool MySqlConnect::rollback()
{
	return mysql_rollback(m_conn);//bool类型，函数成功返回TRUE，失败返回FALSE
}
//刷新起始空闲时间
void MySqlConnect::refreshAliveTime()
{
	m_alivetime = steady_clock::now();
}

long long MySqlConnect::getAliveTime()
{
	nanoseconds res = steady_clock::now() - m_alivetime;
	//将纳秒转换为毫秒
	milliseconds millsec = duration_cast<milliseconds>(res);
	return millsec.count();
}
```
### ConnectionPool.h
```cpp
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
```
### ConnectionPool.cpp
```cpp
#include "ConnectionPool.h"
#include <json/json.h>
#include <fstream>
#include <mysql.h>
#include <thread> //加载多线程
using namespace Json;
//静态函数
ConnectionPool* ConnectionPool::getConnectPool()
{
    static ConnectionPool pool; //静态局部对象，不管后面调用多少次，得到的都是同一块内存地址

    return &pool;
}
//打开数据库信息文件，并判断是否读取到相关信息
bool ConnectionPool::paraseJsonFile()
{
    try
    {
        ifstream ifs("dbconf.json");
        Reader rd;
        Value root;
        rd.parse(ifs, root);
        if (root.isObject())
        {
            m_ip = root["ip"].asString();
            m_port = root["port"].asInt();
            m_user = root["userName"].asString();
            m_passwd = root["password"].asString();
            m_dbName = root["dbName"].asString();
            m_minSize = root["minSize"].asInt();
            m_maxSize = root["maxSize"].asInt();
            m_maxIdleTime = root["maxIdlTime"].asInt();
            m_timeout = root["timeout"].asInt();
            return true;
        }
        throw("读取连接数据库json失败！");
        return false;
    }
    catch (exception& e)
    {
        cout << e.what() << endl;
    }
    
    
}
//子线程对应的任务函数，生成新的可用连接
void ConnectionPool::produceConnection()
{
    //
    while (true) 
    {
        //判断当前连接池是否够用
        //uniuqe模版类，mutex互斥锁类型 locker对象管理
        unique_lock<mutex> locker(m_mutexQ);
        while (m_connectionQ.size() >= m_minSize)
        {
            //阻塞条件变量
            m_cond.wait(locker);
        }
        //生产一个数据库连接
        addConnection();
        //调用对应的唤醒函数，唤醒的所有消费者
        m_cond.notify_all();
    }
}

//当空闲的链接数量过多
void ConnectionPool::recycleConnection()
{
    while (true)
    {
        //休息一段时间,每隔一秒种，进行一次检测
        this_thread::sleep_for(chrono::milliseconds(500));
        //进行加锁
        lock_guard<mutex>locker(m_mutexQ);
        //当大于最小连接数
        while (m_connectionQ.size() > m_minSize)
        {
            //先进后出
            MySqlConnect* conn = m_connectionQ.front(); //取出队头元素
            //判断队头元素存活时长是不是大于指定的最长存活时长
            if (conn->getAliveTime() >= m_maxIdleTime)
            {
                m_connectionQ.pop(); //将队头的链接销毁
                delete conn;
            }
            else
            {
                break;
            }
        }
    }
}
void ConnectionPool::addConnection()
{
    MySqlConnect* conn = new MySqlConnect;
    conn->connect(m_user, m_passwd, m_dbName, m_ip, m_port);
    //数据库连接之后就开始记录时间错
    conn->refreshAliveTime();
    m_connectionQ.push(conn);
}
shared_ptr<MySqlConnect> ConnectionPool::getConnection()
{
    //封装互斥锁,保证线程安全
    unique_lock<mutex> locker(m_mutexQ);
    //检查是否有可用的连接，如果没有阻塞一会
    while (m_connectionQ.empty())
    {
        if (cv_status::timeout == m_cond.wait_for(locker, chrono::milliseconds(m_timeout)))
 
        {
            if (m_connectionQ.empty())
            {
                continue;
            }
        }
    }
    shared_ptr<MySqlConnect> connptr(m_connectionQ.front(), [this](MySqlConnect* conn) 
        {
            //加锁
            lock_guard<mutex> locker(m_mutexQ);
           //刷新起始空闲时间
            conn->refreshAliveTime();   
            m_connectionQ.push(conn);
        });
    m_connectionQ.pop();

    m_cond.notify_all();//唤醒生产者

    return connptr;
}
//线程池析构函数
ConnectionPool::~ConnectionPool()
{
    while (m_connectionQ.empty())
    {
        MySqlConnect* conn = m_connectionQ.front();
        m_connectionQ.pop();
        delete conn;
    }
}
//构造函数的实现
ConnectionPool::ConnectionPool()
{
    //加载配置文件
    if (!paraseJsonFile())
    {
        cout << "数据库连接失败" << endl;
        return;
    }
    //初始化配置连接数
    for (int i = 0; i < m_minSize; ++i)  //连接数
    { 
        //如果队列总数小于最大数量
        if (m_connectionQ.size() < m_maxSize)
        {
            //实例化对象
            MySqlConnect* conn = new MySqlConnect;
            //链接数据库
            conn->connect(m_user, m_passwd, m_dbName, m_ip, m_port);

            m_connectionQ.push(conn);
        }
        //当连接总数大于允许连接的最大数量
        else
        {
            cout << "当前连接数量以超过允许连接的总数" << endl;
            break;
        }
    }
    //当前实例对象this指针，单例模式，
    thread producer(&ConnectionPool::produceConnection,this); //生成线程池的连接
    thread recycler(&ConnectionPool::recycleConnection,this); //有没有需要销毁的连接
    /*
    将producer线程与当前线程分离，使得它们可以独立执行，
    */
    //主线程和子线程分离
    producer.detach();
    recycler.detach();
}
```
### main.cpp
```cpp
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
```

参考资料:
* [B站爱编程的大丙](https://www.bilibili.com/video/BV1Fr4y1s7w4/)
* 《深入设计模式》-亚历山大什韦茨
