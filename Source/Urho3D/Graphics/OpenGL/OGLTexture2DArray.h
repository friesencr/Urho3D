//
// Copyright (c) 2008-2015 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "../../Graphics/RenderSurface.h"
#include "../../Container/Ptr.h"
#include "../../Graphics/Texture.h"

namespace Urho3D
{

class Image;
class XMLFile;

/// 2D texture resource.
class URHO3D_API Texture2DArray : public Texture
{
    OBJECT(Texture2DArray);
    
public:
    /// Construct.
    Texture2DArray(Context* context);
    /// Destruct.
    virtual ~Texture2DArray();
    /// Register object factory.
    static void RegisterObject(Context* context);
    
    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    virtual bool EndLoad();
    /// Mark the GPU resource destroyed on context destruction.
    virtual void OnDeviceLost();
    /// Recreate the GPU resource and restore data if applicable.
    virtual void OnDeviceReset();
    /// Release the texture.
    virtual void Release();
    
    /// Set size, format and usage. Zero size will follow application window size. Return true if successful.
    bool SetSize(int width, int height, int layers, unsigned format, TextureUsage usage = TEXTURE_STATIC);
    /// Set data either partially or fully on a mip level. Return true if successful.
    bool SetData(unsigned level, int x, int y, int width, int height, int depth, const void* data);
    /// Set data from a list of images. Return true if successful. Optionally make a single channel image alpha-only.
    bool SetData(Vector<SharedPtr<Image> > images, bool useAlpha = false);
    /// Set data from an image. Return true if successful. Optionally make a single channel image alpha-only.
    bool SetData(unsigned depth, SharedPtr<Image> image, bool useAlpha = false);
    /// Get data from a mip level. The destination buffer must be big enough. Return true if successful.
    bool GetData(unsigned level, unsigned depth, void* dest) const;
    
protected:
    /// Create texture.
    virtual bool Create();
    
private:
    /// Image file acquired during BeginLoad.
    Vector<SharedPtr<Image> > loadImages_;
    /// Parameter file acquired during BeginLoad.
    SharedPtr<XMLFile> loadParameters_;
    /// Number of texture in array
    unsigned layers_;
	/// Holds memory usage for all images
	Vector<int> memoryUsages_;
};

}
