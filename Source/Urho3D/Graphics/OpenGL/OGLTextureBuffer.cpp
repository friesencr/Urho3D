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
#include "../../Graphics/TextureBuffer.h"
#include "../../Resource/XMLFile.h"

#include "../../DebugNew.h"

namespace Urho3D
{

TextureBuffer::TextureBuffer(Context* context) :
    Texture(context)
{
    target_ = GL_TEXTURE_BUFFER;
}

TextureBuffer::~TextureBuffer()
{
    Release();
}

void TextureBuffer::RegisterObject(Context* context)
{
    context->RegisterFactory<TextureBuffer>();
}

bool TextureBuffer::BeginLoad(Deserializer& source)
{
    return false;
}

bool TextureBuffer::EndLoad()
{
    return false;
}

void TextureBuffer::OnDeviceLost()
{
    GPUObject::OnDeviceLost();
}

void TextureBuffer::OnDeviceReset()
{
    if (!object_ || dataPending_)
    {
        if (!object_)
        {
            Create();
            dataLost_ = true;
        }
    }
    
    dataPending_ = false;
}

void TextureBuffer::UpdateParameters()
{

}

void TextureBuffer::Release()
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
    if (indexBuffer_)
        indexBuffer_ = 0;
}

bool TextureBuffer::SetSize(unsigned format, TextureUsage usage)
{
    usage_ = usage;
	format_ = GL_R32UI;
    
    return Create();
}

bool TextureBuffer::SetData(IndexBuffer* indexBuffer)
{
    PROFILE(SetTextureData);

    if (!object_ || !graphics_)
    {
        LOGERROR("No texture created, can not set data");
        return false;
    }
    
    if (!indexBuffer || !indexBuffer->GetGPUObject())
    {
        LOGERROR("Null source for setting data");
        return false;
    }

    if (!format_)
    {
        LOGERROR("Format is required.");
        return false;
    }
    
    if (graphics_->IsDeviceLost())
    {
        LOGWARNING("Texture data assignment while device is lost");
        dataPending_ = true;
        return true;
    }
    
    indexBuffer_ = indexBuffer;
    graphics_->SetTextureForUpdate(this);
    glTexBuffer(target_, format_, indexBuffer->GetGPUObject());
    graphics_->SetTexture(0, 0);
    return true;
}

bool TextureBuffer::Create()
{
    Release();
    
    if (!graphics_)
        return false;
    
    if (graphics_->IsDeviceLost())
    {
        LOGWARNING("Texture creation while device is lost");
        return true;
    }

    glGenTextures(1, &object_);
    
    return true;
}

}
