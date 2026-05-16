// =============================================================================
// UberLit.hlsl — Uber Shader for Forward Shading
// =============================================================================
// Preprocessor Definitions (C++ 에서 D3D_SHADER_MACRO 로 전달):
//   LIGHTING_MODEL_GOURAUD  1  — 정점 단계 라이팅 (Gouraud Shading)
//   LIGHTING_MODEL_LAMBERT  1  — 픽셀 단계 Diffuse only (Lambert)
//   LIGHTING_MODEL_PHONG    1  — 픽셀 단계 Diffuse + Specular (Blinn-Phong)
//
// 아무 라이팅 모델 매크로도 없으면 기본값 = Blinn-Phong
//   LIGHTING_MODEL_UNLIT   1  — 라이팅 없음 (Albedo + Wireframe)
// =============================================================================

#include "Common/Functions.hlsli"
#include "Common/VertexLayouts.hlsli"
#include "Common/SystemSamplers.hlsli"

#if !defined(LIGHTING_MODEL_UNLIT)
#include "Common/ForwardLighting.hlsli"
#endif

// ── 기본값 설정 ──
#if !defined(LIGHTING_MODEL_GOURAUD) && !defined(LIGHTING_MODEL_LAMBERT) && !defined(LIGHTING_MODEL_PHONG) && !defined(LIGHTING_MODEL_UNLIT)
#define LIGHTING_MODEL_PHONG 1
#endif

// =============================================================================
// 텍스처
// =============================================================================
Texture2D DiffuseTexture : register(t0);
Texture2D NormalTexture : register(t1);


// ── Per-Object Material (b2) — 기존 StaticMesh 와 레이아웃 동일 (호환성) ──
cbuffer PerShader1 : register(b2)
{
    float4 SectionColor;
    float HasNormalMap;
    float3 _pad;
};

// 머티리얼 확장 파라미터 — 팀원 A CB 시스템 완성 후 b2 확장 예정
static const float4 g_DefaultEmissive = float4(0, 0, 0, 0);
static const float g_DefaultShininess = 32.0f;

// =============================================================================
// VS ↔ PS 인터페이스
// =============================================================================
struct UberVS_Output
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR0;
    float2 texcoord : TEXCOORD0;
    float3 worldPos : TEXCOORD1;
    float4 tangent : TANGENT;
    float selectedBoneWeight : TEXCOORD4;
#if defined(LIGHTING_MODEL_GOURAUD) && LIGHTING_MODEL_GOURAUD
    float3 litDiffuse  : TEXCOORD2;
    float3 litSpecular : TEXCOORD3;
#endif
};


// =============================================================================
// Vertex Shader
// =============================================================================
UberVS_Output BuildUberVSOutput(
    float3 position,
    float3 normal,
    float4 color,
    float2 texcoord,
    float4 tangent,
    float selectedBoneWeight)
{
    UberVS_Output output;
    
    float3x3 M = (float3x3) Model;

    float4 worldPos4 = mul(float4(position, 1.0f), Model);
    output.worldPos = worldPos4.xyz;
    output.position = mul(mul(worldPos4, View), Projection);
    output.normal = normalize(mul(normal, (float3x3) NormalMatrix));
    output.color = color * SectionColor;
    output.texcoord = texcoord;
    output.selectedBoneWeight = selectedBoneWeight;

    float3 T = normalize(mul(tangent.xyz, M));
    T = normalize(T - output.normal * dot(output.normal, T));
    output.tangent = float4(T, tangent.w);

#if defined(LIGHTING_MODEL_GOURAUD) && LIGHTING_MODEL_GOURAUD
    float3 N =  output.normal;

    if (HasNormalMap > 0.5f)
    {
        float3 B = normalize(cross(N, T) * tangent.w);
        float3x3 TBN = float3x3(T, B, N);

        float3 tangentNormal = NormalTexture.SampleLevel(LinearWrapSampler, texcoord, 0).xyz * 2.0f - 1.0f;

        N = normalize(mul(tangentNormal, TBN));
    }

    float3 V = normalize(CameraWorldPos - output.worldPos);
    output.litDiffuse = AccumulateDiffuseVS(output.worldPos, N);
    output.litSpecular = AccumulateSpecularVS(output.worldPos, N, V, g_DefaultShininess);

#endif

    return output;
}

UberVS_Output VS(VS_Input_PNCTT input)
{
    return BuildUberVSOutput(input.position, input.normal, input.color, input.texcoord, input.tangent, 0.0f);
}

UberVS_Output SkeletalVS(VS_Input_PNCTBW input)
{
    float selectedWeight = 0.0f;

    // CPU skinning 결과는 이미 position/normal/tangent에 반영되어 있으므로 여기서는 선택 bone weight만 전달한다.
    // TODO: GPU Skinning 단계에서 bGPUSkinning이 켜지면 StructuredBuffer bone matrix로 vertex skinning을 수행한다.
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        if (input.boneIndices[i] == SelectedBoneIndex)
        {
            selectedWeight = input.boneWeights[i];
        }
    }

    return BuildUberVSOutput(input.position, input.normal, input.color, input.texcoord, input.tangent, selectedWeight);
}

// =============================================================================
// MRT 출력 구조체
// =============================================================================
struct UberPS_Output
{
    float4 Color : SV_TARGET0; // 최종 색상 (기존 프레임 버퍼)
    float4 Normal : SV_TARGET1; // World Normal (GBuffer Normal RT)
    float4 Culling : SV_TARGET2; // Tile Culling Heatmap
};

float3 GetBoneWeightHeatmapColor(float Weight)
{
    // Unreal weight paint에 가까운 heat ramp. 0 weight 색은 눈부심을 줄이기 위해 핫핑크보다 어둡게 둔다.
    float w = saturate(Weight);

    float3 c0 = float3(0.55f, 0.00f, 0.72f); // 영향 없음: 어두운 핑크/보라
    float3 c1 = float3(0.00f, 0.00f, 1.00f); // 낮음: 파랑
    float3 c2 = float3(0.00f, 0.85f, 1.00f); // 청록
    float3 c3 = float3(0.00f, 1.00f, 0.00f); // 초록
    float3 c4 = float3(1.00f, 1.00f, 0.00f); // 노랑
    float3 c5 = float3(1.00f, 0.45f, 0.00f); // 주황
    float3 c6 = float3(1.00f, 0.00f, 0.00f); // 높음: 빨강

    if (w < 0.10f)
    {
        return lerp(c0, c1, smoothstep(0.00f, 0.10f, w));
    }
    if (w < 0.28f)
    {
        return lerp(c1, c2, smoothstep(0.10f, 0.28f, w));
    }
    if (w < 0.45f)
    {
        return lerp(c2, c3, smoothstep(0.28f, 0.45f, w));
    }
    if (w < 0.62f)
    {
        return lerp(c3, c4, smoothstep(0.45f, 0.62f, w));
    }
    if (w < 0.78f)
    {
        return lerp(c4, c5, smoothstep(0.62f, 0.78f, w));
    }

    return lerp(c5, c6, smoothstep(0.78f, 1.00f, w));
}

// =============================================================================
// Pixel Shader
// =============================================================================
UberPS_Output PS(UberVS_Output input)
{
    UberPS_Output output;

    // Bone Weight Heatmap은 viewport debug 출력이므로 material/lighting/shadow 계산보다 먼저 반환한다.
    if (bBoneWeightHeatmap != 0)
    {
        // 선택 bone이 없으면 전체를 0 weight 색으로 보여 선택 상태가 비어 있음을 명확히 한다.
        float3 heatColor = GetBoneWeightHeatmapColor(0.0f);

        if (SelectedBoneIndex >= 0)
        {
            heatColor = GetBoneWeightHeatmapColor(input.selectedBoneWeight);
        }

        output.Color = float4(heatColor, 1.0f);
        output.Normal = float4(normalize(input.normal), 1.0f);
        output.Culling = float4(0, 0, 0, 0);
        return output;
    }

    float4 texColor = DiffuseTexture.Sample(LinearWrapSampler, input.texcoord);
    if (texColor.a < 0.001f)
        texColor = float4(1.0f, 1.0f, 1.0f, 1.0f);

    float4 baseColor = texColor * input.color;

    float3 N = normalize(input.normal);

#if !defined(LIGHTING_MODEL_GOURAUD)
    if (HasNormalMap >= 0.5)
    {
        float3 T = normalize(input.tangent.xyz);
        T = normalize(T - N * dot(N, T));

        float3 B = normalize(cross(N, T) * input.tangent.w);
        float3x3 TBN = float3x3(T, B, N);

        float3 tangentNormal = NormalTexture.Sample(LinearWrapSampler, input.texcoord).xyz * 2.0f - 1.0f;
        N = normalize(mul(tangentNormal, TBN));
    }
#endif

    float3 V = normalize(CameraWorldPos - input.worldPos);

#if defined(LIGHTING_MODEL_UNLIT) && LIGHTING_MODEL_UNLIT
    // Unlit: 라이팅 없이 Albedo만 출력
    float3 finalColor = ApplyWireframe(baseColor.rgb);
    output.Culling = float4(0, 0, 0, 0);

#else
    float3 diffuse = float3(0, 0, 0);
    float3 specular = float3(0, 0, 0);

#if defined(LIGHTING_MODEL_GOURAUD) && LIGHTING_MODEL_GOURAUD
    // Gouraud: VS에서 정점 단위로 계산 → PS에서 보간된 값 사용
    diffuse  = input.litDiffuse;
    specular = input.litSpecular;

#elif defined(LIGHTING_MODEL_LAMBERT) && LIGHTING_MODEL_LAMBERT
    diffuse = AccumulateDiffuse(input.worldPos, N, input.position);

#elif defined(LIGHTING_MODEL_PHONG) && LIGHTING_MODEL_PHONG
    diffuse = AccumulateDiffuse(input.worldPos, N, input.position);
    specular = AccumulateSpecular(input.worldPos, N, V, g_DefaultShininess, input.position);

#endif

    output.Culling = ComputeCullingHeatmap(input.position, input.worldPos);
    // Diffuse에만 albedo를 곱하고, Specular는 빛 색상 그대로 더한다
    // (비금속 표면: specular 반사 = 빛의 색, 물체 색이 아님)
    float3 finalColor = baseColor.rgb * diffuse + specular + g_DefaultEmissive.rgb;
    finalColor = ApplyWireframe(finalColor);
#endif

    output.Color = float4(finalColor, baseColor.a);
    output.Normal = float4(N, 1.0f); // alpha=1: 유효한 노말 마킹
    
    return output;
}
