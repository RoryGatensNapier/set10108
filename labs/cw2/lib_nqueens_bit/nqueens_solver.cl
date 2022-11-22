__kernel void solveBoard(__global int *Board, __global int *Validity)
{
	Validity[get_global_id(0)] = get_group_id(0);
}