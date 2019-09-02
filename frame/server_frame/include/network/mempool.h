
#ifndef _COMM_MEMPOOL_H_
#define _COMM_MEMPOOL_H_

#include <map>
#include <list>
#include <set>


class CMemPool
{
public:
    CMemPool()
    {
        _allocated_size = 0;
        _water_mark = C_MAX_WATER_MARK;
    }
    ~CMemPool();

    /*
    * ����:�����ڴ�
    * ����˵��:
    *IN size:����Ҫ���ڴ��С
    *OUT result_size:ʵ�ʷ�����ڴ��С
    *����ֵ:
    * �ɹ�:�����ڴ�ĵ�ַ,ʧ��:NULL
    */
    void* allocate(unsigned int size, unsigned int & result_size);

    /*
    * ����:�����ڴ�
    * ����˵��:
    * IN mem:�����յ��ڴ��ַ
    * IN mem_size: �ڴ��С
    * ����ֵ:
    * �ɹ�:0,ʧ��:��0
    */
    int recycle(void* mem, unsigned mem_size);
    
    
    void * simple_alloc(unsigned int size, unsigned int & result_size);

    void * simple_realloc(void * mem, unsigned int size, unsigned int & result_size);

    void simple_free(void* mem, unsigned mem_size);
    
    
private:
    int extend(unsigned int size, std::list<void*>* l, std::set<void*>* s);
    int extend_new_size(unsigned int size);
    int release(unsigned int size, std::list<void*>* l, std::set<void*>* s);

    unsigned int release_size(unsigned int block_size, unsigned int free_count, unsigned int stub_count);
    unsigned int fit_block_size(unsigned int size)
    {
        unsigned int i = 10;
        size = (size>>10);
        for(; size; i++, size = (size>>1));
        return 1 << (i<10 ? 10 : i);
    }
    unsigned int fit_extend_set(unsigned size);
    int release_size(unsigned int mem_size);
    typedef std::map<unsigned int, std::list<void*>*> mml;
    typedef std::map<unsigned int, std::set<void*>*> mms;

    //�����ڴ�����
    mml _free;
    //�ڴ����
    mms _stub;
    //�ѷ����ڴ��ܴ�С
    unsigned int _allocated_size;
    //�ڴ�������ˮλ
    unsigned int _water_mark;

    static const unsigned int C_MAX_POOL_SIZE = 2000 * (1 << 20); // 2G
    static const unsigned int C_MAX_WATER_MARK = 1 << 30; // 1G
};

#endif

