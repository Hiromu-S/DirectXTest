#include "Light.h"

Light::Light()
{
}

Light::~Light()
{
}

void Light::SetAmbientColor(float r, float g, float b, float a) {
	mAmbientColor = XMFLOAT4(r, g, b, a);
}

void Light::SetDiffuseColor(float r, float g, float b, float a) {
	mDiffuseColor = XMFLOAT4(r, g, b, a);
}

void Light::SetSpecularColor(float r, float g, float b, float a) {
	mSpecularColor = XMFLOAT4(r, g, b, a);
}

void Light::SetDirection(float x, float y, float z) {
	mDirection = XMFLOAT3(x, y, z);
}
	
void Light::SetSpecularPower(float power) {
	mSpecularPower = power;
}
