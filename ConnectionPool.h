#pragma once
//���ӳ�ͷ�ļ�
#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H
#include<queue>
#include "MySQLConnect.h"
#include <mutex> //C++��ռ�Ļ�����
#include <condition_variable> //����C++��������
using namespace std;
//���ӳ�
class ConnectionPool
{
public:
	//��̬ʵ��,ͨ����̬�������Ψһ�ĵ�������
	static ConnectionPool* getConnectPool();
	//ɾ�������캯��
	ConnectionPool(const ConnectionPool& obj) = delete;
	//�ƶ���ֵ�������أ�ɾ��������ֹ����ĸ���
	ConnectionPool& operator =(const ConnectionPool& obj)= delete;
	//��ȡ����ʱ����һ�����õ�����,���ع��������ָ��
	shared_ptr<MySqlConnect> getConnection();
	//��������
	~ConnectionPool();
private:
	ConnectionPool();
	//����JSON�ļ��ĺ���
	bool paraseJsonFile();
	//�����������ݿ�����
	void produceConnection();
	
	//�����������ݿ����� �������ݿ�����
	void recycleConnection();
	//�������
	void addConnection();

	//���ݿ������Ϣ
	// ͨ�����������ļ�Json�������û�ָ�������ݿ�
	//���ݿ�ip
	string m_ip; 
	//���ݿ��û�
	string m_user;
	//���ݿ�����
	string m_passwd;
	//���ݿ�����
	string m_dbName;
	//���ݿ���ʶ˿�
	unsigned short m_port;
	//������������
	int m_minSize;
	//�������ӵ�����
	int m_maxSize;

	//�����̵߳ȴ����ʱ��,��λ����
	//��ʱʱ��
	int m_timeout; 
	//������ʱ����λ����
	int m_maxIdleTime;

	//�洢�������ݿ����Ӷ���
	queue<MySqlConnect*> m_connectionQ;
	//���û�����
	mutex m_mutexQ;
	//������������
	condition_variable m_cond;

};






#endif // !CONNECTIONPOOL_H

