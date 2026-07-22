//-----------------------------------------------------------------------------
// File: FmodSoundManager.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "FmodSoundManager.h"

#if defined(_WIN64)
#if defined(_DEBUG)
#pragma comment(lib, "External/FMOD/lib/x64/fmodL_vc.lib")
#else
#pragma comment(lib, "External/FMOD/lib/x64/fmod_vc.lib")
#endif
#endif

bool CFmodSoundManager::Initialize(int nMaxChannels)
{
	if (m_pSystem) return(true);

	FMOD::System* pSystem = NULL;
	if (!CheckResult(FMOD::System_Create(&pSystem), "FMOD::System_Create"))
		return(false);

	FMOD_RESULT result = pSystem->init(nMaxChannels, FMOD_INIT_NORMAL, NULL);
	if (result != FMOD_OK)
	{
		CheckResult(result, "FMOD::System::init");
		pSystem->release();
		return(false);
	}

	m_pSystem = pSystem;
	OutputDebugStringA("[FMOD] Sound system initialized.\n");
	return(true);
}

void CFmodSoundManager::Release()
{
	UnloadAllSounds();

	for (ONE_SHOT_SOUND& oneShotSound : m_OneShotSounds)
	{
		if (oneShotSound.pSound)
			oneShotSound.pSound->release();
	}
	m_OneShotSounds.clear();

	if (m_pSystem)
	{
		m_pSystem->close();
		m_pSystem->release();
		m_pSystem = NULL;
	}
}

void CFmodSoundManager::Update()
{
	if (!m_pSystem) return;

	for (auto iter = m_OneShotSounds.begin(); iter != m_OneShotSounds.end(); )
	{
		bool bPlaying = false;
		if (iter->pChannel)
			iter->pChannel->isPlaying(&bPlaying);

		if (!bPlaying)
		{
			if (iter->pSound)
				iter->pSound->release();
			iter = m_OneShotSounds.erase(iter);
		}
		else
		{
			++iter;
		}
	}

	m_pSystem->update();
}

bool CFmodSoundManager::LoadSound(const std::string& strKey, const std::string& strFilePath, bool bLoop)
{
	if (!m_pSystem || strKey.empty() || strFilePath.empty()) return(false);

	UnloadSound(strKey);

	FMOD::Sound* pSound = NULL;
	const FMOD_MODE mode = FMOD_DEFAULT | (bLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
	if (!CheckResult(m_pSystem->createSound(strFilePath.c_str(), mode, NULL, &pSound),
		"FMOD::System::createSound"))
	{
		return(false);
	}

	m_Sounds[strKey] = pSound;
	return(true);
}

void CFmodSoundManager::UnloadSound(const std::string& strKey)
{
	auto iter = m_Sounds.find(strKey);
	if (iter == m_Sounds.end()) return;

	if (iter->second)
		iter->second->release();
	m_Sounds.erase(iter);
}

void CFmodSoundManager::UnloadAllSounds()
{
	for (auto& soundPair : m_Sounds)
	{
		if (soundPair.second)
			soundPair.second->release();
	}
	m_Sounds.clear();
}

bool CFmodSoundManager::PlaySound(const std::string& strKey, float fVolume)
{
	return(PlaySoundDelayed(strKey, 0.0f, fVolume));
}

bool CFmodSoundManager::PlaySoundDelayed(const std::string& strKey, float fDelaySeconds, float fVolume)
{
	if (!m_pSystem) return(false);

	auto iter = m_Sounds.find(strKey);
	if (iter == m_Sounds.end() || !iter->second) return(false);

	FMOD::Channel* pChannel = NULL;
	if (!CheckResult(m_pSystem->playSound(iter->second, NULL, true, &pChannel),
		"FMOD::System::playSound"))
	{
		return(false);
	}

	if (pChannel)
	{
		pChannel->setVolume(ClampVolume(fVolume));
		if (fDelaySeconds > 0.0f)
		{
			unsigned long long nDspClock = 0;
			unsigned long long nParentClock = 0;
			int nSoftwareRate = 0;
			pChannel->getDSPClock(&nDspClock, &nParentClock);
			m_pSystem->getSoftwareFormat(&nSoftwareRate, NULL, NULL);
			if (nSoftwareRate > 0)
			{
				const unsigned long long nDelayClock =
					static_cast<unsigned long long>(fDelaySeconds * static_cast<float>(nSoftwareRate));
				pChannel->setDelay(nParentClock + nDelayClock, 0, false);
			}
		}
		pChannel->setPaused(false);
	}
	return(true);
}

bool CFmodSoundManager::PlayOneShot(const std::string& strFilePath, float fVolume)
{
	if (!m_pSystem || strFilePath.empty()) return(false);

	FMOD::Sound* pSound = NULL;
	if (!CheckResult(m_pSystem->createSound(strFilePath.c_str(), FMOD_DEFAULT | FMOD_LOOP_OFF,
		NULL, &pSound), "FMOD::System::createSound(one-shot)"))
	{
		return(false);
	}

	FMOD::Channel* pChannel = NULL;
	if (!CheckResult(m_pSystem->playSound(pSound, NULL, false, &pChannel),
		"FMOD::System::playSound(one-shot)"))
	{
		pSound->release();
		return(false);
	}

	if (pChannel)
		pChannel->setVolume(ClampVolume(fVolume));

	ONE_SHOT_SOUND oneShotSound;
	oneShotSound.pSound = pSound;
	oneShotSound.pChannel = pChannel;
	m_OneShotSounds.push_back(oneShotSound);
	return(true);
}

bool CFmodSoundManager::CheckResult(FMOD_RESULT result, const char* pContext)
{
	if (result == FMOD_OK) return(true);

	std::ostringstream errorStream;
	errorStream << "[FMOD] " << pContext << " failed. result=" << result << "\n";
	m_strLastError = errorStream.str();
	OutputDebugStringA(m_strLastError.c_str());
	return(false);
}

float CFmodSoundManager::ClampVolume(float fVolume) const
{
	if (fVolume < 0.0f) return(0.0f);
	if (fVolume > 1.0f) return(1.0f);
	return(fVolume);
}
