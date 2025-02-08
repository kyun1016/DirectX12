#pragma once

#include<vector>
#include<DirectXMath.h>

class Waves
{
public:
	Waves(int row, int col, float dx, float dt, float speed, float damping);
	Waves(const Waves& rhs) = delete;
	Waves& operator=(const Waves& rhs) = delete;
	~Waves() {}

	int   RowCount()      const	{ return mNumRows; }
	int   ColumnCount()   const	{ return mNumCols; }
	int   VertexCount()   const	{ return mVertexCount; }
	int   TriangleCount() const	{ return mTriangleCount; }
	float Width()         const	{ return mNumCols * mSpatialStep; }
	float Depth()         const	{ return mNumRows * mSpatialStep; }

	const DirectX::XMFLOAT3& Position(int i)const { return mCurrSolution[i]; }
	const DirectX::XMFLOAT3& Normal  (int i)const { return mNormals[i]; }
	const DirectX::XMFLOAT3& TangentX(int i)const { return mTangentX[i]; }

	void Update(const float dt);
	void Disturb(const int i, const int j, const float magnitude);

private:
	int mNumRows;
	int mNumCols;
	int mVertexCount;
	int mTriangleCount;

	float mSpatialStep;
	float mTimeStep;

	float mK1 = 0.0f;
	float mK2 = 0.0f;
	float mK3 = 0.0f;

	std::vector<DirectX::XMFLOAT3> mPrevSolution;
	std::vector<DirectX::XMFLOAT3> mCurrSolution;
	std::vector<DirectX::XMFLOAT3> mNormals;
	std::vector<DirectX::XMFLOAT3> mTangentX;
};

