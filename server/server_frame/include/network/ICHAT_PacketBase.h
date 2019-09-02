#pragma once

#include <string>
#include <string.h>
#include <queue>
#include <vector>
#include <deque>
#include <list>
#include <iostream>
#include <map>
#include <arpa/inet.h>

using namespace std;
typedef unsigned char BYTE;
typedef unsigned int  UINT;
typedef unsigned long ULONG;
typedef unsigned short USHORT;
typedef USHORT *PUSHORT;
typedef unsigned char UCHAR;
typedef wchar_t WCHAR;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;
typedef unsigned long long _u64_;

#pragma pack(1)
struct TPkgHeader
{
    UINT        uin;         // uin(�������ڲ�ת����)
    USHORT      cmd;      	 // ��Ϣid	
    USHORT      datalen;     // ��Ϣ���ݳ���(��������ͷ)
};
#pragma pack()

#pragma pack(1)
struct PhpHeader
{
    USHORT      datalen;     // ��Ϣ���ݳ���(��������ͷ)
    USHORT      cmd;      	 // ��Ϣid	
};
#pragma pack()
