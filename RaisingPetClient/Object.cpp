//-----------------------------------------------------------------------------
// File: Object.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Object.h"
#include "Shader.h"
#include "GameFramework.h"

inline float RandF(float fMin, float fMax)
{
	return(fMin + ((float)rand() / (float)RAND_MAX) * (fMax - fMin));
}

XMVECTOR RandomUnitVectorOnSphere()
{
	XMVECTOR xmvOne = XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f);
	XMVECTOR xmvZero = XMVectorZero();

	while (true)
	{
		XMVECTOR v = XMVectorSet(RandF(-1.0f, 1.0f), RandF(-1.0f, 1.0f), RandF(-1.0f, 1.0f), 0.0f);
		if (!XMVector3Greater(XMVector3LengthSq(v), xmvOne)) return(XMVector3Normalize(v));
	}
}

CGameObject::CGameObject()
{
	m_xmf4x4World = Matrix4x4::Identity();
}

CGameObject::~CGameObject()
{
	if (m_pMesh) m_pMesh->Release();
	if (m_pShader) m_pShader->Release();
}

void CGameObject::SetMesh(CMesh *pMesh)
{
	if (m_pMesh) m_pMesh->Release();
	m_pMesh = pMesh;
	if (m_pMesh) m_pMesh->AddRef();
}

void CGameObject::SetShader(CShader *pShader)
{
	if (m_pShader) m_pShader->Release();
	m_pShader = pShader;
	if (m_pShader) m_pShader->AddRef();
}

void CGameObject::Animate(float fTimeElapsed)
{
}

void CGameObject::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
}

void CGameObject::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	XMFLOAT4X4 xmf4x4World;
	XMStoreFloat4x4(&xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));
	pd3dCommandList->SetGraphicsRoot32BitConstants(1, 16, &xmf4x4World, 0);

	pd3dCommandList->SetGraphicsRoot32BitConstants(1, 3, &m_xmf3Color, 16);
}
void CGameObject::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList, XMFLOAT4X4* pxmf4x4World)
{
	XMFLOAT4X4 xmf4x4World;
	XMStoreFloat4x4(&xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(pxmf4x4World)));
	pd3dCommandList->SetGraphicsRoot32BitConstants(1, 16, &xmf4x4World, 0);
	pd3dCommandList->SetGraphicsRoot32BitConstants(1, 3, &m_xmf3Color, 16);
}

void CGameObject::ReleaseShaderVariables()
{
}

void CGameObject::Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera)
{
	OnPrepareRender();

	if (m_pShader) m_pShader->Render(pd3dCommandList, pCamera);

	UpdateShaderVariables(pd3dCommandList);

	if (m_pMesh) m_pMesh->Render(pd3dCommandList);
}

void CGameObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, XMFLOAT4X4* pxmf4x4World)
{
	OnPrepareRender();

	if (m_pShader) m_pShader->Render(pd3dCommandList, pCamera);

	UpdateShaderVariables(pd3dCommandList, pxmf4x4World);

	if (m_pMesh) m_pMesh->Render(pd3dCommandList);
}

void CGameObject::ReleaseUploadBuffers()
{
	if (m_pMesh) m_pMesh->ReleaseUploadBuffers();
}

void CGameObject::SetPosition(float x, float y, float z)
{
	m_xmf4x4World._41 = x;
	m_xmf4x4World._42 = y;
	m_xmf4x4World._43 = z;
}

void CGameObject::SetPosition(XMFLOAT3 xmf3Position)
{
	SetPosition(xmf3Position.x, xmf3Position.y, xmf3Position.z);
}

XMFLOAT3 CGameObject::GetPosition()
{
	return(XMFLOAT3(m_xmf4x4World._41, m_xmf4x4World._42, m_xmf4x4World._43));
}

XMFLOAT3 CGameObject::GetLook()
{
	return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._31, m_xmf4x4World._32, m_xmf4x4World._33)));
}

XMFLOAT3 CGameObject::GetUp()
{
	return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._21, m_xmf4x4World._22, m_xmf4x4World._23)));
}

XMFLOAT3 CGameObject::GetRight()
{
	return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._11, m_xmf4x4World._12, m_xmf4x4World._13)));
}

void CGameObject::MoveStrafe(float fDistance)
{
	XMFLOAT3 xmf3Position = GetPosition();
	XMFLOAT3 xmf3Right = GetRight();
	xmf3Position = Vector3::Add(xmf3Position, xmf3Right, fDistance);
	CGameObject::SetPosition(xmf3Position);
}

void CGameObject::MoveUp(float fDistance)
{
	XMFLOAT3 xmf3Position = GetPosition();
	XMFLOAT3 xmf3Up = GetUp();
	xmf3Position = Vector3::Add(xmf3Position, xmf3Up, fDistance);
	CGameObject::SetPosition(xmf3Position);
}

void CGameObject::MoveForward(float fDistance)
{
	XMFLOAT3 xmf3Position = GetPosition();
	XMFLOAT3 xmf3Look = GetLook();
	xmf3Position = Vector3::Add(xmf3Position, xmf3Look, fDistance);
	CGameObject::SetPosition(xmf3Position);
}

void CGameObject::Rotate(float fPitch, float fYaw, float fRoll)
{
	XMMATRIX mtxRotate = XMMatrixRotationRollPitchYaw(XMConvertToRadians(fPitch), XMConvertToRadians(fYaw), XMConvertToRadians(fRoll));
	m_xmf4x4World = Matrix4x4::Multiply(mtxRotate, m_xmf4x4World);
}

void CGameObject::Rotate(XMFLOAT3 *pxmf3Axis, float fAngle)
{
	XMMATRIX mtxRotate = XMMatrixRotationAxis(XMLoadFloat3(pxmf3Axis), XMConvertToRadians(fAngle));
	m_xmf4x4World = Matrix4x4::Multiply(mtxRotate, m_xmf4x4World);
}

void CGameObject::LookTo(XMFLOAT3& xmf3LookTo, XMFLOAT3& xmf3Up)
{
	XMFLOAT4X4 xmf4x4View = Matrix4x4::LookToLH(GetPosition(), xmf3LookTo, xmf3Up);
	m_xmf4x4World._11 = xmf4x4View._11; m_xmf4x4World._12 = xmf4x4View._21; m_xmf4x4World._13 = xmf4x4View._31;
	m_xmf4x4World._21 = xmf4x4View._12; m_xmf4x4World._22 = xmf4x4View._22; m_xmf4x4World._23 = xmf4x4View._32;
	m_xmf4x4World._31 = xmf4x4View._13; m_xmf4x4World._32 = xmf4x4View._23; m_xmf4x4World._33 = xmf4x4View._33;
}

void CGameObject::LookAt(XMFLOAT3& xmf3LookAt, XMFLOAT3& xmf3Up)
{
	XMFLOAT4X4 xmf4x4View = Matrix4x4::LookAtLH(GetPosition(), xmf3LookAt, xmf3Up);
	m_xmf4x4World._11 = xmf4x4View._11; m_xmf4x4World._12 = xmf4x4View._21; m_xmf4x4World._13 = xmf4x4View._31;
	m_xmf4x4World._21 = xmf4x4View._12; m_xmf4x4World._22 = xmf4x4View._22; m_xmf4x4World._23 = xmf4x4View._32;
	m_xmf4x4World._31 = xmf4x4View._13; m_xmf4x4World._32 = xmf4x4View._23; m_xmf4x4World._33 = xmf4x4View._33;
}

void CGameObject::UpdateBoundingBox()
{
	if (m_pMesh)
	{
		m_pMesh->m_xmOOBB.Transform(m_xmOOBB, XMLoadFloat4x4(&m_xmf4x4World));
		XMStoreFloat4(&m_xmOOBB.Orientation, XMQuaternionNormalize(XMLoadFloat4(&m_xmOOBB.Orientation)));
	}
}

int CGameObject::PickObjectByRayIntersection(XMVECTOR& xmvPickPosition, XMMATRIX& xmmtxView, float* pfHitDistance)
{
	if (!m_pMesh) return(0);

	XMVECTOR xmvPickRayOrigin;
	XMVECTOR xmvPickRayDirection;
	GenerateRayForPicking(xmvPickPosition, xmmtxView, xmvPickRayOrigin, xmvPickRayDirection);
	return(m_pMesh->CheckRayIntersection(xmvPickRayOrigin, xmvPickRayDirection, pfHitDistance));
}
void CGameObject::GenerateRayForPicking(XMVECTOR& xmvPickPosition, XMMATRIX& xmmtxView, XMVECTOR& xmvPickRayOrigin, XMVECTOR& xmvPickRayDirection)
{
	XMMATRIX xmmtxToModel = XMMatrixInverse(NULL, XMLoadFloat4x4(&m_xmf4x4World) * xmmtxView);

	XMFLOAT3 xmf3CameraOrigin(0.0f, 0.0f, 0.0f);
	xmvPickRayOrigin = XMVector3TransformCoord(XMLoadFloat3(&xmf3CameraOrigin), xmmtxToModel);
	xmvPickRayDirection = XMVector3TransformCoord(xmvPickPosition, xmmtxToModel);
	xmvPickRayDirection = XMVector3Normalize(xmvPickRayDirection - xmvPickRayOrigin);
}
void CGameObject::SetRotationTransform(XMFLOAT4X4* pmxf4x4Transform)
{
	m_xmf4x4World._11 = pmxf4x4Transform->_11; m_xmf4x4World._12 = pmxf4x4Transform->_12; m_xmf4x4World._13 = pmxf4x4Transform->_13;
	m_xmf4x4World._21 = pmxf4x4Transform->_21; m_xmf4x4World._22 = pmxf4x4Transform->_22; m_xmf4x4World._23 = pmxf4x4Transform->_23;
	m_xmf4x4World._31 = pmxf4x4Transform->_31; m_xmf4x4World._32 = pmxf4x4Transform->_32; m_xmf4x4World._33 = pmxf4x4Transform->_33;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
// CPet

CPet::CPet() : m_RandomEngine(std::random_device{}())
{
	m_fStateRemainingTime = GetRandomStateDuration(MOVE_STATE::STOP);
}

void CPet::Animate(float fTimeElapsed)
{
	if (fTimeElapsed <= 0.0f) return;

	UpdatePossession(fTimeElapsed);
	UpdateMoveSpeed(fTimeElapsed);

	XMFLOAT3 position = GetPosition();
	position.x += m_fMoveDirection * m_fCurrentMoveSpeed * fTimeElapsed;
	bool reachedMoveBoundary = false;
	if (position.x <= -50.0f)
	{
		position.x = -50.0f;
		reachedMoveBoundary = (m_MoveState == MOVE_STATE::LEFT);
	}
	else if (position.x >= 50.0f)
	{
		position.x = 50.0f;
		reachedMoveBoundary = (m_MoveState == MOVE_STATE::RIGHT);
	}
	SetPosition(position);
	if (reachedMoveBoundary)
	{
		ChangeMoveState(MOVE_STATE::STOP);
		m_fStateRemainingTime = GetRandomStateDuration(MOVE_STATE::STOP);
	}

	UpdateRotation(fTimeElapsed);
	UpdateBoundingBox();

	m_fStateRemainingTime -= fTimeElapsed;
	if (m_fStateRemainingTime <= 0.0f)
	{
		DecideNextState();
	}
}

void CPet::AnimateWithoutMovement(float fTimeElapsed)
{
	if (fTimeElapsed <= 0.0f) return;

	UpdateMoveSpeed(fTimeElapsed);
	UpdateRotation(fTimeElapsed);
	UpdateBoundingBox();

	m_fStateRemainingTime -= fTimeElapsed;
	if (m_fStateRemainingTime <= 0.0f)
		DecideNextState();
}

void CPet::UpdatePossession(float fTimeElapsed)
{
	if (m_nNowPossession == 0)
	{
		m_fFullPossessionElapsedTime = 0.0f;
		m_bAutoCollectRequested = false;
		if (m_nPay > 0 && m_nMaxPossession > 0)
		{
			m_fMaxChargeTime = static_cast<float>((static_cast<UINT64>(m_nMaxPossession)
				+ m_nPay - 1) / m_nPay);
		}
		else
		{
			m_fMaxChargeTime = 0.0f;
		}
	}

	if (m_nNowPossession >= m_nMaxPossession)
	{
		m_nNowPossession = m_nMaxPossession;
		m_fPossessionElapsedTime = 0.0f;
		if (m_fMaxChargeTime > 0.0f && !m_bAutoCollectRequested)
		{
			m_fFullPossessionElapsedTime += fTimeElapsed;
			if (m_fFullPossessionElapsedTime >= m_fMaxChargeTime)
				m_bAutoCollectRequested = true;
		}
		return;
	}

	m_fFullPossessionElapsedTime = 0.0f;
	if (m_nPay == 0) return;

	m_fPossessionElapsedTime += fTimeElapsed;
	while (m_fPossessionElapsedTime >= 1.0f && m_nNowPossession < m_nMaxPossession)
	{
		m_fPossessionElapsedTime -= 1.0f;
		const UINT nRemainingPossession = m_nMaxPossession - m_nNowPossession;
		m_nNowPossession += (m_nPay < nRemainingPossession) ? m_nPay : nRemainingPossession;
	}
}

bool CPet::ConsumeAutoCollectRequest()
{
	if (!m_bAutoCollectRequested) return(false);
	m_bAutoCollectRequested = false;
	m_fFullPossessionElapsedTime = 0.0f;
	return(true);
}
void CPet::DecideNextState()
{
	const float positionX = GetPosition().x;
	std::uniform_int_distribution<int> chanceDistribution(0, 99);
	const int chance = chanceDistribution(m_RandomEngine);
	MOVE_STATE nextState = m_MoveState;

	if (m_MoveState == MOVE_STATE::STOP)
	{
		// At either boundary, never select a direction that points outside the movement area.
		if (positionX <= -50.0f)
			nextState = (chance < 80) ? MOVE_STATE::RIGHT : MOVE_STATE::STOP;
		else if (positionX >= 50.0f)
			nextState = (chance < 80) ? MOVE_STATE::LEFT : MOVE_STATE::STOP;
		else if (chance < 40)
			nextState = MOVE_STATE::LEFT;
		else if (chance < 80)
			nextState = MOVE_STATE::RIGHT;
		else
			nextState = MOVE_STATE::STOP;
	}
	else if (m_MoveState == MOVE_STATE::LEFT)
	{
		if (positionX <= -50.0f)
			nextState = MOVE_STATE::STOP;
		else if (chance < 70)
			nextState = MOVE_STATE::STOP;
		else if (chance < 80)
			nextState = MOVE_STATE::RIGHT;
		else
			nextState = MOVE_STATE::LEFT;
	}
	else
	{
		if (positionX >= 50.0f)
			nextState = MOVE_STATE::STOP;
		else if (chance < 70)
			nextState = MOVE_STATE::STOP;
		else if (chance < 80)
			nextState = MOVE_STATE::LEFT;
		else
			nextState = MOVE_STATE::RIGHT;
	}

	ChangeMoveState(nextState);
	m_fStateRemainingTime = GetRandomStateDuration(nextState);
}

float CPet::GetRandomStateDuration(MOVE_STATE state)
{
	if (state == MOVE_STATE::STOP)
	{
		std::uniform_real_distribution<float> durationDistribution(1.5f, 4.0f);
		return durationDistribution(m_RandomEngine);
	}

	std::uniform_real_distribution<float> durationDistribution(2.0f, 7.0f);
	return durationDistribution(m_RandomEngine);
}

void CPet::ChangeMoveState(MOVE_STATE newState)
{
	if (newState == m_MoveState) return;

	m_MoveState = newState;
	m_fRotationStartYaw = m_fCurrentYaw;
	m_fRotationTargetYaw = 0.0f;
	if (m_MoveState == MOVE_STATE::LEFT)
	{
		m_fRotationTargetYaw = 45.0f;
		m_fMoveDirection = -1.0f;
	}
	else if (m_MoveState == MOVE_STATE::RIGHT)
	{
		m_fRotationTargetYaw = -45.0f;
		m_fMoveDirection = 1.0f;
	}

	m_fRotationElapsedTime = 0.0f;
	m_bRotating = true;

	m_fSpeedStart = m_fCurrentMoveSpeed;
	m_fSpeedTarget = (m_MoveState == MOVE_STATE::STOP) ? 0.0f : m_fMoveSpeed;
	m_fSpeedElapsedTime = 0.0f;
	m_bSpeedTransitioning = true;
}

void CPet::UpdateRotation(float fTimeElapsed)
{
	if (!m_bRotating) return;

	m_fRotationElapsedTime += fTimeElapsed;
	float t = m_fRotationElapsedTime / m_fRotationDuration;
	if (t >= 1.0f)
	{
		t = 1.0f;
		m_bRotating = false;
	}

	const float easedT = 0.5f - (0.5f * cosf(XM_PI * t));
	m_fCurrentYaw = m_fRotationStartYaw + ((m_fRotationTargetYaw - m_fRotationStartYaw) * easedT);

	XMFLOAT4X4 rotationTransform;
	XMStoreFloat4x4(&rotationTransform, XMMatrixRotationY(XMConvertToRadians(m_fCurrentYaw)));
	SetRotationTransform(&rotationTransform);
}

void CPet::UpdateMoveSpeed(float fTimeElapsed)
{
	if (!m_bSpeedTransitioning) return;

	m_fSpeedElapsedTime += fTimeElapsed;
	float t = m_fSpeedElapsedTime / m_fSpeedTransitionDuration;
	if (t >= 1.0f)
	{
		t = 1.0f;
		m_bSpeedTransitioning = false;
	}

	const float easedT = 0.5f - (0.5f * cosf(XM_PI * t));
	m_fCurrentMoveSpeed = m_fSpeedStart + ((m_fSpeedTarget - m_fSpeedStart) * easedT);

	if (!m_bSpeedTransitioning && m_fCurrentMoveSpeed <= 0.0f)
	{
		m_fCurrentMoveSpeed = 0.0f;
		m_fMoveDirection = 0.0f;
	}
}
