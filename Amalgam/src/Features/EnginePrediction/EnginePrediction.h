#pragma once
#include "../../SDK/SDK.h"

struct DatamapRestore_t
{
	byte* m_pData = nullptr;
	size_t m_iSize = 0;
};

struct RestoreInfo_t
{
	Vec3 m_vOrigin = {};
	Vec3 m_vMins = {};
	Vec3 m_vMaxs = {};
};

class CEnginePrediction
{
private:
	CMoveData m_tMoveData = {};
	DatamapRestore_t m_tLocal = {};
	std::unordered_map<CTFPlayer*, RestoreInfo_t> m_mRestore = {};

	// Seed cache — avoids re-hashing the same command_number each probe iteration
	int m_nSeedCommandNumber = -1;
	int m_iSeed = -1;

	// Batch state: while open, other players are adjusted once and restored once at EndBatch
	int  m_iBatchDepth    = 0;
	bool m_bBatchAdjusted = false;

public:
	void Start(CTFPlayer* pLocal, CUserCmd* pCmd);
	void End(CTFPlayer* pLocal, CUserCmd* pCmd);
	void Simulate(CTFPlayer* pLocal, CUserCmd* pCmd);   // now public — Misc probes call this

	// Z-velocity helpers used by TextureBug / EdgeBug surface detection
	float GetTargetPredictZVelocity() const;
	bool  IsTargetPredictZVelocity(float flVz, float flEpsilon = 0.1f) const;

	void Unload();

	void AdjustPlayers(CBaseEntity* pLocal);
	void RestorePlayers();

	// RAII batch helpers — wrap probe loops so other players are adjusted only once
	void BeginBatch() { m_iBatchDepth++; }
	void EndBatch()
	{
		if (--m_iBatchDepth <= 0)
		{
			m_iBatchDepth = 0;
			if (m_bBatchAdjusted)
			{
				RestorePlayers();
				m_bBatchAdjusted = false;
			}
		}
	}

	bool m_bInPrediction = false;

	int   m_nOldTickCount    = 0;
	float m_flOldCurrentTime = 0.f;
	float m_flOldFrameTime   = 0.f;

	// Published after each Simulate so Misc features can read predicted state
	Vec3 m_vOrigin    = {};
	Vec3 m_vVelocity  = {};
	Vec3 m_vDirection = {};
	Vec3 m_vAngles    = {};
};

ADD_FEATURE(CEnginePrediction, EnginePrediction);

// RAII batch guard — probe loops have many early returns so restore must be automatic
struct EnginePredictionBatch_t
{
	EnginePredictionBatch_t()  { F::EnginePrediction.BeginBatch(); }
	~EnginePredictionBatch_t() { F::EnginePrediction.EndBatch(); }

	EnginePredictionBatch_t(const EnginePredictionBatch_t&) = delete;
	EnginePredictionBatch_t& operator=(const EnginePredictionBatch_t&) = delete;
};
