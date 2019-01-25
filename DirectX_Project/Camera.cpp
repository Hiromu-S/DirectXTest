#include "Camera.h"

using namespace DirectX;

Camera::Camera() {
	mPositionX = 0.0f;
	mPositionY = 0.0f;
	mPositionZ = 0.0f;

	mRotationX = 0.0f;
	mRotationY = 0.0f;
	mRotationZ = 0.0f;
}

Camera::~Camera() {

}

void Camera::SetPosition(float x, float y, float z) {
	mPositionX = x;
	mPositionY = y;
	mPositionZ = z;
}

void Camera::SetRotation(float x, float y, float z) {
	mRotationX = x;
	mRotationY = y;
	mRotationZ = z;
}

void Camera::Render() {
	XMFLOAT3 up;
	XMFLOAT3 position;
	XMFLOAT3 lookAt;
	float xr, yr, zr, a = 5.0f;

	//Upper
	up.x = 0.0f;
	up.y = 1.0f;
	up.z = 0.0f;

	position.x = mPositionX;
	position.y = mPositionY;
	position.z = mPositionZ;

	xr = mRotationX * 0.0174532925f;
	yr = mRotationY * 0.0174532925f;
	zr = mRotationZ * 0.0174532925f;

	lookAt.x = mPositionX + a * sinf(yr) + a * sinf(zr);
	lookAt.y = mPositionY + a * cosf(zr) + a * cosf(xr);
	lookAt.z = mPositionZ + a * cosf(yr) + a * sinf(xr);

	mViewMatrix = XMMatrixLookAtRH({ position.x, position.y, position.z }, { lookAt.x, lookAt.y, lookAt.z }, { up.x, up.y, up.z });
}
	
void Camera::Rotate(float x, float y, float z) {
	mViewMatrix = XMMatrixRotationX(XMConvertToRadians(x));
	mViewMatrix = XMMatrixRotationY(XMConvertToRadians(y));
	mViewMatrix = XMMatrixRotationZ(XMConvertToRadians(z));
}

void Camera::GetViewMatrix(XMMATRIX& viewMatrix) {
	viewMatrix = mViewMatrix;
}

void Camera::Render2Reflection(float height) {
	XMFLOAT3 up, position, lookAt;
	float radians;

	//Upper
	up.x = 0.0f;
	up.y = 1.0f;
	up.z = 0.0f;

	position.x = mPositionX;
	position.y = -mPositionY + (height * 2.0f);
	position.z = mPositionZ;

	radians = mRotationY * 0.0174532925f;

	lookAt.x = sinf(radians) + mPositionX;
	lookAt.y = position.y;
	lookAt.z = cosf(radians) + mPositionZ;

	mReflectionViewMatrix = XMMatrixLookAtLH({ position.x, position.y, position.z }, { lookAt.x, lookAt.y, lookAt.z }, { up.x, up.y, up.z });

	return;
}

XMMATRIX Camera::GetReflectionViewMatrix() {
	return mReflectionViewMatrix;
}