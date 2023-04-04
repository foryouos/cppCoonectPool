#include "MySQLConnect.h"

//�ͷŽ�����ռ�
void MySqlConnect::freeResult()
{
	if (m_result)
	{
		//�ͷ����ݿ�����
		mysql_free_result(m_result);
		//�����ݿ�ָ���ÿ�
		m_result = nullptr;
	}
}
//����MySQL���ݿ�
MySqlConnect::MySqlConnect()
{
	//��ʼ��MySQL
	m_conn = mysql_init(nullptr);
	//����MySQL�ĸ�ʽ�ַ�utf8
	mysql_set_character_set(m_conn, "utf8");
}
//MySQL����������
MySqlConnect::~MySqlConnect()
{
	//������Ӳ�Ϊ��
	if (m_conn != nullptr)
	{
		//�ر�MySQL����
		mysql_close(m_conn);
	}
	freeResult();
}

bool MySqlConnect::connect(string user, string passwd, string dbName, string ip, unsigned short port)
{
	//ip����Ϊstring��ʹ��.str��ipתΪchar *����
	MYSQL* ptr = mysql_real_connect(m_conn, ip.c_str(), user.c_str(), passwd.c_str(), dbName.c_str(), port, nullptr, 0);
	//���ӳɹ�����true
	//������ӳɹ�����TRUE��ʧ�ܷ���FALSE
	return ptr != nullptr;
}

bool MySqlConnect::update(string sql)
{
	//queryִ�гɹ�����0
	try
	{
		if (mysql_query(m_conn, sql.c_str()))
		{
			throw invalid_argument("ִ��������/����/ɾ��ʧ��!�������ݿ�");
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
	//queryִ�гɹ�����0
	try
	{
		if (mysql_query(m_conn, sql.c_str()))
		{
			throw invalid_argument("��ѯ���ݿ�ʧ��!");
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
	//��������Ϊ����û�б�Ҫ����
	if (m_result != nullptr)
	{
		//�����ŵ�ǰ�ֶε������е���ֵ
		m_row = mysql_fetch_row(m_result);
		//����ֶβ�Ϊ��
		if (m_row != nullptr)
		{
			return true;
		}
		
	}
	return false;
}

string MySqlConnect::value(int index)
{
	//��ʾ�е�����
	int row_num = mysql_num_fields(m_result); //�����õ�������е�����
	//�����ѯ�ĵ�index�д������У���С��0���Ǵ����
	if (index >= row_num || index < 0)
	{
		return string();
	}
	char* val = m_row[index]; //��Ϊ���������ݣ��м�����"\0"��
	unsigned long length = mysql_fetch_lengths(m_result)[index];
	return string(val, length); //����length�Ͳ�����"\0"Ϊ������������ͨ�����ȰѶ�Ӧ���ַ�ת��Ϊstring����
}

bool MySqlConnect::transaction()
{
	return mysql_autocommit(m_conn, false); //��������ֵ�������bool����
}

bool MySqlConnect::commit()
{
	return mysql_commit(m_conn);//�ύ
}

bool MySqlConnect::rollback()
{
	return mysql_rollback(m_conn);//bool���ͣ������ɹ�����TRUE��ʧ�ܷ���FALSE
}
//ˢ����ʼ����ʱ��
void MySqlConnect::refreshAliveTime()
{
	m_alivetime = steady_clock::now();
}

long long MySqlConnect::getAliveTime()
{
	nanoseconds res = steady_clock::now() - m_alivetime;
	//������ת��Ϊ����
	milliseconds millsec = duration_cast<milliseconds>(res);
	return millsec.count();
}
