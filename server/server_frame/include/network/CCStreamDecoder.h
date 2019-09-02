#ifndef _CCStreamDecoder_H_
#define _CCStreamDecoder_H_

#include "ICC_Decoder.h"
#include "ICHAT_PacketBase.h"
 
class CCStreamDecoder : public ICC_Decoder
 {
	 public:
		CCStreamDecoder(){};
		virtual ~CCStreamDecoder(){};

		virtual int ParsePacket(char * data, int len)
		{
			//����һ����ͷ����������
			if( len < this->GetHeadLen(data,len) )
				return 0;

			//����һ����ͷ���жϰ�ͷ�Ϸ���
			if( ! this->CheckHead(  data,   len))
				return -1;

			//����һ����ͷ���ж��������Ϸ���
			int pkglen = this->GetPacketLen( data,   len);
			if( pkglen<0 )
			{
				return -1;
			}
			//����δ��������
			if(len < pkglen)
			{
				return 0;
			}
			//�յ�һ��������
			return pkglen;
		}

		protected:
			virtual int GetHeadLen(const char * data, int len)=0;
			virtual bool CheckHead(const char * data, int len)=0;
			virtual int GetPacketLen(const char * data, int len)=0;
	
 };
 


#endif


