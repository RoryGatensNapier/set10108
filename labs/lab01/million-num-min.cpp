#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <chrono>
#include <thread>
#include <fstream>

//	Thread and workload defintions
#ifndef THREADS
#define THREADS 10
#endif

#ifndef WORKLOAD
#define WORKLOAD 10000000
#endif

using namespace std;

//	Function to initialise the results vector for latter workers to solve
vector<float> initialise()
{
	//	Creating initial vector for value storage and threads
	vector<float> randomValues(WORKLOAD, 0);
	vector<thread> threads;
	threads.reserve(THREADS);

	random_device r;
	//	Create random number generator
	default_random_engine e(r());
	//	Seed with real random number if available
	uniform_real_distribution<float> distribution(1, WORKLOAD);

	// Initiating the work threads with a lambda function that assigns random values to initial results vector
	for (int i = 0; i < THREADS; ++i)
	{
		int chunkSize{ WORKLOAD / THREADS };	//	Getting the chunk size to divide to workers TODO: ensure that unevenly distributed results are accounted for
		threads.push_back(
			thread([i, chunkSize, &e, &distribution, &randomValues] { 
			auto initialVal = chunkSize * i;
			auto finalVal = (chunkSize * (i + 1)) - 1;
			for (int x = initialVal; x < finalVal; ++x)	//	Perhaps an area to include loop unrolling
			{
				randomValues[x] = distribution(e);
			}
		}));
	}
	//	Joining all worker threads until workload is complete
	for (auto& t : threads)
	{
		t.join();
	}
	return randomValues;
}

//	Function that for solves lowest value in the given vector
int SortForLowest(vector<float> work)
{
	//	Creating initial threads and remaining lowest values vectors
	vector<thread> threads;
	threads.reserve(THREADS);
	vector<float> lowestValuesFound(THREADS, 0);

	// Initialising workers with lambda solving for lowest value in their given workset
	for (int i = 0; i < 10; ++i)
	{
		auto chunkSize{ work.size() / lowestValuesFound.size() };
		threads.push_back(thread([i, chunkSize, &work, &lowestValuesFound] {
			auto initialVal = chunkSize * i;
			auto finalVal = (chunkSize * (i + 1)) - 1;
			for (int x = initialVal; x < finalVal; ++x)
			{
				if (lowestValuesFound.size() == 0)
				{
					break;
				}
				if (x == initialVal)
				{
					lowestValuesFound[i] = work[x];
				}
				else if (work[x] < lowestValuesFound[i])
				{
					lowestValuesFound[i] = work[x];
				}
			}
		}));
	}
	// Joining all threads until entire workload has been completed
	for (auto& t : threads)
	{
		t.join();
	}
	// intialising final result
	float lowest{ 0 };

	// Final main thread sort out of thread scoring vector
	for (int i = 0; i < lowestValuesFound.size(); ++i)
	{
		if (i > 0 && lowestValuesFound[i] < lowest)
		{
			lowest = lowestValuesFound[i];
		}
		else
		{
			lowest = lowestValuesFound[i];
		}
	}
	cout << lowest << endl;	// Print to console
	return lowest;
}

int main(int argc, char** argv)
{
	// Initialising data for sort
	auto work_set{ initialise() };
	
	// Sort function that prints result to console
	SortForLowest(work_set);

	return 0;
}
