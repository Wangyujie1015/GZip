#include <iostream>
using namespace std;

#include "FileCompress.h"

int main()
{
	FileCompress fc;
	fc.CompressFile("1.txt");
	fc.UNCompressFile("2.hzp");
}