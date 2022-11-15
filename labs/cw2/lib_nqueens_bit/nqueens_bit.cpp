#include <omp.h>
#include "nqueens_bit.h"

bool nq_bit::NQueen::testValidMove(std::bitset<8> row, std::vector<std::bitset<8>> board, int rowPos)
{
	std::bitset<8> toTest = row;
	std::vector<std::bitset<8>> gameboard = board;
	for (int i{ 1 }; i == rowPos; ++i)
	{
		if (row.to_ulong() != (board[rowPos - i] << i).to_ulong() && 
			row.to_ulong() != board[rowPos - i].to_ulong() && 
			row.to_ulong() != (board[rowPos - i] >> i).to_ulong())
		{
			return true;
		}
		return false;
	}
}

void nq_bit::NQueen::Run()
{
	std::bitset<8> row(0);
	std::vector<std::bitset<8>> board(8, row);

#pragma omp parallel num_threads(4)
	for (int i{ 0 }; i < 8; ++i)
	{
		board[0].set(0);
		board[0] <<= omp_get_thread_num();

	}
}