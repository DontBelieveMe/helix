#pragma once

#include "system.h"

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
			{ return intrusive_list_iterator<T>((T*) m_node->get_next()); }

		intrusive_list_iterator<T> operator--(int)
			{ return intrusive_list_iterator<T>((T*) m_node->get_prev()); }

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

		/**
		 * Return if the list is empty (that is `begin() == end()`)
		 * 
		 * @return true  The list has no elements.
		 * @return false The list has at least one element.
		 */
		bool empty() const { return begin() == end(); }

		iterator       begin() { return iterator(static_cast<T*>(m_sentinel.get_next())); }
		iterator       end()   { return iterator(static_cast<T*>(&m_sentinel));           }

		const_iterator begin() const { return const_iterator(static_cast<const T*>(m_sentinel.get_next())); }
		const_iterator end()   const { return const_iterator(static_cast<const T*>(&m_sentinel)); }

		/**
		 * Return the number of nodes in this list.
		 * 
		 * This is a O(1) operation, since it is cached and kept up to date
		 * internally by functions that modify the lists internal state.
		 * 
		 * This value may be incorrect if the structure of the list is modified
		 * externally (e.g. by using set_prev/set_next on nodes outside this class).
		 * In the case that external modification causes the size of the list to change,
		 * call recompute_size() to update the cached size with the actual
		 * size of the list.
		 * 
		 * @return size_t The number of nodes in this list
		 */
		size_t size() const { return m_size; }

		/// WARNING: Only use if your doing manual manipulation of the list
		///          structure!!!
		void set_size(size_t new_size)
		{
			m_size = new_size;
		}

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
			if (!where.is_valid())
				return;

			T* node = where.m_node;

			T* next = static_cast<T*>(node->get_next());
			T* prev = static_cast<T*>(node->get_prev());

			/* Nothing to remove from the list if both the next & previous
			   links are null.  */
			if (!next && !prev)
				return;

			helix_assert(next && prev, "Both next & prev links need to be valid :)");

			prev->set_next(next);
			next->set_prev(prev);

			node->set_next(nullptr);
			node->set_prev(nullptr);

			m_size--;
		}

		/**
		 * Replace the given node in the list with another node.
		 * After replacement the next & previous nodes on the original will be
		 * null.
		 * 
		 * `original_value` must be a member of this list, and `new_value` cannot
		 * already be a node in this list.
		 * 
		 * @param original_value The element that will be replaced.
		 * @param new_value The element to replace with.
		 */
		void replace(T* original_value, T* new_value)
		{
			new_value->set_next(original_value->get_next());
			new_value->set_prev(original_value->get_prev());

			original_value->get_prev()->set_next(new_value);
			original_value->get_next()->set_prev(new_value);

			original_value->set_next(nullptr);
			original_value->set_prev(nullptr);
		}

		/**
		 * Append the given element to the end of the list.
		 * Functionally equivilant of `insert_before(end(), value)`.
		 * 
		 * @param value Element to append to the back list. 
		 */
		void push_back(T* value) { this->insert_before(end(), value); }

		/**
		 * Return a reference to the last item in the list (that is the element
		 * before end()).
		 * Assertion will trigger if the last element is null.
		 * 
		 * @return T& Reference to last item in the list.
		 */
		T& back() { return *static_cast<T*>(m_sentinel.get_prev()); }

		/**
		 * Return a constant reference to the last item in the list (that is the element
		 * before end()).
		 * Assertion will trigger if the last element is null. 
		 * 
		 * @return const T& Constant reference to the last item in the list
		 */
		const T& back() const { return *static_cast<T*>(m_sentinel.get_prev()); }

		T& front() { return *static_cast<T*>(m_sentinel.get_next()); }
		const T& front() const { return *static_cast<T*>(m_sentinel.get_next()); }

	private:
		intrusive_list_node m_sentinel;
		size_t m_size = 0 ;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}
