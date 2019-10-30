//                  
//          
//  AsynchronousMethods.cpp  
//          
//          

#include "pch.h"
#include "..\Content\Hvy3DScene.h"
#include "..\Common\DirectXHelper.h"

using namespace Parallux;
using namespace DirectX;
using namespace Windows::Foundation;



void Hvy3DScene::InputImage_LaunchFileOpenPicker()
{
    this->InputImage_GetFilePickerChoice().then([this]()
    {
            if (this->e_chosen_file_storage_file)
            {
                this->e_file_picker_complete = true;

                InputImage_ReadInputImageFile().then([this](Windows::Foundation::Size returnedSize)
                {
                        e_TessellationInvalidated = true; // this line fixed long-standing invalidate bug;
                });
            }
    });
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


Concurrency::task<void> Hvy3DScene::InputImage_GetFilePickerChoice()
{
    Windows::Storage::Pickers::FileOpenPicker^ filePicker = ref new Windows::Storage::Pickers::FileOpenPicker();
    filePicker->SuggestedStartLocation = Windows::Storage::Pickers::PickerLocationId::Desktop;
    filePicker->FileTypeFilter->Append(".png");

    //    Get a handle on the FAL FutureAccessList : 
    //    The MSFT documentations specifically recommends to 
    //    quote 
    //      "Access the methods of the StorageApplicationPermissions class statically"
    //    end quote. 
    // 

    namespace  WSAC = Windows::Storage::AccessCache;

    // this->e_FAL_list = WSAC::StorageApplicationPermissions::FutureAccessList;


    return concurrency::create_task(
        filePicker->PickSingleFileAsync()
    ).then([this](Windows::Storage::StorageFile^ tmp_file)
    {
        if (tmp_file)
        {
            this->e_chosen_file_storage_file = tmp_file;
            
            e_FAL_Token = WSAC::StorageApplicationPermissions::FutureAccessList->Add(tmp_file, "ghv_metadata2"); // undo

            this->e_chosen_file_name = tmp_file->Name;
            this->e_chosen_file_path = tmp_file->Path;
        }
        else
        {
            this->e_chosen_file_storage_file = nullptr;
            this->e_chosen_file_name = nullptr;
            this->e_chosen_file_path = nullptr;
        }
    });
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


Concurrency::task<Windows::Foundation::Size> Hvy3DScene::InputImage_ReadInputImageFile()
{
    //      
    //  HP:   https://docs.microsoft.com/en-us/cpp/cppcx/array-and-writeonlyarray-c-cx?view=vs-2017 
    //      
    //  Copy into either a Platform::Array or a Platform::ArrayReference but not both:  
    //  Alt: malloc a Platform::Array named "bytes" of sufficient size;
    //  Alt: auto bytes = ref new Platform::Array<unsigned char>(e_bitmapNumBytes);  
    //  Alt: dataReader->ReadBytes(bytes);
    //                     

    using namespace Windows::Storage;
    using namespace Windows::Storage::Streams;
    using namespace Concurrency;

    Windows::Storage::IStorageFile^ xfile  = e_chosen_file_storage_file;  

    IAsyncOperation<IBuffer^> ^iaoReadBufferOperation = FileIO::ReadBufferAsync(xfile); 

    return concurrency::create_task(iaoReadBufferOperation).then([this](Streams::IBuffer^ fileBuffer) 
    {
        auto   dataReader = Windows::Storage::Streams::DataReader::FromBuffer(fileBuffer);

        dataReader->ByteOrder = Windows::Storage::Streams::ByteOrder::LittleEndian;

        e_InputImage_bitmapNumBytes = dataReader->UnconsumedBufferLength;

        //  At this point e_BitmapNumBytes = operating system file size in Bytes; 

        this->e_InputImage_bitmapBuffer = new unsigned char[e_InputImage_bitmapNumBytes]; 
        dataReader->ReadBytes(Platform::ArrayReference<unsigned char>(e_InputImage_bitmapBuffer, e_InputImage_bitmapNumBytes));
        this->e_InputImageTextureSize = InputImage_CreateInputResourceView();

        return this->e_InputImageTextureSize;
    });
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                        
Concurrency::task<Windows::Foundation::Size> Hvy3DScene::InputImage_UseStartupAssetFile(std::wstring const&  relativeFileName)
{
    using namespace Windows::Storage;
    using namespace Concurrency;

    auto folder = Windows::ApplicationModel::Package::Current->InstalledLocation;  //  resolves to "AppX"; 

    return create_task(folder->GetFileAsync(Platform::StringReference(relativeFileName.c_str()))).then([this](StorageFile^ file)
    {
        return FileIO::ReadBufferAsync(file);

    }).then([this](Streams::IBuffer^ fileBuffer) -> Windows::Foundation::Size //  formerly UINT 
    {
        auto   dataReader = Windows::Storage::Streams::DataReader::FromBuffer(fileBuffer);

        dataReader->ByteOrder = Windows::Storage::Streams::ByteOrder::LittleEndian;

        e_InputImage_bitmapNumBytes = dataReader->UnconsumedBufferLength;

        this->e_InputImage_bitmapBuffer = new unsigned char[e_InputImage_bitmapNumBytes]; 
        dataReader->ReadBytes(Platform::ArrayReference<unsigned char>(e_InputImage_bitmapBuffer, e_InputImage_bitmapNumBytes));
        this->e_InputImageTextureSize = InputImage_CreateInputResourceView();

        return this->e_InputImageTextureSize;
    });
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Windows::Foundation::Size Hvy3DScene::InputImage_CreateInputResourceView()
{
    Microsoft::WRL::ComPtr<ID3D11Resource>   inputD3D11Resource;

    HRESULT hrInputTexture = DirectX::CreateWICTextureFromMemory(
        m_deviceResources->GetD3DDevice(),
        &e_InputImage_bitmapBuffer[0],
        (size_t)e_InputImage_bitmapNumBytes,
        inputD3D11Resource.GetAddressOf(),
        e_InputImage_READONLY_SRV.GetAddressOf(),
        0     //   maxsize parameter: zero ==> maxsize will be determined by the device's feature level; 
    );

    DX::ThrowIfFailed(hrInputTexture);

    //  
    //  Reinterpret the D3D11 Resource as a Texture2D: 
    //  

    Microsoft::WRL::ComPtr<ID3D11Texture2D> inputTex2D;
    HRESULT hrTexInterpret = inputD3D11Resource.As(&inputTex2D);
    DX::ThrowIfFailed(hrTexInterpret);

    //  
    //  Get the size of the Texture2D: 
    //  

    D3D11_TEXTURE2D_DESC descTexture2D;
    inputTex2D->GetDesc(&descTexture2D);
    Windows::Foundation::Size inputTexture2DSize;
    inputTexture2DSize.Width = (float)descTexture2D.Width;
    inputTexture2DSize.Height = (float)descTexture2D.Height;

    return inputTexture2DSize; 
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

void Hvy3DScene::GoFileSavePicker()
{
    e_auto_roving = false;


    using namespace concurrency;
    using namespace Windows::Storage;
    using namespace Windows::Storage::Pickers;

    auto folder = ApplicationData::Current->TemporaryFolder;

    wchar_t fname[_MAX_PATH];
    wcscpy_s(fname, folder->Path->Data());
    wcscat_s(fname, L"\\hyperbolic.png"); // formerly jpg;



    // note that context use is not thread-safe

    auto immContext = m_deviceResources->GetD3DDeviceContext();

    HRESULT hr = DirectX::SaveWICTextureToFile(
        immContext, 
        output_D3D11_Resource.Get(),      //  usually  backBuffer.Get() BUT WORKS ON OTHER D3D11 RESOURCES AS WELL!!! 
        GUID_ContainerFormatPng,
        fname
    );

    DX::ThrowIfFailed(hr);


    auto pngExtensions = ref new Platform::Collections::Vector<Platform::String^>();
    pngExtensions->Append(".png");

    auto savePicker = ref new FileSavePicker();

    savePicker->SuggestedStartLocation = Windows::Storage::Pickers::PickerLocationId::PicturesLibrary; // or could use Desktop;

    savePicker->FileTypeChoices->Insert("Portable Network Graphics", pngExtensions);

    savePicker->SuggestedFileName = "Hyperbolic";


    create_task(savePicker->PickSaveFileAsync()).then([](StorageFile^ file)
    {
            if (file)
            {

                auto folder = Windows::Storage::ApplicationData::Current->TemporaryFolder;

                auto task = create_task(folder->GetFileAsync("hyperbolic.png")); // formerly jpg;


                task.then([file](StorageFile^ tempFile) ->IAsyncAction^
                    {
                        return tempFile->MoveAndReplaceAsync(file);
                    });
            }
    });


}



