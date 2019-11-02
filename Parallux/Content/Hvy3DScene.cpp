

#include "pch.h"
#include "Hvy3DScene.h"
#include "..\Common\DirectXHelper.h"
#include "..\ConfigHvyDx\pound_defines.h"

using namespace Parallux;
using namespace DirectX;
using namespace Windows::Foundation;

std::random_device                              Hvy3DScene::e_rng_seed;
std::mt19937                                    Hvy3DScene::e_rng_gen(Hvy3DScene::e_rng_seed());
std::uniform_real_distribution<float>           Hvy3DScene::e_rng_dist(+0.5f, +2.f);;

uint8_t     e_sj_left = 0x0001; 
uint8_t     e_sj_right = 0x0002; 
uint8_t     e_sj_up = 0x0004; 
uint8_t     e_sj_down = 0x0008; 
uint8_t     e_sj_step = 0x0010; 
uint8_t     e_sj_jump = 0x0020; 




        
void Hvy3DScene::gvInitializeSchlafliSymbol(unsigned int SchlafliP, unsigned int SchlafliQ)
{
    this->e_Schlafli_p = SchlafliP; 
    this->e_Schlafli_q = SchlafliQ; 

    if ((e_Schlafli_p - 2) * (e_Schlafli_q - 2) <= 4) 
    {
        throw "(p-2)(q-2) must be greater than 4";
    }

    e_apothem = HvyDXBase::HC_ApothemFromSchlafli_D(SchlafliP, SchlafliQ);

    e_circumradius = HvyDXBase::HC_CircumradiusFromSchlafli_D(SchlafliP, SchlafliQ);
}




Hvy3DScene::Hvy3DScene(const std::shared_ptr<DX::DeviceResources>& deviceResources) 
    :
	m_loadingComplete(false),
	m_degreesPerSecond(45),
	m_TextureCubeIndexCount(0),
	m_MonoQuadIndexCount(0),
    g0_debouncer_processed_keyboard(false),
    e_auto_roving(false), 
    e_FDTLocator_TrianglePosition(0.f, 0.f), 
    e_VelocityNOPO(0.5f, 0.5f), 
    e_ShowSpin(1),
    e_TessellationInvalidated(true), 
	m_deviceResources(deviceResources)
{
    // Tessellation {5, 5}:  
    gvInitializeSchlafliSymbol(5, 5); 

    // Dio:  gvInitializeSchlafliSymbol(4, 5);

    //  okay:  gvInitializeSchlafliSymbol(6, 5);

    // Tessellation {10, 5} tested good:   gvInitializeSchlafliSymbol(10, 5);

    //  tested ok    gvInitializeSchlafliSymbol(7, 3);




    // THIS TRIANGLE IS OUT OF BOUNDS gvInitializeSchlafliSymbol(3, 14); // TRIANGLE OUT OF BOUNDS. 
    
    //  try a 14-gon:     gvInitializeSchlafliSymbol(14, 3);

	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();

    m_keyboard = std::make_unique<DirectX::Keyboard>();
    m_mouse = std::make_unique<DirectX::Mouse>();
    Windows::UI::Core::CoreWindow  ^better_core_win = Windows::UI::Core::CoreWindow::GetForCurrentThread();
    m_keyboard->SetWindow(better_core_win);
    m_mouse->SetWindow(better_core_win);
}




void Hvy3DScene::CreateWindowSizeDependentResources()
{
    // Initializes view parameters when the window size changes.

	Size outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(   //  left-handed chirality;
		fovAngleY,
		aspectRatio,
		0.01f,
		100.0f
		);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	XMStoreFloat4x4( &e_FDTLocator_WVP_Data.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix) );

	XMStoreFloat4x4( &e_ConBufTessWVPData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix) );

	static const XMVECTORF32 eye = { 0.0f, 0.0f, -4.5f, 0.0f };  //  cameraPosition:  use negative z to pull camera back;
	static const XMVECTORF32 at = { 0.0f, 0.0f, 0.0f, 0.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };

	XMStoreFloat4x4(
        &e_FDTLocator_WVP_Data.view, 
        XMMatrixTranspose(
            XMMatrixLookAtLH(eye, at, up)
        )
    ); // left-handed chirality;

	XMStoreFloat4x4(
        &e_ConBufTessWVPData.view, 
        XMMatrixTranspose(
            XMMatrixLookAtLH(eye, at, up)
        )
    ); // left-handed chirality;
}





void Hvy3DScene::gv_ScaleToRenderTarget(float& p_xPosition, float& p_yPosition, float& p_xyScale)
{
    //  Convert the locator triangle position from Logical Coordinates 
    //  to Physical Coordinates (pixels). 
    //    
    //    
    //  
    //  Convert from 
    //      
    //         -1         0        +1
    //          |---------+---------|
    //  to
    //      
    //          0                   1024
    //          |---------+---------|
    //          
    //      
    //      
    //      xLogical - -1      xPhysical - 0
    //      -------------  ::  -------------
    //        +1  -  -1        TextureWidth
    //  
    //      
    //  xPhysical = TextureWidth * (xLogical + 1) / 2; 
    //      
    //      
    //  Remember maxWidthPhysical = m_deviceResources->GetOutputSize().Width; 
    //      
    //               
    //  The FDT is a hyperbolic right triangle whose sides are: 
    //      1. The hypotenuse, a Euclidean-straight line segment;
    //      2. The hyperbolic geodesic arc segment; 
    //      3. Another Euclidean-straight line segment. 
    //  "edgeLength3" specifies #3 in the list above. 
    //      

    float xLocatorTrianglePositionScreen = e_InputImageTextureSize.Width * (1.f + e_FDTLocator_TrianglePosition.x) / 2.f; 
    float yLocatorTrianglePositionScreen = e_InputImageTextureSize.Height * (1.f + e_FDTLocator_TrianglePosition.y) / 2.f; 

    //  There isn't any mathematical basis for the choice of scaling - it is 
    //  purely pragmatic or aesthetic: 

    float edge3LengthPhysical = inputImageWidth / 2.f;  // Physical length is expressed in pixels;

    float edge3LengthMathematical = (float)e_apothem; // Euclidean length in the complex plane;

    float edge3Scaling = edge3LengthPhysical / edge3LengthMathematical; // pixels/Math; 

    p_xPosition = xLocatorTrianglePositionScreen - edge3LengthPhysical / 2.f;
    p_yPosition = yLocatorTrianglePositionScreen;
    p_xyScale = edge3Scaling;
}





float Hvy3DScene::gv_ScaleToRenderTarget()
{
    float edge3LengthPhysical = inputImageWidth / 2.f;  // Physical length is expressed in pixels;
    float edge3LengthMathematical = (float)e_apothem; // Euclidean length in the complex plane;

    float edge3Scaling = edge3LengthPhysical / edge3LengthMathematical; // pixels/Math; 

    return edge3Scaling;
}





void Hvy3DScene::debouncer_for_keyboard(void)
{
    static uint32_t idx_waiting = 0;
    if (g0_debouncer_processed_keyboard)
    {
        idx_waiting++;
        if (idx_waiting > 10)  //  use period = 30; 
        {
            idx_waiting = 0;
            g0_debouncer_processed_keyboard = false;
        }
    }
}




        
bool Hvy3DScene::gvAnim_CollisionImminent()
{
    const float epsilon = 0.45f; // gold = 0.35f;

    bool retval = false;

    //  Domain: 

    float domainXmin = -1.0f; 
    float domainXMax = +1.0f;
    float domainYmin = -1.0f;
    float domainYMax = +1.0f;


    bool hLeft = (std::abs(e_FDTLocator_TrianglePosition.x - domainXmin) < epsilon);
    bool hRight = (std::abs(e_FDTLocator_TrianglePosition.x - domainXMax) < epsilon);

    bool vTop = (std::abs(e_FDTLocator_TrianglePosition.y - domainYmin) < epsilon);
    bool vBottom = (std::abs(e_FDTLocator_TrianglePosition.y - domainYMax) < epsilon);

    float aabs = 0.f;
    float aLimited = 0.f;


    if ( (hLeft && vTop) || (hLeft && vBottom) || (hRight && vTop)  || (hRight && vBottom) )
    {
        //  corner cases, literally: reflect both components of velocity: 

        e_VelocityNOPO.x *= -1.f;
        e_VelocityNOPO.y *= -1.f;
    }
    else if (hLeft || hRight)
    {
        // reflect horizontal velocity: 

        e_VelocityNOPO.x *= -1.f;
    }
    else if (vTop || vBottom)
    {
        // reflect vertical velocity: 

        e_VelocityNOPO.y *= -1.f;
    }
    else
    {
        // no collision is imminent, so take the opportunity
        // to randomize the velocity

        e_VelocityNOPO.x *= std::abs(e_rng_dist(e_rng_gen));
        aabs = std::abs(e_VelocityNOPO.x);
        aLimited = (std::min)(aabs, 1.6f);
        e_VelocityNOPO.x = copysign(aLimited, e_VelocityNOPO.x);



        e_VelocityNOPO.y *= std::abs(e_rng_dist(e_rng_gen));
        aabs = std::abs(e_VelocityNOPO.y);
        aLimited = (std::min)(aabs, 1.6f);
        e_VelocityNOPO.y = copysign(aLimited, e_VelocityNOPO.y);
    }

    //  
    //  Now that the velocity's direction has been corrected, 
    //  can scale the velocity's magnitude as long as signum is unchanged
    //  

    return retval;
}







void Hvy3DScene::gvAnim_AnimationTick()
{
    const float xyStepDistance = 0.024f; 

    if (e_auto_roving == true)
    {
        //  Causes the Reticle Grid to move randomly like a Pong arcade game;

        gvAnim_CollisionImminent();
        e_FDTLocator_TrianglePosition.x += e_VelocityNOPO.x * xyStepDistance;
        e_FDTLocator_TrianglePosition.y += e_VelocityNOPO.y * xyStepDistance;
        e_TessellationInvalidated = true;
    }
}





void Hvy3DScene::gvAnim_JumpStep(uint8_t pBits)
{
    const float xyStepDistance = 0.024f; 
    const float xyJumpDistance = 0.12f; 
    float dist = 0.f;

    if ((pBits & e_sj_step) == e_sj_step)
    {
        dist = xyStepDistance;
    }
    else if ((pBits & e_sj_jump) == e_sj_jump)
    {
        dist = xyJumpDistance;
    }




    float halfWidth = inputImageWidth / 2.f; 
    float quarterWidth = inputImageWidth / 4.f; 


    if ((pBits & e_sj_left) == e_sj_left)
    {
        float finalPos = (e_FDTLocator_TrianglePosition.x - dist) * halfWidth - quarterWidth; 
        if (finalPos > -halfWidth)
        {
            e_FDTLocator_TrianglePosition.x -= dist;
            e_TessellationInvalidated = true;
        }
    }
    else if ((pBits & e_sj_right) == e_sj_right)
    {
        float crXProjectionPhysical = (float)e_circumradius * gv_ScaleToRenderTarget() * cos(XM_PI / e_Schlafli_p);

        float finalPos = (dist + e_FDTLocator_TrianglePosition.x) * halfWidth - quarterWidth + crXProjectionPhysical; 
        if (finalPos < halfWidth)
        {
            e_FDTLocator_TrianglePosition.x += dist;
            e_TessellationInvalidated = true;
        }
    }
    else if ((pBits & e_sj_up) == e_sj_up)
    {
        float finalPos = (e_FDTLocator_TrianglePosition.y - dist) * halfWidth; 
        if (finalPos > -halfWidth)
        {
            e_FDTLocator_TrianglePosition.y -= dist;
            e_TessellationInvalidated = true;
        }
    }
    else if ((pBits & e_sj_down) == e_sj_down)
    {
        float crYProjectionPhysical = (float)e_circumradius * gv_ScaleToRenderTarget() * sin(XM_PI / e_Schlafli_p);

        float finalPos = (e_FDTLocator_TrianglePosition.y + dist) * halfWidth + crYProjectionPhysical; 
        if (finalPos < halfWidth)
        {
            e_FDTLocator_TrianglePosition.y += dist; 
            e_TessellationInvalidated = true;
        }
    }
}





void Hvy3DScene::gvAnim_StepLeft()
{
    gvAnim_JumpStep(e_sj_step | e_sj_left); 
}

void Hvy3DScene::gvAnim_StepRight()
{
    gvAnim_JumpStep(e_sj_step | e_sj_right); 
}

void Hvy3DScene::gvAnim_StepUp()
{
    gvAnim_JumpStep(e_sj_step | e_sj_up); 
}

void Hvy3DScene::gvAnim_StepDown()
{
    gvAnim_JumpStep(e_sj_step | e_sj_down); 
}

void Hvy3DScene::gvAnim_JumpLeft()
{
    gvAnim_JumpStep(e_sj_jump | e_sj_left); 
}

void Hvy3DScene::gvAnim_JumpRight()
{
    gvAnim_JumpStep(e_sj_jump | e_sj_right); 
}

void Hvy3DScene::gvAnim_JumpUp()
{
    gvAnim_JumpStep(e_sj_jump | e_sj_up); 
}

void Hvy3DScene::gvAnim_JumpDown()
{
    gvAnim_JumpStep(e_sj_jump | e_sj_down); 
}







void Hvy3DScene::Update(DX::StepTimer const& timer)
{
    DirectX::Keyboard::State           kb = m_keyboard->GetState();

    if (kb.O)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;
            e_auto_roving = false;
            e_TessellationInvalidated = false; // halt CS Compute Shader invocation;
            InputImage_LaunchFileOpenPicker();
        }
    }


    if (kb.S)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;
            GoFileSavePicker();
        }
    }


    if (kb.I)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;
			e_ShowSpin = (e_ShowSpin) ? 0 : 1;
            e_TessellationInvalidated = true;
        }
    }


    if (kb.F3)
    {
        if (e_auto_roving == true)
        {
            e_auto_roving = false;
            e_TessellationInvalidated = false;
        }
    } 

    if (kb.F4)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;
            e_TessellationInvalidated = true;
            if (e_auto_roving == false)
            {
                e_auto_roving = true;
            }
        }
    } 

    if(kb.Left)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;
            gvAnim_StepLeft();
        }
    } 

    if(kb.A)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;
            gvAnim_JumpLeft();
        }
    } 


    if(kb.Right)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;
            gvAnim_StepRight();
        }
    } 

    if(kb.D)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;
            gvAnim_JumpRight();
        }
    } 


    if(kb.Up)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;
            gvAnim_StepUp();
        }
    } 

    if(kb.W)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;
            gvAnim_JumpUp();
        }
    } 

    if(kb.Down)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;
            gvAnim_StepDown();
        }
    } 

    if(kb.X)
    {
        if (!g0_debouncer_processed_keyboard)
        {
            g0_debouncer_processed_keyboard = true;
            gvAnim_JumpDown();
        }
    } 

    // Animation speed: framePeriod = 0.666 sec to be used with 30 frames per second rendering; 
    const float framePeriod = 0.666f;  


    static float accumTime = 0.f; 
    accumTime += (float)timer.GetElapsedSeconds();
    if (accumTime > framePeriod)
    {
        gvAnim_AnimationTick(); 
        accumTime = 0.f;
    }

    float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
	double totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
    float radians = static_cast<float>(fmod(totalRotation, XM_2PI));
    // XMMATRIX cubeRotation = XMMatrixRotationY(radians);
    XMMATRIX cubeRotation = XMMatrixIdentity(); 

    XMMATRIX cubeTranslation = XMMatrixTranslation(-3.5f, -1.5f, 0.f);

#ifdef GHV_OPTION_DUAL_VIEWPORTS
    cubeTranslation = XMMatrixIdentity();
#endif

    float unif = 0.9f;  // Use uniform scaling of 0.5f; 
    XMMATRIX cubeScaling = XMMatrixScaling(unif, unif, unif);

    //  First rotate the cube about the world origin, 
    //  then translate the rotated cube from origin to destination;
    XMMATRIX cubeWorldXform = cubeScaling * cubeRotation * cubeTranslation; 


    XMStoreFloat4x4(
        &e_FDTLocator_WVP_Data.model, 
        XMMatrixTranspose(
            cubeWorldXform
        )
    );

    XMStoreFloat4x4(
        &e_ConBufTessWVPData.model, 
        XMMatrixTranspose(  // Keep XMMatrixTranspose here as a future reminder.
            XMMatrixIdentity()
        )
    );
}




void Hvy3DScene::Render()
{
	if (!m_loadingComplete)
	{
		return; // Loading is asynchronous. Only draw geometry after it's loaded.
	}

    this->debouncer_for_keyboard(); 

    //  
    //  Subject one: render the 3D FDT (fundamental domain triangle) locator cube: 
    //  
    //  The cube faces are rendered as two layers which are blended together. 
    //  
    //  Layer 1, the bottom layer, is simply a sampling the Texture2D which is created
    //  when the user interacts with the file picker and chooses an image file 
    //  to be tessellated.  
    //  
    //  Layer 2, the top-most layer, is calculated by a Compute Shader. 
    //  Layer 2 is then rendered by a pixel shader. 
    //  
    //  The  BlendState allows Layer1 to partially shine through Layer2, 
    //  which makes the red triangle of Layer2 appear semi-transparent.
    //  


    // Draw Layer 1: 
    FDTLocator_RenderInputImageAsBackground();   // Draws only the InputImage Texture2D on the 3D cube; No triangle is drawn;
	

    auto context = m_deviceResources->GetD3DDeviceContext();
    context->OMSetBlendState(e_BlendState.Get(), NULL, 0xFFFFFFFF);  //  keywords: blendstate, alpha, blending;
    context->OMSetDepthStencilState(ghv_DepthStencilState.Get(), 0);


    // Draw Layer 2: 
    FDTLocator_DispatchComputeShader(); // Draws black background except where it draws solid red triangle;
    FDTLocator_PixelShaderRender(); 

    //  
    //  Subject two: render the hyperbolic tessellation on the 2D quad: 
    //  
    gv_DispatchComputeShaderTessellation(); 
}



void Hvy3DScene::CreateDeviceDependentResources()
{
#ifdef GHV_OPTION_DUAL_VIEWPORTS
    gv_CreateDualViewports();  
#endif

    gv_CreateDepthStencilState(); 

    gv_CreateBlendState();  

    gv_CreateSamplerState();

	auto loadVSTask = DX::ReadDataAsync(L"t1VertexShader.cso");

	auto loadPSTask = DX::ReadDataAsync(L"t1PixelShader.cso");

	auto loadCSFDTLocatorTask = DX::ReadDataAsync(L"FDTLocatorComputeShader.cso");

	auto loadCSTessTask = DX::ReadDataAsync(L"TessellationComputeShader.cso");

    InputImage_UseStartupAssetFile(L"\\Assets\\0_Default_Input_Image_1024.png");  

    FDTLocator_CreateOutputResourceViews();  

    gvCreateOutputResourceViewsForTess();  

    auto createCSFDTLocatorTask = loadCSFDTLocatorTask.then([this](const std::vector<byte>& fileData)
    {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateComputeShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&e_FDTLocator_ComputeShader
				)
			);
    });

    auto createCSTessTask = loadCSTessTask.then([this](const std::vector<byte>& fileData)
    {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateComputeShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&e_ComputeShaderTess
				)
			);
    });

	auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData) {

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_vertexShader
				)
			);

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc [] =
		{
            //      
            // float3 for VHG_CubeFaceVertexPT.e_pos;
            // float2 for VHG_CubeFaceVertexPT.e_texco;
            //      
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA,   0 }, 
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA,   0 }  
		};

		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				&fileData[0],
				fileData.size(),
				&m_inputLayout
				)
			);
	});





	auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData) {
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_pixelShader
				)
			);

        //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

		CD3D11_BUFFER_DESC constantBuffer_WVP_Desc(
            sizeof(ModelViewProjectionConstantBuffer) , 
            D3D11_BIND_CONSTANT_BUFFER
        );

        static_assert( 
            (sizeof(ModelViewProjectionConstantBuffer) % 16) == 0, 
            "Constant Buffer struct must be 16-byte aligned" 
        );

		DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateBuffer( // 1 of 4; 
                &constantBuffer_WVP_Desc, 
                nullptr, 
                &e_FDTLocator_WVP_Buffer
            )
        );

        //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        static_assert( 
            (sizeof(ModelViewProjectionConstantBuffer) % 16) == 0, 
            "Constant Buffer struct must be 16-byte aligned" 
        );

        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateBuffer(  // 2 of 4; 
                &constantBuffer_WVP_Desc, 
                nullptr, 
                &e_ConBufTessWVPBuffer 
            ) 
        );

        //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

		CD3D11_BUFFER_DESC constantBuffer_FDT_Desc(
            sizeof(VHG_ConBufFDT) , 
            D3D11_BIND_CONSTANT_BUFFER
        );

        static_assert( 
            (sizeof(VHG_ConBufFDT) % 16) == 0, 
            "Constant Buffer struct must be 16-byte aligned" 
        );

        DX::ThrowIfFailed( 
            m_deviceResources->GetD3DDevice()->CreateBuffer(  // 3 of 4; 
                &constantBuffer_FDT_Desc, 
                nullptr, 
                &e_FDTLocator_Trigl_Buffer 
            ) 
        );
        
        //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

		CD3D11_BUFFER_DESC constantBuffer_Tess_Desc(
            sizeof(VHG_ConBufTess) , 
            D3D11_BIND_CONSTANT_BUFFER
        );

        static_assert( 
            (sizeof(VHG_ConBufTess) % 16) == 0, 
            "Constant Buffer struct must be 16-byte aligned" 
        );

        DX::ThrowIfFailed( 
            m_deviceResources->GetD3DDevice()->CreateBuffer(  // 4 of 4; 
                &constantBuffer_Tess_Desc, 
                nullptr, 
                &e_ConBufTessBuffer 
            ) 
        );
    });




        //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	auto createMeshTask = (createCSFDTLocatorTask && createCSTessTask && createPSTask && createVSTask).then([this] () 
    {
		// Load mesh vertices.

        gv_CreateVertexBuffer_MonoQuad(); 
        gv_CreateVertexBuffer_TextureCube(); 
	});

	createMeshTask.then([this] () {
		m_loadingComplete = true;
	});
}




void Hvy3DScene::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;
	m_vertexShader.Reset();
	m_inputLayout.Reset();
	m_pixelShader.Reset();


	e_FDTLocator_WVP_Buffer.Reset();
	e_ConBufTessWVPBuffer.Reset();


	m_TextureCubeVertexBuffer.Reset();
	m_MonoQuadVertexBuffer.Reset();

	m_TextureCubeIndexBuffer.Reset();
	m_MonoQuadIndexBuffer.Reset();
}





