
#include "dbmysql/db_wrap.h"
#include "framework/logger.h"

using namespace std;
using namespace svrlib;

#ifndef CMDTOSTRSQL
#define CMDTOSTRSQL \
    	memset(m_szCommand,0,sizeof(m_szCommand));\
	    va_list	argptr;\
	    va_start(argptr,pCmd);\
	    dVsprintf(m_szCommand,sizeof(m_szCommand),pCmd,argptr);\
	    va_end(argptr);	
#endif    


bool	CDBWrap::dbOpen(const char*host,const char*user,const char* passwd,const char*db,unsigned int port)
{
	try
	{
		return m_clDatabase.open(host,user,passwd,db,port);
	}
	catch(CMySQLException & e)
	{
		LOG_ERROR("CDBOperator::dbOpen[host=%s,user = %s,pwd = %s]:%u(%s)->%s->%s",host,user,passwd,e.uCode,e.szError,e.szMsg,e.szSqlState);		
	}

	return false;
}
//------------------------------ 
void	CDBWrap::dbClose()
{
	m_clDatabase.close();
}
bool	CDBWrap::ExeSql(string strSql)
{
	try
	{		
		CMySQLMultiFree clMultiFree(&m_clDatabase);		
		CMySQLStatement clStatement = m_clDatabase.createStatement();
		if(!clStatement.cmd(strSql.c_str()))
		{				
			return false;
		}
		if(!clStatement.execute())
		{
			LOG_ERROR("sql:%s execute fail!",strSql.c_str()); 			
			return false;
		}		
	}
	catch(CMySQLException & e)
	{		
		LOG_ERROR("%u(%s)->%s->%s-sql:%s",e.uCode,e.szError,e.szMsg,e.szSqlState,strSql.c_str());
		return false;
	}
	return true;		
}
// ���sql ���Ľ������
uint32	CDBWrap::GetResNumExeSql(string strSql)
{
	try
	{
		CMySQLMultiFree	clMultiFree(&m_clDatabase);
		m_clDatabase.cmd(strSql.c_str());
		if(!m_clDatabase.execute())
		{
			LOG_ERROR("execute fail!,%s",strSql.c_str());
			return 0;
		}
		CMySQLResult clResult = m_clDatabase.getResult();
		return clResult.getRowCount();
	}
	catch(CMySQLException & e)
	{		
		LOG_ERROR("Error:%u(%s)->%s->%s->sql:%s",e.uCode,e.szError,e.szMsg,e.szSqlState,strSql.c_str());
		return 0;
	}
	return 0;	
}
// ���Ӱ������
uint32  CDBWrap::GetAffectedNumExeSql(string strSql)
{
	try
	{
		CMySQLMultiFree	clMultiFree(&m_clDatabase);
		m_clDatabase.cmd(strSql.c_str());
		if(!m_clDatabase.execute())
		{
			LOG_ERROR("execute fail!,%s",strSql.c_str());
			return 0;
		}
		return m_clDatabase.getRowAffected();
	}
	catch(CMySQLException & e)
	{
		LOG_ERROR("Error:%u(%s)->%s->%s->sql:%s",e.uCode,e.szError,e.szMsg,e.szSqlState,strSql.c_str());
		return 0;
	}
	return 0;
}
// ��ʽ��ִ��sql���
bool	CDBWrap::ExeSqlCmd(const char*pCmd,...)
{
	if(!pCmd)
		return false;
    CMDTOSTRSQL        

	return ExeSql(m_szCommand);	
}
uint32  CDBWrap::GetResNumExeSqlCmd(const char*pCmd,...)
{
	if(!pCmd)
		return 0;
    
    CMDTOSTRSQL		
	return GetResNumExeSql(m_szCommand);
}
uint32  CDBWrap::GetAffectedNumExeSqlCmd(const char*pCmd,...)
{
	if(!pCmd)
		return 0;

	CMDTOSTRSQL
	return GetAffectedNumExeSql(m_szCommand);
}
int     CDBWrap::Query(const char* strSql, vector<map<string, MYSQLValue> > &vecData)
{
    try
	{
		CMySQLMultiFree	clMultiFree(&m_clDatabase);		
		if(!m_clDatabase.cmd(strSql))
		{
			return -1;
		}
		if(!m_clDatabase.execute())
		{
			LOG_ERROR("mysql execute fail!");
			return -1;
		}		
		CMySQLResult clResult = m_clDatabase.getResult();
		while (clResult.rowMore())
		{
            m_tmpMapData.clear();
            for(uint32 i = 0; i < clResult.getFiledCount(); i++)
            {
                MYSQLValue &value = m_tmpMapData[clResult.getFiledName(i)];
                value.SetData((char*)clResult.getData(i),clResult.getFiledDataLength(i));
            }
            vecData.push_back(m_tmpMapData);
		}
		return 0;
	}
	catch(CMySQLException & e)
	{
		LOG_ERROR("Error:%u(%s)->%s->%s->sql:%s",e.uCode,e.szError,e.szMsg,e.szSqlState,strSql);
	}    
    return 0;
}
// ����˳���ֵ
int     CDBWrap::Query(const char* strSql, vector< vector<MYSQLValue> > &vecData)
{
    try
	{
		CMySQLMultiFree	clMultiFree(&m_clDatabase);		
		if(!m_clDatabase.cmd(strSql))
		{
			return -1;
		}
		if(!m_clDatabase.execute())
		{
			LOG_ERROR("mysql execute fail!");
			return -1;
		}		
		CMySQLResult clResult = m_clDatabase.getResult();
		while (clResult.rowMore())
		{
            m_tmpVecData.clear();
            for(uint32 i = 0; i < clResult.getFiledCount(); i++)
            {
                MYSQLValue value;
                value.SetData((char*)clResult.getData(i),clResult.getFiledDataLength(i));
                m_tmpVecData.push_back(value);
            }
            vecData.push_back(m_tmpVecData);
		}
		return 0;
	}
	catch(CMySQLException & e)
	{
		LOG_ERROR("Error:%u(%s)->%s->%s->sql:%s",e.uCode,e.szError,e.szMsg,e.szSqlState,strSql);
	}    
    return 0;    
}
// ��������
uint32     CDBWrap::Insert(const char* tblName,SQLJoin& data)
{
	string strSql = GetInsertSql(tblName,data);
	try
	{
		CMySQLMultiFree	clMultiFree(&m_clDatabase);
		m_clDatabase.cmd(strSql.c_str());
		if(!m_clDatabase.execute())
		{
			LOG_ERROR("execute fail!,%s",strSql.c_str());
			return 0;
		}
		return m_clDatabase.getInsertIncrement();
	}
	catch(CMySQLException & e)
	{
		LOG_ERROR("Error:%u(%s)->%s->%s->sql:%s",e.uCode,e.szError,e.szMsg,e.szSqlState,strSql.c_str());
		return 0;
	}
	return 0;
}
// ��������
int     CDBWrap::Update(const char* tblName,SQLJoin& data,SQLJoin& where)
{
	return GetAffectedNumExeSql(GetUpdateSql(tblName,data,where));
}
// ������߸���
int     CDBWrap::UpdateOrInsert(const char* tblName,SQLJoin& data)
{
	return GetAffectedNumExeSql(GetUpdateOrInsertSql(tblName,data));
}
// ɾ������
int    CDBWrap::Delete(const char* tblName,SQLJoin& where)
{
	return GetAffectedNumExeSql(GetDeleteSql(tblName,where));
}
// ��������
string  CDBWrap::GetInsertSql(const char* tblName,SQLJoin& data)
{
	stringstream ss;
	ss<< "insert into " << tblName <<"( "
	<< data.keys()
	<< " ) values( "
	<< data.values()
	<< " );";
	return ss.str().c_str();
}
// ��������
string  CDBWrap::GetUpdateSql(const char* tblName,SQLJoin& data,SQLJoin& where)
{
	stringstream ss;
	ss<< "update " << tblName <<" set "
	<< data.pairs()
	<< " where " << where.pairs("and")
	<< ";";
	return ss.str().c_str();
}
// ������߸���
string  CDBWrap::GetUpdateOrInsertSql(const char* tblName,SQLJoin& data)
{
	stringstream ss;
	ss<< "insert into " << tblName <<" set "<<data.pairs()
	<< " ON DUPLICATE KEY UPDATE " << data.pairs();
	return ss.str().c_str();
}
// ɾ������
string  CDBWrap::GetDeleteSql(const char* tblName,SQLJoin& where)
{
	stringstream ss;
	ss<< "delete from " << tblName << " where " << where.pairs("and") << ";";
	return ss.str().c_str();
}
// select
string  CDBWrap::GetSelectSql(const char* tblName,SQLJoin& fileds,SQLJoin& where)
{
	stringstream ss;
	ss<< "select " <<fileds.keys() << " from " << tblName <<" where "<<where.pairs("and")<<";";

	return ss.str().c_str();
}

















