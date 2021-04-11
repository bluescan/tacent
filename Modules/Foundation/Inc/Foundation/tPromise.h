// tPromise.h
//
// A promise is a pledge to give something at a future time. Promises allow asynchronous programming and are
// thread-safe.
//
// The pomisor is the entity that promises something.
// The promisee is an entity the promise is made to.
//
// A promise may be pending (promisor still intends to honour it).
// A promise may be fulfilled (promisor has to honoured it).
// A promise may be reneged (promisor has failed to honour it).
// A promise is settled when it enters a fulfilled or reneged state.
//
// In this implementation the promisor creates the promise. Promises are ref counted via a shared pointer. There may be
// multiple promisees. Whenever the ref count in the shared pointer reaches 0 the promise is deleted.
//
// This is an example of a promise to return an unknown number of random floats in a list.
// Promise<tList<float>> Promiser::GetRandomList();
//
// In this example the promiser is saying I'll give you one promise of a random float at a time. The promiser fulfills
// one float, and promises the next. On the last one it reneges.
// Promise<float> Promiser::GetRandomFloatSequence()

// Copyright (c) 2021 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <mutex>
#include <Foundation/tSmartPointers.h>


template<typename T> class tPromise
{
public:
	enum class State
	{
		Pending,
		Fulfilled,							// Settled.
		Reneged								// Settled.
	};

	tPromise()								: PromiseState(State::Pending), PromisePackage() { }

	State WaitUntilSettled();				// Blocks.
	State GetState();						// Called by promisee. Non-blocking. Promisee may poll.
	T GetItem();							// Called by promisee. Will be default obj if state not fulfilled.

	// Called by promiser. Non-blocking. Promiser reneges on a promise when it can't fulfill it. For example, getting
	// windows share names can be painfully slow, and the promiser doesn't know a-priori how many there are. It can
	// call Fulfill each time and supply another promise, but on the last one (when it knows there are no more) it will
	// have to renege.
	void Renege();

	// Called by promiser. Non-blocking. Promiser may optionally make another promise.
	void Fulfill(T, tSharedPtr<tPromise> nextPromise = nullptr);

private:
	std::mutex Mutex;						// Both PromiseState and PromisePackage are mutex protected.
	State PromiseState;
	struct Package
	{
		T Item;								// Only valid if promise fulfilled.
		tSharedPtr<tPromise> NextPromise;	// Does promiser have something else for ya?
	};
	Package PromisePackage;
};


// Implementation below this line.


template<typename T> inline typename tPromise<T>::State tPromise<T>::WaitUntilSettled()
{
	// Block until settled.
}


template<typename T> inline typename tPromise<T>::State tPromise<T>::GetState()
{
	Mutex.lock();
	tPromise<T>::State state = PromiseState;
	Mutex.unlock();
	return state;
}


template<typename T> inline T tPromise<T>::GetItem()
{
	Mutex.lock();
	T item = PromisePackage.Item;
	Mutex.unlock();
	return item;
}


template<typename T> inline void tPromise<T>::Renege()
{
	Mutex.lock();
	PromiseState = State::Reneged;
	Mutex.unlock();
}


template<typename T> inline void tPromise<T>::Fulfill(T item, tSharedPtr<tPromise> nextPromise)
{
	Mutex.lock();
	PromiseState = State::Fulfilled;
	PromisePackage.Item = item;
	PromisePackage.NextPromise = nextPromise;
	Mutex.unlock();
}
