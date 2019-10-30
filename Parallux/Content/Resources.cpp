

#include "pch.h"
#include "Hvy3DScene.h"
#include "..\Common\DirectXHelper.h"

using namespace Parallux;
using namespace DirectX;
using namespace Windows::Foundation;



void Hvy3DScene::gv_CreateSamplerState()
{
    //  Sampler State Object for the Pixel Shader to render the Bitmap: 
    
    D3D11_SAMPLER_DESC sampDescZ;
    ZeroMemory(&sampDescZ, sizeof(sampDescZ));
    sampDescZ.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDescZ.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDescZ.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDescZ.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDescZ.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDescZ.MinLOD = 0;
    sampDescZ.MaxLOD = D3D11_FLOAT32_MAX;

    DX::ThrowIfFailed( 
        m_deviceResources->GetD3DDevice()->CreateSamplerState( 
            &sampDescZ,
            e_SamplerState.ReleaseAndGetAddressOf() 
        ) 
    );
}



void Hvy3DScene::gv_CreateVertexBuffer_MonoQuad()
{
    //  
    //  ghv 20190410: I altered z_coordinate of the MonoQuad mesh vertices
    //  because it improved render size of the Poincare Disk. 
    //  When z_coord = -1.f, the Poincare Disk is rendered very small on the screen.
    //  But using z_coord = -3.f gives a Poincare Disk large enough to fill the screen.
    //  

    const float z_coord = -3.f; //  Use z_coord = -3.f to render large Poincare Disk; 


    std::vector<VHG_CubeFaceVertexPT> monoQuadVertices = 
    {
        {XMFLOAT3(-1.0f, -1.0f,  z_coord), XMFLOAT2(0.0f, 1.0f)},
        {XMFLOAT3(+1.0f, -1.0f,  z_coord), XMFLOAT2(1.0f, 1.0f)},
        {XMFLOAT3(-1.0f, +1.0f,  z_coord), XMFLOAT2(0.0f, 0.0f)},

        {XMFLOAT3(-1.0f, +1.0f,  z_coord), XMFLOAT2(0.0f, 0.0f)},
        {XMFLOAT3(+1.0f, -1.0f,  z_coord), XMFLOAT2(1.0f, 1.0f)},
        {XMFLOAT3(+1.0f, +1.0f,  z_coord), XMFLOAT2(1.0f, 0.0f)},
    };
    
    m_MonoQuadVertexCount = (uint32_t)monoQuadVertices.size();

    const VHG_CubeFaceVertexPT   *p_vertices = &(monoQuadVertices[0]);

    size_t a_required_allocation = sizeof(VHG_CubeFaceVertexPT) * m_MonoQuadVertexCount;

    D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
    ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
    vertexBufferData.pSysMem = p_vertices;
    vertexBufferData.SysMemPitch = 0;
    vertexBufferData.SysMemSlicePitch = 0;

    CD3D11_BUFFER_DESC vertexBufferDesc((UINT)a_required_allocation, D3D11_BIND_VERTEX_BUFFER);

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &vertexBufferDesc,
            &vertexBufferData,
            &m_MonoQuadVertexBuffer
        )
    );


    //===============================================
    //  Each triangle below is FRONT_FACE_CLOCKWISE: 
    //          
    //  (cf D3D11_RASTERIZER_DESC.FrontCounterClockwise);
    //      
    //  Each trio of index entries represents one triangle. 
    //===============================================

    static const unsigned short quadIndices[] =
    {
        0,2,1,
        5,4,3,
    };

    m_MonoQuadIndexCount = ARRAYSIZE(quadIndices);


    D3D11_SUBRESOURCE_DATA quad_ib_data = { 0 };
    quad_ib_data.pSysMem = quadIndices;
    quad_ib_data.SysMemPitch = 0;
    quad_ib_data.SysMemSlicePitch = 0;

    CD3D11_BUFFER_DESC quad_ib_descr(sizeof(quadIndices), D3D11_BIND_INDEX_BUFFER);

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &quad_ib_descr, 
            &quad_ib_data, 
            &m_MonoQuadIndexBuffer
        )
    );
}





void Hvy3DScene::gv_CreateVertexBuffer_TextureCube()
{
    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    //      
    //                  Texture Coordinates (u,v)
    //                  =========================
    //      
    //  As an ordered pair, (u,v) can take but four values, i.e. 
    //  (u,v) is an element of the set {00, 10, 11, 01}.
    //      
    //  Arrange these four potential values in a circular fashion: 
    //          
    //                                  01
    //                          
    //                            00          11
    //                  
    //                                  10
    //      
    //      
    //  While traversing the circular arrangement clockwise, the correct 
    //  texture coordinate values for the +y face, the -x face and the-z face
    //  are obtained. 
    //  
    //  Then anticlockwise traversal of circle gives the proper texture 
    //  coordinates for the remaining three faces of the cube. 
    //    
    //  Only two orderings can be said to exist for the four tokens 
    //  arranged in the circle: the clockwise ordering and the anticlockwise ordering. 
    //  The starting position on the circle corresponds to order-preserving 
    //  permutations of the four elements, and also correspond to rotations
    //  of the bitmap image as displayed on the meshed cube. 
    //  Rotating the circular quartet of uv pairs shown above effects 
    //  that rotation of the bitmap image on its face of the meshed cube. 
    //  
    //      
    //%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
    //      



    //  Take advantage of C++11 braced initialization for std::vector:


    std::vector<VHG_CubeFaceVertexPT> cubeVertices = {


        //  
        //     the +y face: somebody looking at the cube would call this the "top" of the cube;
        //  Correct uv pairs are 01, 11,10, 00 which are the numbers obtained 
        //  by clockwise traversal of the circular diagram. 
        // 
        { XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT2(0.00f, 1.00f) },  // A  now correct;
        { XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT2(1.00f, 1.00f) },  // B  now correct;
        { XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT2(1.00f, 0.00f) },  // C  now correct;
        { XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT2(0.00f, 0.00f) },  // D  now correct;
        //    
        //     the -y face: the underside, seldom visible face of the cube; 
        //  The circular diagram, traversed ANTICLOCKWISE gives 00, 10, 11, 01: 
        //    
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.00f, 0.00f) }, // A is repaired;
        { XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT2(1.00f, 0.00f) }, // B is repaired;
        { XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT2(1.00f, 1.00f) }, // C is repaired;
        { XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT2(0.00f, 1.00f) }, // D is repaired;





        // 
        //    the  -x face:  a person looking at the cube would call this the "left" face;
        //   
        { XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT2(0.00f, 1.00f) },  // now correct;  A;
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(1.00f, 1.00f) },  // now correct;  B; 
        { XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT2(1.00f, 0.00f) },  // now correct;  C; 
        { XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT2(0.00f, 0.00f) },  // now correct;  D; 
        // 
        //    the +x face: viewer looking at the cube would call this the "right" face;
        //   
        { XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT2(1.00f, 1.00f) }, // A (now correct);
        { XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT2(0.00f, 1.00f) }, // B (now correct);
        { XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT2(0.00f, 0.00f) }, // C (now correct);
        { XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT2(1.00f, 0.00f) }, // D (now correct);




        //  
        //   the front face is the -z face: pick uv values by moving clockwise around the circle;
        //  
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.00f, 1.00f) },   //  A  16  okay;
        { XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT2(1.00f, 1.00f) },   //  B  17  okay;
        { XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT2(1.00f, 0.00f) },   //  C  18  okay;
        { XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT2(0.00f, 0.00f) },   //  D  19  okay;
        //  
        //      the back face is the +z face: uv values given by moving anticlockwise around the circle;
        //  
        { XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT2(1.00f, 1.00f) }, // anticlockwise around the circle;
        { XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT2(0.00f, 1.00f) }, // anticlockwise;
        { XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT2(0.00f, 0.00f) }, // anticlockwise;
        { XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT2(1.00f, 0.00f) }  // anticlockwise;

    };

    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$


    m_TextureCubeVertexCount = (uint32_t)cubeVertices.size();


    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$


    const VHG_CubeFaceVertexPT   *p_vertices = &(cubeVertices[0]);


    size_t a_required_allocation = sizeof(VHG_CubeFaceVertexPT) * m_TextureCubeVertexCount;


    D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
    ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));
    vertexBufferData.pSysMem = p_vertices;
    vertexBufferData.SysMemPitch = 0;
    vertexBufferData.SysMemSlicePitch = 0;


    CD3D11_BUFFER_DESC vertexBufferDesc((UINT)a_required_allocation, D3D11_BIND_VERTEX_BUFFER);

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &vertexBufferDesc,
            &vertexBufferData,
            &m_TextureCubeVertexBuffer
        )
    );


    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    //   
    //     Don't try this without the Index Buffer
    // 

    static const unsigned short cubeIndices[] =
    {
        3,1,0,          2,1,3,
        6,4,5,          7,4,6,
        11,9,8,         10,9,11,
        14,12,13,       15,12,14,
        19,17,16,       18,17,19,
        22,20,21,       23,20,22
    };

    m_TextureCubeIndexCount = ARRAYSIZE(cubeIndices);


    D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
    ZeroMemory(&indexBufferData, sizeof(indexBufferData));
    indexBufferData.pSysMem = cubeIndices;
    indexBufferData.SysMemPitch = 0;
    indexBufferData.SysMemSlicePitch = 0;

    CD3D11_BUFFER_DESC indexBufferDesc(sizeof(cubeIndices), D3D11_BIND_INDEX_BUFFER);

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &indexBufferDesc, &indexBufferData, &m_TextureCubeIndexBuffer
        )
    );
}





void Hvy3DScene::gv_CreateDualViewports()
{
    //  Construct the Major Viewport: 

    float const renderTargetWidth = m_deviceResources->GetOutputSize().Width; 
    float const renderTargetHeight = m_deviceResources->GetOutputSize().Height; 

    e_ViewportMajor = CD3D11_VIEWPORT(
        0.f,                    //  top_leftX;
        0.f,                    //  top_leftY;
        renderTargetWidth,      //  width;
        renderTargetHeight      //  height;
    );


    //  Construct the minor Viewport: 

    float const vpFraction = 0.333f;  //  Use 0.25f or use 0.333f; 



    float const vpAspectRatio = renderTargetWidth / renderTargetHeight;
    float const minorWidth = vpFraction * renderTargetWidth;
    float const minorHeight = minorWidth / vpAspectRatio;

    float top_leftX_viewport_B = renderTargetWidth - minorWidth;
    float top_leftY_viewport_B = renderTargetHeight - minorHeight;

    e_ViewportMinor = CD3D11_VIEWPORT(
        top_leftX_viewport_B,   //  top_leftX;
        top_leftY_viewport_B,   //  top_leftY;
        minorWidth,             //  width;
        minorHeight             //  height;
    );

    /*
    D3D11_VIEWPORT arrViewports[2];
    arrViewports[0] = e_ViewportMajor;
    arrViewports[1] = e_ViewportMinor;
    m_deviceResources->GetD3DDeviceContext->RSSetViewports(2, arrViewports);
    */
}




void Hvy3DScene::gv_CreateDepthStencilState()
{
    D3D11_DEPTH_STENCIL_DESC gv_depth_stencil_descr = { 0 };

    // Depth test parameters
    //---------------------------------------------------------------
    gv_depth_stencil_descr.DepthEnable = true;
    gv_depth_stencil_descr.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    gv_depth_stencil_descr.DepthFunc = D3D11_COMPARISON_ALWAYS;  // ghv : to allow blending;

    // Stencil test parameters
    //---------------------------------------------------------------
    gv_depth_stencil_descr.StencilEnable = true;
    gv_depth_stencil_descr.StencilReadMask = 0xFF;
    gv_depth_stencil_descr.StencilWriteMask = 0xFF;

    // Stencil operations if pixel is front-facing
    //---------------------------------------------------------------
    gv_depth_stencil_descr.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    gv_depth_stencil_descr.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
    gv_depth_stencil_descr.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    gv_depth_stencil_descr.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    // Stencil operations if pixel is back-facing
    //---------------------------------------------------------------
    gv_depth_stencil_descr.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    gv_depth_stencil_descr.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
    gv_depth_stencil_descr.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    gv_depth_stencil_descr.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateDepthStencilState(
            &gv_depth_stencil_descr,
            ghv_DepthStencilState.GetAddressOf()
        )
    );

    //                  
    //      ghv : Bind the Depth-Stencil State to the OM Stage: 
    //                  
    //  m_d3dContext->OMSetDepthStencilState(ghv_DepthStencilState.Get(), 0);
    //                  
}



void Hvy3DScene::gv_CreateBlendState()
{
    D3D11_RENDER_TARGET_BLEND_DESC  rt_blend_descr = { 0 };
    rt_blend_descr.BlendEnable = TRUE;
    rt_blend_descr.SrcBlend = D3D11_BLEND_SRC_ALPHA;        // SrcBlend = D3D11_BLEND_SRC_ALPHA;
    rt_blend_descr.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;   // DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    rt_blend_descr.BlendOp = D3D11_BLEND_OP_ADD;            // BlendOp = D3D11_BLEND_OP_ADD;
    rt_blend_descr.SrcBlendAlpha = D3D11_BLEND_ONE;
    rt_blend_descr.DestBlendAlpha = D3D11_BLEND_ZERO;
    rt_blend_descr.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    rt_blend_descr.RenderTargetWriteMask = 0x0F;


    D3D11_BLEND_DESC  d3d11_blend_descr = { 0 };
    d3d11_blend_descr.AlphaToCoverageEnable = FALSE;
    d3d11_blend_descr.IndependentBlendEnable = TRUE;
    d3d11_blend_descr.RenderTarget[0] = { rt_blend_descr };

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBlendState(
            &d3d11_blend_descr,
            e_BlendState.GetAddressOf()
        )
    );
}






