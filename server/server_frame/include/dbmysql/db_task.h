
#ifndef DB_TASK_H__
#define DB_TASK_H__

#include "utility/basicTypes.h"
#include "utility/basicFunctions.h"
#include "utility/stringFunctions.h"
#include "utility/thread.h"
#include <string>
#include <queue>
#include "dbmysql/dbmysql.h"
#include "dbmysql/db_wrap.h"
#include "config/config.h"

using namespace std;
using namespace svrlib;

#ifndef MAX_DB_ADD_PARAM
#define MAX_DB_ADD_PARAM 5
#endif

enum emDBCALLBACK
{
    emDBCALLBACK_NULL   = 0, //���ص�
    emDBCALLBACK_QUERY  = 1, //��ѯ�����
    emDBCALLBACK_AFFECT = 2, //Ӱ������
    
};

class CDBEventReq
{  
public:
    CDBEventReq()
    {
        Reset();
    }
    ~CDBEventReq()
    {
        Reset();
    }
    CDBEventReq(const CDBEventReq& ev)  
    {  
        eventID = ev.eventID;
        needCb  = ev.needCb;
        sqlStr  = ev.sqlStr;
        memcpy(params,ev.params,sizeof(params));
    }  
    void operator=(const CDBEventReq& ev)
	{
        eventID = ev.eventID;
        needCb  = ev.needCb;
        sqlStr  = ev.sqlStr;
        memcpy(params,ev.params,sizeof(params));
	}
    void Reset()
    {
        eventID = 0;
        needCb  = 0;
        sqlStr  = "";
        memset(params,0,sizeof(params));
    }
	uint32 	eventID;	                // �¼�ID
	uint8   needCb;                     // �Ƿ���Ҫ�ص�
    int64   params[MAX_DB_ADD_PARAM];   // ���Ӳ���
	string  sqlStr;                     // sql���
};
class CDBEventRep
{   
public:
    CDBEventRep()
    {
        Reset();
    }
    ~CDBEventRep()
    {
        Reset();
    }
    CDBEventRep(const CDBEventRep& ev)  
    {  
        eventID = ev.eventID;
        vecData = ev.vecData;
        memcpy(params,ev.params,sizeof(params));
    }  
    void operator=(const CDBEventRep& ev)
	{
        eventID = ev.eventID;
        vecData = ev.vecData;
        memcpy(params,ev.params,sizeof(params));
	}    
    void Reset()
    {
        eventID = 0;
        vecData.clear();
        memset(params,0,sizeof(params));
    }
    uint32  eventID;    // �¼�ID
    int64   params[MAX_DB_ADD_PARAM];   // ���Ӳ���
    vector<map<string, MYSQLValue> > vecData;// ����      
};

class  CDBTask : public Thread,public CDBWrap
{
public:
	CDBTask();
	virtual ~CDBTask();

	bool	init(const stDBConf& conf);
	virtual void stop();
	virtual void run();
    virtual void writeLog(string logStr);
    
	void	push(string sql); 
	void	pushCmd(const char*pCmd,...);
    // �첽��������
    void    AsyncInsert(const char* tblName,SQLJoin& data);
    // �첽��������
    void    AsyncUpdate(const char* tblName,SQLJoin& data,SQLJoin& where);
    // �첽������߸���
    void    AsyncUpdateOrInsert(const char* tblName,SQLJoin& data);
    // �첽ɾ������
    void    AsyncDelete(const char* tblName,SQLJoin& where);

	void 	setDBIndex(int32 index);

    // �첽����,�ⲿ�����ڴ�
    void    AsyncQuery(CDBEventReq *pReq);
    // �ⲿ�ͷ��ڴ�
    CDBEventRep* GetAsyncQueryResult();

    
    CDBEventReq* MallocDBEventReq(uint16 eventID,uint8 needCb);
    void    FreeDBEventReq(CDBEventReq * pReq);

    CDBEventRep* MallocDBEventRep();
    void    FreeDBEventRep(CDBEventRep* pRep);
    
        
private:
    // ����������ݿ����
    void    AddDBReqEvent(CDBEventReq* pReqEvent);    
    // ������ݿⷵ���¼�
    void    AddDBRepEvent(CDBEventRep* pRepEvent);   
    // ���������¼�
    bool    OnProcessEvent(CDBEventReq* pReqEvent);
        
private:
	bool	ConnectDB();
    
private:
	bool			m_bRun;
	stDBConf		m_conf;

	TLock			        m_lockReq;// �������
    TLock                   m_lockRep;// ���ض���                  
    
	queue<CDBEventReq*>	    m_QueueReq;//�������
	queue<CDBEventRep*>     m_QueueRep;//���ض���
    
	int32           m_lastCountTime;
	int32           m_Counts;
	int32           m_dbIndex;	

};

#endif //DB_TASK_H__


