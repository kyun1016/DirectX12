
RWTexture2D<float> gPrevImg : register(u0);
RWTexture2D<float> gCurrImg : register(u1);

cbuffer cbSettings : register(b0)
{
    float w00;
    float w01;
    float w02;
    float w03;
    float w04;
    float w10;
    float w11;
    float w12;
    float w13;
    float w14;
    float w20;
    float w21;
    float w22;
    float w23;
    float w24;
    float w30;
    float w31;
    float w32;
    float w33;
    float w34;
    float w40;
    float w41;
    float w42;
    float w43;
    float w44;
};

static const int gMaxBlurRadius = 5;

#define N 256
#define CacheSize (N + 2*gMaxBlurRadius)
groupshared float4 gCache[CacheSize];

[numthreads(N, N, 1)]
void BlurCS(int3 GTid : SV_GroupThreadID,
				int3 DTid : SV_DispatchThreadID)
{
	// This thread group runs N threads.  To get the extra 2*BlurRadius pixels, 
	// have 2*BlurRadius threads sample an extra pixel.
    if (GTid.x < gMaxBlurRadius)
    {
		// Clamp out of bound samples that occur at image borders.
        int x = max(DTid.x - gMaxBlurRadius, 0);
        gCache[GTid.x] = gPrevImg[int2(x, DTid.y)];
    }
    if (GTid.x >= N - gMaxBlurRadius)
    {
		// Clamp out of bound samples that occur at image borders.
        int x = min(DTid.x + gMaxBlurRadius, gPrevImg.Length.x - 1);
        gCache[GTid.x + 2 * gMaxBlurRadius] = gPrevImg[int2(x, DTid.y)];
    }
    
    // This thread group runs N threads.  To get the extra 2*BlurRadius pixels, 
	// have 2*BlurRadius threads sample an extra pixel.
    if (GTid.y < gMaxBlurRadius)
    {
		// Clamp out of bound samples that occur at image borders.
        int y = max(DTid.y - gMaxBlurRadius, 0);
        gCache[GTid.y] = gPrevImg[int2(y, DTid.y)];
    }
    if (GTid.x >= N - gMaxBlurRadius)
    {
		// Clamp out of bound samples that occur at image borders.
        int y = min(DTid.y + gMaxBlurRadius, gPrevImg.Length.y - 1);
        gCache[GTid.y + 2 * gMaxBlurRadius] = gPrevImg[int2(y, DTid.y)];
    }

	// Clamp out of bound samples that occur at image borders.
    gCache[GTid.xy + gMaxBlurRadius] = gPrevImg[min(DTid.xy, gPrevImg.Length.xy - 1)];

	// Wait for all threads to finish.
    GroupMemoryBarrierWithGroupSync();
	
	//
	// Now blur each pixel.
	//

    float4 blurColor = float4(0, 0, 0, 0);
	
    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        int k = GTid.x + gBlurRadius + i;
		
        blurColor += weights[i + gBlurRadius] * gCache[k];
    }
	
    gOutput[DTid.xy] = blurColor;
}

[numthreads(1, N, 1)]
void VerBlurCS(int3 GTid : SV_GroupThreadID,
				int3 DTid : SV_DispatchThreadID)
{
	// Put in an array for each indexing.
    float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };

	//
	// Fill local thread storage to reduce bandwidth.  To blur 
	// N pixels, we will need to load N + 2*BlurRadius pixels
	// due to the blur radius.
	//
	
	// This thread group runs N threads.  To get the extra 2*BlurRadius pixels, 
	// have 2*BlurRadius threads sample an extra pixel.
    if (GTid.y < gBlurRadius)
    {
		// Clamp out of bound samples that occur at image borders.
        int y = max(DTid.y - gBlurRadius, 0);
        gCache[GTid.y] = gInput[int2(DTid.x, y)];
    }
    if (GTid.y >= N - gBlurRadius)
    {
		// Clamp out of bound samples that occur at image borders.
        int y = min(DTid.y + gBlurRadius, gInput.Length.y - 1);
        gCache[GTid.y + 2 * gBlurRadius] = gInput[int2(DTid.x, y)];
    }
	
	// Clamp out of bound samples that occur at image borders.
    gCache[GTid.y + gBlurRadius] = gInput[min(DTid.xy, gInput.Length.xy - 1)];


	// Wait for all threads to finish.
    GroupMemoryBarrierWithGroupSync();
	
	//
	// Now blur each pixel.
	//

    float4 blurColor = float4(0, 0, 0, 0);
	
    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        int k = GTid.y + gBlurRadius + i;
		
        blurColor += weights[i + gBlurRadius] * gCache[k];
    }
	
    gOutput[DTid.xy] = blurColor;
}


[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DTid)
{
}