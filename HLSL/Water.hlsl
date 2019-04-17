cbuffer MatrixBuffer : register(b0)
{
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projectionMatrix;
};

cbuffer ReflectionBuffer : register(b1)
{
	matrix reflectionMatrix;
};

cbuffer WaterBuffer : register(b2)
{
	float waterTranslation;
	float reflectRefractScale;
	float2 padding;
};

SamplerState SampleType : register(s0);
Texture2D reflectionTexture : register(t0);
Texture2D refractionTexture : register(t1);
Texture2D normalTexture : register(t2);

struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float4 reflectionPosition : TEXCOORD1;
	float4 refractionPosition : TEXCOORD2;
};

struct VertexInputType
{
	float4 position : POSITION;
	float2 tex : TEXCOORD0;
};

PixelInputType WaterVertexShader(VertexInputType input)
{
	PixelInputType output;
	matrix reflectProjectWorld;
	matrix viewProjectWorld;

	input.position.w = 1.0f;

	//Calculate the position of the vertex against the world, view, and projection matrices.
	output.position = mul(input.position, worldMatrix);
	output.position = mul(output.position, viewMatrix);
	output.position = mul(output.position, projectionMatrix);

	//Store the texture coordinates for the pixel shader.
	output.tex = input.tex;

	reflectProjectWorld = mul(reflectionMatrix, projectionMatrix);
	reflectProjectWorld = mul(worldMatrix, reflectProjectWorld);

	output.reflectionPosition = mul(input.position, reflectProjectWorld);

	viewProjectWorld = mul(viewMatrix, projectionMatrix);
	viewProjectWorld = mul(worldMatrix, viewProjectWorld);

	output.refractionPosition = mul(input.position, viewProjectWorld);

	return output;
}


float4 WaterPixelShader(PixelInputType input) : SV_TARGET
{
	float2 reflectTexCoord;
float2 refractTexCoord;
float4 normalMap;
float3 normal;
float4 reflectionColor;
float4 refractionColor;
float4 color;

input.tex.y += waterTranslation;

reflectTexCoord.x = input.reflectionPosition.x / input.reflectionPosition.w / 2.0f + 0.5f;
reflectTexCoord.y = -input.reflectionPosition.y / input.reflectionPosition.w / 2.0f + 0.5f;

refractTexCoord.x = input.refractionPosition.x / input.refractionPosition.w / 2.0f + 0.5f;
refractTexCoord.y = -input.refractionPosition.y / input.refractionPosition.w / 2.0f + 0.5f;

normalMap = normalTexture.Sample(SampleType, input.tex);

normal = (normalMap.xyz * 2.0f) - 1.0f;

//波紋
reflectTexCoord = reflectTexCoord + (normal.xy * reflectRefractScale);
refractTexCoord = refractTexCoord + (normal.xy * reflectRefractScale);

reflectionColor = reflectionTexture.Sample(SampleType, reflectTexCoord);
refractionColor = refractionTexture.Sample(SampleType, refractTexCoord);

color = lerp(reflectionColor, refractionColor, 0.4f);

color.x += 0.0f;
color.y += 0.0f;
color.z += 0.0f;

color = float4(color.x, color.y, color.z, color.w);

return color;
}
