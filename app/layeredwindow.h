#pragma once

/*
    AlphaWindow

    Create windows that honor the alpha channel of an 
    image as it displays.

https://docs.microsoft.com/en-us/archive/msdn-magazine/2009/december/windows-with-c-layered-windows-with-direct2d
https://docs.microsoft.com/en-us/archive/msdn-magazine/2014/june/windows-with-c-high-performance-window-layering-using-the-windows-composition-engine
*/
#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "nativewindow.h"


class LayeredWindowInfo
{
public:
    POINT fSourcePosition;
    POINT fWindowPosition;
    SIZE fSize;
    BLENDFUNCTION   fBlendFunction;
    UPDATELAYEREDWINDOWINFO fInfo;
    int fLastError;


    LayeredWindowInfo(int width, int height)
        :fSourcePosition(),
        fWindowPosition(),
        fSize({width, height}),
        fBlendFunction(),
        fInfo(),
        fLastError(0)
    {
        // When SourceConstantAlpha == 255
        // the layered window will use per pixel
        // alpha when compositing
        // AC_SRC_ALPHA indicates the source bitmap has
        // an alpha channel
        //fBlendFunction.BlendOp = AC_SRC_OVER;
        //fBlendFunction.BlendFlags = 0;
        fBlendFunction.SourceConstantAlpha = 255;
        fBlendFunction.AlphaFormat = AC_SRC_ALPHA;
    
        fInfo.cbSize = sizeof(UPDATELAYEREDWINDOWINFO);
        fInfo.pptSrc = &fSourcePosition;
        fInfo.pptDst = &fWindowPosition;
        fInfo.psize = &fSize;
        fInfo.pblend = &fBlendFunction;
        fInfo.dwFlags = ULW_ALPHA;

        //fInfo.hdcSrc = fSourceDC;  // DC of dibsection

    }

    // an alpha value of 255 means we'll use the per pixel alpha
    // values.  Anything less than this, and we'll use the specified
    // value for the whole window, rather than doing per  pixel
    // alpha blending.
    void setGlobalAlpha(BYTE alpha)
    {
        fBlendFunction.SourceConstantAlpha = alpha;
    }
    
    int getLastError() const {return fLastError;}
    int getWidth() const {return fSize.cx;}
    int getHeight() const {return fSize.cy;}

    // Called when the window is supposed to display itself
    bool display(HWND win, HDC source)
    {
        fInfo.hdcSrc = source;
        RECT wRect;
        BOOL bResult = ::GetWindowRect(win, &wRect);
        
        fWindowPosition.x = wRect.left;
        fWindowPosition.y = wRect.top;
        
        bResult = ::UpdateLayeredWindowIndirect(win, &fInfo);

        if (!bResult) {
            fLastError = ::GetLastError();
            return false;
        }

        return true;
    }



};
