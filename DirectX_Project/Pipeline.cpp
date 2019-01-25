#include "Pipeline.h"

Pipeline::Pipeline()
{
}


Pipeline::~Pipeline()
{
}

void Pipeline::RenderShader(ID3D12GraphicsCommandList* cmdList, int drawIndexCount) {
	cmdList->DrawInstanced(drawIndexCount, 1, 0, 0);
}