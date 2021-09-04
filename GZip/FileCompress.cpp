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
	// 1. ͳ��Դ�ļ���ÿ���ֽڳ��ֵĴ���---����
	FILE* pf = fopen(filePath.c_str(), "rb");
	if (nullptr == pf)
	{
		cout << "�򿪴�ѹ���ļ�ʧ��!!!" << endl;
		return false;
	}

	// �ļ���С��ʱ��֪��----�˴���Ҫ����ѭ���ķ�ʽ����ȡԴ�ļ�������
	uch readBuff[1024];
	while (true)
	{
		// ���⣺��Ҫһ���Զ�ȡ1024���ֽڣ���ʵ�ʶ�ȡ����rdsize���ֽ�
		size_t rdsize = fread(readBuff, 1, 1024, pf);
		if (0 == rdsize)
		{
			// �����Ѿ���ȡ���ļ���ĩβ
			break;
		}

		// ����ͳ��
		for (size_t i = 0; i < rdsize; ++i)
		{
			// ���õ��ˣ�ֱ�Ӷ��Ʒ�---���ַ���ASCIIֵ��Ϊ������±������п��ٵ�ͳ��
			fileByteInfo[readBuff[i]].appearCount++;
		}
	}

	//////////////////////////////////////////////////////////////////////
	// 2. ����ͳ�ƵĽ������huffman��
	// ע�⣺�ڴ���huffman���ڼ䣬��Ҫ�����ִ���Ϊ0���ֽ��޳���
	HuffmanTree<ByteInfo> ht;
	ByteInfo invalid;
	ht.CreateHuffmanTree(fileByteInfo, 256, invalid);

	////////////////////////////////////////////////////////////////////////
	// 3. ����huffman����ȡÿ���ֽڵı���
	GenerateHuffmanCode(ht.GetRoot());

	/////////////////////////////////////////////////////////////////////
	/// 4. д��ѹ��ʱ����Ҫ�õ�����Ϣ
	FILE* fOut = fopen("2.hzp", "wb");
	WriteHead(fOut, filePath);

	////////////////////////////////////////////////////////////////////////
	// 5. ʹ���ֽڵı����Դ�ļ����½��и�д
	// ע�⣺�ں����ȡpf�ļ�ʱ����Ҫ��pf�ļ�ָ��Ų�����ļ���ʼλ��
	// ��Ϊ���տ�ʼ��ͳ���ļ����ֽڳ��ִ�����ʱ�Ѿ���ȡ��һ���ļ��ˣ�pf�Ѿ����ļ���ĩβ��
	//fseek(pf, 0, SEEK_SET);
	rewind(pf);

	uch ch = 0;
	uch bitCount = 0;
	while (true)
	{
		size_t rdsize = fread(readBuff, 1, 1024, pf);
		if (0 == rdsize)
			break;

		// �ñ����д�ֽ�---��д�Ľ����Ҫ���õ�ѹ������ļ�����
		for (size_t i = 0; i < rdsize; ++i)
		{
			// readBuff[i]---->'A'--->"100"
			string& strCode = fileByteInfo[readBuff[i]].strCode;

			// ֻ��Ҫ���ַ�����ʽ�Ķ����Ʊ������ֽ��д��
			for (size_t j = 0; j < strCode.size(); ++j)
			{
				ch <<= 1;   // ��λ��������λ��0
				if ('1' == strCode[j])
					ch |= 1;

				// ��ch�е�8������λ�����֮����Ҫ�����ֽ�д�뵽ѹ���ļ�����
				bitCount++;
				if (8 == bitCount)
				{
					fputc(ch, fOut);
					bitCount = 0;
				}
			}
		}
	}

	// ��⣺ch����8������λ��ʵ����û��д��ȥ��
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
	// 1. ��ѹ���ļ��ж�ȡ��ѹ������Ҫ�õ�����Ϣ
	FILE* fIn = fopen(filePath.c_str(), "rb");
	if (nullptr == fIn)
	{
		cout << "��ѹ���ļ�ʧ��" << endl;
		return false;
	}

	// ��ȡԴ�ļ���׺
	string postFix;
	GetLine(fIn, postFix);

	// Ƶ����Ϣ������
	string strContent;
	GetLine(fIn, strContent);
	size_t lineCount = atoi(strContent.c_str());

	// ѭ����ȡlineCount�У���ȡ�ֽڵ�Ƶ����Ϣ
	strContent = "";
	for (size_t i = 0; i < lineCount; ++i)
	{
		GetLine(fIn, strContent);
		if ("" == strContent)
		{
			// ˵���ոն�ȡ������һ������
			strContent += "\n";
			GetLine(fIn, strContent);
		}

		fileByteInfo[(uch)strContent[0]].appearCount = atoi(strContent.c_str() + 2);
		strContent = "";
	}

	// 2. �ָ�huffman��
	HuffmanTree<ByteInfo> ht;
	ByteInfo invalid;
	ht.CreateHuffmanTree(fileByteInfo, 256, invalid);

	// 3. ��ȡѹ�����ݣ����huffman�����н�ѹ��
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
			// ����ֽڱ���λ�����н�ѹ��
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
					// ����ɹ���ѹ���ֽڵĸ�����Դ�ļ���С��ͬʱ���ѹ������
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
	// 1. ��ȡԴ�ļ��ĺ�׺
	string postFix = filePath.substr(filePath.rfind('.'));
	postFix += "\n";
	fwrite(postFix.c_str(), 1, postFix.size(), fOut);

	// 2. �����ֽ�Ƶ����Ϣ�Ѿ�ͳ����Ч�ֽ��ܵ�����
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

	// 3. д�������Լ�Ƶ����Ϣ
	string totalLine = to_string(lineCount);
	totalLine += "\n";
	fwrite(totalLine.c_str(), 1, totalLine.size(), fOut);
	fwrite(chAppearCount.c_str(), 1, chAppearCount.size(), fOut);
}

void FileCompress::GenerateHuffmanCode(HuffmanTreeNode<ByteInfo>* root)
{
	if (nullptr == root)
		return;

	// ��Ϊ��huffman�����У�������Ч��ȨֵȨֵ��Ҷ�ӽڵ��λ��
	// ��������Ҷ�ӽڵ��λ��ʱ����Ȩֵ��Ӧ�ı�����õ���
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