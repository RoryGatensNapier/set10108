#include <chrono>
#include <math.h>
#include <omp.h>
#include <queue>
#include <iostream>
#include "nqueens_bit.h"

bool nq_bit::testValidMove(std::bitset<8> row, std::vector<std::bitset<8>> board, int rowPos)
{
	std::bitset<8> toTest = row;
	std::vector<std::bitset<8>> gameboard = board;
	int linesCleared{ 0 };
	for (int i{ 1 }; i <= rowPos; ++i)
	{
		if (row.to_ulong() != (board[rowPos - i] << i).to_ulong() && 
			row.to_ulong() != board[rowPos - i].to_ulong() && 
			row.to_ulong() != (board[rowPos - i] >> i).to_ulong())
		{
			++linesCleared;
		}
	}
	
	return linesCleared == rowPos;
}

bool nq_bit::testValidMove64(std::bitset<8> row, std::bitset<64> board, int blockIdx)
{
	std::bitset<8> test = row;
	std::bitset<64> out = board;
	std::bitset<8> extracted(0);
	
	int lineCleared{ 0 };
	for (int j{0}; j < blockIdx; ++j)
	{
		extracted = 0;
		for (int i{ 0 }; i < 8; ++i)
		{
			auto test = i + (8 * j);
			extracted[i] = out.test(test);
		}
		if (row.to_ulong() != (extracted << j+1).to_ulong() &&
			row.to_ulong() != (extracted >> j+1).to_ulong() &&
			row.to_ulong() != extracted.to_ulong())
		{
			++lineCleared;
		}
	}
	return (lineCleared == blockIdx);
}

bool nq_bit::TestValidMove64_ver2(std::bitset<8> row, std::bitset<64> board, int rowToTestFor, int rowToTestAgainst)
{
	std::bitset<8> test = row;
	std::bitset<64> out = board;
	std::bitset<8> extracted(0);
	for (int i{ 0 }; i < 8; ++i)
	{
		auto test = i + (8 * rowToTestAgainst);
		extracted[i] = out.test(test);
	}
	bool leftDiagonalSafe = row.to_ulong() != (extracted << (rowToTestFor - rowToTestAgainst)).to_ulong();
	bool rightDiagonalSafe = row.to_ulong() != (extracted >> (rowToTestFor - rowToTestAgainst)).to_ulong();
	bool columnSafe = row.to_ulong() != extracted.to_ulong();

	return leftDiagonalSafe && rightDiagonalSafe && columnSafe;
}

bool TestValidMove64_ver3(std::bitset<8> rowToTest, std::bitset<8> rowToCheckAgainst, int posToTest, int posToCheckAgainst)
{
	bool leftDiagonalSafe = rowToTest.to_ulong() != (rowToCheckAgainst << (posToTest - posToCheckAgainst)).to_ulong();
	bool rightDiagonalSafe = rowToTest.to_ulong() != (rowToCheckAgainst >> (posToTest - posToCheckAgainst)).to_ulong();
	bool columnSafe = rowToTest.to_ulong() != rowToCheckAgainst.to_ulong();

	return leftDiagonalSafe && rightDiagonalSafe && columnSafe;
}

double CalculateBoardPermutation(int queens)
{
	int boardSize{ queens * queens };
	double combinations{ 1 };
	double boardMinusQueensFac{ 1 };
	double qFac{ 1 };
	double permutations{ 1 };
	for (int f{ boardSize }; f > 0; --f)
	{
		combinations *= f;
	}
	for (int h{ boardSize - queens }; h > 0; --h)
	{
		boardMinusQueensFac *= h;
	}
	for (int h{ queens }; h > 0; --h)
	{
		qFac *= h;
	}
	permutations = combinations / (boardMinusQueensFac * qFac);
	return permutations;
}

bool BitsetCompare(std::bitset<8> bit1, std::bitset<8> bit2)
{
	return bit1.to_ullong() < bit2.to_ullong();
}

void nq_bit::Run_v2(int Queens)
{
	/*double queensFactorial{ 1 };
	for (int i{ Queens }; i > 0; --i)
	{
		queensFactorial *= i;
	}*/

	std::vector<std::bitset<8>> board(8, 0);
	std::vector<std::vector<std::bitset<8>>> queue;
	std::vector<std::vector<std::bitset<8>>> solutions;
	for (int i{ 0 }; i < Queens; ++i)
	{
		board[i].set(i);
	}

	queue.resize(queue.size() + 1);
	queue.back() = board;
	while (std::next_permutation(board.begin(), board.end(), BitsetCompare))
	{
		queue.resize(queue.size() + 1);
		queue.back() = board;
	}
	board.~vector();
	int threadCount = omp_get_max_threads();
	auto time = std::chrono::high_resolution_clock::now();
#pragma omp parallel for num_threads(threadCount) schedule(dynamic)
	for (int i{0}; i < queue.size(); ++i)
	{
		std::vector<std::bitset<8>> testBoard;
		testBoard = queue[i];
		std::bitset<8> innerTests(0);
		std::bitset<8> rowsPassed(0);
		for (int boardProgress{ 0 }; boardProgress < Queens; ++boardProgress)
		{
			for (int lookback{ 0 }; lookback < boardProgress; ++lookback)
			{
				innerTests[lookback] = TestValidMove64_ver3(testBoard[boardProgress], testBoard[lookback], boardProgress, lookback);
			}
			rowsPassed[boardProgress] = innerTests.to_ullong() == pow(2, boardProgress)-1;
		}
		if (rowsPassed.to_ullong() == pow(2, Queens) - 1)
		{
#pragma omp critical
			{
				solutions.resize(solutions.size() + 1);
				solutions.back() = testBoard;
			}
		}
	}
	auto time2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - time);
	queue.~vector();
	/*for (auto s : solutions)
	{
		for (auto i : s)
		{
			std::cout << i.to_string().c_str() << std::endl;
		}
		std::cout << std::endl;
	}*/


	//auto permutations = CalculateBoardPermutation(Queens);
	//std::vector<bool> board(Queens * Queens);
	//std::queue<std::vector<bool>> queue;
	//for (int i{ 0 }; i < Queens; ++i)
	//{
	//	board[i * Queens] = true;
	//}
	//queue.push(board);
	//for (int i{0}; i < permutations; ++i)
	//{		
	//	std::next_permutation(board.begin(), board.end());
	//	//queue.push(board);
	//}
	//queue = queue;
}

void nq_bit::Run()
{
	std::queue<std::pair<std::bitset<64>, int>> queue;

#pragma omp parallel num_threads(8) shared(queue)
	{
		int rowToCheck{ 1 };
		std::bitset<64> board(0);
		bool boardSet{ true };
		board.set(omp_get_thread_num());
		std::bitset<8> localTests(1);

		while (rowToCheck <= 6)
		{
			std::vector<std::pair<std::bitset<64>, int>> _q(0);
			localTests = 1;

			for (int i{ 0 }; i < 7; ++i)
			{
				if (testValidMove64(localTests, board, rowToCheck) && log2(localTests.to_ulong()) + (8 * rowToCheck) < 64)
				{
					auto output = board;
					auto one = log2(localTests.to_ulong());
					auto two = 8 * rowToCheck;
					auto three = one + two;
					output.set(three);
					_q.resize(_q.size() + 1);
					_q.back().first = output;
					_q.back().second = rowToCheck + 1;
				}
				localTests <<= 1;
			}

#pragma omp critical
			for (auto s : _q)
			{
				queue.push(s);
			}
			
#pragma omp critical
			if (!queue.empty())
			{
				board = queue.front().first;
				rowToCheck = queue.front().second;
				queue.pop();
			}
		}
	}
}