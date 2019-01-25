#pragma once 

#include <DirectXMath.h>

class Camera {
public:
	Camera();
	~Camera();

	void SetPosition(float x, float y, float z);
	void SetRotation(float x, float y, float z);

	void Rotate(float x, float y, float z);

	void Render();
	void GetViewMatrix(DirectX::XMMATRIX& view);

	void Render2Reflection(float);
	DirectX::XMMATRIX GetReflectionViewMatrix();

private:
	float mPositionX, mPositionY, mPositionZ;
	float mRotationX, mRotationY, mRotationZ;
	DirectX::XMMATRIX mViewMatrix, mReflectionViewMatrix;
};