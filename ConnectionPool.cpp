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