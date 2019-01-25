#pragma once

#include <Windows.h>
#include <wrl/client.h>
#include <d3d12.h>

class Object
{
public:
	Object();
	~Object();

	virtual HRESULT Initialize(ID3D12Device* dev, ID3D12Resource* res) = 0;
	virtual HRESULT Update() = 0;
	virtual HRESULT Draw(ID3D12GraphicsCommandList* cmdList) = 0;

private:
	float mass;
};

