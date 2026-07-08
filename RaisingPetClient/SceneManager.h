#pragma once

#include "Scene.h"

enum class SCENE_TYPE
{
	LOGIN,
	GAME
};

class CSceneManager
{
public:
	~CSceneManager();

	void RequestSceneChange(SCENE_TYPE sceneType);
	bool HasPendingSceneChange() const { return m_bSceneChangePending; }
	CScene* ApplyPendingSceneChange();
	CScene* GetCurrentScene() const { return m_pCurrentScene; }
	void ReleaseCurrentScene();

private:
	CScene* m_pCurrentScene = NULL;
	SCENE_TYPE m_ePendingSceneType = SCENE_TYPE::LOGIN;
	bool m_bSceneChangePending = false;
};
