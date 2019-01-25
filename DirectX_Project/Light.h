#pragma once
#include <DirectXMath.h>
using namespace DirectX;

class Light
{
public:
	Light();
	~Light();

	void SetAmbientColor(float r, float g, float b, float a);
	void SetDiffuseColor(float r, float g, float b, float a);
	void SetSpecularColor(float r, float g, float b, float a);
	void SetDirection(float x, float y, float z);
	void SetSpecularPower(float power);

	XMFLOAT4 GetAmbientColor() const { return mAmbientColor; }
	XMFLOAT4 GetDiffuseColor() const { return mDiffuseColor; }
	XMFLOAT4 GetSpecularColor() const { return mSpecularColor; }
	XMFLOAT3 GetDirection() const { return mDirection; }
	float GetSpecularPower() const { return mSpecularPower; }

private:
	XMFLOAT4 mAmbientColor;
	XMFLOAT4 mDiffuseColor;
	XMFLOAT4 mSpecularColor;
	XMFLOAT3 mDirection;
	float mSpecularPower;
};

