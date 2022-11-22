#define __CL_ENABLE_EXCEPTIONS

#include <algorithm>
#include <CL/cl.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <omp.h>
#include <thread>
#include "nqueens_bit.h"
#include <fstream>

std::vector<int> generateBoards(int NumberOfQueens)
{
	std::vector<int> start(NumberOfQueens);
	for (int i{ 0 }; i < NumberOfQueens; ++i)
	{
		start[i] = pow(2, i);
	}
	std::vector<int> out;
	for (auto s : start)
	{
		out.resize(out.size() + 1);
		out.back() = s;
	}
	while (std::next_permutation(start.begin(), start.end()))
	{
		for (auto s : start)
		{
			out.resize(out.size() + 1);
			out.back() = s;
		}
	}
	return out;
}

void createAndRunKernel_CL(std::vector<int> &h_Boards, int NumberOfQueens)
{
	unsigned long long d_Boards_DataSize = sizeof(int) * h_Boards.size();
	std::vector<int> out(h_Boards.size() / NumberOfQueens, 0);
	unsigned long long d_Tests_DataSize = sizeof(int) * out.size();
	try
	{
		std::vector<cl::Platform> cl_Platforms;
		cl::Platform::get(&cl_Platforms);

		std::vector<cl::Device> cl_Devices;
		cl_Platforms[0].getDevices(CL_DEVICE_TYPE_GPU, &cl_Devices);

		std::cout << "Found GPU: " << cl_Devices[0].getInfo<CL_DEVICE_NAME>() << std::endl;
		std::cout << "Max Work Group Size: " << cl_Devices[0].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << std::endl;

		cl::Context cl_Context(cl_Devices);

		cl::CommandQueue cl_Queue(cl_Context, cl_Devices[0]);

		cl::Buffer d_intVecBuf_Boards(cl_Context, CL_MEM_READ_ONLY, d_Boards_DataSize);
		cl::Buffer d_intVec_Out(cl_Context, CL_MEM_WRITE_ONLY, d_Tests_DataSize);

		cl_Queue.enqueueWriteBuffer(d_intVecBuf_Boards, CL_TRUE, 0, d_Boards_DataSize, h_Boards.data());

		std::ifstream cl_File("nqueens_solver.cl");
		std::string cl_Kernel_Code(std::istreambuf_iterator<char>(cl_File), (std::istreambuf_iterator<char>()));

		cl::Program::Sources cl_Source(1, std::make_pair(cl_Kernel_Code.c_str(), cl_Kernel_Code.length() + 1));
		cl::Program cl_Program(cl_Context, cl_Source);

		cl_Program.build(cl_Devices);

		cl::Kernel kernel_nQueens(cl_Program, "solveBoard");

		kernel_nQueens.setArg(0, d_intVecBuf_Boards);
		kernel_nQueens.setArg(1, d_intVec_Out);

		cl::NDRange global(out.size());

		cl_Queue.enqueueNDRangeKernel(kernel_nQueens, cl::NullRange, global);

		cl_Queue.enqueueReadBuffer(d_intVec_Out, CL_TRUE, 0, d_Tests_DataSize, out.data());

		std::sort(out.begin(), out.end());

		for (auto a : out)
		{
			std::cout << a << std::endl;
		}
	}
	catch (cl::Error error)
	{
		std::cout << "OpenCL ERROR: " << error.what() << "(" << error.err() << ")" << std::endl;
	}
}

void nqueen_solver::Run(int Queens)
{
	std::vector<int> Boards(0);
	Boards = generateBoards(Queens);
	//std::unique_ptr<std::thread> genBoards(std::make_unique<std::thread>([&] { Boards = generateBoards(Queens); }));
	//genBoards->join();

	createAndRunKernel_CL(Boards, Queens);

	std::cout << "beep" << std::endl;
}