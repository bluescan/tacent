// tTask.h
//
// Simple and efficient task management using a heap-based priority queue.
//
// Copyright (c) 2006, 2017, 2023, 2025 Tristan Grimmer.
// Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby
// granted, provided that the above copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
// AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <Foundation/tPriorityQueue.h>


// All tasks that you want in a task-set must be derived from a tTask. You need to implement at least one of ExecuteF or
// ExecuteI. Use ExecuteI if you want to put the task on a tTaskSetI which works by returning an integer rather than a
// floating-point number for the next execution time.
struct tTask
{
	// Tasks get executed when a TaskSet's update is called. If too much time has passed and the count is too large, it
	// may be desireable to schedule the next execute time for the task to compensate for the tardy Update call. If
	// tardinessCompensation, the next update time retrieved from the task is reduced by how late (tardy) the current
	// execution call is. This can help keep the overall number of executions per time interval more consistent. For
	// example, if 1/60 is returned by Execute, having compensate on will ensure we get 60 of them in a second even if
	// the update is often late. Without compensation the tardiness accumulates after every execute.
	tTask(bool tardinessCompensation = true)																			: TardinessCompensation(tardinessCompensation) { }
	virtual ~tTask()																									{ }

	// Return the next time you want Execute to be called in seconds. If you return 0.0 or less the task will execute on
	// the next update.
	virtual double Execute(double deltaTime)																			{ return 0.0; }

	// Return the next time you want Execute to be called in integer nanoseconds. If you return 0 or less the task will
	// execute on the next update.
	virtual int64 Execute(int64 deltaTime_ns)																			{ return 0; }

	bool TardinessCompensation;
};


class tTaskSetF
{
public:
	// CounterFreq must be given in Hz. MaxTimeDelta is the ceiling on the elapsed time that the Execute() fn gets
	// called with. Useful for things like collision detection where we need some guarantees. If the max is hit because
	// the fps is slow then the objects will, only at that point, start to slow down.
	tTaskSetF(int64 counterFreq, double maxTimeDelta);

	// Sometimes it's not convenient to set the counter freq and max delta in the constructor. Call SetCounter if you
	// use this constructor.
	tTaskSetF();
	~tTaskSetF()																											{ }

	// If you need to adjust the counterFreq dynamically you can do so here.
	void SetCounter(int64 counterFreq, double maxTimeDelta)																{ CounterFreq = counterFreq; MaxTimeDelta = maxTimeDelta; }

	// Inserts a task in O(lg(n)) time. Memory for tTask is managed by the caller. When a task is first inserted, it
	// gets scheduled to be executed on the next call to Update. After that, the task controls the next execution time
	// by returning the desired number of seconds.
	void Insert(tTask* t)																								{ PriorityQueue.Insert( tPQ<tTask*>::tItem(t, UpdateTime) ); }

	// Removes a task in O(n) time. You'll probably want to delete it after. Internally the tQueueItem isn't removed
	// until it's about to be executed again, but you don't need to know that.
	void Remove(tTask* t)																								{ PriorityQueue.Replace( t, nullptr ); }

	// Executes any tasks that are ready. O(lg(n)). Call this as often as you like.
	void Update(int64 counter);

private:
	int64 UpdateTime;					// Time Update was called last.
	int64 CounterFreq;					// How quickly the counter value that gets passed to Update() is going in Hz.
	double MaxTimeDelta;
	tPriorityQueue<tTask*> PriorityQueue;

	static const int NumTasks = 64;
	static const int GrowSize = 32;
};
