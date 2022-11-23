__kernel void solveBoard(__global int *BoardData, __global int *Validity, __global int *Queens, __local int *Board)
{
	for (int i = 0; i < *Queens; ++i)
	{
		Board[i] = BoardData[get_global_id(0)];
	}
	int k = *Queens - 1;
	Validity[get_global_id(0)] = Board[k];
}