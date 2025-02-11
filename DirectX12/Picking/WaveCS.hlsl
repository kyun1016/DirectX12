cbuffer cbUpdateSettings
{
    float gWaveConstant0;
    float gWaveConstant1;
    float gWaveConstant2;
	
    float gDisturbMag;
    int2 gDisturbIndex;
};

RWTexture2D<float> gPrevSolInput : register(u0);
RWTexture2D<float> gCurrSolInput : register(u1);
RWTexture2D<float> gOutput : register(u2);

[numthreads(1, 1, 1)]
void WaveDisturbCS(int3 groupThreadID : SV_GroupThreadID,
                    int3 dispatchThreadID : SV_DispatchThreadID)
{
	// We do not need to do BoundingBox checking because:
	//	 *out-of-BoundingBox reads return 0, which works for us--it just means the boundary of 
	//    our water simulation is clamped to 0 in local space.
	//   *out-of-BoundingBox writes are a no-op.
	
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

[numthreads(16, 16, 1)]
void WaveUpdateCS(int3 dispatchThreadID : SV_DispatchThreadID)
{
	// We do not need to do BoundingBox checking because:
	//	 *out-of-BoundingBox reads return 0, which works for us--it just means the boundary of 
	//    our water simulation is clamped to 0 in local space.
	//   *out-of-BoundingBox writes are a no-op.
	
    int x = dispatchThreadID.x;
    int y = dispatchThreadID.y;

    gOutput[int2(x, y)] =
		gWaveConstant0 * gPrevSolInput[int2(x, y)].r +
		gWaveConstant1 * gCurrSolInput[int2(x, y)].r +
		gWaveConstant2 * (
			gCurrSolInput[int2(x, y + 1)].r +
			gCurrSolInput[int2(x, y - 1)].r +
			gCurrSolInput[int2(x + 1, y)].r +
			gCurrSolInput[int2(x - 1, y)].r);
}