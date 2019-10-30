//      
//      
//      ghv 20190409:  Compute Shader for real-time hyperbolic image editing;
//      
//      Tessellation Compute Shader for tiling by arbitrary Schlafli Symbol; 
//      


#include "..\ConfigHvyDx\pound_defines.h"


Texture2D               input_READONLY  : register(t0);
RWTexture2D<float4>     output_as_uav   : register(u0);
SamplerState            genericSampler  : register(s0);


cbuffer conBufTess : register(b0)
{
    float4      FDTLocator; // stores xLocatorPosition, yLocatorPosition, xyLocatorScale, zero;
    float4      TessellationSize;
    float2      Apothem2f;   // apothem value stored in x;
    uint2       SchlafliSymbol; // schlafli_p, schlafli_q;
    uint2       LoopIndex;
	uint2		Spin;
};


#include "plexlibHyperbolicMethods.hlsl"


float lerpLerp(float4 p_gathered, float2 p_fraction)
{
    float top_row_red = lerp(p_gathered.w, p_gathered.z, p_fraction.x);
    float bottom_row_red = lerp(p_gathered.x, p_gathered.y, p_fraction.x);

    float red0 = lerp(top_row_red, bottom_row_red, p_fraction.y);
    return red0; 
}



float3 bilinear(float2 texcoord, float tex_dimension)
{
    //  Bilinear Interpolation using the four texels returned by HLSL Gather() intrinsic; 

    float2 pixel = texcoord * tex_dimension + 0.5;
    float2 fract = frac(pixel);

    // red channel
    float4 red4Samples = input_READONLY.GatherRed(genericSampler, texcoord);
    float averageRed = lerpLerp(red4Samples, fract); 
            
    // green channel
    float4 green4Samples = input_READONLY.GatherGreen(genericSampler, texcoord);
    float averageGreen = lerpLerp(green4Samples, fract); 
            
    // blue channel
    float4 blue4Samples = input_READONLY.GatherBlue(genericSampler, texcoord);
    float averageBlue = lerpLerp(blue4Samples, fract); 

    float3 result = float3(averageRed, averageGreen, averageBlue);
    return result;
}



float4 rgbInvert(float4 somePixel)
{
    return float4(
        1.0 - somePixel.x,
        1.0 - somePixel.y,
        1.0 - somePixel.z,
        somePixel.w
    );
}



bool FundamentalRegionReversePixelLookup(float2 aPoint, out float2 zInsideFDT, inout uint p_spinCount)
{
    //  The choice of name for this method was influenced by the paper 
    //  "Creation of Hyperbolic Ornaments" (Martin von Gagern, 2014) which
    //  details the Reverse Pixel Lookup algorithm. The mathematical 
    //  essence of this method I learned by studying work published on the web 
    //  by Arnaud Cheritat. See the bibliography included with this repository.
    //          
    //          
    //  Given some arbitrary input "aPoint" in the unit circle. 
    //  Compute its corresponding "pre-image" inside the fundamental domain. 
    //          

    bool amInside = false; 
    float2 zInvrtd = float2(0.f, 0.f);

    //  Initialize zIt to the input "aPoint". On subsequent iterations,
    //  zIt propagates its value from end of one loop to the beginning 
    //  of the next. 

    float2 zIt = aPoint;

    for (int idxIteration = 0; idxIteration < maxIterations; idxIteration++)
    {
        // Apply the first isometry, namely rotation:

        float2 rotz = plexlibFloorSector(SchlafliSymbol.x, zIt); 

        if (rotz.y < 0.0) 
        {
            p_spinCount++;
            rotz = plexlibComplexConjugate(rotz);
        }

        //  Test whether or not inside: 

        if (plexlibInsideFDTStrict(Apothem2f.x, SchlafliSymbol.x, rotz))
        {
            amInside = true;
            zInsideFDT = rotz;
            return false; // TODO: change to use "break" instead; 
        }

        //  Apply the second isometry, namely inversion in a circle. 
        //  By the phrase "a circle" I mean THE circle 
        //  containing the hyperbolic geodesic arc that forms an edge 
        //  of the Fundamental Domain triangle. 

        zInvrtd = plexlibIsometryOfInversion(Apothem2f.x, rotz);

        p_spinCount++;

        //  Test whether or not inside: 

        if (plexlibInsideFDTStrict(Apothem2f.x, SchlafliSymbol.x, zInvrtd)) 
        {
            amInside = true; 
            zInsideFDT = zInvrtd; 
            return false; // TODO: change to use "break" instead; 
        }

        //  Iterate as needed to get the given point 
        //  inside the fundamental domain: 

        zIt = zInvrtd;
    }

    return true; // Fail!
}





struct gvSharedData
{
    //  Interpretation:  
    //          pixelRGB.x = redAccum;
    //          pixelRGB.y = greenAccum;
    //          pixelRGB.z = blueAccum; 
    //      
    float3      pixelRGB;  //  <==========  cf ComputeShader in BC6HEncode.hlsl; 
};

groupshared   gvSharedData   shared_temp[msaaSampleCount * msaaSampleCount];  // one-dimensional array with card msaaSampleCount^2; 

[numthreads(msaaSampleCount, msaaSampleCount, 1)]  // Each Thread Group contains msaaSampleCount^2 Threads;
void cs_main(uint3 p_SVGroupID : SV_GroupID,  uint3 p_SVGTID : SV_GroupThreadID, uint GI : SV_GroupIndex) 
{
    shared_temp[GI].pixelRGB.x = 0.0;
    shared_temp[GI].pixelRGB.y = 0.0;
    shared_temp[GI].pixelRGB.z = 0.0;

    uint spinCount = 0; 

    int xidx = (512 * LoopIndex.x) + p_SVGroupID.x;
    int yidx = (512 * LoopIndex.y) + p_SVGroupID.y;
    uint3 outputImagePixelXY = uint3(xidx, yidx, 0);

    //  
    //  LoopIndex.x ranges from 0 to 3 inclusive. 
    //  p_SVGroupID.x ranges from 0 to 511 inclusive. 
    //  Thus xidx ranges from 512 * 0 + 0 to 512 * 3 + 511; 
    //  
    //  xidx ranges from 0 to 2047  (same for yidx). 
    //      
    //  Note that the output image size is hard-coded in 
    //  TessellationRenderer.cpp as 2048 x 2048 pixels squared. 
    //          
    //  So the set of all ordered pairs <xidx, yidx> 
    //  defines the valid pixel addresses in the output image 
    //  at its normal resolution of 2048 x 2048. 
    //      
    //==================================================================
    //==================================================================

    double2 aOutXYFractional;  // higher sampling rate;

    aOutXYFractional.x = (double)(xidx + p_SVGTID.x / (double)msaaSampleCount);
    aOutXYFractional.y = (double)(yidx + p_SVGTID.y / (double)msaaSampleCount);

    //  
    //  p_SVGTID.x aka SV_GroupThreadID.x ranges from 0 to 7 because of numthreads = 8; 
    //      
    //  p_SVGTID.y aka SV_GroupThreadID.y ranges from 0 to 7 because of numthreads = 8; 
    //  
    //  Consequently the quotient p_SVGTID.x / msaaSampleCount 
    //  is always a fraction like 0/8, 1/8, 2/8, ... 7/8. 
    //          
    //  aOutXYFractional is computed from (xidx, yidx) ordered pairs. 
    //          
    //  Moreover, aOutXYFractional is the "higher sampling rate" that 
    //  is characteristic of MSAA multisample anti-aliasing. 
    //          
    //  Indeed, xidx has sampling rate of 2048 samples (width) per image; 
    //  
    //  But aOutXYFractional has sampling rate of 8 * 2048 samples (width) 
    //  per image because aOutXYFractional subdivides each [0, 1) interval 
    //  into eight fractional "eighth" sub-intervals. 
    //  
    //==================================================================
    //==================================================================

    float2 aHypXYFractional = float2(
        TessellationSize.x + TessellationSize.z * aOutXYFractional.x, 
        TessellationSize.y + TessellationSize.z * aOutXYFractional.y
    ); 


    if (plexlibAbs((float2)aHypXYFractional) <= 1.00)
    {
        //  For arbitary <x,y> in the unit circle, compute its corresponding 
        //  pre-image inside the FDT fundamental domain: 

        float2 xyLocationInFDT = float2(0.f, 0.f);
        if(FundamentalRegionReversePixelLookup(aHypXYFractional, xyLocationInFDT, spinCount) == true)
        {
            // Fail! Exhausted all iterations but didn't get into fundamental domain...
            output_as_uav[outputImagePixelXY.xy] = float4(0.f, 1.f, 0.f, 1.f); // Set the color at pixel (xidx, yidx) in the outputImage;
        }

        //  From the point known to be inside fundamental domain,
        //  compute its correspondent on the input plane: 

        float2 bXYInputPosition = float2(
            FDTLocator.x + FDTLocator.z * xyLocationInFDT.x, 
            FDTLocator.y + FDTLocator.z * xyLocationInFDT.y 
        ); 

        //      
        //  We now have a precise <x,y> position on the input plane, 
        //  but we must not assume such a location corresponds directly to a pixel. 
        //  Rather, use bilinear interpolation on the four actual pixels 
        //  surrounding bXYInputPosition. 
        //      

        float4 bInterpolatedColor = float4(0.f, 0.f, 0.f, 1.f);
        uint width_texels;
        uint height_texels;
        input_READONLY.GetDimensions(width_texels, height_texels);

        bInterpolatedColor.xyz = bilinear(bXYInputPosition / width_texels, width_texels);
        
        //  
        //  Can this fail? 
        //  "Fail" would mean that the x,y inputPosition 
        //  doesn't map to the interior of the inputImage. 
        //  
        //  But that should never happen: I constrain movement 
        //  of the FDT Locator triangle so that it is always 
        //  over the interior of the inputImage. 
        //      

        if (Spin.x > 0 && spinCount % 2 == 0)
        { 
            bInterpolatedColor = rgbInvert(bInterpolatedColor); 
        }

        shared_temp[GI].pixelRGB.xyz = bInterpolatedColor.xyz;
    } // if inside the Poincare disk;
    else
    {
        //  This pixel belongs to the background: 
        shared_temp[GI].pixelRGB.xyz = float3(1.f, 1.f, 0.f); // Color for background;
    }

    //##################################################################

    GroupMemoryBarrierWithGroupSync();

    //##################################################################

    if (GI == 0)
    {
        uint numSamples = (uint)(msaaSampleCount * msaaSampleCount);
        float3 accumulatedRGB = float3(0.0, 0.0, 0.0); 

        for (int jj = 0; jj < msaaSampleCount * msaaSampleCount; jj++)
        {
            accumulatedRGB += shared_temp[jj].pixelRGB; 
        }

        //  
        //  SuperSampling: 
        //  The "downsampling" step takes the arithmetic mean 
        //  of each color channel over numSamples. 
        //  

        float4 superSamplingAveragedRGBA = float4( 
            accumulatedRGB.x / numSamples,
            accumulatedRGB.y / numSamples,
            accumulatedRGB.z / numSamples,
            1.0
        );

        output_as_uav[outputImagePixelXY.xy] = superSamplingAveragedRGBA; // Set the color at pixel (xidx, yidx) in the outputImage;
    }
}




