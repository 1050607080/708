

/**
 * @file  heap_timer.h
 */

#ifndef _HEAP_TIMER_H_H_H_
#define _HEAP_TIMER_H_H_H_

#include <stdint.h>
#include "heap.h"

/**
 * @brief ��ʱ���������
 */
class CTimerNotify : public HeapEntry
{
public:

    /**
     * @brief ��ʱ֪ͨ����, ����ʵ���߼�
     */
    virtual void OnTimer() { return; };
    
    /**
     *  @brief ��Ԫ��ȡֵ����, ���ڷ���ֵ�Ƚ�, ���Ӻ���ʵ��, ����Ĭ������
     *  @return ��Ԫ��ӳ���ֵ
     */
    virtual unsigned long long HeapValue() {
        return (unsigned long long)_time_expired;
    }; 

    /**
     * @brief ���캯��
     */
    CTimerNotify() : _time_expired(0) {};

    /**
     * @brief ����������
     */
    virtual ~CTimerNotify(){};

    /**
     * @brief ���þ��Գ�ʱʱ��, ��λms
     * @param expired ���Գ�ʱʱ�� ms��λ
     */
    void set_expired_time(uint64_t expired) {
        _time_expired = expired;    
    };

    /**
     * @brief ��ȡ���Գ�ʱʱ��, ��λms
     * @return ���Գ�ʱʱ�� ms��λ
     */
    uint64_t get_expired_time() {
        return _time_expired;        
    };

private:

    uint64_t        _time_expired;     // ���Եĳ�ʱʱ��ms��λ
};


/**
 * @brief ��ʱ����������
 */
class CTimerMng
{
public:


    /**
     * @brief ���캯��
     * @param max_item ���ɹ���Ķ�ʱ��������Ŀ(ָ����Ŀ)
     */
    explicit CTimerMng(uint32_t max_item = 100000);    

    /**
     * @brief ��������
     */
    ~CTimerMng();

    /**
     * @brief ��ʱ�����ú���
     * @param timerable ��ʱ������
     * @param interval  ��ʱ�ļ�� ms��λ
     * @return �ɹ�����true, ����ʧ��
     */
    bool start_timer(CTimerNotify* timerable, uint32_t interval);    

    /**
     * @brief ��ʱ��ֹͣ�ӿں���
     * @param timerable ��ʱ������
     */
    void stop_timer(CTimerNotify* timerable);

    /**
     * @brief ��ʱ����ʱ��⺯��
     */
    void check_expired();

private:
    
    HeapList*           _heap;      // ��С��ָ��
};

#endif

