//-----------------------------------------------------------------------------
// File: Scene.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Scene.h"
#include "GameFramework.h"
extern CGameFramework* g_pFramework;

CScene::CScene(CPlayer* pPlayer)
{
	m_pPlayer = pPlayer;
}

CScene::~CScene()
{
}

void CScene::BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
	
}

ID3D12RootSignature *CScene::CreateGraphicsRootSignature(ID3D12Device *pd3dDevice)
{
	ID3D12RootSignature *pd3dGraphicsRootSignature = NULL;

	D3D12_ROOT_PARAMETER pd3dRootParameters[3];
	pd3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[0].Constants.Num32BitValues = 4; //Time, ElapsedTime, xCursor, yCursor
	pd3dRootParameters[0].Constants.ShaderRegister = 0; //Time
	pd3dRootParameters[0].Constants.RegisterSpace = 0;
	pd3dRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[1].Constants.Num32BitValues = 19; //16 + 3
	pd3dRootParameters[1].Constants.ShaderRegister = 1; //World
	pd3dRootParameters[1].Constants.RegisterSpace = 0;
	pd3dRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[2].Constants.Num32BitValues = 35; //16 + 16 + 3
	pd3dRootParameters[2].Constants.ShaderRegister = 2; //Camera
	pd3dRootParameters[2].Constants.RegisterSpace = 0;
	pd3dRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc;
	::ZeroMemory(&d3dRootSignatureDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));
	d3dRootSignatureDesc.NumParameters = _countof(pd3dRootParameters);
	d3dRootSignatureDesc.pParameters = pd3dRootParameters;
	d3dRootSignatureDesc.NumStaticSamplers = 0;
	d3dRootSignatureDesc.pStaticSamplers = NULL;
	d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;

	ID3DBlob *pd3dSignatureBlob = NULL;
	ID3DBlob *pd3dErrorBlob = NULL;
	D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pd3dSignatureBlob, &pd3dErrorBlob);
	pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void **)&pd3dGraphicsRootSignature);
	if (pd3dSignatureBlob) pd3dSignatureBlob->Release();
	if (pd3dErrorBlob) pd3dErrorBlob->Release();

	return(pd3dGraphicsRootSignature);
}

void CScene::ReleaseObjects()
{
	
}

void CScene::ReleaseUploadBuffers()
{

}

void CScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
}
void CScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
}

bool CScene::ProcessInput()
{
	return(false);
}

void CScene::Animate(float fTimeElapsed)
{
}

void CScene::PrepareRender(ID3D12GraphicsCommandList* pd3dCommandList)
{
	pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
}

void CScene::Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera)
{
}
void CScene::BuildGraphicsRootSignature(ID3D12Device* pd3dDevice)
{
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);
}

//ÅÊÅ© Scene////////////////////////////////////////////////////////////////////////////////////////////////
CTankScene::CTankScene(CPlayer* pPlayer) : CScene(pPlayer) {}
void CTankScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);
	CPseudoLightingShader* pShader = new CPseudoLightingShader();
	pShader->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature);
	pShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	using namespace std;
	default_random_engine dre{ random_device{}() };
	uniform_real_distribution<float> uid{ 0.0f,1.0f };

	uniform_real_distribution<float> uid_x{ 0,18.0f };
	uniform_real_distribution<float> uid_z{ 0,18.0f };
	uniform_int_distribution<int> uid_x_int(-9, 9);
	uniform_int_distribution<int> uid_z_int(-9, 9);
	uniform_real_distribution<float> uid_rot{ 0,360.0f };

	for (int i = 0; i < m_nTanks; i++)
	{
		float red = uid(dre);
		float green = uid(dre);
		float blue = uid(dre);

		m_pTank[i] = nullptr;
		m_pTank[i] = new CTankObject();
		CMesh* pTankMesh = new CMesh(pd3dDevice, pd3dCommandList, "Models/Tank.obj");
		m_pTank[i]->SetMesh(pTankMesh);
		m_pTank[i]->SetShader(pShader);
		m_pTank[i]->SetColor(XMFLOAT3(red, green, blue));
		m_pTank[i]->SetPosition(uid_x(dre) - 9.0f, 0.0f, uid_z(dre) - 9.0f);
		m_pTank[i]->Rotate(0.0f, uid_rot(dre), 0.0f);
		m_pTank[i]->UpdateBoundingBox();

		m_pTank[i]->bullet = new CGameObject();
		CMesh* pMesh = new CMesh(pd3dDevice, pd3dCommandList, "Models/Bullet.obj");
		m_pTank[i]->bullet->SetMesh(pMesh);
		m_pTank[i]->bullet->SetColor(XMFLOAT3(red, green, blue));
		m_pTank[i]->bullet->SetPosition(-2.0f + 0.5f * i, 0.0f, 1.0f);
		m_pTank[i]->bullet->SetShader(pShader);
		m_pTank[i]->bullet->UpdateBoundingBox();

		CCubeMesh* pCubeMesh = new CCubeMesh(pd3dDevice, pd3dCommandList, 0.05f, 0.05f, 0.05f);
		m_pTank[i]->m_pExplosionObjects = new CExplosionObject();
		m_pTank[i]->m_pExplosionObjects->SetMesh(pCubeMesh);
		m_pTank[i]->m_pExplosionObjects->SetShader(pShader);
		m_pTank[i]->m_pExplosionObjects->SetColor(XMFLOAT3(red, green, blue));
		m_pTank[i]->m_pExplosionObjects->SetPosition(0.0f, 0.0f, 1.0f);
		m_pTank[i]->m_pExplosionObjects->UpdateBoundingBox();
	}
	for (int i = 0; i < m_nCubeObjects; i++) {
		CCubeMesh* pCubeMesh = new CCubeMesh(pd3dDevice, pd3dCommandList, 1.0f, 1.0f, 1.0f);
		m_pCubeObjects[i] = new CCubeObject();
		m_pCubeObjects[i]->SetMesh(pCubeMesh);
		m_pCubeObjects[i]->SetShader(pShader);
		m_pCubeObjects[i]->SetColor(XMFLOAT3(0.0f, 0.0f, 1.0f));
		m_pCubeObjects[i]->SetPosition((float)uid_x_int(dre), 0.3f, (float)uid_z_int(dre));
		m_pCubeObjects[i]->UpdateBoundingBox();
	}
	CCubeMesh* pCubeMesh = new CCubeMesh(pd3dDevice, pd3dCommandList, 1.0f, 0.0f, 1.0f);
	m_pFloorObject = new CCubeObject();
	m_pFloorObject->SetMesh(pCubeMesh);
	m_pFloorObject->SetShader(pShader);
	m_pFloorObject->SetColor(XMFLOAT3(1.0f, 1.0f, 1.0f));
	m_pFloorObject->SetPosition(0.0f, -0.2f, 0.0f);
	m_pFloorObject->UpdateBoundingBox();

	CMesh* cTitleMesh = new CMesh(pd3dDevice, pd3dCommandList, "Models/YouWin.obj");
	m_pYWObjects = new CTitleObject();
	m_pYWObjects->SetMesh(cTitleMesh);
	m_pYWObjects->SetColor(XMFLOAT3(1.0f, 0.0f, 0.0f));
	m_pYWObjects->SetShader(pShader);
	m_pYWObjects->SetPosition(0.0f, 1.0f, 0.0f);
	m_pYWObjects->UpdateBoundingBox();
}

void CTankScene::ReleaseObjects()
{
	if (m_pd3dGraphicsRootSignature) m_pd3dGraphicsRootSignature->Release();
	if (m_pFloorObject) delete m_pFloorObject;
	for (int i = 0; i < m_nTanks; i++) {
		if (m_pTank[i]->bullet)delete m_pTank[i]->bullet;
		if (m_pTank[i]->m_pExplosionObjects)delete m_pTank[i]->m_pExplosionObjects;
		if (m_pTank[i])delete m_pTank[i];
	}
	for (int i = 0; i < m_nCubeObjects; i++)
		if (m_pCubeObjects[i])delete m_pCubeObjects[i];
	if (m_pYWObjects) delete m_pYWObjects;
}
void CTankScene::ReleaseUploadBuffers()
{
	if (m_pFloorObject) m_pFloorObject->ReleaseUploadBuffers();
	for (int i = 0; i < m_nTanks; i++) {
		if (m_pTank[i]) m_pTank[i]->ReleaseUploadBuffers();
		if (m_pTank[i]->m_pExplosionObjects) m_pTank[i]->m_pExplosionObjects->ReleaseUploadBuffers();
	}
	for (int i = 0; i < m_nCubeObjects; i++) {
		if (m_pCubeObjects[i]) m_pCubeObjects[i]->ReleaseUploadBuffers();
	}
	if (m_pYWObjects) m_pYWObjects->ReleaseUploadBuffers();
}
void CTankScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	pCamera->SetViewportsAndScissorRects(pd3dCommandList);
	pCamera->UpdateShaderVariables(pd3dCommandList);

	if (m_pPlayer) m_pPlayer->Render(pd3dCommandList, pCamera);
	if (m_pFloorObject) {
		for (int i = 0; i < 20; i++) {
			for (int j = 0; j < 20; j++) {
				m_pFloorObject->SetPosition(-10.0f + 1.0f * i, -0.2f, -10.0f + 1.0f * j);
				m_pFloorObject->Render(pd3dCommandList, pCamera);
			}
		}
	}
	for (int i = 0; i < m_nTanks; i++) {
		
		if (m_pTank[i]->IsExist()) {
			if (m_pTank[i]->IsBlowingUp()) {
				m_pTank[i]->m_pExplosionObjects->Render(pd3dCommandList, pCamera);
			}
			else {
				m_pTank[i]->Render(pd3dCommandList, pCamera);
			}
		}
	}

	for (int i = 0; i < m_nCubeObjects; i++) {
		m_pCubeObjects[i]->Render(pd3dCommandList, pCamera);
	}
	if (m_pYWObjects && GameSet >= 10) m_pYWObjects->Render(pd3dCommandList, pCamera);
}
void CTankScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	extern CGameFramework* g_pFramework;
	CTankPlayer* pTankPlayer = dynamic_cast<CTankPlayer*>(m_pPlayer);
	switch (nMessageID)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 'W':
			if (m_pPlayer->move_z < 1)m_pPlayer->move_z += 1;
			break;
		case 'S':
			if (m_pPlayer->move_z > -1)m_pPlayer->move_z -= 1;
			break;
		case 'A':
			if (m_pPlayer->move_x > -1)m_pPlayer->move_x -= 1;
			break;
		case 'D':
			if (m_pPlayer->move_x < 1)m_pPlayer->move_x += 1;
			break;
		case VK_SPACE:
			for (int i = 0; i < 10; i++)
				if (!m_pTank[i]->IsBlowingUp()) {
					m_pTank[i]->PrepareExplosion();
					GameSet++;
				}
			break;
		case 'Q':
			if (pTankPlayer) {
				pTankPlayer->SwitchShild();
			}
			break;
		case 'E':
			if (pTankPlayer && !pTankPlayer->shot) {
				pTankPlayer->SwitchBullet();
				pTankPlayer->SetBulletPosition();
			}
			break;
			break;
		case VK_RETURN:
			if (pTankPlayer->Toggle) {
				pTankPlayer->Toggle = false;
				m_pFloorObject->SetColor(XMFLOAT3(1.0f, 1.0f, 1.0f));
			}
			else {
				pTankPlayer->Toggle = true;
				m_pFloorObject->SetColor(XMFLOAT3(1.0f, 0.0f, 0.0f));
			}
			break;
		default:
			break;
		}
		break;
	case WM_KEYUP:
		switch (wParam)
		{
		case 'W':
			if (m_pPlayer->move_z > -1)m_pPlayer->move_z -= 1;
			break;
		case 'S':
			if (m_pPlayer->move_z < 1)m_pPlayer->move_z += 1;
			break;
		case 'A':
			if (m_pPlayer->move_x < 1)m_pPlayer->move_x += 1;
			break;
		case 'D':
			if (m_pPlayer->move_x > -1)m_pPlayer->move_x -= 1;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}
CGameObject* CTankScene::PickObjectPointedByCursor(int xClient, int yClient, CCamera* pCamera)
{

	XMFLOAT3 xmf3PickPosition;
	xmf3PickPosition.x = (((2.0f * xClient) / (float)pCamera->m_d3dViewport.Width) - 1) / pCamera->m_xmf4x4Projection._11;
	xmf3PickPosition.y = -(((2.0f * yClient) / (float)pCamera->m_d3dViewport.Height) - 1) / pCamera->m_xmf4x4Projection._22;
	xmf3PickPosition.z = 1.0f;

	XMVECTOR xmvPickPosition = XMLoadFloat3(&xmf3PickPosition);
	XMMATRIX xmmtxView = XMLoadFloat4x4(&pCamera->m_xmf4x4View);

	float fNearestHitDistance = FLT_MAX;
	CGameObject* pNearestObject = NULL;
	for (int i = 0; i < m_nTanks; i++) {
		if (m_pTank[i])
		{
			int hit = m_pTank[i]->PickObjectByRayIntersection(xmvPickPosition, xmmtxView, &fNearestHitDistance);
			if (hit > 0)
			{
				pNearestObject = m_pTank[i];
			}
		}
	}
	return(pNearestObject);

}
void CTankScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	CTankPlayer* pTankPlayer = dynamic_cast<CTankPlayer*>(m_pPlayer);
	switch (nMessageID)
	{
	case WM_RBUTTONDOWN:
	{
		if (pTankPlayer->Toggle) {

			int x = LOWORD(lParam);
			int y = HIWORD(lParam);
			CCamera* pCamera = m_pPlayer->GetCamera();

			CGameObject* pPickedObject = PickObjectPointedByCursor(x, y, pCamera);

			if (pPickedObject) {
				for (int i = 0; i < m_nTanks; i++) {
					if (pPickedObject == m_pTank[i]) {
						pTankPlayer->ToggleObject = m_pTank[i];
					}
				}
			}

			break;
		}
	}
	}
}
void CTankScene::CheckTankByBulletCollisions()
{
	CTankPlayer* pTankPlayer = dynamic_cast<CTankPlayer*>(m_pPlayer);
	if (pTankPlayer && pTankPlayer->shot) {
		for (int i = 0; i < 10; i++)
		{
			if (m_pTank[i]->IsExist())
				if (m_pTank[i]->m_xmOOBB.Intersects(pTankPlayer->m_pBullet->m_xmOOBB))
				{
					if (!m_pTank[i]->IsBlowingUp()) {
						m_pTank[i]->PrepareExplosion();
						pTankPlayer->shot = false;
						pTankPlayer->bullet_timer = 0;
						pTankPlayer->ToggleObject = NULL;
						GameSet++;
					}
				}
		}
	}
}
void CTankScene::CheckPlayerByBulletCollisions()
{
	CTankPlayer* pTankPlayer = dynamic_cast<CTankPlayer*>(m_pPlayer);
	if (pTankPlayer) {
		for (int i = 0; i < m_nTanks; i++)
		{
			if (!pTankPlayer->OnShild) {
				if (m_pTank[i]->IsExist() && m_pTank[i]->IsShot())
					if (pTankPlayer->m_xmOOBB.Intersects(m_pTank[i]->bullet->m_xmOOBB))
					{
						XMFLOAT3 color = m_pTank[i]->m_xmf3Color;

						pTankPlayer->SetColor(color);
						pTankPlayer->m_pBullet->SetColor(color);
						pTankPlayer->m_pShild->SetColor(color);

						m_pTank[i]->SwitchShot();
					}
			}
			else {
				if (m_pTank[i]->IsExist() && m_pTank[i]->IsShot())
					if (pTankPlayer->m_pShild->m_xmOOBB.Intersects(m_pTank[i]->bullet->m_xmOOBB))
					{
						m_pTank[i]->SwitchShot();
					}
			}
		}
	}
}

void CTankScene::CheckPlayerByObjectCollisions(float fElapsedTime)
{
	CTankPlayer* pTankPlayer = dynamic_cast<CTankPlayer*>(m_pPlayer);
	if (!pTankPlayer) return;

	const XMFLOAT3& moveVec = pTankPlayer->GetMoveVector();

	BoundingOrientedBox movedBox = pTankPlayer->m_xmOOBB;
	movedBox.Center.x += moveVec.x;
	movedBox.Center.z += moveVec.z;

	bool blocked = false;
	for (int i = 0; i < m_nCubeObjects; i++) {
		if (m_pCubeObjects[i] && movedBox.Intersects(m_pCubeObjects[i]->m_xmOOBB)) {
			blocked = true;
			break;
		}
	}

	if (!blocked) {
		XMFLOAT3 nowPos = pTankPlayer->GetPosition();
		pTankPlayer->SetPosition(nowPos.x + moveVec.x, nowPos.y, nowPos.z + moveVec.z);
		if (pTankPlayer->m_pShild) {
			pTankPlayer->m_pShild->SetPosition(nowPos.x + moveVec.x, nowPos.y, nowPos.z + moveVec.z);
		}
	}

	pTankPlayer->UpdateBoundingBox();
	if (pTankPlayer->m_pShild) pTankPlayer->m_pShild->UpdateBoundingBox();

	pTankPlayer->ClearMoveVector();
}

void CTankScene::CheckBulletByObjectCollisions()
{
	CTankPlayer* pTankPlayer = dynamic_cast<CTankPlayer*>(m_pPlayer);
	if (pTankPlayer) {
		for (int i = 0; i < m_nCubeObjects; i++)
		{
			if (m_pCubeObjects[i] && pTankPlayer->shot)
				if (pTankPlayer->m_pBullet->m_xmOOBB.Intersects(m_pCubeObjects[i]->m_xmOOBB))
				{
					pTankPlayer->shot = false;
					pTankPlayer->bullet_timer = 0;
					pTankPlayer->ToggleObject = NULL;
				}
		}
	}
}

void CTankScene::Animate(float fElapsedTime)
{
	for (int i = 0; i < m_nTanks; i++) {
		if (m_pTank[i]) {
			m_pTank[i]->Animate(fElapsedTime);
			if (m_pTank[i]->IsBlowingUp()) {
				for (int j = 0; j < EXPLOSION_DEBRISES; j++) {
					m_pTank[i]->m_pExplosionObjects->m_pxmf4x4Transforms[j] = m_pTank[i]->m_pxmf4x4Transforms[j];
					m_pTank[i]->m_pExplosionObjects->m_pxmf3SphereVectors[j] = m_pTank[i]->m_pxmf3SphereVectors[j];
				}
			}
		}
	}

	CTankPlayer* pTankPlayer = dynamic_cast<CTankPlayer*>(m_pPlayer);
	pTankPlayer->Animate(fElapsedTime);
	if (m_pYWObjects && GameSet >= 10) m_pYWObjects->Animate(fElapsedTime);

	CheckPlayerByObjectCollisions(fElapsedTime);
	CheckBulletByObjectCollisions();
	CheckTankByBulletCollisions();
	CheckPlayerByBulletCollisions();
}