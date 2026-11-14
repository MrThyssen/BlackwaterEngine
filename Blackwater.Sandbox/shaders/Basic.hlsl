//
// Basic.hlsl -- one vertex shader, two pixel shaders:
//   PSLit   -- ambient + diffuse (Lambert) from one directional light
//   PSUnlit -- the vertex colour as-is (the grid)
//
// Compiled at runtime by bw::Shader (vs_5_0 / ps_5_0).
//

// register(b0) matches ConstantBuffer::BindToVertexStage(context, 0).
//
// The matrices arrive already transposed. DirectXMath stores matrices
// row-major; HLSL packs float4x4 constants column-major. Transposing on the
// CPU makes those two conventions cancel out, so mul(vector, matrix) below
// does what it reads like.
// PORT(M5): HLSL stays the source language. For Metal it is cross-compiled at
// build time (HLSL -> SPIR-V -> MSL); register(b0) maps to a Metal buffer index.
cbuffer Transforms : register(b0)
{
    float4x4 WorldViewProjection;
    float4x4 World;               // LearnOpenGL's "model"
};

// register(b1) matches ConstantBuffer::BindToPixelStage(context, 1).
cbuffer Lighting : register(b1)
{
    float3 DirectionToLight;      // unit vector, world space, pointing *at* the sun
    float  Ambient;               // shares the 16-byte row with the float3
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float4 colour   : COLOR;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 normal   : NORMAL;    
    float4 colour   : COLOR;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;

    // w = 1: a point, so the translation row applies.
    output.position = mul(float4(input.position, 1.0f), WorldViewProjection);

    // w = 0: a direction, so translation is ignored -- moving the cube must
    // not tilt its normals. Correct for translation, rotation and uniform
    // scale. Non-uniform scale needs the inverse-transpose ("normal matrix")
    // instead; nothing we draw does that yet.
    output.normal = mul(float4(input.normal, 0.0f), World).xyz;
    output.colour = input.colour;

    return output;
}

// SV_TARGET means "this is the colour for render target 0".
float4 PSLit(VSOutput input) : SV_TARGET
{
    // Interpolating unit vectors across a triangle shortens them, and scaling
    // shortens or lengthens them. dot() is only the cosine for unit vectors.
    const float3 normal = normalize(input.normal);

    // Lambert: brightness is the cosine of the angle between the surface and
    // the light. max() keeps faces turned away at zero rather than negative.
    const float diffuse = max(dot(normal, DirectionToLight), 0.0f);

    // White light, so "light colour * object colour" is just the object colour.
    return float4((Ambient + diffuse) * input.colour.rgb, input.colour.a);
}

// Use color as it is and ignore light (PSUnlit)
float4 PSUnlit(VSOutput input) : SV_TARGET
{
    return input.colour;
}
