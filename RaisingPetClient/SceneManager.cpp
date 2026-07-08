#include "stdafx.h"
#include "SceneManager.h"
#include "LoginScene.h"
#include "GameScene.h"

CSceneManager::~CSceneManager()
{
	ReleaseCurrentScene();
}

void CSceneManager::RequestSceneChange(SCENE_TYPE sceneType)
{
	m_ePendingSceneType = sceneType;
	m_bSceneChangePending = true;
}

CScene* CSceneManager::ApplyPendingSceneChange()
{
	if (!m_bSceneChangePending) return(m_pCurrentScene);
	ReleaseCurrentScene();
	m_pCurrentScene = (m_ePendingSceneType == SCENE_TYPE::LOGIN)
		? static_cast<CScene*>(new CLoginScene())
		: static_cast<CScene*>(new CGameScene());
	m_bSceneChangePending = false;
	return(m_pCurrentScene);
}

void CSceneManager::ReleaseCurrentScene()
{
	if (!m_pCurrentScene) return;
	m_pCurrentScene->ReleaseObjects();
	delete m_pCurrentScene;
	m_pCurrentScene = NULL;
}
