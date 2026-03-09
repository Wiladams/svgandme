#pragma once

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>  // For Microsoft::WRL::ComPtr
#include <vector>
#include <iostream>

// Link libraries
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#include "pixelaccessor.h"
#include "maths.h"

using Microsoft::WRL::ComPtr;

namespace waavs {
    struct D3DScreenSnapshot : public PixelAccessor<vec4b>
    {
        BLImage fImage{};
        
        ComPtr<ID3D11Device>           gDevice{};
        ComPtr<ID3D11DeviceContext>    gContext{};
        ComPtr<IDXGIOutputDuplication> gDeskDupl{};

    private:
        bool initD3D()
        {
            // Create D3D11 device
            UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
            D3D_FEATURE_LEVEL featureLevel;
            ComPtr<ID3D11Device> device;
            ComPtr<ID3D11DeviceContext> context;

            HRESULT hr = D3D11CreateDevice(
                nullptr,                    // pAdapter (use default)
                D3D_DRIVER_TYPE_HARDWARE,   // Driver type
                nullptr,                    // Software
                creationFlags,              // Flags
                nullptr, 0,                 // Feature levels
                D3D11_SDK_VERSION,
                &device,
                &featureLevel,
                &context);

            if (FAILED(hr))
            {
                //std::cerr << "Failed to create D3D11 Device.\n";
                return;
            }

            gDevice = device;
            gContext = context;
        }
        
        bool initDesktopDuplication()
        {
            // Obtain DXGI device from D3D11 device
            ComPtr<IDXGIDevice> dxgiDevice;
            HRESULT hr = gDevice.As(&dxgiDevice);
            if (FAILED(hr)) return false;

            // Get DXGI adapter
            ComPtr<IDXGIAdapter> dxgiAdapter;
            hr = dxgiDevice->GetAdapter(&dxgiAdapter);
            if (FAILED(hr)) return false;

            // Enumerate outputs of that adapter
            ComPtr<IDXGIOutput> dxgiOutput;
            hr = dxgiAdapter->EnumOutputs(0, &dxgiOutput);  // typically index 0 for primary
            if (hr == DXGI_ERROR_NOT_FOUND)
            {
                std::cerr << "No output found on adapter.\n";
                return false;
            }

            // Query for IDXGIOutput1
            ComPtr<IDXGIOutput1> dxgiOutput1;
            hr = dxgiOutput.As(&dxgiOutput1);
            if (FAILED(hr)) return false;

            // Duplicate the output
            hr = dxgiOutput1->DuplicateOutput(gDevice.Get(), &gDeskDupl);
            if (FAILED(hr))
            {
                std::cerr << "DuplicateOutput failed.\n";
                return false;
            }

            return true;
        }

    public:
        D3DScreenSnapshot()
        {
            initD3D();
            initDesktopDuplication();
            
        }
        

        bool isValid() const
        {
			return ((gDevice != nullptr) && (gContext != nullptr) && (gDeskDupl != nullptr));
        }
        
        // Capture a single frame
        bool update()
        {
            if (!isValid())
                return false;

            // Time-out value in milliseconds (how long to wait for a new frame)
            UINT timeout = 100;
            ComPtr<IDXGIResource> desktopResource;
            DXGI_OUTDUPL_FRAME_INFO frameInfo;
            ZeroMemory(&frameInfo, sizeof(frameInfo));

            HRESULT hr = gDeskDupl->AcquireNextFrame(timeout, &frameInfo, &desktopResource);
            if (hr == DXGI_ERROR_WAIT_TIMEOUT)
            {
                // No new frame within the timeout period.
                return false;
            }
            else if (FAILED(hr))
            {
                std::cerr << "AcquireNextFrame failed. HR=0x" << std::hex << hr << "\n";
                return false;
            }

            // Query for ID3D11Texture2D from the resource
            ComPtr<ID3D11Texture2D> acquiredTex;
            hr = desktopResource.As(&acquiredTex);
            if (FAILED(hr))
            {
                gDeskDupl->ReleaseFrame();
                return false;
            }

            // Now acquiredTex is the new desktop texture. 
            // If you need CPU access, create a staging resource & copy.

            // 1) Describe a staging texture for CPU read
            D3D11_TEXTURE2D_DESC desc;
            acquiredTex->GetDesc(&desc);
            desc.Usage = D3D11_USAGE_STAGING;
            desc.BindFlags = 0;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            desc.MiscFlags = 0;

            ComPtr<ID3D11Texture2D> stagingTex;
            hr = gDevice->CreateTexture2D(&desc, nullptr, &stagingTex);
            if (FAILED(hr))
            {
                gDeskDupl->ReleaseFrame();
                return false;
            }

            // 2) Copy from GPU desktop texture to our CPU staging texture
            gContext->CopyResource(stagingTex.Get(), acquiredTex.Get());

            // 3) Map the staging texture so we can access the pixels
            D3D11_MAPPED_SUBRESOURCE mapped;
            hr = gContext->Map(stagingTex.Get(), 0, D3D11_MAP_READ, 0, &mapped);
            if (SUCCEEDED(hr))
            {
                // Now mapped.pData is the pointer to the pixel data
                // mapped.RowPitch is the number of bytes per row

                // Do what you want with the data here—copy it into your BLImage, etc.
                // e.g., 
                //   auto bytes = (BYTE*)mapped.pData;
                //   for (UINT row = 0; row < desc.Height; row++)
                //   {
                //       // Each row has rowPitch bytes
                //       // ...
                //       bytes += mapped.RowPitch;
                //   }

                gContext->Unmap(stagingTex.Get(), 0);
            }

            // 4) Release the frame
            gDeskDupl->ReleaseFrame();
            return true;
        }

        const BLImage& getImage() const
        {
            return fImage;
        }
    };
}