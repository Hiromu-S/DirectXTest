
Texture2D<float4>tBase : register(t0);
Texture2D<float>shadowMap : register(t1);
SamplerState samp0 : register(s0);
SamplerState samp1 : register(s1);

struct VSIn {
	float3 pos : POSITION;
	float3 normal : NORMAL;
	float2 tex : TEXCOORD;
};

struct VSOut {
	float4 pos : SV_POSITION;
	//float4 posSM : SV_POSITION;
	float4 normal : NORMAL;
	float2 tex : TEXCOORD;
};

cbuffer Scene : register(b0) {
	float4x4 worldViewProjMatrix;
	float4x4 worldMatrix;
};

cbuffer Light : register(b1) {
	float4x4 lightVP;
	float4 lightColor;
	float3 lightDir;
};

VSOut VSMain(VSIn vsIn) {
	VSOut output;

	float4 pos = float4(vsIn.pos, 1.0f);
	float4 normal = float4(vsIn.normal, 0.0f);
	output.pos = mul(pos, worldViewProjMatrix);
	output.normal = mul(normal, worldMatrix);
	output.tex = vsIn.tex;

	pos = mul(pos, worldMatrix);
	pos = mul(pos, lightVP);
	pos.xyz = pos.xyz / pos.w;
	output.pos.x = (1.0f + pos.x) / 2.0f;
	output.pos.x = (1.0f - pos.y) / 2.0f;
	output.pos.z = pos.z;

	//output.pos = mul(float4(vsIn.pos.xyz, 1), worldViewProjMatrix);
	//output.tex = vsIn.tex;
	return output;
}

/*
float4 PSMain(VSOut vsOut) : SV_TARGET {
	return float4(tBase.SampleLevel(sLinear, vsOut.tex, 0).rgb, 1);
}*/

float4 PSMain(VSOut vsOut) : SV_TARGET {
	float sm = shadowMap.Sample(samp1, vsOut.pos.xy);
	float sma = (vsOut.pos.z - 0.005f < sm) ? 1.0f : 0.5f;

	return tBase.Sample(samp0, vsOut.tex) * sma;
}

//シャドーマップ計算用頂点シェーダ
float4 VSShadowMap(VSIn vsIn) : SV_POSITION{
	float4 pos = float4(vsIn.pos, 1.0f);

	pos = mul(pos, worldMatrix);
	pos = mul(pos, lightVP);

	return pos;
}

/*
#define NUM_LIGHTS 1

Texture2D<float4> shadowMap : register(t0);
Texture2D<float4> diffuseMap : register(t1);
Texture2D<float4> normalMap : register(t2);

SamplerState samplerWrap : register(s0);
SamplerState samplerClamp : register(s1);

struct LightState {
	float3 position;
	float3 direction;
	float4 color;
	float4 falloff;
	float4x4 view;
	float4x4 projection;
};

struct PSInput {
	float4 position : SV_POSITION;
	float4 worldPosition : POSITION;
	float2 uv : TEXCOORD0;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
};

cbuffer sceneConstantBuffer : register(b0) {
	float4x4 model;
	float4x4 view;
	float4x4 projection;
	float4 ambientColor;
	bool sampleShadowMap;
	LightState lights[NUM_LIGHTS];
};

*/