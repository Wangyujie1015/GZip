#pragma once

#include <string>
using namespace std;

#include "HuffmanTree.hpp"

typedef unsigned char uch;

struct ByteInfo
{
	uch ch;
	int appearCount;  // ch字节出现的次数
	string strCode;   // ch字节对应的编码

	ByteInfo(int count = 0)
		: appearCount(count)
	{}

	ByteInfo operator+(const ByteInfo& b)const
	{
		ByteInfo temp;
		temp.appearCount = appearCount + b.appearCount;
		return temp;
	}

	bool operator>(const ByteInfo& b)const
	{
		return appearCount > b.appearCount;
	}

	bool operator!=(const ByteInfo& b)const
	{
		return appearCount != b.appearCount;
	}

	bool operator==(const ByteInfo& b)const
	{
		return appearCount == b.appearCount;
	}
};

class FileCompress
{
public:
	FileCompress();
	bool CompressFile(const string& filePath);
	bool UNCompressFile(const string& filePath);


private:
	void GetLine(FILE* fIn, string& strContent);
	void WriteHead(FILE* fOut, const string& filePath);
	void GenerateHuffmanCode(HuffmanTreeNode<ByteInfo>* root);
	/////////////////////////////////////////////////
	// 文件最终在磁盘上是以字节方式存储的
	// 字节：总共有256种状态
	// 只需给一个包含256个ByteInfo类型的数组来保存字节出现的频次信息
	ByteInfo fileByteInfo[256];
};