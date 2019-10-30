//      
//      
//      ghv 20190409:  One of the Compute Shaders for real-time hyperbolic tessellation;
//      
//      Fundamental Domain Triangle (FDT) Locator Compute Shader;
//      

#include "..\ConfigHvyDx\pound_defines.h"

Texture2D               input_READONLY  : register(t0);
RWTexture2D<float4>     output_as_uav   : register(u0);

cbuffer conBufFDT : register(b0)
{
    float4      FDTLocator; // stores xLocatorPosition, yLocatorPosition, xyLocatorScale, zero;
    float2      Apothem2f;   // apothem value stored in x;
    uint2       SchlafliSymbol; // schlafli_p, schlafli_q;
};

static const float4 triangleColor = float4(1.0, 0.0, 0.0, 0.7); // Use 0.5 for alpha blending; 

#include "plexlibHyperbolicMethods.hlsl"


[numthreads(10, 10, 1)]
void cs_main(uint3 DTid : SV_DispatchThreadID)
{
    float4 inputImageTexel = input_READONLY.Load(int3(DTid.xy, 0)); 
    float2 pointInInputImage = float2(DTid.x, DTid.y);

    float2 pointInPoincareDisk = float2(
        FDTLocator.x + FDTLocator.z * pointInInputImage.x,
        FDTLocator.y + FDTLocator.z * pointInInputImage.y
    );
 
    //  Test whether or not inside: 

    if (plexlibInsideFDTStrict(Apothem2f.x, SchlafliSymbol.x, pointInPoincareDisk))
    {
        output_as_uav[DTid.xy] = triangleColor; //  Point is inside the FDT triangle; 
    }
    else
    {
        output_as_uav[DTid.xy] = float4(inputImageTexel.x, inputImageTexel.y, inputImageTexel.z, 1.0);
    }
}




