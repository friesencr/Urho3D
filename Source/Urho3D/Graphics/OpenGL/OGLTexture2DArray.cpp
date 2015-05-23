//
// Copyright (c) 2008-2015 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rightsR
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

#include "../../Core/Context.h"
#include "../../IO/FileSystem.h"
#include "../../Graphics/Graphics.h"
#include "../../Graphics/GraphicsEvents.h"
#include "../../Graphics/GraphicsImpl.h"
#include "../../Resource/Image.h"
#include "../../IO/Log.h"
#include "../../Core/Profiler.h"
#include "../../Graphics/Renderer.h"
#include "../../Resource/ResourceCache.h"
#include "../../Graphics/Texture2DArray.h"
#include "../../Resource/XMLFile.h"

#include "../../DebugNew.h"

namespace Urho3D
{

Texture2DArray::Texture2DArray(Context* context) :
    Texture(context)
{
    target_ = GL_TEXTURE_2D_ARRAY;
}

Texture2DArray::~Texture2DArray()
{
    Release();
}

void Texture2DArray::RegisterObject(Context* context)
{
    context->RegisterFactory<Texture2DArray>();
}

bool Texture2DArray::BeginLoad(Deserializer& source)
{

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    
    // In headless mode, do not actually load the texture, just return success
    if (!graphics_)
        return true;
    
    // If device is lost, retry later
    if (graphics_->IsDeviceLost())
    {
        LOGWARNING("Texture load while device is lost");
        dataPending_ = true;
        return true;
    }
    
    cache->ResetDependencies(this);

    String texPath, texName, texExt;
    SplitPath(GetName(), texPath, texName, texExt);
    
    loadParameters_ = (new XMLFile(context_));
    if (!loadParameters_->Load(source))
    {
        loadParameters_.Reset();
        return false;
    }
    
    loadImages_.Clear();

    XMLElement textureElem = loadParameters_->GetRoot();
    XMLElement imageElem = textureElem.GetChild("image");
    // Single image and multiple faces with layout
    if (imageElem)
    {
        XMLElement layerElem = textureElem.GetChild("layer");
        while (layerElem)
        {
            String name = layerElem.GetAttribute("name");
            
            // If path is empty, add the XML file path
            if (GetPath(name).Empty())
                name = texPath + name;
            
            loadImages_.Push(cache->GetTempResource<Image>(name));
            cache->StoreResourceDependency(this, name);
            
            layerElem = layerElem.GetNext("layer");
        }
    }
    
    // Precalculate mip levels if async loading
    if (GetAsyncLoadState() == ASYNC_LOADING)
    {
        for (unsigned i = 0; i < loadImages_.Size(); ++i)
        {
            if (loadImages_[i])
                loadImages_[i]->PrecalculateLevels();
        }
    }

    return true;
}

bool Texture2DArray::EndLoad()
{
    // In headless mode, do not actually load the texture, just return success
    if (!graphics_ || graphics_->IsDeviceLost())
        return true;
    
    // If over the texture budget, see if materials can be freed to allow textures to be freed
    CheckTextureBudget(GetTypeStatic());

    SetParameters(loadParameters_);
    
    for (unsigned i = 0; i < loadImages_.Size(); ++i)
        SetData(i, loadImages_[i]);
    
    loadImages_.Clear();
    loadParameters_.Reset();
    
    return true;
}

void Texture2DArray::OnDeviceLost()
{
    GPUObject::OnDeviceLost();
}

void Texture2DArray::OnDeviceReset()
{
    if (!object_ || dataPending_)
    {
        // If has a resource file, reload through the resource cache. Otherwise just recreate.
        ResourceCache* cache = GetSubsystem<ResourceCache>();
        if (cache->Exists(GetName()))
            dataLost_ = !cache->ReloadResource(this);

        if (!object_)
        {
            Create();
            dataLost_ = true;
        }
    }
    
    dataPending_ = false;
}

void Texture2DArray::Release()
{
    if (object_)
    {
        if (!graphics_)
            return;
        
        if (!graphics_->IsDeviceLost())
        {
            for (unsigned i = 0; i < MAX_TEXTURE_UNITS; ++i)
            {
                if (graphics_->GetTexture(i) == this)
                    graphics_->SetTexture(i, 0);
            }
            
            glDeleteTextures(1, &object_);
        }
        
        object_ = 0;
    }
}

bool Texture2DArray::SetSize(int width, int height, int layers, unsigned format, TextureUsage usage)
{
    usage_ = usage;
    
    if (usage >= TEXTURE_RENDERTARGET)
    {
        LOGERROR("Could not set texure usage.  Only static/dynamic usages are allowed.");
        return false;
    }
    
    width_ = width;
    height_ = height;
    layers_ = layers;
    format_ = format;
    memoryUsages_.Resize(layers);
    
    return Create();
}

bool Texture2DArray::SetData(unsigned level, int x, int y, int width, int height, int layer, const void* data)
{
    PROFILE(SetTextureData);

    if (!object_ || !graphics_)
    {
        LOGERROR("No texture created, can not set data");
        return false;
    }

    if (!width_ || !height_ || layers_)
    {
        LOGERROR("Texture dimensions are not set");
        return false;
    }
    
    if (!data)
    {
        LOGERROR("Null source for setting data");
        return false;
    }
    
    if (level >= levels_)
    {
        LOGERROR("Illegal mip level for setting data");
        return false;
    }
    
    if (graphics_->IsDeviceLost())
    {
        LOGWARNING("Texture data assignment while device is lost");
        dataPending_ = true;
        return true;
    }
    
    if (IsCompressed())
    {
        x &= ~3;
        y &= ~3;
    }
    
    int levelWidth = GetLevelWidth(level);
    int levelHeight = GetLevelHeight(level);
    if (x < 0 || x + width > levelWidth || y < 0 || y + height > levelHeight || width <= 0 || height <= 0)
    {
        LOGERROR("Illegal dimensions for setting data");
        return false;
    }
    
    graphics_->SetTextureForUpdate(this);
    
    unsigned format = GetSRGB() ? GetSRGBFormat(format_) : format_;

    if (!IsCompressed())
    {
        glTexSubImage3D(target_, level, x, y, 0, width, height, layer, GetExternalFormat(format_), GetDataType(format_), data);
    }
    else
    {
        glCompressedTexSubImage3D(target_, level, x, y, 0, width, height, layer, format, GetDataSize(width, height, layer), data);
    }
    
    graphics_->SetTexture(0, 0);
    return true;
}

bool Texture2DArray::SetData(unsigned layer, SharedPtr<Image> image, bool useAlpha)
{
    if (!image)
    {
        LOGERROR("Null image, can not set data");
        return false;
    }

    if (!object_)
    {
        LOGERROR("Texture must be created first.");
        return false;
    }

    unsigned memoryUse = 0;
    
    int quality = QUALITY_HIGH;
    Renderer* renderer = GetSubsystem<Renderer>();
    if (renderer)
        quality = renderer->GetTextureQuality();
    
    if (!image->IsCompressed())
    {
        // Convert unsuitable formats to RGBA
        unsigned components = image->GetComponents();
        if (Graphics::GetGL3Support() && ((components == 1 && !useAlpha) || components == 2))
        {
            image = image->ConvertToRGBA();
            if (!image)
                return false;
            components = image->GetComponents();
        }

        unsigned char* levelData = image->GetData();
        int levelWidth = image->GetWidth();
        int levelHeight = image->GetHeight();
        unsigned format = 0;
        
        // Discard unnecessary mip levels
        for (unsigned i = 0; i < mipsToSkip_[quality]; ++i)
        {
            image = image->GetNextLevel();
            levelData = image->GetData();
            levelWidth = image->GetWidth();
            levelHeight = image->GetHeight();
        }
        
        switch (components)
        {
        case 1:
            format = useAlpha ? Graphics::GetAlphaFormat() : Graphics::GetLuminanceFormat();
            break;
            
        case 2:
            format = Graphics::GetLuminanceAlphaFormat();
            break;
            
        case 3:
            format = Graphics::GetRGBFormat();
            break;
            
        case 4:
            format = Graphics::GetRGBAFormat();
            break;
        }
        
        // If image was previously compressed, reset number of requested levels to avoid error if level count is too high for new size
        if (IsCompressed() && requestedLevels_ > 1)
            requestedLevels_ = 0;

        if (!object_)
            return false;
        
        for (unsigned i = 0; i < levels_; ++i)
        {
            SetData(i, 0, 0, levelWidth, levelHeight, layer, levelData);
            memoryUse += levelWidth * levelHeight * components;
            
            if (i < levels_ - 1)
            {
                image = image->GetNextLevel();
                levelData = image->GetData();
                levelWidth = image->GetWidth();
                levelHeight = image->GetHeight();
            }
        }
    }
    else
    {
        int width = image->GetWidth();
        int height = image->GetHeight();
        unsigned levels = image->GetNumCompressedLevels();
        unsigned format = graphics_->GetFormat(image->GetCompressedFormat());
        bool needDecompress = false;
        
        if (!format)
        {
            format = Graphics::GetRGBAFormat();
            needDecompress = true;
        }
        
        unsigned mipsToSkip = mipsToSkip_[quality];
        if (mipsToSkip >= levels)
            mipsToSkip = levels - 1;
        while (mipsToSkip && (width / (1 << mipsToSkip) < 4 || height / (1 << mipsToSkip) < 4))
            --mipsToSkip;
        width /= (1 << mipsToSkip);
        height /= (1 << mipsToSkip);
        
        SetNumLevels(Max((int)(levels - mipsToSkip), 1));
        
        for (unsigned i = 0; i < levels_ && i < levels - mipsToSkip; ++i)
        {
            CompressedLevel level = image->GetCompressedLevel(i + mipsToSkip);
            if (!needDecompress)
            {
                SetData(i, 0, 0, level.width_, level.height_, layer, level.data_);
                memoryUse += level.rows_ * level.rowSize_;
            }
            else
            {
                unsigned char* rgbaData = new unsigned char[level.width_ * level.height_ * 4];
                level.Decompress(rgbaData);
                SetData(i, 0, 0, level.width_, level.height_, layer, rgbaData);
                memoryUse += level.width_ * level.height_ * 4;
                delete[] rgbaData;
            }
        }
    }
    
    memoryUsages_[layer] = memoryUse;
    int sum = 0;
    for (unsigned i = 0; i < memoryUsages_.Size(); ++i)
        sum += memoryUsages_[i];

    SetMemoryUse(sum);
    return true;
}

bool Texture2DArray::SetData(Vector<SharedPtr<Image> > images, bool useAlpha)
{
    unsigned format = 0;
    {
        SharedPtr<Image> image = images.Front();
        if (image->IsCompressed())
        {
            format = graphics_->GetFormat(image->GetCompressedFormat());
            if (!format)
                format = Graphics::GetRGBAFormat();
        }
        else
        {
            unsigned components = image->GetComponents();
            switch (components)
            {
            case 1:
                format = useAlpha ? Graphics::GetAlphaFormat() : Graphics::GetLuminanceFormat();
                break;

            case 2:
                format = Graphics::GetLuminanceAlphaFormat();
                break;

            case 3:
                format = Graphics::GetRGBFormat();
                break;

            case 4:
                format = Graphics::GetRGBAFormat();
                break;
            }
        }
        if (!SetSize(image->GetWidth(), image->GetHeight(), images.Size(), format))
            return false;
    }

    bool success = true;
    for (unsigned i = 0; i < images.Size(); ++i)
    {
        SharedPtr<Image> image = images[i];
        success = SetData(i, image);

    }
}

bool Texture2DArray::GetData(unsigned level, unsigned layer, void* dest) const
{
    #ifndef GL_ES_VERSION_2_0
    if (!object_ || !graphics_)
    {
        LOGERROR("No texture created, can not get data");
        return false;
    }
    
    if (!dest)
    {
        LOGERROR("Null destination for getting data");
        return false;
    }
    
    if (level >= levels_)
    {
        LOGERROR("Illegal mip level for getting data");
        return false;
    }
    
    if (graphics_->IsDeviceLost())
    {
        LOGWARNING("Getting texture data while device is lost");
        return false;
    }
    
    graphics_->SetTextureForUpdate(const_cast<Texture2DArray*>(this));
    
    if (!IsCompressed())
        glGetTexImage(target_, level, GetExternalFormat(format_), GetDataType(format_), dest);
    else
        glGetCompressedTexImage(target_, level, dest);
    
    graphics_->SetTexture(0, 0);
    return true;
    #else
    LOGERROR("Getting texture data not supported");
    return false;
    #endif
}

bool Texture2DArray::Create()
{
    Release();
    
    if (!graphics_ || !width_ || !height_ || layers_)
        return false;
    
    if (graphics_->IsDeviceLost())
    {
        LOGWARNING("Texture creation while device is lost");
        return true;
    }

    unsigned format = GetSRGB() ? GetSRGBFormat(format_) : format_;
    unsigned externalFormat = GetExternalFormat(format_);
    unsigned dataType = GetDataType(format_);

    glGenTextures(1, &object_);
    
    // Ensure that our texture is bound to OpenGL texture unit 0
    graphics_->SetTextureForUpdate(this);
    
    // If not compressed, create the initial level 0 texture with null data
    bool success = true;
    
    if (!IsCompressed())
    {
        glGetError();
        glTexImage3D(target_, 0, format, width_, height_, layers_, 0, externalFormat, dataType, 0);
        if (glGetError())
        {
            LOGERROR("Failed to create texture");
            success = false;
        }
    }
    
    // Set mipmapping
    levels_ = requestedLevels_;
    if (!levels_)
    {
        unsigned maxSize = Max((int)width_, (int)height_);
        while (maxSize)
        {
            maxSize >>= 1;
            ++levels_;
        }
    }
    
    // Set initial parameters, then unbind the texture
    UpdateParameters();
    graphics_->SetTexture(0, 0);
    
    return success;
}

}
