#define _CRT_SECURE_NO_WARNINGS

#include "FileCompress.h"


#include <iostream>



FileCompress::FileCompress()
{
	for (int i = 0; i < 256; ++i)
	{
		fileByteInfo[i].ch = i;
	}
}

bool FileCompress::CompressFile(const string& filePath)
{
	// 1. 统计源文件中每个字节出现的次数---保存
	FILE* pf = fopen(filePath.c_str(), "rb");
	if (nullptr == pf)
	{
		cout << "打开待压缩文件失败!!!" << endl;
		return false;
	}

	// 文件大小暂时不知道----此处需要采用循环的方式来获取源文件中内容
	uch readBuff[1024];
	while (true)
	{
		// 本意：想要一次性读取1024个字节，但实际读取到了rdsize个字节
		size_t rdsize = fread(readBuff, 1, 1024, pf);
		if (0 == rdsize)
		{
			// 现在已经读取到文件的末尾
			break;
		}

		// 进行统计
		for (size_t i = 0; i < rdsize; ++i)
		{
			// 利用到了：直接定制法---以字符的ASCII值作为数组的下标来进行快速的统计
			fileByteInfo[readBuff[i]].appearCount++;
		}
	}

	//////////////////////////////////////////////////////////////////////
	// 2. 根据统计的结果创建huffman树
	// 注意：在创建huffman树期间，需要将出现次数为0的字节剔除掉
	HuffmanTree<ByteInfo> ht;
	ByteInfo invalid;
	ht.CreateHuffmanTree(fileByteInfo, 256, invalid);

	////////////////////////////////////////////////////////////////////////
	// 3. 借助huffman树获取每个字节的编码
	GenerateHuffmanCode(ht.GetRoot());

	/////////////////////////////////////////////////////////////////////
	/// 4. 写解压缩时候需要用到的信息
	FILE* fOut = fopen("2.hzp", "wb");
	WriteHead(fOut, filePath);

	////////////////////////////////////////////////////////////////////////
	// 5. 使用字节的编码对源文件重新进行改写
	// 注意：在后序读取pf文件时，需要将pf文件指针挪动到文件起始位置
	// 因为：刚开始在统计文件中字节出现次数的时已经读取过一遍文件了，pf已经在文件的末尾了
	//fseek(pf, 0, SEEK_SET);
	rewind(pf);

	uch ch = 0;
	uch bitCount = 0;
	while (true)
	{
		size_t rdsize = fread(readBuff, 1, 1024, pf);
		if (0 == rdsize)
			break;

		// 用编码改写字节---改写的结果需要放置到压缩结果文件当中
		for (size_t i = 0; i < rdsize; ++i)
		{
			// readBuff[i]---->'A'--->"100"
			string& strCode = fileByteInfo[readBuff[i]].strCode;

			// 只需要将字符串格式的二进制编码往字节中存放
			for (size_t j = 0; j < strCode.size(); ++j)
			{
				ch <<= 1;   // 高位丢弃，低位补0
				if ('1' == strCode[j])
					ch |= 1;

				// 当ch中的8个比特位填充满之后，需要将该字节写入到压缩文件当中
				bitCount++;
				if (8 == bitCount)
				{
					fputc(ch, fOut);
					bitCount = 0;
				}
			}
		}
	}

	// 检测：ch不够8个比特位，实际是没有写进去的
	if (bitCount > 0 && bitCount < 8)
	{
		ch <<= (8 - bitCount);
		fputc(ch, fOut);
	}

	fclose(pf);
	fclose(fOut);
	return true;
}


bool FileCompress::UNCompressFile(const string& filePath)
{
	// 1. 从压缩文件中读取解压缩是需要用到的信息
	FILE* fIn = fopen(filePath.c_str(), "rb");
	if (nullptr == fIn)
	{
		cout << "打开压缩文件失败" << endl;
		return false;
	}

	// 读取源文件后缀
	string postFix;
	GetLine(fIn, postFix);

	// 频次信息总行数
	string strContent;
	GetLine(fIn, strContent);
	size_t lineCount = atoi(strContent.c_str());

	// 循环读取lineCount行：获取字节的频次信息
	strContent = "";
	for (size_t i = 0; i < lineCount; ++i)
	{
		GetLine(fIn, strContent);
		if ("" == strContent)
		{
			// 说明刚刚读取到的是一个换行
			strContent += "\n";
			GetLine(fIn, strContent);
		}

		fileByteInfo[(uch)strContent[0]].appearCount = atoi(strContent.c_str() + 2);
		strContent = "";
	}

	// 2. 恢复huffman树
	HuffmanTree<ByteInfo> ht;
	ByteInfo invalid;
	ht.CreateHuffmanTree(fileByteInfo, 256, invalid);

	// 3. 读取压缩数据，结合huffman树进行解压缩
	string filename("3");
	filename += postFix;
	FILE* fOut = fopen(filename.c_str(), "wb");

	uch readBuff[1024];
	uch bitCount = 0;
	HuffmanTreeNode<ByteInfo>* cur = ht.GetRoot();
	const int fileSize = cur->weight.appearCount;
	int compressSize = 0;
	while (true)
	{
		size_t rdsize = fread(readBuff, 1, 1024, fIn);
		if (0 == rdsize)
			break;

		for (size_t i = 0; i < rdsize; ++i)
		{
			// 逐个字节比特位来进行解压缩
			uch ch = readBuff[i];
			bitCount = 0;
			while (bitCount < 8)
			{
				if (ch & 0x80)
					cur = cur->right;
				else
					cur = cur->left;

				if (nullptr == cur->left && nullptr == cur->right)
				{
					fputc(cur->weight.ch, fOut);
					cur = ht.GetRoot();
					compressSize++;
					// 如果成功解压缩字节的个数与源文件大小相同时则解压缩结束
					if (compressSize == fileSize)
						break;
				}

				bitCount++;
				ch <<= 1;
			}
		}
	}

	fclose(fIn);
	fclose(fOut);
	return true;
}

void FileCompress::GetLine(FILE* fIn, string& strContent)
{
	uch ch;
	while (!feof(fIn))
	{
		ch = fgetc(fIn);
		if (ch == '\n')
			break;

		strContent += ch;
	}
}

// 1.txt
void FileCompress::WriteHead(FILE* fOut, const string& filePath)
{
	// 1. 获取源文件的后缀
	string postFix = filePath.substr(filePath.rfind('.'));
	postFix += "\n";
	fwrite(postFix.c_str(), 1, postFix.size(), fOut);

	// 2. 构造字节频次信息已经统计有效字节总的行数
	string chAppearCount;
	size_t lineCount = 0;
	for (size_t i = 0; i < 256; ++i)
	{
		if (fileByteInfo[i].appearCount > 0)
		{
			chAppearCount += fileByteInfo[i].ch;
			chAppearCount += ':';
			chAppearCount += to_string(fileByteInfo[i].appearCount);
			chAppearCount += "\n";
			lineCount++;
		}
	}

	// 3. 写总行数以及频次信息
	string totalLine = to_string(lineCount);
	totalLine += "\n";
	fwrite(totalLine.c_str(), 1, totalLine.size(), fOut);
	fwrite(chAppearCount.c_str(), 1, chAppearCount.size(), fOut);
}

void FileCompress::GenerateHuffmanCode(HuffmanTreeNode<ByteInfo>* root)
{
	if (nullptr == root)
		return;

	// 因为在huffman树当中，所有有效的权值权值在叶子节点的位置
	// 当遍历到叶子节点的位置时，该权值对应的编码就拿到了
	if (nullptr == root->left && nullptr == root->right)
	{
		HuffmanTreeNode<ByteInfo>* cur = root;
		HuffmanTreeNode<ByteInfo>* parent = cur->parent;
		string& strCode = fileByteInfo[cur->weight.ch].strCode;
		while (parent)
		{
			if (cur == parent->left)
				strCode += '0';
			else
				strCode += '1';
			cur = parent;
			parent = cur->parent;
		}

		reverse(strCode.begin(), strCode.end());
	}

	GenerateHuffmanCode(root->left);
	GenerateHuffmanCode(root->right);
}