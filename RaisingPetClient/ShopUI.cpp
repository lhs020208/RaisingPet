#include "stdafx.h"
#include "ShopUI.h"
#include "GameFramework.h"
#include "DDSTextureLoader12.h"
#include "d3dx12.h"

extern CGameFramework* g_pFramework;

namespace
{
#include "ShopUI.Internal.h"
}

#include "ShopUI.Resources.cpp"
#include "ShopUI.Pages.cpp"
#include "ShopUI.Render.cpp"
#include "ShopUI.Layout.cpp"
#include "ShopUI.State.cpp"
#include "ShopUI.FinancialAndLogs.cpp"
#include "ShopUI.Input.cpp"
