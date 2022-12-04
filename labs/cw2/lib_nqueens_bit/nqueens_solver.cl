__kernel void solveBoard(__global int *BoardData, __global int *Validity, __global int *Queens, __local int *Board)
{
	int q = *Queens;	//	load queens count
	int WS_idx = get_global_id(0);	//	get solution index
	int board_ID = (get_global_id(0) - get_local_id(0)) / q;
	int WG_idx = get_local_id(0);	//	get solution index
	__local atomic_int wgTests;
	if (WG_idx == 0)
	{
		atomic_init(&wgTests, 0);
	}
	Board[WG_idx] = BoardData[WS_idx];
	work_group_barrier(CLK_LOCAL_MEM_FENCE);	//	Ensure board has been completed before work group starts comparing against the other rows
	float boardtestLeft = 0;	//	preload a local variable for testing left shift
	float boardtestRight = 0;	//	preload a local variable for testing right shift
	float rowValue = convert_float(Board[WG_idx]);	//	this isn't actually needed
	int totalTests = 0;	//	Total tests conducted
	for (int m = 0; m < q; ++m)
	{
		int shiftAmount = abs_diff(WG_idx, m);
		int shiftedLeft = Board[m] << shiftAmount;
		int shiftedRight = Board[m] >> shiftAmount;
		boardtestLeft = convert_float(shiftedLeft);
		boardtestRight = convert_float(shiftedRight);
		int isnEq_L = isnotequal(rowValue, boardtestLeft);
		int isnEq_R = isnotequal(rowValue, boardtestRight);
		int isnEq = isnEq_L & isnEq_R;
		totalTests += isnEq;
	}
	atomic_fetch_add(&wgTests, totalTests);
	if (WG_idx == 0)
	{
		Validity[board_ID] = atomic_load(&wgTests);
	}
}