


#include "pch.h"
#include "..\Content\Hvy3DScene.h"
#include "..\Common\DirectXHelper.h"

#include "..\ConfigHvyDx\pound_defines.h"

using namespace Parallux;
using namespace DirectX;
using namespace Windows::Foundation;



        
void Hvy3DScene::gvConbuf_PrepareConbufData_Tessellation(unsigned int loopIndex_X, unsigned int loopIndex_Y)
{
    float xPhysicalPosition = 0.f;
    float yPhysicalPosition = 0.f;
    float xyScale = 0.f; 
    gv_ScaleToRenderTarget(xPhysicalPosition, yPhysicalPosition, xyScale);

    float outputScale = 2.f / (float)((float)outputImageWidth - 2 * (float)outputPadding - 1.f);

    float outputXYTranslate = -1.f - outputScale * outputPadding;

    // float4
    // ======================================
    e_ConBufTessData.FDTLocator.x = xPhysicalPosition;
    e_ConBufTessData.FDTLocator.y = yPhysicalPosition;
    e_ConBufTessData.FDTLocator.z = xyScale;
    e_ConBufTessData.FDTLocator.w = 0.0f;

    // float4
    // ======================================
    e_ConBufTessData.TessellationSize.x = outputXYTranslate;
    e_ConBufTessData.TessellationSize.y = outputXYTranslate;
    e_ConBufTessData.TessellationSize.z = outputScale;
    e_ConBufTessData.TessellationSize.w = 0.0f;

    // float2
    // ======================================
    e_ConBufTessData.Apothem.x = (float)this->e_apothem; 
    e_ConBufTessData.Apothem.y = 0.f; // unused; 

    // uint2
    // ======================================
    e_ConBufTessData.SchlafliSymbol.x = this->e_Schlafli_p; 
    e_ConBufTessData.SchlafliSymbol.y = this->e_Schlafli_q; 

    // uint2
    // ======================================
    e_ConBufTessData.LoopIndex.x = loopIndex_X; 
    e_ConBufTessData.LoopIndex.y = loopIndex_Y;

    // uint2
    // ======================================
	e_ConBufTessData.Spin.x = e_ShowSpin;  //  1 for true, zero for false; 
	e_ConBufTessData.Spin.y = 0; // unused;
}





void Hvy3DScene::gv_DispatchComputeShaderTessellation()
{
	auto context = m_deviceResources->GetD3DDeviceContext();


    if (e_TessellationInvalidated == true)
    {
        ID3D11ShaderResourceView* nullSRV[] = { nullptr }; // singleton array;
        context->PSSetShaderResources(0, 1, nullSRV);

        //      
        //  The following pair of "for" loops have upper bound = 4. 
        //  Four derives from the fact that the outputImage is 2048 pixels 
        //  on an edge and a Dispatch of 512 thread groups seems to work well, 
        //  so
        //          2048 / 512 = 4  <------ number of loop iterations; 
        //      

        for (unsigned int idx_X = 0; idx_X < 4; idx_X++)
        {
            for (unsigned int idx_Y = 0; idx_Y < 4; idx_Y++)
            {
                gvConbuf_PrepareConbufData_Tessellation(idx_X, idx_Y);
                context->UpdateSubresource1(e_ConBufTessBuffer.Get(), 0, NULL, &e_ConBufTessData, 0, 0, 0);

                context->CSSetShader(e_ComputeShaderTess.Get(), nullptr, 0);

                //  Bind input_READONLY_SRV, output_TESS_as_UAV and conbuf to the compute shader:  
                context->CSSetShaderResources(0, 1, e_InputImage_READONLY_SRV.GetAddressOf());
                context->CSSetUnorderedAccessViews(0, 1, output_TESS_as_UAV.GetAddressOf(), nullptr);
                context->CSSetConstantBuffers(0, 1, e_ConBufTessBuffer.GetAddressOf());

                context->CSSetSamplers(0, 1, e_SamplerState.GetAddressOf()); // undo;

                context->Dispatch(512, 512, 1); //  does 512 perform better than 2048 ?
            }
        }

        //  Un-bind output_as_UAV from the Compute Shader so that output_as_SRV can be bound to the Pixel Shader: 

        ID3D11UnorderedAccessView* nullUAV[] = { nullptr }; // singleton array;

        context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);

        e_TessellationInvalidated = false;
    }

    //      
    //     VS Vertex Shader: 
    //      

	context->UpdateSubresource1( e_ConBufTessWVPBuffer.Get(), 0, NULL, &e_ConBufTessWVPData, 0, 0, 0 );

	UINT stride = sizeof(VHG_CubeFaceVertexPT);
	UINT offset = 0;
	context->IASetVertexBuffers( 0, 1, m_MonoQuadVertexBuffer.GetAddressOf(), &stride, &offset );
    // Each index is one 16-bit unsigned integer (short).
	context->IASetIndexBuffer( m_MonoQuadIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0 ); 
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(m_inputLayout.Get());
	context->VSSetShader( m_vertexShader.Get(), nullptr, 0 );
	context->VSSetConstantBuffers1( 0, 1, e_ConBufTessWVPBuffer.GetAddressOf(), nullptr, nullptr );

    //      
    //     PS Pixel Shader: 
    //      

	context->PSSetShader( m_pixelShader.Get(), nullptr, 0 );
    context->PSSetShaderResources(0, 1, output_TESS_as_SRV.GetAddressOf()); // Pixel Shader must be able to read the output image;
    context->PSSetSamplers(0, 1, e_SamplerState.GetAddressOf());



#ifdef GHV_OPTION_DUAL_VIEWPORTS

    context->RSSetViewports(1, &e_ViewportMajor);

#endif


	context->DrawIndexed( m_MonoQuadIndexCount, 0, 0 );
}








Windows::Foundation::Size Hvy3DScene::gvCreateOutputResourceViewsForTess()
{
    output_D3D11_Resource.Reset();
    wchar_t file_path_to_image[] = L"Assets\\0_Output_Image_Surrogate_Tessellation.png";

    HRESULT hrOutputSurrogate = CreateWICTextureFromFileEx(
        m_deviceResources->GetD3DDevice(), 
        file_path_to_image, 
        0,  //  maxsize;
        D3D11_USAGE_DEFAULT,   //  the D3D11_USAGE; 
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS,  //  the bindFlags (unsigned int); 
        0,   //  cpuAccessFlags; 
        0,   //  miscFlags;  
        WIC_LOADER_DEFAULT,   //  loadFlags   (can be _DEFAULT, _FORCE_SRGB or WIC_LOADER_IGNORE_SRGB); 
        output_D3D11_Resource.GetAddressOf(),  // the Texture being created; 
        nullptr   //  usually srv_something.GetAddressOf(), 
    ); 

    DX::ThrowIfFailed(hrOutputSurrogate);

    //  
    //  Reinterpret the D3D11 Resource as a Texture2D: 
    //  

    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
    HRESULT hrTex = output_D3D11_Resource.As(&tex);
    DX::ThrowIfFailed(hrTex);

    //  Create a readable resource view on the output image: 

    CD3D11_SHADER_RESOURCE_VIEW_DESC output_as_SRV_descr(tex.Get(), D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_UNKNOWN);

    DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateShaderResourceView(
        output_D3D11_Resource.Get(), &output_as_SRV_descr, &output_TESS_as_SRV)
    );

    //  Create a writable resource view on the output image: 

    CD3D11_UNORDERED_ACCESS_VIEW_DESC output_as_UAV_descr(tex.Get(), D3D11_UAV_DIMENSION_TEXTURE2D, DXGI_FORMAT_UNKNOWN);

    DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateUnorderedAccessView(
        output_D3D11_Resource.Get(), &output_as_UAV_descr, &output_TESS_as_UAV)
    );

    //  Get the size of the Texture2D: 

    D3D11_TEXTURE2D_DESC descTexture2D;
    tex->GetDesc(&descTexture2D);

    Windows::Foundation::Size outputTextureSize;
    outputTextureSize.Width = (float)descTexture2D.Width;
    outputTextureSize.Height = (float)descTexture2D.Height;

    return outputTextureSize; 
}






