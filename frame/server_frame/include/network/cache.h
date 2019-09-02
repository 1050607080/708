
#ifndef _COMM_SOCKCOMMU_CACHE_H_
#define _COMM_SOCKCOMMU_CACHE_H_

#include <sys/time.h>
#include "mempool.h"


//��cache����
class CRawCache
{
public:
    CRawCache(CMemPool& mp);
    ~CRawCache();

    char* data();
    unsigned data_len();
    void append(const char* data, unsigned data_len);
    void skip(unsigned length);

private:
    //�ڴ�ض���
    CMemPool& _mp;
    //�ڴ��ַ
    char* _mem;
    //�ڴ��С
    unsigned _block_size;
    //ʵ��ʹ���ڴ���ʼƫ����
    unsigned _data_head;
    //ʵ��ʹ���ڴ泤��
    unsigned _data_len;
};

#endif

