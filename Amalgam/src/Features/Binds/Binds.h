#pragma once
#include "../../SDK/SDK.h"

// Main bind type categories
Enum(Bind, Key, Class, WeaponType, ItemSlot, Misc, Health, Movement, Condition)
namespace BindEnum
{
	// Key sub-types: how the key activates
	Enum(Key, Hold, Toggle, DoubleClick)

	// Class conditions
	Enum(Class, Scout, Soldier, Pyro, Demoman, Heavy, Engineer, Medic, Sniper, Spy)

	// Weapon type conditions
	Enum(WeaponType, Hitscan, Projectile, Melee, Throwable)

	//Enum(ItemType, First, Second, Third, Fourth, Fifth, Sixth, Seventh, Eighth, Ninth)

	// Misc game-state conditions
	Enum(Misc, Spectated, SpectatedFirst, SpectatedThird, Zoomed, Aiming)

	// Health threshold conditions
	// m_iInfo stores the percent (0-100); m_iInfoAux stores comparison mode (0=below, 1=above)
	Enum(Health, BelowPercent, AbovePercent)

	// Movement / physics state conditions
	Enum(Movement, Moving, InAir, Ducking, InWater)

	// Player condition flags (TFCond-based)
	Enum(Condition, Burning, Cloaked, Disguised, Ubered, Overhealed, Bleeding, Stunned, Taunting, HasAmmo)
}

Enum(BindVisibility, Always, WhileActive, Hidden)

// ============================================================
//  Bind_t — one node in the bind tree
//
//  AND support:
//    m_bAndEnabled  — when true, BOTH the primary event AND
//                     the AND-event must be active to trigger.
//    m_iAndType     — secondary event type  (same BindEnum::* values)
//    m_iAndInfo     — secondary event info  (same sub-type values)
//    m_iAndInfoAux  — secondary auxiliary   (e.g. health %)
//
//  m_iInfoAux: used by Health binds to store the percent threshold (0-100).
// ============================================================
struct Bind_t
{
	std::string m_sName = "";

	// Primary event
	int m_iType = 0;
	int m_iInfo = 0;
	int m_iInfoAux = 50;   // auxiliary param (e.g. health threshold %)

	// Key
	int m_iKey = 0;

	bool m_bEnabled = true;
	int  m_iVisibility = BindVisibilityEnum::Always;
	bool m_bNot = false;
	bool m_bActive = false;
	KeyStorage m_tKeyStorage = {};

	int m_iParent = DEFAULT_BIND;

	// AND condition ─────────────────────────────────────────
	bool m_bAndEnabled = false;
	int  m_iAndType    = 0;
	int  m_iAndInfo    = 0;
	int  m_iAndInfoAux = 50;
	bool m_bAndNot     = false;   // negate the AND condition independently
	// ───────────────────────────────────────────────────────

	std::vector<BaseVar*> m_vVars = {};
};

class CBinds
{
public:
	void Run();
	void SetVars(CTFPlayer* pLocal, CTFWeaponBase* pWeapon, bool bManage = true);

	bool GetBind(int iID, Bind_t* pBind);

	void AddBind(int iBind, Bind_t& tCond);
	void RemoveBind(int iBind, bool bForce = true);
	int  GetParent(int iBind);
	bool HasChildren(int iBind);
	bool WillBeEnabled(int iBind);

	void Move(int i1, int i2);

	std::vector<Bind_t> m_vBinds = {};
	bool m_bDisplay = false;
};

ADD_FEATURE(CBinds, Binds);
