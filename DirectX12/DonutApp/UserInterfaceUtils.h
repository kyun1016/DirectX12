/*
* Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "../DonutCore/math.h"
#include <string>

namespace donut::engine
{
    struct Material;
    class Light;
    class DirectionalLight;
    class PointLight;
    class SpotLight;
}

namespace donut::app
{
    bool FileDialog(bool bOpen, const char* pFilters, std::string& fileName);
    bool FolderDialog(const char* pTitle, const char* pDefaultFolder, std::string& outFolderName);
    
    bool MaterialEditor(engine::Material* material, bool allowMaterialDomainChanges);

    bool LightEditor_Directional(engine::DirectionalLight& light);
    bool LightEditor_Point(engine::PointLight& light);
    bool LightEditor_Spot(engine::SpotLight& light);
    bool LightEditor(engine::Light& light);

    bool AzimuthElevationSliders(math::double3& direction, bool negative = false);
}
