#include "Binds.h"

#include "../ImGui/Menu/Menu.h"
#include "../Configs/Configs.h"
#include <functional>

#define IsType(t) pBase->m_iType == typeid(t).hash_code()

template <class T>
static inline void SetMain(BaseVar*& pBase, int iBind)
{
	auto pVar = pBase->As<T>();

	if (pVar->Map.contains(iBind))
		pVar->Value = pVar->Map[iBind];
}
#define Set(t, b) if (IsType(t)) SetMain<t>(pBase, b);

static std::unordered_mapset<BaseVar*> s_mVars = {};
static bool s_bUI = false, s_bMenu = false;

static inline void LoopVars(int iBind, std::vector<BaseVar*>& vVars = G::Vars)
{
	const bool bDefault = iBind == DEFAULT_BIND;
	for (auto pBase : vVars)
	{
		if (s_mVars.contains(pBase) || pBase->m_iFlags & (NOSAVE | NOBIND) && !bDefault)
			continue;

		s_mVars[pBase];
		Set(bool, iBind)
		else Set(int, iBind)
		else Set(float, iBind)
		else Set(IntRange_t, iBind)
		else Set(FloatRange_t, iBind)
		else Set(std::string, iBind)
		else Set(VA_LIST(std::vector<std::pair<std::string, Color_t>>), iBind)
		else Set(Color_t, iBind)
		else Set(Gradient_t, iBind)
		else Set(Vec3, iBind)
		else Set(DragBox_t, iBind)
		else Set(WindowBox_t, iBind)
	}
}

// ============================================================
//  EvaluateEventActive
//
//  Resolves a single event (type + info + infoAux) against
//  the current game state.  Used for both the primary event
//  and the AND event — same evaluation, no code duplication.
//
//  Returns the raw bool BEFORE applying a Not flag.
//  The caller is responsible for negation.
// ============================================================
static bool EvaluateEventActive(int iType, int iInfo, int iInfoAux,
                                 CTFPlayer* pLocal, CTFWeaponBase* pWeapon,
                                 KeyStorage* pKeyStorage, int iKey,
                                 bool& bActiveInOut /* only modified for Key type (toggle) */)
{
	switch (iType)
	{
	// ── KEY ──────────────────────────────────────────────────────────
	case BindEnum::Key:
	{
		bool bKey = false;
		switch (iInfo)
		{
		case BindEnum::KeyEnum::Hold:
			bKey = U::KeyHandler.Down(iKey, false, pKeyStorage);
			break;
		case BindEnum::KeyEnum::Toggle:
			bKey = U::KeyHandler.Pressed(iKey, false, pKeyStorage);
			break;
		case BindEnum::KeyEnum::DoubleClick:
			bKey = U::KeyHandler.Double(iKey, false, pKeyStorage);
			break;
		}
		bKey &= !s_bUI || s_bMenu && (!F::Menu.m_bWindowHovered || iKey != VK_LBUTTON && iKey != VK_RBUTTON);

		// Toggle / DoubleClick flip bActiveInOut and return the result
		switch (iInfo)
		{
		case BindEnum::KeyEnum::Hold:
			return bKey;
		case BindEnum::KeyEnum::Toggle:
		case BindEnum::KeyEnum::DoubleClick:
			if (bKey)
				bActiveInOut = !bActiveInOut;
			return bActiveInOut;
		}
		return false;
	}

	// ── CLASS ────────────────────────────────────────────────────────
	case BindEnum::Class:
	{
		const int iClass = pLocal ? pLocal->m_iClass() : TF_CLASS_UNDEFINED;
		switch (iInfo)
		{
		case BindEnum::ClassEnum::Scout:    return iClass == TF_CLASS_SCOUT;
		case BindEnum::ClassEnum::Soldier:  return iClass == TF_CLASS_SOLDIER;
		case BindEnum::ClassEnum::Pyro:     return iClass == TF_CLASS_PYRO;
		case BindEnum::ClassEnum::Demoman:  return iClass == TF_CLASS_DEMOMAN;
		case BindEnum::ClassEnum::Heavy:    return iClass == TF_CLASS_HEAVY;
		case BindEnum::ClassEnum::Engineer: return iClass == TF_CLASS_ENGINEER;
		case BindEnum::ClassEnum::Medic:    return iClass == TF_CLASS_MEDIC;
		case BindEnum::ClassEnum::Sniper:   return iClass == TF_CLASS_SNIPER;
		case BindEnum::ClassEnum::Spy:      return iClass == TF_CLASS_SPY;
		}
		return false;
	}

	// ── WEAPON TYPE ──────────────────────────────────────────────────
	case BindEnum::WeaponType:
		return iInfo == BindEnum::WeaponTypeEnum::Throwable
			? G::Throwing
			: iInfo + 1 == int(SDK::GetWeaponType(pWeapon));

	// ── ITEM SLOT ────────────────────────────────────────────────────
	case BindEnum::ItemSlot:
		return iInfo == (pWeapon ? pWeapon->GetSlot() : -1);

	// ── MISC ─────────────────────────────────────────────────────────
	case BindEnum::Misc:
	{
		switch (iInfo)
		{
		case BindEnum::MiscEnum::Spectated:
		case BindEnum::MiscEnum::SpectatedFirst:
		case BindEnum::MiscEnum::SpectatedThird:
		{
			bool bFirst = false, bThird = false;
			if (auto pResource = H::Entities.GetResource())
			{
				int iLocal = I::EngineClient->GetLocalPlayer();
				for (int n = 1; n <= I::EngineClient->GetMaxClients(); n++)
				{
					auto pPlayer = I::ClientEntityList->GetClientEntity(n)->As<CTFPlayer>();

					if (iLocal == n || pResource->IsFakePlayer(n)
						|| !pPlayer || !pPlayer->IsPlayer() || pPlayer->IsAlive() || pPlayer->IsDormant()
						|| pResource->m_iTeam(iLocal) != pResource->m_iTeam(n))
						continue;

					int iObserverTarget = pPlayer->m_hObserverTarget().GetEntryIndex();
					int iObserverMode   = pPlayer->m_iObserverMode();
					if (iObserverTarget != iLocal)
						continue;

					switch (iObserverMode)
					{
					case OBS_MODE_FIRSTPERSON: bFirst = true; break;
					case OBS_MODE_THIRDPERSON: bThird = true; break;
					}
				}
			}
			switch (iInfo)
			{
			case BindEnum::MiscEnum::Spectated:      return bFirst || bThird;
			case BindEnum::MiscEnum::SpectatedFirst: return bFirst;
			case BindEnum::MiscEnum::SpectatedThird: return bThird;
			}
			return false;
		}
		case BindEnum::MiscEnum::Zoomed:
			return pLocal ? pLocal->InCond(TF_COND_ZOOMED) : false;
		case BindEnum::MiscEnum::Aiming:
			return pLocal ? pLocal->InCond(TF_COND_AIMING) : false;
		}
		return false;
	}

	// ── HEALTH PERCENT ───────────────────────────────────────────────
	//  iInfo    : HealthEnum (BelowPercent / AbovePercent)
	//  iInfoAux : threshold 0-100
	case BindEnum::Health:
	{
		if (!pLocal)
			return false;

		const int iHP    = pLocal->m_iHealth();
		const int iMaxHP = pLocal->GetMaxHealth();
		if (iMaxHP <= 0)
			return false;

		// Clamp threshold to valid range
		const float flThresh = static_cast<float>(std::clamp(iInfoAux, 0, 100));
		const float flPct    = (static_cast<float>(iHP) / static_cast<float>(iMaxHP)) * 100.f;

		switch (iInfo)
		{
		case BindEnum::HealthEnum::BelowPercent: return flPct < flThresh;
		case BindEnum::HealthEnum::AbovePercent: return flPct > flThresh;
		}
		return false;
	}

	// ── MOVEMENT / PHYSICS STATE ─────────────────────────────────────
	case BindEnum::Movement:
	{
		if (!pLocal)
			return false;

		const int iFlags = pLocal->m_fFlags();
		switch (iInfo)
		{
		case BindEnum::MovementEnum::Moving:
		{
			// Consider moving if horizontal speed > 5 ups (filters tiny network jitter)
			const Vec3& vel = pLocal->m_vecVelocity();
			const float flSpeed2D = std::sqrtf(vel.x * vel.x + vel.y * vel.y);
			return flSpeed2D > 5.f;
		}
		case BindEnum::MovementEnum::InAir:
			// Not on ground AND not on a ladder
			return !(iFlags & FL_ONGROUND);
		case BindEnum::MovementEnum::Ducking:
			return (iFlags & FL_DUCKING) != 0;
		case BindEnum::MovementEnum::InWater:
			// nWaterLevel: 0=not, 1=feet, 2=waist, 3=eyes
			return pLocal->m_nWaterLevel() >= 1;
		}
		return false;
	}

	// ── PLAYER CONDITIONS (TFCond-based + ammo) ──────────────────────
	case BindEnum::Condition:
	{
		switch (iInfo)
		{
		case BindEnum::ConditionEnum::Burning:
			return pLocal ? pLocal->InCond(TF_COND_BURNING) : false;

		case BindEnum::ConditionEnum::Cloaked:
			// True when spy is fully or partially cloaked
			return pLocal
				? pLocal->InCond(TF_COND_STEALTHED)
				  || pLocal->InCond(TF_COND_STEALTHED_BLINK)
				  || pLocal->InCond(TF_COND_STEALTHED_USER_BUFF)
				  || pLocal->InCond(TF_COND_STEALTHED_USER_BUFF_FADING)
				: false;

		case BindEnum::ConditionEnum::Disguised:
			return pLocal
				? pLocal->InCond(TF_COND_DISGUISED)
				  || pLocal->InCond(TF_COND_DISGUISING)
				: false;

		case BindEnum::ConditionEnum::Ubered:
			// Any medigun uber variant
			return pLocal ? pLocal->IsUbered() : false;

		case BindEnum::ConditionEnum::Overhealed:
			return pLocal ? pLocal->InCond(TF_COND_HEALTH_OVERHEALED) : false;

		case BindEnum::ConditionEnum::Bleeding:
			return pLocal ? pLocal->InCond(TF_COND_BLEEDING) : false;

		case BindEnum::ConditionEnum::Stunned:
			return pLocal ? pLocal->InCond(TF_COND_STUNNED) : false;

		case BindEnum::ConditionEnum::Taunting:
			return pLocal ? pLocal->IsTaunting() : false;

		case BindEnum::ConditionEnum::HasAmmo:
			// True when the active weapon has at least one round in clip
			// OR has reserve ammo (clip1 == -1 means weapon uses no clip)
			if (!pWeapon) return false;
			{
				const int iClip = pWeapon->m_iClip1();
				return iClip == -1 || iClip > 0;
			}
		}
		return false;
	}

	} // end switch(iType)

	return false;
}

// ============================================================
//  GetBinds  — recursive tree walker
// ============================================================
static inline void GetBinds(int iParent, CTFPlayer* pLocal, CTFWeaponBase* pWeapon,
                             std::vector<Bind_t>& vBinds, bool bManage = true)
{
	if (vBinds.empty())
		return;

	for (int i = 0; i < static_cast<int>(vBinds.size()); i++)
	{
		auto& tBind = vBinds[i];
		if (iParent != tBind.m_iParent || !tBind.m_bEnabled)
			continue;

		if (bManage)
		{
			// ── Evaluate primary event ──────────────────────────────
			bool bPrimary = EvaluateEventActive(
				tBind.m_iType, tBind.m_iInfo, tBind.m_iInfoAux,
				pLocal, pWeapon,
				&tBind.m_tKeyStorage, tBind.m_iKey,
				tBind.m_bActive   // toggle state lives here
			);
			if (tBind.m_bNot)
				bPrimary = !bPrimary;

			// ── Evaluate AND event (if enabled) ────────────────────
			if (tBind.m_bAndEnabled)
			{
				// AND condition gets a dummy KeyStorage; AND events
				// are expected to be stateless (Class/Misc/Health/etc).
				// If someone puts a Toggle in the AND slot the toggle
				// state is not persisted between frames — acceptable
				// since AND-toggles are an edge case.
				bool bAndActive = false;
				KeyStorage dummyKS = {};
				bool bAndActive2   = false; // toggle state for AND (not persisted, see above)
				bool bAnd = EvaluateEventActive(
					tBind.m_iAndType, tBind.m_iAndInfo, tBind.m_iAndInfoAux,
					pLocal, pWeapon,
					&dummyKS, 0,
					bAndActive2
				);
				if (tBind.m_bAndNot)
					bAnd = !bAnd;

				// Both must pass
				tBind.m_bActive = bPrimary && bAnd;
			}
			else
			{
				tBind.m_bActive = bPrimary;
			}
		}

		if (tBind.m_bActive)
		{
			GetBinds(i, pLocal, pWeapon, vBinds, bManage);
			LoopVars(i, tBind.m_vVars);
		}
	}
}

void CBinds::SetVars(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, bool bManage)
{
	s_mVars.clear();
	s_bUI   = I::EngineVGui->IsGameUIVisible() || I::MatSystemSurface->IsCursorVisible() && !I::EngineClient->IsPlayingDemo();
	s_bMenu = F::Menu.m_bIsOpen && !ImGui::GetIO().WantTextInput && !F::Menu.m_bInKeybind;

	GetBinds(DEFAULT_BIND, pLocal, pWeapon, m_vBinds, bManage);
	LoopVars(DEFAULT_BIND);

	m_bDisplay = F::Menu.m_bIsOpen || Vars::Menu::BindWindow.Value && !s_bUI;
}

void CBinds::Run()
{
	if (G::Unload)
		return;

	auto pLocal  = H::Entities.GetLocal();
	auto pWeapon = H::Entities.GetWeapon();

	for (auto& tBind : m_vBinds)
	{	// don't delay inputs for binds
		if (tBind.m_iType != BindEnum::Key)
			continue;

		auto& tKey = tBind.m_tKeyStorage;

		bool bOldIsDown     = tKey.m_bIsDown;
		bool bOldIsPressed  = tKey.m_bIsPressed;
		bool bOldIsDouble   = tKey.m_bIsDouble;
		bool bOldIsReleased = tKey.m_bIsReleased;

		U::KeyHandler.StoreKey(tBind.m_iKey, &tKey);

		tKey.m_bIsDown     = tKey.m_bIsDown     || bOldIsDown;
		tKey.m_bIsPressed  = tKey.m_bIsPressed  || bOldIsPressed;
		tKey.m_bIsDouble   = tKey.m_bIsDouble   || bOldIsDouble;
		tKey.m_bIsReleased = tKey.m_bIsReleased || bOldIsReleased;
	}

	SetVars(pLocal, pWeapon);

	for (auto& tBind : m_vBinds)
	{	// clear inputs for binds
		if (tBind.m_iType != BindEnum::Key)
			continue;

		auto& tKey = tBind.m_tKeyStorage;
		U::KeyHandler.StoreKey(tBind.m_iKey, &tKey);
	}
}



bool CBinds::GetBind(int iID, Bind_t* pBind)
{
	if (iID > DEFAULT_BIND && iID < static_cast<int>(m_vBinds.size()))
	{
		*pBind = m_vBinds[iID];
		return true;
	}
	return false;
}

void CBinds::AddBind(int iBind, Bind_t& tBind)
{
	if (iBind == DEFAULT_BIND || iBind >= static_cast<int>(m_vBinds.size()))
		m_vBinds.push_back(tBind);
	else
		m_vBinds[iBind] = tBind;
}

#define HasType(t, b) IsType(t) && pBase->As<t>()->contains(b)

template <class T>
static inline void RemoveMain(BaseVar*& pBase, int iBind)
{
	auto pVar = pBase->As<T>();

	std::unordered_map<int, T> mMap = {};
	for (auto it = pVar->Map.begin(); it != pVar->Map.end(); it++)
	{
		int  iKey = it->first;
		auto tVal = it->second;
		if (iBind == iKey)
			continue;
		else if (iBind < iKey)
			iKey--;
		mMap[iKey] = tVal;
	}
	pVar->Map = mMap;
}
#define Remove(t, b) if (IsType(t)) RemoveMain<t>(pBase, b);

void CBinds::RemoveBind(int iBind, bool bForce)
{
	if (!bForce)
	{
		for (auto& pBase : G::Vars)
		{
			if (HasType(bool, iBind)
				|| HasType(int, iBind)
				|| HasType(float, iBind)
				|| HasType(IntRange_t, iBind)
				|| HasType(FloatRange_t, iBind)
				|| HasType(std::string, iBind)
				|| HasType(VA_LIST(std::vector<std::pair<std::string, Color_t>>), iBind)
				|| HasType(Color_t, iBind)
				|| HasType(Gradient_t, iBind)
				|| HasType(Vec3, iBind)
				|| HasType(DragBox_t, iBind)
				|| HasType(WindowBox_t, iBind))
				return;
		}
		for (auto& tBind : F::Binds.m_vBinds)
		{
			if (tBind.m_iParent == iBind)
				return;
		}
	}

	std::vector<int> vErases = {};
	std::function<void(int)> fSearchBinds = [&](int iIndex)
	{
		for (int iBind2 = 0; iBind2 < static_cast<int>(m_vBinds.size()); iBind2++)
		{
			auto& tBind = m_vBinds[iBind2];
			if (iIndex == tBind.m_iParent && iIndex != iBind2)
				fSearchBinds(iBind2);
		}
		vErases.push_back(iIndex);
	};
	auto fRemoveBind = [&](int iIndex)
	{
		if (iIndex < static_cast<int>(m_vBinds.size()))
			m_vBinds.erase(std::next(m_vBinds.begin(), iIndex));
		for (auto& tBind : m_vBinds)
		{
			if (tBind.m_iParent != DEFAULT_BIND && tBind.m_iParent > iIndex)
				tBind.m_iParent--;
		}

		for (auto& pBase : G::Vars)
		{
			Remove(bool, iIndex)
			else Remove(int, iIndex)
			else Remove(float, iIndex)
			else Remove(IntRange_t, iIndex)
			else Remove(FloatRange_t, iIndex)
			else Remove(std::string, iIndex)
			else Remove(VA_LIST(std::vector<std::pair<std::string, Color_t>>), iIndex)
			else Remove(Color_t, iIndex)
			else Remove(Gradient_t, iIndex)
			else Remove(Vec3, iIndex)
			else Remove(DragBox_t, iIndex)
			else Remove(WindowBox_t, iIndex)
		}
	};
	fSearchBinds(iBind);
	std::sort(vErases.begin(), vErases.end(), [&](const int a, const int b) -> bool
	{
		return a > b;
	});
	for (auto iIndex : vErases)
		fRemoveBind(iIndex);
}

int CBinds::GetParent(int iBind)
{
	if (iBind > DEFAULT_BIND && iBind < static_cast<int>(m_vBinds.size()))
		return m_vBinds[iBind].m_iParent;
	return DEFAULT_BIND;
}

bool CBinds::HasChildren(int iBind)
{
	auto it = std::ranges::find_if(m_vBinds, [&](const auto& tBind) { return iBind == tBind.m_iParent; });
	return it != m_vBinds.end();
}

bool CBinds::WillBeEnabled(int iBind)
{
	Bind_t tBind;
	while (GetBind(iBind, &tBind))
	{
		if (!tBind.m_bEnabled)
			return false;
		iBind = tBind.m_iParent;
	}
	return true;
}

template <class T>
static inline void SwapMain(BaseVar*& pBase, int iBind1, int iBind2)
{
	auto pVar = pBase->As<T>();

	bool bHas1 = pVar->contains(iBind1), bHas2 = pVar->contains(iBind2);
	if (bHas1 && bHas2)
	{
		auto& tVal1 = pVar->Map[iBind1];
		auto& tVal2 = pVar->Map[iBind2];
		auto  tTemp = tVal1;
		tVal1 = tVal2;
		tVal2 = tTemp;
	}
	else if (bHas1)
	{
		pVar->Map[iBind2] = pVar->Map[iBind1];
		pVar->Map.erase(iBind1);
	}
	else if (bHas2)
	{
		pVar->Map[iBind1] = pVar->Map[iBind2];
		pVar->Map.erase(iBind2);
	}
}
#define Swap(t, i1, i2) if (IsType(t)) SwapMain<t>(pBase, i1, i2);

void CBinds::Move(int i1, int i2)
{
	auto& tBind1 = m_vBinds[i1];
	auto& tBind2 = m_vBinds[i2];
	auto  tTemp  = tBind1;
	tBind1 = tBind2;
	tBind2 = tTemp;

	std::vector<Bind_t*> vBinds1, vBinds2;
	for (auto& tBind : m_vBinds)
	{
		if      (tBind.m_iParent == i1) vBinds1.push_back(&tBind);
		else if (tBind.m_iParent == i2) vBinds2.push_back(&tBind);
	}
	for (auto pBind : vBinds1) pBind->m_iParent = i2;
	for (auto pBind : vBinds2) pBind->m_iParent = i1;

	for (auto& pBase : G::Vars)
	{
		Swap(bool, i1, i2)
		else Swap(int, i1, i2)
		else Swap(float, i1, i2)
		else Swap(IntRange_t, i1, i2)
		else Swap(FloatRange_t, i1, i2)
		else Swap(std::string, i1, i2)
		else Swap(VA_LIST(std::vector<std::pair<std::string, Color_t>>), i1, i2)
		else Swap(Color_t, i1, i2)
		else Swap(Gradient_t, i1, i2)
		else Swap(Vec3, i1, i2)
		else Swap(DragBox_t, i1, i2)
		else Swap(WindowBox_t, i1, i2)
	}
}
