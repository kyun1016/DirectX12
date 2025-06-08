#pragma once
#include "ECSConfig.h"
#include "DX12_Core.h"

struct RenderTargetComponent {
    ECS::RepoHandle rtvHandle;
    ECS::RepoHandle dsvHandle;
};