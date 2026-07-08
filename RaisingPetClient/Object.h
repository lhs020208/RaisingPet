//------------------------------------------------------- ----------------------
// File: Object.h
//-----------------------------------------------------------------------------

#pragma once

#include "Mesh.h"
#include "Camera.h"

class CShader;

class CGameObject
{
public:
	CGameObject();
	virtual ~CGameObject();

public:
	XMFLOAT4X4						m_xmf4x4World;
	CMesh							*m_pMesh = NULL;

	CShader							*m_pShader = NULL;

	XMFLOAT3						m_xmf3Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	BoundingOrientedBox				m_xmOOBB = BoundingOrientedBox();

	void SetMesh(CMesh *pMesh);
	void SetShader(CShader *pShader);

	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList, XMFLOAT4X4* pxmf4x4World);
	virtual void ReleaseShaderVariables();

	virtual void Animate(float fTimeElapsed);
	virtual void OnPrepareRender() { }
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera = NULL);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, XMFLOAT4X4* pxmf4x4World);
	virtual void ReleaseUploadBuffers();

	XMFLOAT3 GetPosition();
	XMFLOAT3 GetLook();
	XMFLOAT3 GetUp();
	XMFLOAT3 GetRight();

	void LookTo(XMFLOAT3& xmf3LookTo, XMFLOAT3& xmf3Up);
	void LookAt(XMFLOAT3& xmf3LookAt, XMFLOAT3& xmf3Up);

	virtual void UpdateBoundingBox();

	void SetPosition(float x, float y, float z);
	void SetPosition(XMFLOAT3 xmf3Position);
	void SetRotationTransform(XMFLOAT4X4* pmxf4x4Transform);

	void SetColor(XMFLOAT3 xmf3Color) { m_xmf3Color = xmf3Color; }

	void MoveStrafe(float fDistance = 1.0f);
	void MoveUp(float fDistance = 1.0f);
	void MoveForward(float fDistance = 1.0f);

	virtual void Rotate(float fPitch = 0.0f, float fYaw = 0.0f, float fRoll = 0.0f);
	virtual void Rotate(XMFLOAT3 *pxmf3Axis, float fAngle);

	int PickObjectByRayIntersection(XMVECTOR& xmvPickPosition, XMMATRIX& xmmtxView, float* pfHitDistance);
	void GenerateRayForPicking(XMVECTOR& xmvPickPosition, XMMATRIX& xmmtxView, XMVECTOR& xmvPickRayOrigin, XMVECTOR& xmvPickRayDirection);
};

class CPet : public CGameObject
{
public:
	CPet();
	virtual ~CPet() {}

	virtual void Animate(float fTimeElapsed) override;
	void AnimateWithoutMovement(float fTimeElapsed);
	void CopyRuntimeStateFrom(const CPet& sourcePet);

private:
	enum class MOVE_STATE
	{
		STOP,
		LEFT,
		RIGHT
	};

	void DecideNextState();
	float GetRandomStateDuration(MOVE_STATE state);
	void ChangeMoveState(MOVE_STATE newState);
	void UpdateRotation(float fTimeElapsed);
	void UpdateMoveSpeed(float fTimeElapsed);
	void UpdatePossession(float fTimeElapsed);

	MOVE_STATE m_MoveState = MOVE_STATE::STOP;
	std::mt19937 m_RandomEngine;

	float m_fStateRemainingTime = 0.0f;
	float m_fPossessionElapsedTime = 0.0f;
	float m_fMaxChargeTime = 0.0f;
	float m_fFullPossessionElapsedTime = 0.0f;
	bool m_bAutoCollectRequested = false;
	float m_fMoveSpeed = 1.0f;
	float m_fMoveDirection = 0.0f;
	float m_fCurrentMoveSpeed = 0.0f;
	float m_fSpeedStart = 0.0f;
	float m_fSpeedTarget = 0.0f;
	float m_fSpeedElapsedTime = 0.0f;
	float m_fSpeedTransitionDuration = 0.35f;
	bool m_bSpeedTransitioning = false;

	float m_fCurrentYaw = 0.0f;
	float m_fRotationStartYaw = 0.0f;
	float m_fRotationTargetYaw = 0.0f;
	float m_fRotationElapsedTime = 0.0f;
	float m_fRotationDuration = 0.35f;
	bool m_bRotating = false;

public:
	string GetName() const { return m_sName; }
	void SetName(const string& newName) { m_sName = newName;}

	UINT GetPay() { return m_nPay; }
	UINT GetMaxPossession() { return m_nMaxPossession; }
	UINT GetNowPossession() { return m_nNowPossession; }
	float GetMaxChargeTime() const { return m_fMaxChargeTime; }
	bool ConsumeAutoCollectRequest();

	void SetPay(UINT p) { m_nPay = p; }
	void GetMaxPossession(UINT p) { m_nMaxPossession = p; }
	void GetNowPossession(UINT p) { m_nNowPossession = p; }

private:
	string m_sName;
	UINT m_nPay = 1;
	UINT m_nMaxPossession = 20;
	UINT m_nNowPossession = 0;
};
