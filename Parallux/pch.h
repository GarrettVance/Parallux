
#pragma once

#include <wrl.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <memory>
#include <agile.h>  // ghv : if want to #include <agile.h> then must compile with /ZW.
#include <ppltasks.h>       //  ghv : for asynchronous file reader tasks;
#include <concrt.h>

#include <complex>    
#include <vector>    
#include <random>    //  ghv : for Mersenne twister;
#include "Keyboard.h"
#include "Mouse.h"
#include "WICTextureLoader.h" 
#include "ScreenGrab.h"
#include <collection.h>

#pragma comment(lib, "DirectXTK")


#undef GHV_OPTION_DUAL_VIEWPORTS // Tested and  working as of 20190727;


namespace HvyDXBase
{
    extern double const konst_pi;

    using HvyPlex = std::complex<double>;
}


template <typename Fn>
struct _Finally : public Fn
{
    _Finally(Fn&& Func) : Fn(std::forward<Fn>(Func)) {}

    _Finally(const _Finally&); // generate link error if copy constructor is called

    ~_Finally() { this->operator()(); }
};


template <typename Fn>
inline _Finally<Fn> Finally(Fn&& Func)
{
    return { std::forward<Fn>(Func) };
}


