#include "ConnectionPool.h"
#include <json/json.h>
#include <fstream>
#include <mysql.h>
#include <thread> //���ض��߳�
using namespace Json;
//��̬����
ConnectionPool* ConnectionPool::getConnectPool()
{
    static ConnectionPool pool; //��̬�ֲ����󣬲��ܺ�����ö��ٴΣ��õ��Ķ���ͬһ���ڴ��ַ

    return &pool;
}
//�����ݿ���Ϣ�ļ������ж��Ƿ��ȡ�������Ϣ
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
        throw("��ȡ�������ݿ�jsonʧ�ܣ�");
        return false;
    }
    catch (exception& e)
    {
        cout << e.what() << endl;
    }
    
    
}
//���̶߳�Ӧ���������������µĿ�������
void ConnectionPool::produceConnection()
{
    //
    while (true) 
    {
        //�жϵ�ǰ���ӳ��Ƿ���
        //uniuqeģ���࣬mutex���������� locker�������
        unique_lock<mutex> locker(m_mutexQ);
        while (m_connectionQ.size() >= m_minSize)
        {
            //������������
            m_cond.wait(locker);
        }
        //����һ�����ݿ�����
        addConnection();
        //���ö�Ӧ�Ļ��Ѻ��������ѵ�����������
        m_cond.notify_all();
    }
}

//�����е�������������
void ConnectionPool::recycleConnection()
{
    while (true)
    {
        //��Ϣһ��ʱ��,ÿ��һ���֣�����һ�μ��
        this_thread::sleep_for(chrono::milliseconds(500));
        //���м���
        lock_guard<mutex>locker(m_mutexQ);
        //��������С������
        while (m_connectionQ.size() > m_minSize)
        {
            //�Ƚ����
            MySqlConnect* conn = m_connectionQ.front(); //ȡ����ͷԪ��
            //�ж϶�ͷԪ�ش��ʱ���ǲ��Ǵ���ָ��������ʱ��
            if (conn->getAliveTime() >= m_maxIdleTime)
            {
                m_connectionQ.pop(); //����ͷ����������
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
    //���ݿ�����֮��Ϳ�ʼ��¼ʱ���
    conn->refreshAliveTime();
    m_connectionQ.push(conn);
}
shared_ptr<MySqlConnect> ConnectionPool::getConnection()
{
    //��װ������,��֤�̰߳�ȫ
    unique_lock<mutex> locker(m_mutexQ);
    //����Ƿ��п��õ����ӣ����û������һ��
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
            //����
            lock_guard<mutex> locker(m_mutexQ);
           //ˢ����ʼ����ʱ��
            conn->refreshAliveTime();   
            m_connectionQ.push(conn);
        });
    m_connectionQ.pop();

    m_cond.notify_all();//����������

    return connptr;
}
//�̳߳���������
ConnectionPool::~ConnectionPool()
{
    while (m_connectionQ.empty())
    {
        MySqlConnect* conn = m_connectionQ.front();
        m_connectionQ.pop();
        delete conn;
    }
}
//���캯����ʵ��
ConnectionPool::ConnectionPool()
{
    //���������ļ�
    if (!paraseJsonFile())
    {
        cout << "���ݿ�����ʧ��" << endl;
        return;
    }
    //��ʼ������������
    for (int i = 0; i < m_minSize; ++i)  //������
    { 
        //�����������С���������
        if (m_connectionQ.size() < m_maxSize)
        {
            //ʵ��������
            MySqlConnect* conn = new MySqlConnect;
            //�������ݿ�
            conn->connect(m_user, m_passwd, m_dbName, m_ip, m_port);

            m_connectionQ.push(conn);
        }
        //���������������������ӵ��������
        else
        {
            cout << "��ǰ���������Գ����������ӵ�����" << endl;
            break;
        }
    }
    //��ǰʵ������thisָ�룬����ģʽ��
    thread producer(&ConnectionPool::produceConnection,this); //�����̳߳ص�����
    thread recycler(&ConnectionPool::recycleConnection,this); //��û����Ҫ���ٵ�����
    /*
    ��producer�߳��뵱ǰ�̷߳��룬ʹ�����ǿ��Զ���ִ�У�
    */
    //���̺߳����̷߳���
    producer.detach();
    recycler.detach();
}