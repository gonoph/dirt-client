#ifndef __LIST_H
#define __LIST_H



template <class T>
struct Node
{
	T data;
	Node *next;
};

#define FOREACH(type,name,list) for(type name = list.rewind(); name; name = list.next())

// A linked list. Contains many nodes.
template <class T>
class List
{
	
	public:
	
	List() : first(NULL), next_node(NULL) {}

	void insert (const T& x);
	void append (const T& x);
	void remove (const T& x);
	inline T rewind();
	inline T next();
	
	T operator[] (int no) const;
	
	int count() const;
	
private:
	Node <T> *first, *next_node;
};

template <class T> int List<T>::count() const
{
	int count = 0;
	for (Node<T> *n = first; n; n = n->next)
		count++;

	return count;
}

template<class T> T List<T>::operator[] (int no) const
{
    int count = 0;

	for (Node<T> *n = first; n; n = n->next, count++)
		if (count == no)
			return n->data;
	
	return 0;
}

template<class T> T List<T>::next()
{
    Node<T> *p = next_node;

	if (p)
		next_node = next_node->next;
	
	return p ? p->data : 0;
}

template<class T> void List<T>::remove(const T& x)
{
    Node<T> *p;
    assert (first != NULL);

	if (first->data == x)
	{
		p = first;
		first = first->next;
	}
	else
	{
		Node<T> *prev;
		for (prev = first; prev && prev->next->data != x; prev = prev->next)
			;
		
        if (!prev || prev->next->data != x)
            abort();
		
		p = prev->next;
		prev->next = prev->next->next;
	}
	
	if (next_node && next_node->data == x)
		next_node = p->next;
		
    delete p;
}

template<class T> T List<T>::rewind()
{

	if (first)
		next_node = first->next;
	else
		next_node = NULL;
		
	return first ? first->data : 0;
}

template <class T> void List<T>::insert (const T& x)
{
	Node<T> *node = new Node<T>;
    Node<T> *prev;

    node->data = x;

	for (prev = first; prev && prev->next;prev = prev->next)
		;
	
	if (!prev)
	{
		node->next = first;
		first = node;
	}
	else
	{
		prev->next = node;
		node->next = NULL;
    }

}


// This is really *PRE* pend. What was I thinking?
template <class T> void List<T>::append (const T& x)
{
	Node<T> *node = new Node<T>;
	
	node->data = x;
	node->next = first;
    first = node;
}


#endif
