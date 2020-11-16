#ifndef TRIE_H
#define TRIE_H

#define MAX_WORD_LENGTH 30
#define TRANS_FLAG 0x3f3f3f3f

template <class NodeType>
class TrieNode
{
public:
	NodeType *info;
	TrieNode<NodeType> *firstSon;
	TrieNode<NodeType> *rightBrother;
	int count;
	int transcount;
	TrieNode();
	~TrieNode();
	void Clear();
};

template <class NodeType>
class Trie
{
public:
	TrieNode<NodeType> *root;
	Trie();
	~Trie();
	void insert(const NodeType *word, int length, int cnt);
	int find(const NodeType *word, int length);
	int match(const NodeType *word, NodeType *result, int length, int start);
};

template <class NodeType>
TrieNode<NodeType>::TrieNode()
{
	info = new NodeType;
	firstSon = NULL;
	rightBrother = NULL;
	count = 0;
	transcount = 0;
}

template <class NodeType>
TrieNode<NodeType>::~TrieNode()
{

}

template <class NodeType>
void TrieNode<NodeType>::Clear()
{
	if (firstSon != NULL)
	{
		firstSon->Clear();
		delete firstSon;
	}
	if (rightBrother != NULL)
	{
		rightBrother->Clear();
		delete rightBrother;
	}
}


template <class NodeType>
Trie<NodeType>::Trie()
{
	root = new TrieNode<NodeType>;
}

template <class NodeType>
Trie<NodeType>::~Trie()
{
	root->Clear();
	delete root;
}

template <class NodeType>
void Trie<NodeType>::insert(const NodeType *word, int length, int cnt)
{
	TrieNode<NodeType>* currentNode = root;
	TrieNode<NodeType>* tmpNode = NULL;
	for (int i = 0; i < length; i++)
	{
		tmpNode = currentNode->firstSon;
		while (tmpNode != NULL)
		{
			if (*(tmpNode->info) == word[i]) break;
			tmpNode = tmpNode->rightBrother;
		}
		if (tmpNode == NULL)
		{
			tmpNode = new TrieNode<NodeType>();
			memcpy(tmpNode->info, &word[i], sizeof(NodeType));
			tmpNode->rightBrother = currentNode->firstSon;
			currentNode->firstSon = tmpNode;
		}
		currentNode = tmpNode;
	}
	if (cnt == TRANS_FLAG) currentNode->transcount += 1;
	else currentNode->count += cnt;
}

template <class NodeType>
int Trie<NodeType>::find(const NodeType *word, int length)
{
	TrieNode<NodeType>* currentNode = root;
	TrieNode<NodeType>* tmpNode = NULL;
	for (int i = 0; i < length; i++)
	{
		tmpNode = currentNode->firstSon;
		while (tmpNode != NULL)
		{
			if (*(tmpNode->info) == word[i]) break;
			tmpNode = tmpNode->rightBrother;
		}
		if (tmpNode == NULL)
		{
			return 0;
		}
		currentNode = tmpNode;
	}
	return currentNode->count;
}

//������ƥ�䵽dict����ĵ��ʣ�����ֵΪ�´�ƥ�����ʼ��
template <class NodeType>
int Trie<NodeType>::match(const NodeType *word, NodeType *result, int length, int start)
{
	int end = start;

	TrieNode<NodeType>* currentNode = root;
	TrieNode<NodeType>* tmpNode = NULL;
	for (int i = start; i < length; i++)
	{
		tmpNode = currentNode->firstSon;
		while (tmpNode != NULL)
		{
			if (*(tmpNode->info) == word[i])	//�ҵ�������ȥ�ķ�֧
			{
				if (tmpNode->count != 0)	//��0����ʾ��ƥ���ϵ���ǰ��һ����
				{
					end = i + 1;
				}
				break;
			}
			tmpNode = tmpNode->rightBrother;
		}
		if (tmpNode == NULL)	//û�з�֧��������ȥ
		{
			//endΪstart��ʾû���κο���ƥ��Ľ��
			if (end == start)
				return end;
			//���򣬽�ƥ��������result��
			for (int j = start; j < end; j++)
				result[j - start] = word[j];
			result[end - start] = '\0';

			return end;
			//return false;
		}
		currentNode = tmpNode;
	}
	if (currentNode->count != 0)	//ƥ�������ȫ����ƥ����
	{
		for (int j = start; j < length; j++)
			result[j - start] = word[j];
		result[length - start] = '\0';

		return end;
	}
	else
	{
		if (end == start)
			return end;
		//���򣬽�ƥ��������result��
		for (int j = start; j < end; j++)
			result[j - start] = word[j];
		result[end - start] = '\0';
		return end;
	}
}

#endif
