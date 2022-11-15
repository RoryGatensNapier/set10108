#pragma once
#include <bitset>
#include <cstdlib>
#include <vector>

namespace nq_bit
{
	class NQueen
	{
	public:
		bool testValidMove(std::bitset<8> row, std::vector<std::bitset<8>> board, int rowPos);
		void Run();
	};
}