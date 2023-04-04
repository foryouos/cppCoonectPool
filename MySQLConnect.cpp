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
