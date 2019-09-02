
/**
  *   @filename  heap.h
  *   @info ������ɾ���Ķ�,  �����û�����ɾ������, ������std::make_heap
  */

#ifndef __HEAP_ENTRY_H_H_H_
#define __HEAP_ENTRY_H_H_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define  heap_assert(statement)
//#define  heap_assert(statement)   assert(statement)


class HeapEntry;            //  ��Ԫ����, �̳�ʵ����չ
class HeapList;             //  �ѹ�����, ͨ��

/**
 *  @brief ��С�ѵĶ�Ԫ�ض���, ���ڹ���ͨ�õĶ�, �̳и�Ԫ�ؼ�����չ
 */
class HeapEntry
{    
private:    
    int  _index;          ///<  ��Ԫ���±�, ���ڿ�������ɾ������
    
public:
	friend class HeapList;

    /**
     *  @brief ����������������
     */
	HeapEntry():_index(0){};
	virtual ~HeapEntry(){};

    /**
     *  @brief ��Ԫ��ȡֵ����, ���ڷ���ֵ�Ƚ�, ���Ӻ���ʵ��, ����Ĭ������
     *  @return ��Ԫ��ӳ���ֵ
     */
    virtual unsigned long long HeapValue() = 0; 

    
    /**
     *  @brief �ѱ����ӿ�, ���ڵ���, �ڱ���ÿ��Ԫ��ʱ������, ��ѡʵ��
     */
    virtual void HeapIterate() {  
        return;
    };

    /**
     *  @brief ��Ԫ�ز������
     *  @param list ��ָ��
     *  @return 0 �ɹ�; ����ʧ��  -1 ����; -2 �ظ�����
     */
    inline int InsertIntoHeap(HeapList* list); 

    /**
     *  @brief ��Ԫ�شӶ���ɾ��
     *  @param list ��ָ��
     *  @return 0 �ɹ�; ����ʧ��  -1 �ѿ�; -2 �ظ�ɾ����������
     */
    inline int DeleteFromHeap(HeapList* list); 

    
    /**
     *  @brief ��Ԫ���±���Ϣ��ȡ, �ڲ�����ʹ��
     *  @return ��Ԫ���ڶ����±���Ϣ
     */
	inline int GetIndex() {
		return _index;
	};

private:
    
    inline int HeapValueCmp(HeapEntry* rhs) {
        if (this->HeapValue() == rhs->HeapValue()) {
            return 0;
        } else if (this->HeapValue() > rhs->HeapValue()) {
            return 1;
        } else {
            return -1;
        }
    };

	inline void SetIndex(int index) {
		_index = index;
	};


};


/**
 *  @brief ��С�Ѷ�����, ͨ����
 */
class HeapList 
{
private:
	HeapEntry**  _list;         // ��Ԫ�ص�ָ������, Ŀǰ����
	int          _max;          // �ѿɹ������Ԫ�ظ���
	int          _count;        // ���Ѿ������Ԫ�ظ���    
    
public:
    
    /**
     *  @brief ���캯������������
     */
	explicit HeapList(int max = 100000) {
        _max = (max > 0) ? max : 100000;
		_list = (HeapEntry**)malloc (sizeof(HeapEntry*) * (_max+1));
		heap_assert(_list);
		memset(_list, 0, sizeof(HeapEntry*) * (_max+1));
		_count = 0;
	};
	virtual ~HeapList()  {
		if (_list) {
		    free(_list);
			_list = NULL;
		}
        _max = 0;
        _count = 0;
	};

    /**
     *  @brief ��չheap�Ĵ�С, ��С�����
     *  @param size �µĶ�Ԫ�ظ���
     *  @return 0 �ɹ�; -1 ʧ��
     */
    int HeapResize(int size) {
        if (_max >= size) {
            return 0;
        }
        
        HeapEntry** new_list = (HeapEntry**)malloc(sizeof(HeapEntry*) * (size+1));
        if (NULL == new_list) {
            return -1;
        }
        memset(new_list, 0, sizeof(HeapEntry*) * (size+1));
        memcpy(new_list, _list, sizeof(HeapEntry*) * (_max+1));
        free(_list);
        _list = new_list;
        _max = size;

        return 0;
    };
	

    /**
     *  @brief �����Ԫ��
     *  @param entry ��Ԫ��ָ��
     *  @return 0 �ɹ�; ����ʧ��  -1 ����; -2 �ظ�����
     */
	int HeapPush(HeapEntry* entry);

    /**
     *  @brief ȡ�Ѷ�Ԫ��, ���Ƴ���Ԫ��
     *  @return �Ѷ�Ԫ��ָ��, NULL ��ʾ��Ϊ��
     */
	HeapEntry* HeapPop();

    /**
     *  @brief �Ƴ������Ԫ��
     *  @param entry ��Ԫ��ָ��
     *  @return 0 �ɹ�; ����ʧ��  -1 �ѿ�; -2 �ظ�ɾ����������
     */
	int HeapDelete(HeapEntry* entry);

    /**
     *  @brief ���Խӿ�, ��2��ѷ�ʽ��ӡԪ��, ͬʱ����ÿԪ�صĵ����ӿ�
     */
	void HeapForeach();

    /**
     *  @brief ��ȡ�ѵ�Ԫ�ظ���
     *  @return ��Ԫ��ʵ����Ŀ
     */
    int HeapSize() {
        return _count;
    };

    /**
     *  @brief ȡ�Ѷ�Ԫ��, ���Ƴ���Ԫ��
     *  @return �Ѷ�Ԫ��ָ��, NULL ��ʾ��Ϊ��
     */
    HeapEntry* HeapTop() {
        return (_count > 0) ? _list[1] : NULL;
    };

private:

    /**
     *  @brief �ж����Ƿ���
     *  @return true ��
     */
	bool HeapFull() {
		return (_count >= _max);
	};

    /**
     *  @brief �ж����Ƿ��
     *  @return true ��
     */
	bool HeapEmpty() {
		return (_count == 0);
	};

    /**
     *  @brief ���ȽϺ���, �������Ŷ�Ԫ��
     */
	void HeapUp();

    /**
     *  @brief ���ȽϺ���, �������Ŷ�Ԫ��
     */    
	void HeapDown(int index);

};

/**
 *  @brief ���ȽϺ���, �������Ŷ�Ԫ��
 */
inline void HeapList::HeapUp()
{
	for (int pos = _count; pos > 0; pos = pos/2)  
	{
		if (pos/2 < 1)   // pos == 1 �Ѿ�����, 0 ���ڱ���
		{
			break;
		}

        if (_list[pos]->HeapValueCmp(_list[pos/2]) < 0)
		{
			HeapEntry* tmp = _list[pos/2];
			_list[pos/2] = _list[pos];
			_list[pos] = tmp;

			_list[pos]->SetIndex(pos);
			_list[pos/2]->SetIndex(pos/2);
		}
		else
		{
			break;
		}
	}
}


/**
 *  @brief ���ȽϺ���, �������Ŷ�Ԫ��
 *  @param index �Ӹ�λ�ÿ�ʼ����
 */   
inline void HeapList::HeapDown(int index)
{
	int  min_son;	
	for (int pos = index; pos <= _count;  pos = min_son)
	{
		if  (pos*2 > _count)  // pos��Ҷ�ӽڵ���
		{
			break;
		}
		else if (pos*2 == _count)
		{
			min_son = pos*2;
		}
		else 
		{
            if (_list[pos*2+1]->HeapValueCmp(_list[pos*2]) < 0)
			{
				min_son = pos*2+1;
			}
			else
			{
				min_son = pos*2;
			}
		}

		if  (_list[pos]->HeapValueCmp(_list[min_son]) > 0)
		{
			HeapEntry* tmp = _list[min_son];
			_list[min_son] = _list[pos];
			_list[pos] = tmp;

			_list[pos]->SetIndex(pos);
			_list[min_son]->SetIndex(min_son);
		}
		else
		{
			break;
		}
	}
}


/**
 *  @brief �����Ԫ��
 *  @param entry ��Ԫ��ָ��
 *  @return 0 �ɹ�; ����ʧ��  -1 ����; -2 �ظ�����
 */
inline int HeapList::HeapPush(HeapEntry*  item)
{
	if (HeapFull()) {
        heap_assert(0); // ��, �������ǿ��ܵ�, ʵ�����в�̫���ܹ�10W
		return -1;
	}
    
    if (item->GetIndex() != 0) {
        heap_assert(0); // �ظ�����
        return -2;
    }     
    
	_count++;
	_list[_count] = item;
    item->SetIndex(_count);

	HeapUp();

	return 0;
}


/**
 *  @brief ȡ�Ѷ�Ԫ��, ���Ƴ���Ԫ��
 *  @return �Ѷ�Ԫ��ָ��, NULL ��ʾ��Ϊ��
 */
inline HeapEntry* HeapList::HeapPop()
{
	if  (HeapEmpty()) {
		return NULL;
	}

	HeapEntry* top = _list[1];	// 0 ����

	_list[1] = _list[_count];
    _list[1]->SetIndex(1);
    _list[_count] = 0;
    
	_count--;
	HeapDown(1);
	
    heap_assert(top->GetIndex() == 1);
	top->SetIndex(0);
	return top;
}

/**
 *  @brief �Ƴ������Ԫ��
 *  @param entry ��Ԫ��ָ��
 *  @return 0 �ɹ�; ����ʧ��  -1 �ѿ�; -2 �ظ�ɾ����������
 */
inline int  HeapList::HeapDelete(HeapEntry* item)
{
	if  (HeapEmpty()) {
		return -1;
	}

	int pos = item->GetIndex() ;
	if  ((pos > _count)  ||(pos <= 0))
	{
        heap_assert(0); // �Ƿ����ݻ��ظ�ɾ��
		return -2;
	}

	HeapEntry* del = _list[pos];
	_list[pos] = _list[_count];
    _list[pos]->SetIndex(pos);

    _list[_count] = 0;
	_count--;

	HeapDown(pos);
    heap_assert(pos == del->GetIndex());
	del->SetIndex(0);
	return 0;
}


/**
 *  @brief ���Խӿ�, ��2��ѷ�ʽ��ӡԪ��, ͬʱ����ÿԪ�صĵ����ӿ�
 */
inline void HeapList::HeapForeach()
{
    int per = 1;
	for (int i = 1; i <= _count; i++)
	{
	    if (i >= per*2)
	    {
	        printf("\n");
	        per *=2;
	    }
		printf("%llu ", _list[i]->HeapValue());

        _list[i]->HeapIterate();
	}
}

/**
 *  @brief ��Ԫ�ز������
 *  @param list ��ָ��
 *  @return 0 �ɹ�; ����ʧ��  -1 ����; -2 �ظ�����
 */
inline int HeapEntry::InsertIntoHeap(HeapList* list) {
    return list->HeapPush(this);
};

/**
 *  @brief ��Ԫ�شӶ���ɾ��
 *  @param list ��ָ��
 *  @return 0 �ɹ�; ����ʧ��  -1 �ѿ�; -2 �ظ�ɾ����������
 */
inline int HeapEntry::DeleteFromHeap(HeapList* list) {
    return list->HeapDelete(this);
};


#endif


