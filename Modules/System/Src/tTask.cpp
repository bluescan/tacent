// tTask.cpp
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

#include "System/tTask.h"


tTaskSetF::tTaskSetF(int64 counterFreq, double maxTimeDelta) :
	UpdateTime(0),
	CounterFreq(counterFreq),
	MaxTimeDelta(maxTimeDelta),
	PriorityQueue(NumTasks, GrowSize)
{
}


tTaskSetF::tTaskSetF() :
	UpdateTime(0),
	CounterFreq(0),
	MaxTimeDelta(0),
	PriorityQueue(NumTasks, GrowSize)
{
}


void tTaskSetF::Update(int64 counter)
{
	bool runningTasks = true;
	while (runningTasks)
	{
		if (PriorityQueue.GetNumItems() == 0)
			return;

		// Take tasks that are due from the front of the priority queue.
		int64 keyCount = PriorityQueue.GetMin().Key;
		if (keyCount <= counter)
		{
			tPQ<tTask*>::tItem qn = PriorityQueue.GetRemoveMin();
			tTask* t = (tTask*)qn.Data;

			// If there is no function tTask pointer we're all done. The node is already removed from the queue.
			if (t)
			{
				double td = double(counter - UpdateTime) / double(CounterFreq);
				if (td > MaxTimeDelta)
					td = MaxTimeDelta;

				double nextTime = t->Execute(td);
				int64 nextTimeDelta = int64( nextTime*double(CounterFreq) );

				if (t->TardinessCompensation)
				{
					int64 tardiness = counter - keyCount;
					nextTimeDelta -= tardiness;
					// It is OK if nextDeltaTime becomes negative here.
				}

				// The 1 guarantees no infinite loop here. That is, if a task execute returns 0, it will be run on the
				// next update. This also deals with a possibly negative nextDeltaTime due to extreme tardiness and
				// compensation being enabled. All we can do in this case is execute as early as possible.
				if (nextTimeDelta <= 0)
					nextTimeDelta = 1;

				qn.Key = counter + nextTimeDelta;
				PriorityQueue.Insert(qn);
			}
		}
		else
		{
			runningTasks = false;
		}
	}

	UpdateTime = counter;
}
