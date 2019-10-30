#pragma once

#include "..\Common\DeviceResources.h"
#include "..\Common\StepTimer.h"
#include "..\HyperbolicMethods\HyperbolicMethods.h"

//  
//  This application renders a 3D scene containing two independent "subjects":
//  First, a 3D mesh cube. Secondly a 2D flat quad.
//  

namespace Parallux
{
    using HvyPlex = std::complex<double>;


    struct VHG_ConBufFDT
    {
        DirectX::XMFLOAT4       FDTLocator; // stores xLocatorPosition, yLocatorPosition, xyLocatorScale, zero;
        DirectX::XMFLOAT2       Apothem;
        DirectX::XMUINT2        SchlafliSymbol;   //  x stores p,  y stores q;
    };


    struct VHG_ConBufTess
    {
        DirectX::XMFLOAT4       FDTLocator; // stores xLocatorPosition, yLocatorPosition, xyLocatorScale, zero;
        DirectX::XMFLOAT4       TessellationSize;
        DirectX::XMFLOAT2       Apothem;
        DirectX::XMUINT2        SchlafliSymbol;   //  x stores p,  y stores q;
        DirectX::XMUINT2        LoopIndex;   //  x stores loopIndex_X,  y stores loopIndex_Y;
		DirectX::XMUINT2		Spin;
    };


    struct VHG_CubeFaceVertexPT
    {
        DirectX::XMFLOAT3       e_pos;
        DirectX::XMFLOAT2       e_texco;
    };


	struct ModelViewProjectionConstantBuffer
	{
		DirectX::XMFLOAT4X4 model;
		DirectX::XMFLOAT4X4 view;
		DirectX::XMFLOAT4X4 projection;
	};


    class Hvy3DScene
    {
    public:
        Hvy3DScene(const std::shared_ptr<DX::DeviceResources>& deviceResources);

        static std::random_device                               e_rng_seed;
        static std::mt19937                                     e_rng_gen;
        static std::uniform_real_distribution<float>            e_rng_dist;

        void CreateDeviceDependentResources();
        void CreateWindowSizeDependentResources();
        void ReleaseDeviceDependentResources();
        void Update(DX::StepTimer const& timer);
        void Render();

    private:

        void        gv_CreateDepthStencilState(); 


        void                        gv_DispatchComputeShaderTessellation(); 




        void                        debouncer_for_keyboard(void);


        void                        gvAnim_AnimationTick();

        bool                        gvAnim_CollisionImminent();


        void                        gvAnim_JumpStep(uint8_t pBits);


        void                        gvAnim_StepLeft();
        void                        gvAnim_StepRight();
        void                        gvAnim_StepUp();
        void                        gvAnim_StepDown();


        void                        gvAnim_JumpLeft();
        void                        gvAnim_JumpRight();
        void                        gvAnim_JumpUp();
        void                        gvAnim_JumpDown();


        void                        gv_ScaleToRenderTarget(float& p_xPosition, float& p_yPosition, float& p_xyScale);

        float                       gv_ScaleToRenderTarget();



        void                        gvInitializeSchlafliSymbol(unsigned int SchlafliP, unsigned int SchlafliQ);

        void                        gvConbuf_PrepareConbufData_Tessellation(unsigned int loopIndex_X, unsigned int loopIndex_Y);


        void                        gv_CreateDualViewports();
        void                        gv_CreateBlendState();
        void                        gv_CreateSamplerState();
        void                        gv_CreateVertexBuffer_TextureCube();
        void                        gv_CreateVertexBuffer_MonoQuad();


        void                        GoFileSavePicker();









        void                                            InputImage_LaunchFileOpenPicker();
        Concurrency::task<void>                         InputImage_GetFilePickerChoice();
        Concurrency::task<Windows::Foundation::Size>    InputImage_ReadInputImageFile();
        Concurrency::task<Windows::Foundation::Size>    InputImage_UseStartupAssetFile(std::wstring const&  relativeFileName);
        Windows::Foundation::Size                       InputImage_CreateInputResourceView();






        void                        FDTLocator_RenderInputImageAsBackground(); 
        void                        FDTLocator_DispatchComputeShader(); 
        void                        FDTLocator_CreateOutputResourceViews();
        void                        FDTLocator_PrepareConbufData();
        void                        FDTLocator_PixelShaderRender();







        Windows::Foundation::Size gvCreateOutputResourceViewsForTess();  //  creates two resource views on the outputImage;






    private:


        std::shared_ptr<DX::DeviceResources> m_deviceResources;




        Microsoft::WRL::ComPtr<ID3D11InputLayout>   m_inputLayout;



        Microsoft::WRL::ComPtr<ID3D11Buffer>                            m_MonoQuadVertexBuffer;
        uint32_t                                                        m_MonoQuadVertexCount;
        Microsoft::WRL::ComPtr<ID3D11Buffer>                            m_MonoQuadIndexBuffer;
        uint32_t                                                        m_MonoQuadIndexCount;


        Microsoft::WRL::ComPtr<ID3D11Buffer>                            m_TextureCubeVertexBuffer;
        uint32_t                                                        m_TextureCubeVertexCount;
        Microsoft::WRL::ComPtr<ID3D11Buffer>                            m_TextureCubeIndexBuffer;
        uint32_t                                                        m_TextureCubeIndexCount;





        Microsoft::WRL::ComPtr<ID3D11VertexShader>  m_vertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>   m_pixelShader;



        std::unique_ptr<DirectX::Keyboard>                              m_keyboard;
        std::unique_ptr<DirectX::Mouse>                                 m_mouse;
        bool                                                            g0_debouncer_processed_keyboard;


        unsigned int                                                    e_Schlafli_p;
        unsigned int                                                    e_Schlafli_q; 
        double                                                          e_apothem; 
        double                                                          e_circumradius;



        Platform::String^                                               e_FAL_Token;
        Platform::String^                                               e_chosen_file_name;
        Platform::String^                                               e_chosen_file_path;
        Windows::Storage::StorageFile^                                  e_chosen_file_storage_file;
        bool                                                            e_file_picker_complete;

        unsigned char *                                                 e_InputImage_bitmapBuffer;
        uint32_t                                                        e_InputImage_bitmapNumBytes;
        Windows::Foundation::Size                                       e_InputImageTextureSize; // width, height in pixels;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                e_InputImage_READONLY_SRV;


        Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>               e_FDTLocator_output_as_UAV;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                e_FDTLocator_output_as_SRV;
        Microsoft::WRL::ComPtr<ID3D11Buffer>                            e_FDTLocator_WVP_Buffer;
        ModelViewProjectionConstantBuffer                               e_FDTLocator_WVP_Data;
        Microsoft::WRL::ComPtr<ID3D11Buffer>                            e_FDTLocator_Trigl_Buffer;
        VHG_ConBufFDT                                                   e_FDTLocator_Data;
        Microsoft::WRL::ComPtr<ID3D11ComputeShader>                     e_FDTLocator_ComputeShader;

        DirectX::XMFLOAT2                                               e_FDTLocator_TrianglePosition;  // formerly e_PositionNOPO;

        DirectX::XMFLOAT2                                               e_VelocityNOPO;

        bool                                                            e_TessellationInvalidated;








        Microsoft::WRL::ComPtr<ID3D11SamplerState>                      e_SamplerState;

        Microsoft::WRL::ComPtr<ID3D11ComputeShader>                     e_ComputeShaderTess;


        Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>               output_TESS_as_UAV;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>                output_TESS_as_SRV;


        Microsoft::WRL::ComPtr<ID3D11Resource>                          output_D3D11_Resource;













        Microsoft::WRL::ComPtr<ID3D11Buffer>                    e_ConBufTessBuffer;
        VHG_ConBufTess                                          e_ConBufTessData;



        Microsoft::WRL::ComPtr<ID3D11Buffer>                    e_ConBufTessWVPBuffer;
        ModelViewProjectionConstantBuffer                       e_ConBufTessWVPData;







		unsigned int                                            e_ShowSpin;


        bool                                                    e_auto_roving;





        D3D11_VIEWPORT                                          e_ViewportMajor;
        D3D11_VIEWPORT                                          e_ViewportMinor;


        Microsoft::WRL::ComPtr<ID3D11BlendState>                e_BlendState;


        Microsoft::WRL::ComPtr<ID3D11DepthStencilState> ghv_DepthStencilState;  //  <---- here!!!!






        bool    m_loadingComplete;


        float   m_degreesPerSecond;
    };
}


