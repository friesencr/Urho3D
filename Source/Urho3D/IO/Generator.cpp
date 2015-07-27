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
//
#include "Generator.h"
#include "Log.h"

namespace Urho3D
{

/// Generator function registry.
static HashMap<String, GeneratorFunction> generatorFunctionRegistry_;

Generator::Generator() :
	generatorFunction_(0)
{
}

Generator::~Generator()
{
}

void Generator::RegisterGeneratorFunction(String functionName, GeneratorFunction generatorFunction)
{
    generatorFunctionRegistry_[functionName] = generatorFunction;
}


//bool Generator::BeginLoad(Deserializer& source)
//{
//    if (source.ReadFileID() != "GENR")
//    {
//        LOGERROR(source.GetName() + " is not a valid generator file");
//        return false;
//    }
//
//    parameters_.Clear();
//    generatorFunction_ = 0;
//
//    unsigned memoryUse = sizeof(Generator);
//    SetName(source.ReadString());
//    SetSize(source.ReadUInt());
//    parameters_ = source.ReadVariantMap();
//    return true;
//}

unsigned Generator::Read(void* dest, unsigned size)
{
    if (!generatorFunction_)
        return 0;

    unsigned bytesRead = generatorFunction_(dest, size, position_, parameters_);
    position_ += bytesRead;
    return bytesRead;
}

unsigned Generator::Seek(unsigned position)
{
    return 0;
}

void Generator::SetName(const String& name)
{
    generatorFunction_ = generatorFunctionRegistry_[name];
}

void Generator::SetParameters(VariantMap parameters)
{
    parameters_ = parameters;
}

//bool Generator::Save(Serializer& dest) const
//{
//    // Write ID
//    if (!dest.WriteFileID("GENR"))
//        return false;
//
//    dest.WriteString(Resource::GetName());
//    dest.WriteUInt(GetSize());
//    dest.WriteVariantMap(parameters_);
//
//    return true;
//}

}
