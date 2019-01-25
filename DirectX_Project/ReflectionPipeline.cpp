#include "ReflectionPipeline.h"

ReflectionPipeline::ReflectionPipeline()
{
}

ReflectionPipeline::~ReflectionPipeline()
{
}

bool ReflectionPipeline::Initialize(ID3D12Device* dev) {
	if FAILED(CreateRootSignature(dev)) {
		return false;
	}
	if FAILED(CreatePipelineStateObject(dev)) {
		return false;
	}

	
	return true;
}


HRESULT ReflectionPipeline::CreateRootSignature(ID3D12Device* dev) {
	HRESULT hr{};




	return S_OK;
}

HRESULT ReflectionPipeline::CreatePipelineStateObject(ID3D12Device* dev) {
	HRESULT hr{};




	return S_OK;
}