#define __CL_ENABLE_EXCEPTIONS

#include <algorithm>
#include <chrono>
#include <CL/cl.hpp>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <omp.h>
#include <thread>
#include "nqueens_bit.h"
#include <fstream>

#define OPENCL_DEBUG 0

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

void runOMP(std::vector<int>& h_Boards, int NumberOfQueens)
{
	int threadCount = omp_get_max_threads();
	int counter{ 0 };

#pragma omp parallel for num_threads(threadCount) schedule(runtime) shared(counter)
	for (int i{0}; i < h_Boards.size() / NumberOfQueens; ++i)
	{
		int idx = i;
		int tests = 0;
		std::vector<int> workBoard(NumberOfQueens);
		for (int row{ 0 }; row < NumberOfQueens; ++row)
		{
			int cell = (idx * NumberOfQueens) + row;
			workBoard[row] = h_Boards[cell];
		}
		for (int row{ 0 }; row < NumberOfQueens; ++row)
		{
			int testVal = workBoard[row];
			for (int testRow{ 0 }; testRow < NumberOfQueens; ++testRow)
			{
				int Shift = abs(row - testRow);
				bool testRShift = (workBoard[row] >> Shift) != workBoard[testRow];
				bool testLShift = (workBoard[row] << Shift) != workBoard[testRow];
				tests += (testLShift && testRShift);
			}
		}
		if (tests == NumberOfQueens * (NumberOfQueens - 1))
		{
#pragma omp critical
			++counter;
		}
	}
	//std::cout << "\r\nNumber of solutions for " << NumberOfQueens << " is " << counter << std::endl;
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

#if OPENCL_DEBUG
		std::cout << "Found GPU: " << cl_Devices[0].getInfo<CL_DEVICE_NAME>() << std::endl;
		std::cout << "OpenGL Version: " << cl_Devices[0].getInfo<CL_DEVICE_VERSION>() << std::endl;
		std::cout << "Max Work Group Size: " << cl_Devices[0].getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << std::endl;
		std::cout << "Max Global Memory size: " << cl_Devices[0].getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() << std::endl;
		std::cout << "Max Global Work Item size: " << cl_Devices[0].getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>()[0] << ", " << cl_Devices[0].getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>()[1] << ", " << cl_Devices[0].getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>()[2] << std::endl;
#endif
		cl::Context cl_Context(cl_Devices);

		cl::CommandQueue cl_Queue(cl_Context, cl_Devices[0]);

		cl::Buffer d_intVecBuf_Boards(cl_Context, CL_MEM_READ_ONLY, d_Boards_DataSize);
		cl::Buffer d_intVec_Out(cl_Context, CL_MEM_WRITE_ONLY, d_Tests_DataSize);
		cl::Buffer d_int_NumOfQueens(cl_Context, CL_MEM_READ_ONLY, sizeof(int));

		cl_Queue.enqueueWriteBuffer(d_intVecBuf_Boards, CL_TRUE, 0, d_Boards_DataSize, h_Boards.data());
		cl_Queue.enqueueWriteBuffer(d_int_NumOfQueens, CL_TRUE, 0, sizeof(int), &NumberOfQueens);

		std::ifstream cl_File("nqueens_solver.cl");
		std::string cl_Kernel_Code(std::istreambuf_iterator<char>(cl_File), (std::istreambuf_iterator<char>()));

		cl::Program::Sources cl_Source(1, std::make_pair(cl_Kernel_Code.c_str(), cl_Kernel_Code.length() + 1));
		cl::Program cl_Program(cl_Context, cl_Source);

		cl_Program.build(cl_Devices);

		cl::Kernel kernel_nQueens(cl_Program, "solveBoard");
#if OPENCL_DEBUG
		std::cout << "Preferred Kernel work group size: " << kernel_nQueens.getWorkGroupInfo<CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE>(cl_Devices[0]) << std::endl;
#endif // OPENCL_DEBUG

		kernel_nQueens.setArg(0, d_intVecBuf_Boards);
		kernel_nQueens.setArg(1, d_intVec_Out);
		kernel_nQueens.setArg(2, d_int_NumOfQueens);
		kernel_nQueens.setArg(3, sizeof(int) * NumberOfQueens, nullptr);

		cl::NDRange global(h_Boards.size());
		cl::NDRange local(NumberOfQueens);

		cl_Queue.enqueueNDRangeKernel(kernel_nQueens, cl::NullRange, global, local);

		cl_Queue.enqueueReadBuffer(d_intVec_Out, CL_TRUE, 0, d_Tests_DataSize, out.data());

		int counter = 0;
		for (auto a : out)
		{
			if (a == NumberOfQueens * (NumberOfQueens - 1))
			{
				++counter;
			}
		}
		///std::cout << "\r\nNumber of solutions for " << NumberOfQueens << " is " << counter << std::endl;
	}
	catch (cl::Error error)
	{
		std::cout << "OpenCL ERROR: " << error.what() << "(" << error.err() << ")" << std::endl;
	}
}

void nqueen_solver::Run(int Queens, int Runs)
{
	std::vector<int> Boards(0);
	Boards = generateBoards(Queens);

	std::ofstream resultsFile;

	std::string file = "G:/NapierWork/4th Year/Concurrent and Parallel Systems/CW2_Testing/Parallel/";
	std::string fileName = "Parallel-Queens";
	fileName.append(std::to_string(Queens));
	fileName.append("-Runs");
	fileName.append(std::to_string(Runs));
	fileName.append(".csv");
	file.append(fileName);
	resultsFile.open(file);
	resultsFile << "--OpenCL Results-- | Queen count =" << Queens << std::endl;

	std::chrono::high_resolution_clock t;
	std::chrono::steady_clock::time_point timePreKernel;
	std::chrono::steady_clock::time_point timePostKernel;
	long long tTime;

	for (int x{0}; x < Runs; ++x)
	{
		timePreKernel = t.now();
		createAndRunKernel_CL(Boards, Queens);
		timePostKernel = t.now();
		tTime = std::chrono::duration_cast<std::chrono::milliseconds>(timePostKernel - timePreKernel).count();
		resultsFile << tTime << ",";
	}
	resultsFile << "\r\n--OpenMP implementation-- | Queen count =" << Queens << std::endl;

	for (int x{ 0 }; x < Runs; ++x)
	{
		timePreKernel = t.now();
		runOMP(Boards, Queens);
		timePostKernel = t.now();
		tTime = std::chrono::duration_cast<std::chrono::milliseconds>(timePostKernel - timePreKernel).count();
		resultsFile << tTime << ",";
	}
	resultsFile << "\r\n";
	resultsFile.close();
}