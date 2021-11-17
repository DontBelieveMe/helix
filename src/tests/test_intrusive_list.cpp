#include "../intrusive_list.h"

#include "catch.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct MyInteger : public Helix::intrusive_list_node
{
	MyInteger(int v) : value(v) { }
	int value = 0xcafe;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct ScopeAllocator
{
public:
	~ScopeAllocator()
	{
		std::destroy_n(values.begin(), values.size());
	}

	T* Parent(T* p)
	{
		values.push_back(p);
		return p;
	}

private:
	std::vector<T*> values;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("Empty intrusive_list", "[intrusive_list]")
{
	Helix::intrusive_list<MyInteger> ints;

	REQUIRE(ints.begin() == ints.end());
	REQUIRE(ints.empty());

	int count = 0;

	for (const MyInteger& integer : ints)
	{
		count++;
	}

	REQUIRE(count == 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("Single push_back", "[intrusive_list]")
{
	ScopeAllocator<MyInteger> alloc;

	Helix::intrusive_list<MyInteger> ints;
	ints.push_back(alloc.Parent(new MyInteger(123)));

	const MyInteger& v = *ints.begin();

	REQUIRE(!ints.empty());
	REQUIRE(v.value == 123);

	int sum = 0;
	for (const MyInteger& integer : ints)
	{
		sum += integer.value;
	}

	REQUIRE(sum == 123);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("Multiple push back", "[intrusive_list]")
{
	ScopeAllocator<MyInteger> alloc;

	Helix::intrusive_list<MyInteger> ints;
	
	MyInteger* a = alloc.Parent(new MyInteger(1));
	MyInteger* b = alloc.Parent(new MyInteger(2));
	MyInteger* c = alloc.Parent(new MyInteger(3));
	MyInteger* d = alloc.Parent(new MyInteger(4));

	ints.push_back(a); REQUIRE(ints.back().value == 1);
	ints.push_back(b); REQUIRE(ints.back().value == 2);
	ints.push_back(c); REQUIRE(ints.back().value == 3);
	ints.push_back(d); REQUIRE(ints.back().value == 4);

	std::vector<int> expected = { 1, 2, 3, 4 };

	int index = 0; bool matches = true;
	
	for (const MyInteger& i : ints)
	{
		matches &= (i.value == expected[index]);
		index++;
	}

	REQUIRE(matches);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("Muliple random insertions (insert_before)", "[intrusive_list]")
{
	ScopeAllocator<MyInteger> alloc;

	Helix::intrusive_list<MyInteger> ints;

	MyInteger* a = alloc.Parent(new MyInteger(1));
	MyInteger* b = alloc.Parent(new MyInteger(2));
	MyInteger* c = alloc.Parent(new MyInteger(3));
	MyInteger* d = alloc.Parent(new MyInteger(4));

	ints.push_back(a);
	ints.push_back(b);

	// it points to 'b' (we want to insert a node before 'b')
	Helix::intrusive_list<MyInteger>::iterator it = ints.begin(); ++it;

	ints.insert_before(it, c);
	ints.insert_before(ints.begin(), d);

	std::vector<int> expected = { 4, 1, 3, 2 };
	int index = 0; bool matches = true;
	for (const MyInteger& i : ints)
	{
		matches &= (i.value == expected[index]);
		index++;
	}

	REQUIRE(matches);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("insert_before end() on empty list", "[intrusive_list]")
{
	ScopeAllocator<MyInteger> alloc;
	Helix::intrusive_list<MyInteger> ints;

	MyInteger* a = alloc.Parent(new MyInteger(123));

	ints.insert_before(ints.end(), a);

	REQUIRE(ints.begin()->value == 123);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("insert_before begin() on empty list", "[intrusive_list]")
{
	ScopeAllocator<MyInteger> alloc;
	Helix::intrusive_list<MyInteger> ints;

	MyInteger* a = alloc.Parent(new MyInteger(123));

	ints.insert_before(ints.begin(), a);

	REQUIRE(ints.begin()->value == 123);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("insert_after begin() on empty list", "[intrusive_list]")
{
	ScopeAllocator<MyInteger> alloc;
	Helix::intrusive_list<MyInteger> ints;

	MyInteger* a = alloc.Parent(new MyInteger(123));

	ints.insert_after(ints.begin(), a);

	REQUIRE(ints.begin()->value == 123);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TEST_CASE("insert_after end() on empty list", "[intrusive_list]")
{
	ScopeAllocator<MyInteger> alloc;
	Helix::intrusive_list<MyInteger> ints;

	MyInteger* a = alloc.Parent(new MyInteger(123));

	ints.insert_after(ints.end(), a);

	REQUIRE(ints.begin()->value == 123);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
