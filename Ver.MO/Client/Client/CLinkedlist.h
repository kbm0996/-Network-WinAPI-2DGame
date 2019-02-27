#ifndef __LIST_H__
#define __LIST_H__

template <typename T>
class ObjectList
{
public:
	struct Node
	{
		T _Data;
		Node *_Prev;
		Node *_Next;
	};

	class iterator
	{
	public:
		iterator(Node *node = nullptr)
		{
			_node = node;
		}

		iterator operator ++(int)
		{
			iterator temp = _node;
			_node = _node->_Next;
			return temp;
		}
		iterator operator ++()
		{
			_node = _node->_Next;
			return *this;
		}
		iterator operator --(int)
		{
			iterator temp = _node;
			_node = _node->_Prev;
			return *this;
		}

		iterator operator --()
		{
			_node = _node->_Prev;
			return *this;
		}

		T& operator *()
		{
			return _node->_Data;
		}

		bool operator !=(iterator& iter)
		{
			return _node != iter._node;
		}

		bool operator ==(iterator& iter)
		{
			return _node == iter._node;
		}

		Node* getNode()
		{
			return this->_node;
		}

	private:
		Node *_node;
	};

public:
	ObjectList() :_size(0)
	{
		_head._Data = 0;
		_head._Prev = nullptr;
		_head._Next = &_tail;
		_tail._Data = 0;
		_tail._Prev = &_head;
		_tail._Next = nullptr;
	}

	~ObjectList()
	{
	}

	iterator begin()
	{
		iterator begin(_head._Next);
		return begin;
	}
	iterator end()
	{
		iterator end(&_tail);
		return end;
	}

	void push_front(T data)
	{
		Node *pNode = new Node;

		pNode->_Data = data;
		_size++;

		pNode->_Prev = &_head;
		pNode->_Next = _head._Next;
		pNode->_Prev->_Next = pNode;
		pNode->_Next->_Prev = pNode;
	}

	void push_back(T data)
	{
		Node *pNode = new Node;

		pNode->_Data = data;
		_size++;

		pNode->_Prev = _tail._Prev;
		pNode->_Next = &_tail;
		pNode->_Prev->_Next = pNode;
		pNode->_Next->_Prev = pNode;
	}

	void pop_front()
	{
		Node *pNode = _head._Next;

		if (pNode != &_tail)
		{
			pNode->_Data = 0;
			_size--;

			pNode->_Next->_Prev = &_head;
			pNode->_Prev->_Next = pNode->_Next;
			delete(pNode);
		}
	}

	void pop_back()
	{
		Node *pNode = _tail._Prev;

		if (pNode != &_head)
		{
			pNode->_Data = 0;
			_size--;

			pNode->_Next->_Prev = pNode->_Prev;
			pNode->_Prev->_Next = &_tail;
			delete(pNode);
		}
	}


	void clear()
	{
		while (_size != 0)
			pop_front();
	}

	int size()
	{
		return _size;
	}

	iterator erase(iterator iter)
	{
		iterator temp = iter;
		Node *pNode = temp.getNode();
		temp++;

		pNode->_Data = 0;
		pNode->_Next->_Prev = pNode->_Prev;
		pNode->_Prev->_Next = pNode->_Next;
		delete(pNode);
		_size--;

		return temp;
	}

private:
	int _size;
	Node _head;
	Node _tail;
};

#endif