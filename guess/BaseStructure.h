#ifndef BASESTRUCTURE_H
#define BASESTRUCTURE_H

#include<algorithm>
#include "alphaNumPair.h"
using namespace std;

class BaseStructure
{
public:
	alphaNumPair word[MAX_WORD_LENGTH];
	int count;
	int length;
	BaseStructure();
	~BaseStructure();
};

BaseStructure::BaseStructure()
{
	count = 0;
	length = 0;
}

BaseStructure::~BaseStructure()
{
}

#endif