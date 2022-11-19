#pragma once
#include <bitset>
#include <cstdlib>
#include <vector>

namespace nq_bit
{
	bool testValidMove(std::bitset<8> row, std::vector<std::bitset<8>> board, int rowPos);
	bool TestValidMove64_ver2(std::bitset<8> row, std::bitset<64> board, int, int);
	bool testValidMove64(std::bitset<8>, std::bitset<64>, int);
	void Run();
	void Run_v2(int queens);
}