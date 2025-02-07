#include "pch.h"
#include "Waves.h"
#include <ppl.h>

Waves::Waves(int row, int col, float dx, float dt, float speed, float damping)
	: mNumRows(row)
	, mNumCols(col)
	, mVertexCount(row* col)
	, mTriangleCount((row - 1)* (col - 1) * 2)
	, mSpatialStep(dx)
	, mTimeStep(dt)
	, mPrevSolution(mVertexCount)
	, mCurrSolution(mVertexCount)
	, mNormals(mVertexCount, DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f))
	, mTangentX(mVertexCount, DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f))
{
	float d = damping * dt + 2.0f;
	float e = (speed * speed) * (dt * dt) / (dx * dx);
	mK1 = (damping * dt - 2.0f) / d;
	mK2 = (4.0f - 8.0f * e) / d;
	mK3 = (2.0f * e) / d;

	float halfWidth = (mNumCols - 1) * dx * 0.5f;
	float halfDepth = (mNumRows - 1) * dx * 0.5f;

	for (int i = 0; i < mNumRows; ++i)
	{
		float z = halfDepth - i * dx;
		for (int j = 0; j < mNumCols; ++j)
		{
			float x = -halfWidth + j * dx;
			mPrevSolution[i * mNumCols + j] = DirectX::XMFLOAT3(x, 0.0f, z);
			mCurrSolution[i * mNumCols + j] = DirectX::XMFLOAT3(x, 0.0f, z);
			// mNormals     [i * mNumCols + j] = DirectX::XMFLOAT3(0.0f, 1.0f, 0.0f);
			// mTangentX    [i * mNumCols + j] = DirectX::XMFLOAT3(1.0f, 0.0f, 0.0f);
		}
	}
}

void Waves::Update(const float dt)
{
	static float t = 0;

	// Accumulate time.
	t += dt;

	if (t >= mTimeStep)
	{
		t = 0.0f; // reset time
		concurrency::parallel_for(1, mNumRows - 1, [this](int i)
		//for(int i = 1; i < mNumRows - 1; ++i)
		{
			for (int j = 1; j < mNumCols - 1; ++j)
			{
				const int c = i * mNumCols + j;
				const int l = c - 1;
				const int r = c + 1;
				const int t = c - mNumCols;
				const int b = c + mNumCols;

				mPrevSolution[c].y =
					mK1 * mPrevSolution[c].y +
					mK2 * mCurrSolution[c].y +
					mK3 * (
						mCurrSolution[l].y +
						mCurrSolution[r].y +
						mCurrSolution[t].y +
						mCurrSolution[b].y);
			}
		});

		std::swap(mPrevSolution, mCurrSolution);

		concurrency::parallel_for(1, mNumRows - 1, [this](int i)
		//for(int i = 1; i < mNumRows - 1; ++i)
		{
			for (int j = 1; j < mNumCols - 1; ++j)
			{
				const int c = i * mNumCols + j;

				float l = mCurrSolution[c - 1].y;
				float r = mCurrSolution[c + 1].y;
				float t = mCurrSolution[c - mNumCols].y;
				float b = mCurrSolution[c + mNumCols].y;

				mNormals[c].x = -r + l;
				mNormals[c].y = 2.0f * mSpatialStep;
				mNormals[c].z = b - t;

				DirectX::XMVECTOR n = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&mNormals[c]));
				DirectX::XMStoreFloat3(&mNormals[c], n);

				mTangentX[c] = DirectX::XMFLOAT3(2.0f * mSpatialStep, r - l, 0.0f);
				DirectX::XMVECTOR T = DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&mTangentX[c]));
				DirectX::XMStoreFloat3(&mTangentX[c], T);
			}
		});
	}
}

void Waves::Disturb(const int i, const int j, const float magnitude)
{
	// Don't disturb boundaries.
	assert(i > 1 && i < mNumRows - 2);
	assert(j > 1 && j < mNumCols - 2);

	float halfMag = 0.5f * magnitude;

	const int c = i * mNumCols + j;

	// Disturb the ijth vertex height and its neighbors.
	mCurrSolution[c].y += magnitude;
	mCurrSolution[c + 1].y += halfMag;
	mCurrSolution[c - 1].y += halfMag;
	mCurrSolution[c + mNumCols].y += halfMag;
	mCurrSolution[c - mNumCols].y += halfMag;

	//mCurrSolution[i * mNumCols + j].y += magnitude;
	//mCurrSolution[i * mNumCols + j + 1].y += halfMag;
	//mCurrSolution[i * mNumCols + j - 1].y += halfMag;
	//mCurrSolution[(i + 1) * mNumCols + j].y += halfMag;
	//mCurrSolution[(i - 1) * mNumCols + j].y += halfMag;
}