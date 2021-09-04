#pragma once


#include <queue>
#include <vector>
using namespace std;


template<class W>
struct HuffmanTreeNode
{
	HuffmanTreeNode<W>* left;
	HuffmanTreeNode<W>* right;
	HuffmanTreeNode<W>* parent;
	W weight;

	HuffmanTreeNode(const W& w = W())
		: left(nullptr)
		, right(nullptr)
		, parent(nullptr)
		, weight(w)
	{}
};


template<class W>
struct Com
{
	typedef HuffmanTreeNode<W> Node;
	bool operator()(const Node* left, const Node* right)
	{
		return left->weight > right->weight;
	}
};

template<class W>
class HuffmanTree
{
	typedef HuffmanTreeNode<W> Node;
public:
	HuffmanTree()
		:root(nullptr)
	{}

	~HuffmanTree()
	{
		Destroy(root);
	}

	void CreateHuffmanTree(const W array[], size_t size, const W& invalid)
	{
		// 小堆---优先级队列默认情况下是大堆，只需要修改其比较规则
		std::priority_queue<Node*, vector<Node*>, Com<W>> q;

		// 1. 先使用所给的权值创建只有根节点的二叉树森林
		for (size_t i = 0; i < size; ++i)
		{
			if (array[i] != invalid)
				q.push(new Node(array[i]));
		}

		// 2. 循环进行以下步骤，直到二叉树森林中只剩余一棵二叉树位置
		while (q.size() > 1)
		{
			// 从二叉树森林中先去权值最小的两棵二叉树
			Node* left = q.top();
			q.pop();

			Node* right = q.top();
			q.pop();

			// 将left和right作为某个新节点的左右子树，构造一个新的二叉树，
			// 新二叉树根节点的权值就是其左右孩子权值之和
			Node* parent = new Node(left->weight + right->weight);
			parent->left = left;
			parent->right = right;
			left->parent = parent;
			right->parent = parent;

			// 将新的二叉树插入到二叉树森林中
			q.push(parent);
		}

		root = q.top();
	}

	Node* GetRoot()
	{
		return root;
	}

	void Destroy(Node*& proot)
	{
		if (proot)
		{
			Destroy(proot->left);
			Destroy(proot->right);
			delete proot;
			proot = nullptr;
		}
	}
private:
	Node* root;
};
