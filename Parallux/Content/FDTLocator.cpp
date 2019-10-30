//          
//      
//  FDTLocator.cpp
//          
//


#include "pch.h"
#include "..\Content\Hvy3DScene.h"
#include "..\Common\DirectXHelper.h"
#include "..\ConfigHvyDx\pound_defines.h"

using namespace Parallux;
using namespace DirectX;
using namespace Windows::Foundation;




void Hvy3DScene::FDTLocator_CreateOutputResourceViews()
{
    Microsoft::WRL::ComPtr<ID3D11Resource>  temp_resource;
    wchar_t file_path_to_image[] = L"Assets\\0_Output_Image_Surrogate_FDTLocator.png";

    HRESULT hrOutputSurrogate = CreateWICTextureFromFileEx(
        m_deviceResources->GetD3DDevice(), 
        file_path_to_image, 
        0,  //  maxsize;
        D3D11_USAGE_DEFAULT,   //  the D3D11_USAGE; 
        D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS,  //  the bindFlags (unsigned int); 
        0,   //  cpuAccessFlags; 
        0,   //  miscFlags;  
        WIC_LOADER_DEFAULT,   //  loadFlags   (can be _DEFAULT, _FORCE_SRGB or WIC_LOADER_IGNORE_SRGB); 
        temp_resource.GetAddressOf(),  // the Texture being created; 
        nullptr   //  usually srv_something.GetAddressOf(), 
    ); 

    DX::ThrowIfFailed(hrOutputSurrogate);

    //  
    //  Reinterpret the D3D11 Resource as a Texture2D: 
    //  

    Microsoft::WRL::ComPtr<ID3D11Texture2D> tex;
    HRESULT hrTex = temp_resource.As(&tex);
    DX::ThrowIfFailed(hrTex);

    //  Create a readable resource view on the output image: 

    CD3D11_SHADER_RESOURCE_VIEW_DESC output_as_SRV_descr(
        tex.Get(), 
        D3D11_SRV_DIMENSION_TEXTURE2D, 
        DXGI_FORMAT_UNKNOWN
    );

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateShaderResourceView( 
            temp_resource.Get(), 
            &output_as_SRV_descr, 
            &e_FDTLocator_output_as_SRV
        )
    );

    //  Create a writable resource view on the output image: 

    CD3D11_UNORDERED_ACCESS_VIEW_DESC output_as_UAV_descr(
        tex.Get(), 
        D3D11_UAV_DIMENSION_TEXTURE2D, 
        DXGI_FORMAT_UNKNOWN
    );

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateUnorderedAccessView( 
            temp_resource.Get(), 
            &output_as_UAV_descr, 
            &e_FDTLocator_output_as_UAV
        )
    );
}



        
void Hvy3DScene::FDTLocator_PrepareConbufData()
{
    float xPhysicalPosition = 0.f;
    float yPhysicalPosition = 0.f;
    float xyScale = 0.f; 
    gv_ScaleToRenderTarget(xPhysicalPosition, yPhysicalPosition, xyScale);

    // And invert the scaling: 

    float inverseScale = 1.f / xyScale; // in units of Math/pixel;

    //  Convert from physical coords (in pixels) to logical coords: 

    float xLogicalPosition = -xPhysicalPosition * inverseScale;  // Signum;
    float yLogicalPosition = -yPhysicalPosition * inverseScale;  // Signum;

    // float4
    // ======================================
    e_FDTLocator_Data.FDTLocator.x = xLogicalPosition;
    e_FDTLocator_Data.FDTLocator.y = yLogicalPosition;
    e_FDTLocator_Data.FDTLocator.z = inverseScale;
    e_FDTLocator_Data.FDTLocator.w = 0.0f;
   
    // float2
    // ======================================
    e_FDTLocator_Data.Apothem.x = (float)this->e_apothem; 
    e_FDTLocator_Data.Apothem.y = 0.f; // unused; 

    // uint2
    // ======================================
    e_FDTLocator_Data.SchlafliSymbol.x = this->e_Schlafli_p; 
    e_FDTLocator_Data.SchlafliSymbol.y = this->e_Schlafli_q; 
}








void Hvy3DScene::FDTLocator_DispatchComputeShader()
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    FDTLocator_PrepareConbufData();
    context->UpdateSubresource1(e_FDTLocator_Trigl_Buffer.Get(), 0, NULL, &e_FDTLocator_Data, 0, 0, 0);

    context->CSSetShader(e_FDTLocator_ComputeShader.Get(), nullptr, 0);

    //  Bind e_InputImage_READONLY_SRV, e_FDTLocator_output_as_UAV 
    //  and conbuf to the compute shader:  

    context->CSSetShaderResources(0, 1, e_InputImage_READONLY_SRV.GetAddressOf());
    context->CSSetUnorderedAccessViews(0, 1, e_FDTLocator_output_as_UAV.GetAddressOf(), nullptr);
    context->CSSetConstantBuffers(0, 1, e_FDTLocator_Trigl_Buffer.GetAddressOf());

    context->Dispatch(100, 100, 1);

    //  Un-bind the output UAV from the Compute Shader 
    //  so that output SRV can be bound to the Pixel Shader: 

    ID3D11UnorderedAccessView* nullUAV[] = { nullptr }; // singleton array;
    context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
}








void Hvy3DScene::FDTLocator_PixelShaderRender()
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    //      
    //     VS Vertex Shader: 
    //      

    context->UpdateSubresource1( e_FDTLocator_WVP_Buffer.Get(), 0, NULL, &e_FDTLocator_WVP_Data, 0, 0, 0 );

    UINT stride = sizeof(VHG_CubeFaceVertexPT);
    UINT offset = 0;
    context->IASetVertexBuffers( 0, 1, m_TextureCubeVertexBuffer.GetAddressOf(), &stride, &offset );
    // Each index is one 16-bit unsigned integer (short).
    context->IASetIndexBuffer( m_TextureCubeIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0 ); 
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(m_inputLayout.Get());
    context->VSSetShader( m_vertexShader.Get(), nullptr, 0 );
    context->VSSetConstantBuffers1( 0, 1, e_FDTLocator_WVP_Buffer.GetAddressOf(), nullptr, nullptr );

    //      
    //     PS Pixel Shader: 
    //      

    context->PSSetShader( m_pixelShader.Get(), nullptr, 0 );
    context->PSSetShaderResources(0, 1, e_FDTLocator_output_as_SRV.GetAddressOf()); // Pixel Shader must be able to read the output image;
    context->PSSetSamplers(0, 1, e_SamplerState.GetAddressOf());

#ifdef GHV_OPTION_DUAL_VIEWPORTS
    context->RSSetViewports(1, &e_ViewportMinor);
#endif

    context->DrawIndexed( m_TextureCubeIndexCount, 0, 0 );

    //  Un-bind: 
    ID3D11ShaderResourceView* nullSRV[] = { nullptr }; // singleton array;
    context->PSSetShaderResources(0, 1, nullSRV);
}








        
void Hvy3DScene::FDTLocator_RenderInputImageAsBackground()
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    //      
    //     VS Vertex Shader: 
    //      

    context->UpdateSubresource1( e_FDTLocator_WVP_Buffer.Get(), 0, NULL, &e_FDTLocator_WVP_Data, 0, 0, 0 );

    UINT stride = sizeof(VHG_CubeFaceVertexPT);
    UINT offset = 0;
    context->IASetVertexBuffers( 0, 1, m_TextureCubeVertexBuffer.GetAddressOf(), &stride, &offset );
    // Each index is one 16-bit unsigned integer (short).
    context->IASetIndexBuffer( m_TextureCubeIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0 ); 
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(m_inputLayout.Get());
    context->VSSetShader( m_vertexShader.Get(), nullptr, 0 );
    context->VSSetConstantBuffers1( 0, 1, e_FDTLocator_WVP_Buffer.GetAddressOf(), nullptr, nullptr );

    //      
    //     PS Pixel Shader: 
    //      

    context->PSSetShader( m_pixelShader.Get(), nullptr, 0 );
    context->PSSetShaderResources(0, 1, e_InputImage_READONLY_SRV.GetAddressOf());
    context->PSSetSamplers(0, 1, e_SamplerState.GetAddressOf());

#ifdef GHV_OPTION_DUAL_VIEWPORTS
    context->RSSetViewports(1, &e_ViewportMinor);
#endif

    context->DrawIndexed( m_TextureCubeIndexCount, 0, 0 );
}













