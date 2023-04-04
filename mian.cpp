#define _CRT_SECURE_NO_WARNINGS
#include <json/json.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <mysql.h>
#include <memory>
#include "MySQLConnect.h" //MySQL
#include "ConnectionPool.h" //���ӳ�
using namespace std;
//���߳�: ʹ��/��ʹ�����ӳ�
//���̣߳�ʹ��/��ʹ�����ӳ�
//ѭ�������������begin��end
void op1(int begin, int end)
{
	for (int i = begin; i < end; ++i)
	{
		//����MySQL������
		MySqlConnect conn;
		//����connect��������
		if (conn.connect("root", "5211314", "cpp", "localhost"))
		{
			//����sql����
			char sql[1024] = { 0 };
			//sprintf���ڽ���ʽ�����ַ���д�뵽һ���ַ�������
			// ��һ������str���ڴ洢��ʽ������ַ���
			// �ڶ���format��һ����ʽ���ַ���������ָ������ĸ�ʽ��
			// ������Ϊ����
			//ִ�в������
			sprintf(sql, "insert into person values(%d,25,'man','tom')", i);
			//ִ�в������
			
			conn.update(sql);
		}
		else
		{
			cout << "�������ݿ����ӣ����ݿ�����ʧ��" << endl;
		}
		
	}
}
//ʹ�����ݿ����ӳ�
void op2(ConnectionPool* pool, int begin, int end)
{
	for (int i = begin; i < end; ++i)
	{
		//���ع��������ָ��
		shared_ptr<MySqlConnect> conn = pool->getConnection();
		
		char sql[1024] = { 0 };
		//ִ�в������
		sprintf(sql, "insert into person values(%d,25,'man','tom')", i);
		//ִ��
		conn->update(sql);
	}
}

//���̲߳��Ժ���
void test1()
{
#if 0
	//�����ӳ�, ���̣߳���ʱ:59071963900����, 59071���� ��59��
	cout << "�����ӳأ����̲߳���:" << endl;
	//����ʱ��
	//now����ʾ��ǰʱ���ʱ���
	steady_clock::time_point begin = steady_clock::now();
	op1(0, 5000); //���߳�ÿ�β������ݶ�Ҫ�������ݿ����ӣ������������ݿ�����TCP�����˷ѽ϶�ʱ��
	steady_clock::time_point end = steady_clock::now();
	auto length = end - begin;
	cout << "�����ӳ�,���̣߳���ʱ:" << length.count() << "����,"
		<< length.count() / 1000000 << "���� ��" 
		<< length.count() / 1000000000 << "��" << endl;
#else
	//���ӳ�,��  �̣߳���ʱ:10798039100����,10798���� ��10��
	cout << "���ӳأ����̲߳���:" << endl;
	ConnectionPool* pool = ConnectionPool::getConnectPool();
	//����ʱ��
	steady_clock::time_point begin = steady_clock::now();
	op2(pool,0,5000);
	steady_clock::time_point end = steady_clock::now();
	auto length = end - begin;
	cout << "���ӳ�,��  �̣߳���ʱ:" << length.count() << "����,"
		<< length.count() / 1000000 << "���� ��"
		<< length.count() / 1000000000 << "��" << endl;

#endif
}

//
void test2()
{
#if 0
	//���̷߳����ӳأ���ʱ:23374580700����,23374���� ��23��
	cout << "���̷߳����ӳ�:" << endl;
	//��������ݿ�����
	MySqlConnect conn;
	conn.connect("root", "5211314", "cpp", "localhost");
	steady_clock::time_point begin = steady_clock::now();
	//�����߳�
	thread t1(op1, 0, 1000);
	thread t2(op1, 1000, 2000);
	thread t3(op1, 2000, 3000);
	thread t4(op1, 3000, 4000);
	thread t5(op1, 4000, 5000);
	//ʹ��join�������ȴ��߳�ִ�����
	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
	steady_clock::time_point end = steady_clock::now();
	auto length = end - begin;
	cout << "���̷߳����ӳأ���ʱ:" << length.count() << "����,"
		<< length.count() / 1000000 << "���� ��"
		<< length.count() / 1000000000 << "��" << endl;
#else
	//���ӳ�,��  �̣߳���ʱ:5333870000����,5333���� ��5��
	cout << "���߳����ӳ�:" << endl;
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
	cout << "���ӳ�,��  �̣߳���ʱ:" << length.count() << "����,"
		<< length.count() / 1000000 << "���� ��"
		<< length.count() / 1000000000 << "��" << endl;
#endif
}



int query()
{
	//����MySQL����
	MySqlConnect conn;
	//����connect�����������ݿ�
	conn.connect("root", "5211314", "cpp", "localhost");
	//ִ�в������
	string sql="insert into person values(5,25,'man','tom')";
	//ִ�в��룬���£�ɾ������
	
		
	bool flag = conn.update(sql);
	//boolalpha��true��false��ʽ���
	cout << boolalpha<<  "flag value: " << flag << endl;
	
		
	
	sql = "select * from person";
	//��ѯ���ݿ�

	
	conn.query(sql);
	//���ؽ������¼
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
	//���MySQL�Ļ����Լ�SQL�Ļ���
	printf("MySQL Environment Successful\n");
	//ֱ�Ӳٿ�MySQL���й���
	//query();
	//test1();
	test2();
	return 0;
}