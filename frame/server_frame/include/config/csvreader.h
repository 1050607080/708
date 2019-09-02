/*
**  author: toney
**  create: 2015/12/3
*/

#ifndef __CSV_CONFIG_READER_H__
#define __CSV_CONFIG_READER_H__

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "helper/fileStream.h"

class CCsvConfigReader
{
public:
	CCsvConfigReader(void);
	~CCsvConfigReader(void);

public:
	// ���ļ�
	bool	OpenFile(const char * filename, bool backup = true);
	// �����ļ�
	bool    SaveFile(const char * filename);
	// ����������ݵ�����
	int     GetRows();
	// ����������ݵ�����
	int     GetCols();
public:
	// ��ʼ��ѭ����־
	void    First(int nRow = 0);
	// �ƶ�ѭ����־
	void    Next();
	// �ж��Ƿ��β
	bool    IsDone();
public:
	// �����ַ����ֶ�
	std::string 	GetFieldValueToString();
	short 			GetFieldValueToShort();
	int 			GetFieldValueToInt();
	double 			GetFieldValueToDouble();
	bool 			GetFieldValueToBool();

	// ���ݷ����ַ����ֶ�
	int				GetRowCount();
	std::string 	GetFieldValueToString(int rownum, 	std::string strcol);
	short 			GetFieldValueToShort(int rownum, 	std::string strcol);
	int 			GetFieldValueToInt(int rownum, 	    std::string strcol);
	double 			GetFieldValueToDouble(int rownum,   std::string strcol);
	bool 			GetFieldValueToBool(int rownum, 	std::string strcol);

protected:
	//ֱ�Ӵ��ļ�����ȡֵ
	std::string	    ReadFieldValue(int row, int col);

	//ѭ��������ȡֵ
	std::string	    ReadFieldValue();
	std::string	    GetFieldValue(int row, const std::string & strcol);

private:
	CFileStream 							m_FileReader;			// �ļ���ȡ��
	bool									backup_;				// �ڴ汸�ݿ���
	std::ifstream							instream_;				// �ļ�����������
	std::string								filename_;				// �ļ���
	int										seek_ofset_;			// ��ǰ�ļ��ֽ�ƫ����

	int										currows_;				// ��ǰ�ڴ�������
	int										curcols_;				// ��ǰ�ڴ�������

private:
	std::vector< std::vector<std::string> >	data_;					// �ڴ�����
	int						                rows_;					// �ڴ�������
	int						                cols_;					// �ڴ�������
	std::map< std::string, int >			indexs_;				// ������Ϣ
	
};


#endif // __CSV_CONFIG_READER_H__




