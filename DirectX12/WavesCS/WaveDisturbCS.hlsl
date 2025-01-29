#include "WaveCommon.hlsli"

[numthreads(1, 1, 1)]
void main(int3 groupThreadID : SV_GroupThreadID,
                    int3 dispatchThreadID : SV_DispatchThreadID)
{
	// We do not need to do bounds checking because:
	//	 *out-of-bounds reads return 0, which works for us--it just means the boundary of 
	//    our water simulation is clamped to 0 in local space.
	//   *out-of-bounds writes are a no-op.
	
    int x = gDisturbIndex.x;
    int y = gDisturbIndex.y;

    float halfMag = 0.5f * gDisturbMag;

	// Buffer is RW so operator += is well defined.
    gOutput[int2(x, y)] += gDisturbMag;
    gOutput[int2(x + 1, y)] += halfMag;
    gOutput[int2(x - 1, y)] += halfMag;
    gOutput[int2(x, y + 1)] += halfMag;
    gOutput[int2(x, y - 1)] += halfMag;
}