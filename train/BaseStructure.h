#ifndef BASESTRUCTURE_H
#define BASESTRUCTURE_H

#include<algorithm>
#include "alphaNumPair.h"
#include "Trie.h"
using namespace std;

class BaseStructure
{
public:
	alphaNumPair word[MAX_WORD_LENGTH];
	int count;
	int length;
	BaseStructure();
	~BaseStructure();
	const bool operator<(const BaseStructure & x);
	const bool operator>(const BaseStructure & x);
	const bool operator==(const BaseStructure & x);
};

BaseStructure::BaseStructure()
{
	count = 0;
	length = 0;
}

BaseStructure::~BaseStructure()
{
}

const bool BaseStructure::operator<(const BaseStructure & x)
{
	int l = x.length;
	if (l > length) l = length;
	for (int i = 0; i < l; i++)
		if (word[i] < x.word[i]) return true;
		else if (word[i] > x.word[i]) return false;
	return length < x.length;
}

const bool BaseStructure::operator>(const BaseStructure & x)
{
	int l = x.length;
	if (l > length) l = length;
	for (int i = 0; i < l; i++)
		if (word[i] > x.word[i]) return true;
		else if (word[i] < x.word[i]) return false;
	return length > x.length;
}

const bool BaseStructure::operator==(const BaseStructure & x)
{
	int l = length;
	if (l != x.length) return false;
	for (int i = 0; i < l; i++)
	{
		if (word[i] < x.word[i]) return false;
		if (word[i] > x.word[i]) return false;
	}
	return true;
}

bool bsCountCompare(const BaseStructure &x, const BaseStructure &y)
{
	return x.count > y.count;
}

void printBaseStructureNode(BaseStructure *bs, int &bsCount, TrieNode<alphaNumPair> *node, alphaNumPair *path, int level)
{
	path[level] = *(node->info);
	if (node->count > 0)
	{
		for (int i = 1; i <= level; i++) bs[bsCount].word[i - 1] = path[i];
		bs[bsCount].count = node->count;
		bs[bsCount].length = level;
		bsCount++;
	}
	TrieNode<alphaNumPair> *tmpNode = node->firstSon;
	while (tmpNode != NULL)
	{
		printBaseStructureNode(bs, bsCount, tmpNode, path, level + 1);
		tmpNode = tmpNode->rightBrother;
	}
}

void printToBaseStructureArray(BaseStructure *bs, int &bsCount, Trie<alphaNumPair> *trie)
{
	alphaNumPair *path = new alphaNumPair[MAX_WORD_LENGTH]();
	printBaseStructureNode(bs, bsCount, trie->root, path, 0);
	sort(&bs[0], bs + bsCount, bsCountCompare);
}
#endif