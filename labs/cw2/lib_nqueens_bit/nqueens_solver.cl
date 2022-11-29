__kernel void solveBoard(__global int *BoardData, __global int *Validity, __global int *Queens, __local int *Board)
{
	int i_global_idx = get_global_id(0);
	float f_global_idx = convert_float(get_global_id(0));

	int i_local_idx = get_local_id(0);
	float f_local_idx = convert_float(get_local_id(0));

	Board[i_local_idx] = BoardData[i_global_idx];

	float boardVal = convert_float(Board[i_local_idx]);
	int leftTest = rotate(Board[i_local_idx], 1);
	//printf("Global Idx = %d / Local Idx = %d / Board Value = %d\r\n", i_global_idx, i_local_idx, Board[i_local_idx]);
	int leftTotal = 0;
	int rightTotal = 0;
	for (int i = 0; i < *Queens; ++i)
	{
		float testBoardVal = convert_float(Board[i]);
		float diff = fabs(convert_float(i_local_idx - i));
		float leftShift = convert_float(Board[i_local_idx]) + exp2(diff);
		float rightShift = convert_float(Board[i_local_idx]) - exp2(diff);
		leftTotal += isequal(testBoardVal, leftShift);
		rightTotal += isequal(testBoardVal, rightShift);
	}
	printf("Global ID: %d/Local ID: %d/Left Test: %d/Right Test: %d\r\n", i_global_idx, i_local_idx, leftTotal, rightTotal);
}