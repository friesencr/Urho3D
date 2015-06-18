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

#include "../IO/Deserializer.h"
#include "../IO/Serializer.h"
#include "../Resource/Resource.h"
#include "../Core/Object.h"
#include "MemoryBuffer.h"

namespace Urho3D
{

typedef unsigned (*GeneratorFunction)(Context* context, MemoryBuffer& buffer, unsigned size, unsigned position, VariantMap& parameters);

/// %File opened either through the filesystem or from within a package file.
class URHO3D_API Generator : public Deserializer, public Resource
{
    OBJECT(Generator);
    BASEOBJECT(Resource);

public:
    /// Construct.
    Generator(Context* context);
    /// Destruct. Close the file if open.
    virtual ~Generator();
    /// Registers a generator function.
    static void RegisterGeneratorFunction(String functionName, GeneratorFunction generatorFunction);
    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);
    /// Read bytes from the file. Return number of bytes actually read.
    virtual unsigned Read(void* dest, unsigned size);
    /// Set position from the beginning of the file.
    virtual unsigned Seek(unsigned position);
    /// Save resource. Return true if successful.
    virtual bool Save(Serializer& dest) const;
    /// Change the function name.
    virtual void SetName(const String& name);
    /// Sets the generator function parameters.
    virtual void SetParameters(VariantMap parameters);
    /// Sets the generator data size
    virtual void SetSize(unsigned size) { size_ = size; }
    
private:
    /// Parameters.
    VariantMap parameters_;
    /// Generator function hash.
    StringHash generatorFunctionHash_;
    /// Generator function.
    GeneratorFunction generatorFunction_;
};

}
