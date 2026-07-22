//-----------------------------------------------------------------------------
// File: FmodSoundManager.h
//-----------------------------------------------------------------------------

#pragma once

#include "External/FMOD/include/core/fmod.hpp"
#include <unordered_map>

class CFmodSoundManager
{
public:
	CFmodSoundManager() {}
	~CFmodSoundManager() { Release(); }

	bool Initialize(int nMaxChannels = 32);
	void Release();
	void Update();

	bool IsReady() const { return(m_pSystem != NULL); }
	const std::string& GetLastError() const { return(m_strLastError); }

	bool LoadSound(const std::string& strKey, const std::string& strFilePath, bool bLoop = false);
	void UnloadSound(const std::string& strKey);
	void UnloadAllSounds();

	bool PlaySound(const std::string& strKey, float fVolume = 1.0f);
	bool PlaySoundDelayed(const std::string& strKey, float fDelaySeconds, float fVolume = 1.0f);
	bool PlayOneShot(const std::string& strFilePath, float fVolume = 1.0f);

private:
	struct ONE_SHOT_SOUND
	{
		FMOD::Sound* pSound = NULL;
		FMOD::Channel* pChannel = NULL;
	};

	bool CheckResult(FMOD_RESULT result, const char* pContext);
	float ClampVolume(float fVolume) const;

	FMOD::System* m_pSystem = NULL;
	std::unordered_map<std::string, FMOD::Sound*> m_Sounds;
	std::vector<ONE_SHOT_SOUND> m_OneShotSounds;
	std::string m_strLastError;
};
