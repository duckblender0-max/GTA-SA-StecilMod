// StencilShadows shader prototype.
// This file is packaged for the future Direct3D 9 renderer.
// The current ASI bootstrap does not compile or bind this shader yet.

float4 ShadowColor : register(c0);

struct PS_INPUT {
    float2 uv : TEXCOORD0;
};

float4 MainPS(PS_INPUT input) : COLOR0 {
    return float4(ShadowColor.rgb, saturate(ShadowColor.a));
}

technique StencilShadowTechnique {
    pass P0 {
        PixelShader = compile ps_2_0 MainPS();
    }
}
