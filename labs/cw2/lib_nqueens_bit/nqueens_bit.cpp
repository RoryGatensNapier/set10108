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

void nq_bit::Run_v2()
{
	std::queue<std::bitset<64>> queue;

#pragma omp parallel num_threads(8) shared(queue)
	{
		std::bitset<64> board(0);
		board.set(omp_get_thread_num());
		std::bitset<8> workingRow(1);
		int curRow{ 0 };

#pragma omp critical
		queue.push(board);

#pragma omp barrier

		while (!queue.empty())
		{
			if (workingRow.to_ulong() == 128)
			{
#pragma omp critical
				{
					board = queue.front();
					queue.pop();
					workingRow = 1;
					++curRow;
				}
			}

			bool solutionCheck{ true };

#pragma omp for
			for (int i{ 0 }; i < curRow; ++i)
			{
				solutionCheck = solutionCheck && TestValidMove64_ver2(workingRow, board, curRow, i);
			}

			if (solutionCheck)
			{
				auto outBoard = board;
				outBoard.set(log2(workingRow.to_ulong()) + (8 * curRow));
#pragma omp critical
				{
					queue.push(outBoard);
				}
			}
			workingRow <<= 1;
		}
	}
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

		while (boardSet)
		{
			std::vector<std::pair<std::bitset<64>, int>> _q(0);
			localTests = 1;

//#pragma omp for
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

			if (rowToCheck > 7)
			{
				boardSet = false;
			}

			std::cout << queue.size() << std::endl;
		}
	}

	queue = queue;
}