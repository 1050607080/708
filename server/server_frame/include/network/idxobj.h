#ifndef _IDXOBJ_HPP_
#define _IDXOBJ_HPP_

#include "base.h"

#define MAXDSINFOITEM 4

#define IDXNULL			((int)-1)
#define unused(x) (void)x

#define DECLARE_DYN public: static CObj* CreateObject(void* pMem);

#define IMPLEMENT_DYN(class_name) CObj* class_name::CreateObject( void *pMem )  { CObj::pCurrentObj = (unsigned char *)pMem; return (CObj *)(new class_name); }

#define DECLARE_PREALLOCATED(class_name)\
public:\
	static int	SetObjSeg(int iObjSize,int iObjCount);	\
	static CObjSeg* GetObjSeg(){return class_name::class_name##m_pObjSeg;};\
	static void* operator new( size_t nSize ) throw();\
	static void  operator delete( void *pMem ) ;\
	static class_name* GetObjectByID(short nID){return (class_name *)class_name##m_pObjSeg->GetObj(nID);}\
	static short GetNextObjectID(short nObjID);\
	static short GetNextObjectID(CObj *pObj);\
	virtual short GetObjectID(){return class_name##m_pObjSeg->GetObjId(this);}\
	static short   GetUsedCount(){return class_name##m_pObjSeg->GetUsedCount();}\
	static short   GetFreeCount(){return class_name##m_pObjSeg->GetFreeHead();}\
	static short   GetUsedHead(){return class_name##m_pObjSeg->GetUsedHead();}\
	static short   GetFreeHead(){return class_name##m_pObjSeg->GetFreeHead();}\
	static CObj* CreateObjectByPreMem(void* pMem);\
	static CObjSeg * class_name##m_pObjSeg;\

#define IMPLEMENT_PREALLOCATED(class_name) \
	CObjSeg* class_name::class_name##m_pObjSeg=NULL;\
	void* class_name::operator new( size_t nSize ) throw()\
{\
	if(!class_name##m_pObjSeg)\
	{\
	return NULL;\
	}\
	return  class_name##m_pObjSeg->AllocMemForObject();\
}\
	void  class_name::operator delete( void *pMem )\
{\
	short nIdx = 0;\
	nIdx = class_name##m_pObjSeg->GetObjId((CObj *)pMem);\
	if(nIdx < 0 )\
	{\
	return;\
	}\
	class_name##m_pObjSeg->DestroyObject(nIdx);\
	return;\
}\
	int  class_name::SetObjSeg(int iObjSize,int iObjCount)\
{\
	class_name##m_pObjSeg = new CObjSeg(iObjSize,iObjCount, NULL);\
	class_name##m_pObjSeg->SetCreateObjFunc(class_name::CreateObjectByPreMem); \
	class_name##m_pObjSeg->FormatObj();\
	return 0;\
}\
	CObj* class_name::CreateObjectByPreMem( void *pMem ) \
{ \
	if (NULL == pMem)\
	{\
	return NULL;\
	}\
	return (CObj *)(::new(pMem) class_name);\
}\
	short class_name::GetNextObjectID(short nObjID) \
{ \
	if (0 > nObjID)\
	{\
	return -1;\
	}\
	CIdx *pstIdx = class_name##m_pObjSeg->GetIdx(nObjID);\
	if (NULL == pstIdx)\
	{\
	return -1;\
	}\
	return pstIdx->GetNextIdx(); \
}\
	short class_name::GetNextObjectID(CObj *pstObj) \
{ \
	short nObjID = pstObj->GetObjectID();\
	if (0 > nObjID)\
	{\
	return -1;\
	}\
	return GetNextObjectID(nObjID); \
}\

typedef unsigned char byte;

class CIdx;
class CObjSeg;
class CObj;

//������ɿɱ䳤�ȵģ����������װһ��������������Χ������ͬ��CObjSeg��Ȼ��ԭ����

//һ��������CIdx��CObj���б�����һ��CIdx��Ӧһ��CObj
class CObjSeg
{
public:
	//���캯��
	//nObjSize CObj�Ĵ�С
	//iItemCount CObj ����Ŀ
	//pfCreateObj �ṩһ������CObj����ĺ���ָ��
	CObjSeg(size_t  nObjSize, int iItemCount, CObj* (*pfCreateObj)(void *));
	~CObjSeg();

	//virtual int HashKeyToIdx(const void *pvKey, int iKeyLength);

	int		Init();
	int		FormatObj();						//��ʽ��������
	int		FormatIdx();						//��ʽ��������
	int		RecoverIdx();						//�ָ�������
	int		CreateObject();					//���������ҵ�һ�����еĶ��󣬹ҵ����ö����б���
	int		DestroyObject(int iIdx);			//���·�������б���
	int		ModifyRelation(int iNewIdx, int iCurrent);
	int 		AddToPre(int iNewIdx, int iCurrent);
	int 		AddToNext(int iNewIdx, int iCurrent);

	//int		CreateObjectByKey(const void *pvKey, int iKeyLength);

	int		GetUsedHead();
	int 		GetUsedTail();
	int		GetFreeHead();
	int		GetUsedCount();
	int		GetFreeCount();

	int		GetItemCount();

	CObj* GetObj(int iIdx);
	CIdx* GetIdx(int iIdx);

	//CObj*	GetObjectByKey(const void *pvKey, int iKeyLength);

	short ApplyUsableDSIdx();

	void* operator new( size_t  nSize ) throw();
	void  operator delete( void *pSeg );

	//static CSharedMem* pCurrentShm;
	static size_t CountSegSize( size_t nObjSize, int iItemCount );

private:

	int			m_iIsFormated;
	size_t		m_nObjSize;
	CIdx*		m_pIdxs;
	CObj*		m_pObjs;
	int			m_iItemCount;
	int			m_iFreeHead;
	int			m_iUsedHead;
	int 			m_iUsedTail;
	int			m_iUsedCount;
	short		m_nCurrentUsableDSIdx;
	CObj*		(*m_pfNew)(void *);
};

//���������࣬��Ҫ��һ��CObj�󶨣�һ��һӳ��
class CIdx
{
public:
	CIdx();
	~CIdx();
	void Init();
	void SetFree();
	void SetUsed();
	int		IsUsed();
	int		GetNextIdx();
	void	SetNextIdx(int iIdx);
	int		GetPrevIdx();
	void	SetPrevIdx(int iIdx);
	int		GetUsedPrevIdx();
	void	SetUsedPrevIdx(int iIdx);
	int		GetUsedNextIdx();
	void	SetUsedNextIdx(int iIdx);

	//�����������CIdx����Ϣ����hashmap����һ��bucket����һ���ڵ������ֵ��
	//������nDsIdx���Ա��治ͬ����Ϣ��piDsVal ���ش���Ϣ
	int		GetDsInfo( short nDsIdx, int *piDsVal );
	int		SetDsInfo( short nDsIdx, int iDsVal );

	CObj*	GetAttachedObj();
	int		SetAttachedObj( CObj *pObj );

protected:
	int		m_iUseFlag;
	int		m_iNextIdx;					//��������������ָ�룬ָ���һ����/æ����
	int		m_iPrevIdx;					//��������������ָ�룬ָ��ǰһ����/æ����
	int		m_iUsedNext;				//����������������ӣ�ָ����ͬ�����е�ǰ������
	int		m_iUsedPrev;
	//int		m_iHashNextIdx;				//��ϣ��������ָ�룬ָ����һ����ϣ����
	//int		m_iHashPrevIdx;				//��ϣ��������ָ�룬ָ����һ����ϣ����
	int		m_piDsInfo[MAXDSINFOITEM];

private:
	CObj *m_pAttachedObj;
};

//һ�������࣬Ӧ�ò���̳�����
class CObj
{
public:
	virtual ~CObj(){};
	virtual int Init() = 0;
	virtual void FInit(){};
	virtual int GetHashKey(void *pvKey, int& iKeyLength) = 0;
	virtual int SetHashKey(const void *pvKey, int iKeyLength) = 0;
	virtual int Show(FILE *fpOut) = 0;

	int GetObjectID();

	static int  nCreateMode;

protected:
	static void* operator new( size_t nSize ) throw();
	static void  operator delete( void *pMem );
	static unsigned char *pCurrentObj;

	//private:
public:
	int m_iObjectID;

	friend int CObjSeg::FormatObj();

	DECLARE_DYN
};

#endif
