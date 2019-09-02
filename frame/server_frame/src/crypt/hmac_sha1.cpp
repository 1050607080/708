/*----------------- hmac_sha1.cpp 
*
* Copyright (C) 2013 Mokylin RXTL
* Author: toney
* Version: 1.0
* Date: 2012/6/7 16:36:07
*--------------------------------------------------------------
*
*------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "crypt/hmac_sha1.h"
/*************************************************************/
namespace hmac_sha1
{
	typedef unsigned int __u32;
	typedef unsigned char __u8;
	//------------------------------ 定义结构
#pragma pack(push,1)
	struct SHA1_CTX
	{
		__u32	state[5];
		__u32	count[2];
		__u8	buffer[64];

		/* SHA1 initialization constants */
		void	initialization()
		{
			/*初始化sha1常量*/
			state[0] = 0x67452301;
			state[1] = 0xEFCDAB89;
			state[2] = 0x98BADCFE;
			state[3] = 0x10325476;
			state[4] = 0xC3D2E1F0;
			count[0] = count[1] = 0;
			memset(buffer,0,sizeof(buffer));
		}
	};
#pragma pack(pop)

	//------------------------------ 宏定义
	//--- 取消宏定义
	#if defined(rol)
	#undef rol
	#endif//rol

	#define SHA1HANDSOFF
	#ifndef __LITTLE_ENDIAN
	#define __LITTLE_ENDIAN
	#endif
	
	//--- 宏定义
	#define rol(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))
	/* blk0() and blk() perform the initial expand. */
	/* I got the idea of expanding during the round function from SSLeay */
	#ifdef __LITTLE_ENDIAN
	#define blk0(i) (block->l[i] = (rol(block->l[i],24)&0xFF00FF00) | (rol(block->l[i],8)&0x00FF00FF))
	#else//__LITTLE_ENDIAN
	#define blk0(i) block->l[i]
	#endif//__LITTLE_ENDIAN
	#define blk(i) (block->l[i&15] = rol(block->l[(i+13)&15]^block->l[(i+8)&15] ^ block->l[(i+2)&15]^block->l[i&15],1))

	/* (R0+R1), R2, R3, R4 are the different operations used in SHA1 */
	#define R0(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk0(i)+0x5A827999+rol(v,5);w=rol(w,30);
	#define R1(v,w,x,y,z,i) z+=((w&(x^y))^y)+blk(i)+0x5A827999+rol(v,5);w=rol(w,30);
	#define R2(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0x6ED9EBA1+rol(v,5);w=rol(w,30);
	#define R3(v,w,x,y,z,i) z+=(((w|x)&y)|(w&x))+blk(i)+0x8F1BBCDC+rol(v,5);w=rol(w,30);
	#define R4(v,w,x,y,z,i) z+=(w^x^y)+blk(i)+0xCA62C1D6+rol(v,5);w=rol(w,30);

	//#############################################################
	//############################## 函数定义
	//#############################################################
	/* Hash a single 512-bit block. This is the core of the algorithm. */
	//hash一个单一512bit块，这是该算法的核心
	//-------------------------------------------------------------
	//------------------------------ sha1转换
	void	SHA1Transform(__u32 state[5],__u8 buffer[64])
	{
		__u32 a, b, c, d, e;
#pragma pack(push,1)
		union CHAR64LONG16
		{
			__u8	c[64];
			__u32	l[16];
		};
#pragma pack(pop)

		CHAR64LONG16* block;

#ifdef SHA1HANDSOFF
		static unsigned char workspace[64];
		block = (CHAR64LONG16*)workspace;
		memcpy(block, buffer, 64);
#else//SHA1HANDSOFF
		block = (CHAR64LONG16*)buffer;
#endif//SHA1HANDSOFF
		/* Copy context->state[] to working vars */
		/*copy上下文状态到工作变量*/
		a = state[0];
		b = state[1];
		c = state[2];
		d = state[3];
		e = state[4];
		/* 4 rounds of 20 operations each. Loop unrolled. */
		R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
		R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
		R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
		R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
		R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
		R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
		R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
		R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
		R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
		R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
		R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
		R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
		R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
		R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
		R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
		R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
		R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
		R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
		R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
		R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);
		/* Add the working vars back into context.state[] */
		/*新增工作变量放回上下文。*/
		state[0] += a;
		state[1] += b;
		state[2] += c;
		state[3] += d;
		state[4] += e;
		/* Wipe variables */
		/*擦拭变量*/
		a = b = c = d = e = 0;
	}

	/* Run your data through this. */
	//-------------------------------------------------------------
	//------------------------------ 通过这个运行数据
	void	SHA1Update(SHA1_CTX* context,__u8* data,__u32 len)
	{
		__u32 j = context->count[0];
		if((context->count[0] += len << 3) < j)
			context->count[1]++;

		context->count[1] += (len >> 29);
		j = (j >> 3) & 63;

		__u32 i = 0;
		if((j + len) > 63)
		{
			memcpy(&context->buffer[j], data, (i = 64 - j));
			SHA1Transform(context->state, context->buffer);

			for(;i + 63 < len; i += 64)
				SHA1Transform(context->state, &data[i]);

			j = 0;
		}

		memcpy(&context->buffer[j],&data[i], len - i);
	}

	/* Add padding and return the message digest. */
	//-------------------------------------------------------------
	//------------------------------ 添加填充和返回的消息摘要
	void	SHA1Final(__u8 digest[20], SHA1_CTX* context)
	{
		__u8 finalcount[8];
		for(__u32 i = 0; i < 8; i++)
		{
			finalcount[i] = (__u8)((context->count[(i >= 4 ? 0 : 1)] >> ((3 - (i & 3)) * 8)) & 255); /* Endian independent */
		}

		SHA1Update(context, (__u8*)"\200", 1);
		while((context->count[0] & 504) != 448)
		{
			SHA1Update(context, (__u8*)"\0", 1);
		}

		SHA1Update(context, finalcount, 8);  /* Should cause a SHA1Transform() *//*应引起一个SHA1Transform()*/
		for(__u32 i = 0; i < 20; i++)
		{
			digest[i] = (__u8)((context->state[i>>2] >> ((3 - (i & 3)) * 8)) & 255);
		}

		memset(context->buffer, 0x00, 64);
		memset(context->state, 0x00, 20);
		memset(context->count, 0x00, 8);
		memset(finalcount, 0x00, 8);

	#ifdef SHA1HANDSOFF  /* make SHA1Transform overwrite its own static vars */
		SHA1Transform(context->state, context->buffer);
	#endif//SHA1HANDSOFF
	}

	/* Function to compute the digest */
	//-------------------------------------------------------------
	//------------------------------ 函数来计算摘要
	unsigned int	hmac_sha1(const char*key,unsigned int key_length,const char*data,unsigned int data_length,char*output,unsigned int out_lentth)
	{
		if(!key || !data || !output || !out_lentth)
			return 0;

		SHA1_CTX	_sha1_ctx;
		char		isha[20] = {0};
		char		osha[20] = {0};
		char		_key[20] = {0};
		char		buf[64] = {0};

		if(key_length > 64)
		{
			SHA1_CTX tctx ;

			tctx.initialization();						//初始化上下文
			SHA1Update(&tctx,(__u8*)key, key_length) ;	//更新计算key值
			SHA1Final((__u8*)_key, &tctx) ;

			key			= _key ;
			key_length	= 20 ;
		}

		/**** Inner Digest ****/
		_sha1_ctx.initialization();

		/* Pad the key for inner digest */
		for(unsigned int i = 0;i < key_length ; ++i)
			buf[i] = key[i] ^ 0x36;

		for(unsigned int i = key_length;i < 64 ; ++i)
			buf[i] = 0x36;

		SHA1Update(&_sha1_ctx,(__u8*)buf,64) ;
		SHA1Update(&_sha1_ctx,(__u8*)data,data_length) ;

		SHA1Final((__u8*)isha,&_sha1_ctx) ;

		/**** Outter Digest ****/
		_sha1_ctx.initialization();

		/* Pad the key for outter digest */
		for(unsigned int i = 0 ; i < key_length ; ++i)
			buf[i] = key[i] ^ 0x5C ;

		for(unsigned int i = key_length ; i < 64 ; ++i)
			buf[i] = 0x5C;

		SHA1Update(&_sha1_ctx,(__u8*)buf, 64) ;
		SHA1Update(&_sha1_ctx,(__u8*)isha, 20) ;

		SHA1Final((__u8*)osha, &_sha1_ctx) ;

		/* truncate and print the results */
		out_lentth = out_lentth > 20 ? 20 : out_lentth ;

		memcpy(output,osha,out_lentth);
		return out_lentth;
	}
};



