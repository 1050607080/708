
#pragma once
#include "basicTypes.h"
/*************************************************************/
class CCooling
{
private:
protected:
	uint64	m_uBeginTick;	/*��ȴ��ʼʱ��*/ 
	uint64	m_uEndTick;		/*��ȴ����ʱ��*/ 
	uint8	m_state;		//״̬
public:
	CCooling();
	virtual~CCooling(){}

public:
	void	clearCool();
	/*--->[ �����ȴʱ�� ]*/
	uint32	getCoolTick();
	uint32  getPassTick();
	/*--->[ �������ȴʱ�� ]*/
	uint32	getTotalTick();
	/*--->[ ��ʼ��ȴ ]*/
	bool	beginCooling(uint32 uTick);
	bool 	isTimeOut();
	void 	setState(uint8 state);
	uint8   getState();

};