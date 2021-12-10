#pragma once

#include <assert.h>

namespace Helix
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class intrusive_list_node
	{
	public:
		intrusive_list_node() = default;
		virtual ~intrusive_list_node() = default;

		intrusive_list_node(const intrusive_list_node& other) = delete;
		intrusive_list_node& operator=(const intrusive_list_node& other) = delete;

		intrusive_list_node(intrusive_list_node&& other) = default;
		intrusive_list_node& operator=(intrusive_list_node&& other) = default;

		void set_next(intrusive_list_node* next) { m_next = next; }
		void set_prev(intrusive_list_node* prev) { m_prev = prev; }

		intrusive_list_node* get_next() const { return m_next; }
		intrusive_list_node* get_prev() const { return m_prev; }

	private:
		intrusive_list_node* m_next = nullptr;
		intrusive_list_node* m_prev = nullptr;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <typename T>
	class intrusive_list;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <typename T>
	class intrusive_list_iterator
	{
		friend intrusive_list<T>;

	public:
		intrusive_list_iterator()
			: m_node(nullptr) { }

		intrusive_list_iterator(T* node)
			: m_node(node) { }

		bool is_valid() const
			{ return m_node != nullptr; }

		bool operator==(const intrusive_list_iterator<T>& other) const
			{ return m_node == other.m_node; }

		bool operator!=(const intrusive_list_iterator<T>& other) const
			{ return m_node != other.m_node; }

		T& operator*()
			{ return *m_node; }

		T* operator->()
			{ return m_node; }

		intrusive_list_iterator<T>& operator++()
			{ m_node = static_cast<T*>(m_node->get_next()); return *this; }

		intrusive_list_iterator<T>& operator--()
			{ m_node = static_cast<T*>(m_node->get_prev()); return *this; }

		intrusive_list_iterator<T> operator++(int)
			{ return intrusive_list_iterator<T>(m_node->get_next()); }

		intrusive_list_iterator<T> operator--(int)
			{ return intrusive_list_iterator<T>(m_node->get_prev()); }

		void invalidate() { m_node = nullptr; }

	private:
		T* m_node;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template <typename T>
	class intrusive_list
	{
	public:
		using iterator       = intrusive_list_iterator<T>;
		using const_iterator = intrusive_list_iterator<const T>;

		intrusive_list()
		{
			m_sentinel.set_prev(&m_sentinel);
			m_sentinel.set_next(&m_sentinel);
		}

		bool empty() const { return begin() == end(); }

		iterator       begin() { return iterator(static_cast<T*>(m_sentinel.get_next())); }
		iterator       end()   { return iterator(static_cast<T*>(&m_sentinel));           }

		const_iterator begin() const { return const_iterator(static_cast<const T*>(m_sentinel.get_next())); }
		const_iterator end()   const { return const_iterator(static_cast<const T*>(&m_sentinel)); }

		size_t size() const { return m_size; }

		iterator insert_before(const iterator& where, T* value)
		{
			T* node = where.m_node;
			T* prev = static_cast<T*>(node->get_prev());	

			prev->set_next(value);
			node->set_prev(value);

			value->set_prev(prev);
			value->set_next(node);

			m_size++;

			return iterator(value);
		}

		iterator insert_after(const iterator& where, T* value)
		{
			T* node = where.m_node;
			T* next = static_cast<T*>(node->get_next());

			node->set_next(value);
			next->set_prev(value);

			value->set_prev(node);
			value->set_next(next);

			m_size++;

			return iterator(value);
		}

		void remove(const iterator& where)
		{
			T* node = where.m_node;
			T* next = static_cast<T*>(node->get_next());
			T* prev = static_cast<T*>(node->get_prev());

			prev->set_next(next);
			next->set_prev(prev);

			node->set_next(nullptr);
			node->set_prev(nullptr);

			m_size--;
		}

		void replace(T* original_value, T* new_value)
		{
			new_value->set_next(original_value->get_next());
			new_value->set_prev(original_value->get_prev());

			original_value->get_prev()->set_next(new_value);
			original_value->get_next()->set_prev(new_value);

			original_value->set_next(nullptr);
			original_value->set_prev(nullptr);
		}

		void push_back(T* value) { this->insert_before(end(), value); }

		T& back() { return *static_cast<T*>(m_sentinel.get_prev()); }
		const T& back() const { return *static_cast<T*>(m_sentinel.get_prev()); }

	private:
		intrusive_list_node m_sentinel;
		size_t m_size = 0 ;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
