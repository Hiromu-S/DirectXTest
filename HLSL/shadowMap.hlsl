
Texture2D<float4> tex : register(t0);
Texture2D<float> shadowMapTex : register(t1);
SamplerState shaderTexture : register(s0);
SamplerState shadowMapTexture : register(s1);

cbuffer MatrixBuffer : register(b0) {
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projectionMatrix;
};

cbuffer LightBuffer : register(b1) {
	float4x4 lightVP;
	float4 ambientColor;
	float4 diffuseColor;
	float3 lightDirection;
};

struct VS_INPUT {
	float4 position : POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
};

struct PS_INPUT {
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float4 positionSM;
	float3 normal : NORMAL;
};

PS_INPUT shadowVertexShader(VS_INPUT input) : SV_POSITION {
	PS_INPUT output;
	
	float4 pos = input.position;

	matrix wvp = mul(worldMatrix, lightVP);

	output.position = mul(wvp, pos);
	output.normal = 0;
	output.tex = 0;
	output.positionSM = 0;

	return output;
}

float4 shadowPixelShader(PS_INPUT input) : SV_TARGET {

}