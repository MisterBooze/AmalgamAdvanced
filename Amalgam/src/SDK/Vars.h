#pragma once
#include "../SDK/Definitions/Types.h"
#include "../Utils/Macros/Macros.h"
#include <windows.h>
#include <typeinfo>

#define DEFAULT_BIND -1

template <class T>
class ConfigVar;

class BaseVar
{
public:
	std::vector<const char*> m_vNames;
	int m_iFlags = 0;
	union {
		int i = 0;
		float f;
	} m_unMin;
	union {
		int i = 0;
		float f;
	} m_unMax;
	union {
		int i = 0;
		float f;
	} m_unStep;
	std::vector<const char*> m_vValues = {};
	const char* m_sExtra = nullptr;

protected:
	std::string m_sName;
	const char* m_sSection;

public:
	constexpr const char* Name() const
	{
		return m_sName.c_str();
	}
	constexpr const char* Section() const
	{
		return m_sSection;
	}

public:
	size_t m_iType;

	template <class T>
	inline ConfigVar<T>* As()
	{
		if (typeid(T).hash_code() != m_iType)
			return nullptr;

		return reinterpret_cast<ConfigVar<T>*>(this);
	}
};

namespace G
{
	inline std::vector<BaseVar*> Vars = {};
};

template <class T>
class ConfigVar : public BaseVar
{
public:
	T Value;
	T Default;
	std::unordered_map<int, T> Map = {};

	ConfigVar(T tValue, std::vector<const char*> vNames, const char* sName, const char* sSection, int iFlags = 0, std::vector<const char*> vValues = {}, const char* sNone = nullptr)
	{
		Value = Default = Map[DEFAULT_BIND] = tValue;
		m_iType = typeid(T).hash_code();

		m_vNames = vNames;
		m_sName = std::string(sName).replace(strlen(sName) - 1, 1, "");
		m_sSection = sSection;

		m_iFlags = iFlags;
		m_vValues = vValues;
		m_sExtra = sNone;

		G::Vars.push_back(this);
	}
	ConfigVar(T tValue, std::vector<const char*> vNames, const char* sName, const char* sSection, int iFlags, int iMin, int iMax, int iStep = 1, const char* sFormat = "%i")
	{
		Value = Default = Map[DEFAULT_BIND] = tValue;
		m_iType = typeid(T).hash_code();

		m_vNames = vNames;
		m_sName = std::string(sName).replace(strlen(sName) - 1, 1, "");
		m_sSection = sSection;

		m_iFlags = iFlags;
		m_unMin.i = iMin;
		m_unMax.i = iMax;
		m_unStep.i = iStep;
		m_sExtra = sFormat;

		G::Vars.push_back(this);
	}
	ConfigVar(T tValue, std::vector<const char*> vNames, const char* sName, const char* sSection, int iFlags, float flMin, float flMax, float flStep = 1.f, const char* sFormat = "%g")
	{
		Value = Default = Map[DEFAULT_BIND] = tValue;
		m_iType = typeid(T).hash_code();

		m_vNames = vNames;
		m_sName = std::string(sName).replace(strlen(sName) - 1, 1, "");
		m_sSection = sSection;

		m_iFlags = iFlags;
		m_unMin.f = flMin;
		m_unMax.f = flMax;
		m_unStep.f = flStep;
		m_sExtra = sFormat;

		G::Vars.push_back(this);
	}

	inline T& operator[](int i)
	{
		return Map[i];
	}
	inline bool contains(int i) const
	{
		return Map.contains(i);
	}
};

#define NAMESPACE_BEGIN(name, ...) \
	namespace name { \
		constexpr inline const char* Section() { return !std::string(#__VA_ARGS__).empty() ? ""#__VA_ARGS__ : #name; }
#define NAMESPACE_END(name) \
	}

#define CVar(name, title, value, ...) \
	constexpr inline const char* name##_() { return __FUNCTION__; } \
	inline ConfigVar<decltype(value)> name = { value, { title }, name##_(), Section(), __VA_ARGS__ }
#define CVarValues(name, title, value, flags, none, ...) \
	constexpr inline const char* name##_() { return __FUNCTION__; } \
	inline ConfigVar<decltype(value)> name = { value, { title }, name##_(), Section(), flags, { __VA_ARGS__ }, none }
#define Enum(name, ...) \
	namespace name##Enum { enum name##Enum { __VA_ARGS__ }; }
#define CVarEnum(name, title, value, flags, none, values, ...) \
	CVarValues(name, title, value, flags, none, values); \
	Enum(name, __VA_ARGS__);

#define NONE 0
#define VISUAL (1 << 31)
#define NOSAVE (1 << 30)
#define NOBIND (1 << 29)
#define DEBUGVAR (1 << 28)

// flags to be automatically used in widgets. keep these as the same values as the flags in components, do not include visual flags
#define SLIDER_CLAMP (1 << 2)
#define SLIDER_MIN (1 << 3)
#define SLIDER_MAX (1 << 4)
#define SLIDER_PRECISION (1 << 5)
#define SLIDER_NOAUTOUPDATE (1 << 6)
#define DROPDOWN_MULTI (1 << 2)
#define DROPDOWN_MODIFIABLE (1 << 3)
#define DROPDOWN_NOSANITIZATION (1 << 4)
#define DROPDOWN_CUSTOM (1 << 2)
#define DROPDOWN_AUTOUPDATE (1 << 3)

NAMESPACE_BEGIN(Vars)
	NAMESPACE_BEGIN(Menu)
		CVar(CheatTitle, "Cheat title", std::string("Amalgam"), VISUAL | DROPDOWN_AUTOUPDATE);
		CVar(CheatTag, "Cheat tag", std::string("[Amalgam]"), VISUAL);
		CVar(PrimaryKey, "Primary key", VK_INSERT, NOBIND);
		CVar(SecondaryKey, "Secondary key", VK_F3, NOBIND);

		CVar(BindWindow, "Bind window", true);
		CVar(BindWindowTitle, "Bind window title", true);
		CVar(MenuShowsBinds, "Menu shows binds", false, NOBIND);

		CVarEnum(Indicators, "Indicators", 0b00000, VISUAL | DROPDOWN_MULTI, nullptr,
			VA_LIST("Ticks", "Crit hack", "Spectators", "Ping", "Conditions", "Seed prediction"),
			Ticks = 1 << 0, CritHack = 1 << 1, Spectators = 1 << 2, Ping = 1 << 3, Conditions = 1 << 4, SeedPrediction = 1 << 5);

		CVar(BindsDisplay, "Binds display", DragBox_t(100, 100), VISUAL | NOBIND);
		CVar(TicksDisplay, "Ticks display", DragBox_t(), VISUAL | NOBIND);
		CVar(CritsDisplay, "Crits display", DragBox_t(), VISUAL | NOBIND);
		CVar(SpectatorsDisplay, "Spectators display", DragBox_t(), VISUAL | NOBIND);
		CVar(PingDisplay, "Ping display", DragBox_t(), VISUAL | NOBIND);
		CVar(ConditionsDisplay, "Conditions display", DragBox_t(), VISUAL | NOBIND);
		CVar(SeedPredictionDisplay, "Seed prediction display", DragBox_t(), VISUAL | NOBIND);
		CVar(RecorderDisplay, "Recorder display", DragBox_t(), VISUAL | NOBIND);
		CVar(CheckpointsDisplay, "Checkpoints display", DragBox_t(), VISUAL | NOBIND);

		CVar(Scale, "Scale", 1.f, NOBIND | SLIDER_MIN | SLIDER_PRECISION | SLIDER_NOAUTOUPDATE, 0.75f, 2.f, 0.25f);
		CVar(CheapText, "Cheap text", false, NOBIND);

		NAMESPACE_BEGIN(Theme)
			CVar(Accent, "Accent color", Color_t(175, 150, 255, 255), VISUAL);
			CVar(Background, "Background color", Color_t(0, 0, 0, 250), VISUAL);
			CVar(Active, "Active color", Color_t(255, 255, 255, 255), VISUAL);
			CVar(Inactive, "Inactive color", Color_t(150, 150, 150, 255), VISUAL);
		NAMESPACE_END(Theme)
	NAMESPACE_END(Menu)

	NAMESPACE_BEGIN(Colors)
		CVar(IndicatorGood, "Indicator good", Color_t(0, 255, 100, 255), NOSAVE | DEBUGVAR);
		CVar(IndicatorMid, "Indicator mid", Color_t(255, 200, 0, 255), NOSAVE | DEBUGVAR);
		CVar(IndicatorBad, "Indicator bad", Color_t(255, 0, 0, 255), NOSAVE | DEBUGVAR);
		CVar(IndicatorMisc, "Indicator misc", Color_t(75, 175, 255, 255), NOSAVE | DEBUGVAR);
		CVar(IndicatorTextGood, "Indicator text good", Color_t(150, 255, 150, 255), NOSAVE | DEBUGVAR);
		CVar(IndicatorTextMid, "Indicator text mid", Color_t(255, 200, 0, 255), NOSAVE | DEBUGVAR);
		CVar(IndicatorTextBad, "Indicator text bad", Color_t(255, 150, 150, 255), NOSAVE | DEBUGVAR);
		CVar(IndicatorTextMisc, "Indicator text misc", Color_t(100, 255, 255, 255), NOSAVE | DEBUGVAR);

		CVar(Local, "Local color", Color_t(255, 255, 255), VISUAL);
		CVar(FOVCircle, "FOV circle color", Color_t(255, 255, 255, 100), VISUAL);
		CVar(SpellFootstep, "Spell footstep color", Color_t(255, 255, 255, 255), VISUAL);
		CVar(EdgeBugCircle, "Edge bug circle color", Color_t(255, 255, 255, 255), VISUAL);

		CVar(WorldModulation, VA_LIST("World modulation", "World modulation color"), Color_t(255, 255, 255, 255), VISUAL);
		CVar(SkyModulation, VA_LIST("Sky modulation", "Sky modulation color"), Color_t(255, 255, 255, 255), VISUAL);
		CVar(PropModulation, VA_LIST("Prop modulation", "Prop modulation color"), Color_t(255, 255, 255, 255), VISUAL);
		CVar(ParticleModulation, VA_LIST("Particle modulation", "Particle modulation color"), Color_t(255, 255, 255, 255), VISUAL);
		CVar(FogModulation, VA_LIST("Fog modulation", "Fog modulation color"), Color_t(255, 255, 255, 255), VISUAL);

		CVar(Line, "Line color", Color_t(255, 255, 255, 255), VISUAL);
		CVar(LineIgnoreZ, "Line ignore Z color", Color_t(255, 255, 255, 0), VISUAL);

		CVar(BoneHitboxEdge, "Bone hitbox edge color", Color_t(255, 255, 255, 255), VISUAL);
		CVar(BoneHitboxEdgeIgnoreZ, "Bone hitbox edge ignore Z color", Color_t(255, 255, 255, 0), VISUAL);
		CVar(BoneHitboxFace, "Bone hitbox face color", Color_t(255, 255, 255, 0), VISUAL);
		CVar(BoneHitboxFaceIgnoreZ, "Bone hitbox face ignore Z color", Color_t(255, 255, 255, 0), VISUAL);
		CVar(TargetHitboxEdge, "Target hitbox edge color", Color_t(255, 150, 150, 255), VISUAL);
		CVar(TargetHitboxEdgeIgnoreZ, "Target hitbox edge ignore Z color", Color_t(255, 150, 150, 0), VISUAL);
		CVar(TargetHitboxFace, "Target hitbox face color", Color_t(255, 150, 150, 0), VISUAL);
		CVar(TargetHitboxFaceIgnoreZ, "Target hitbox face ignore Z color", Color_t(255, 150, 150, 0), VISUAL);
		CVar(BoundHitboxEdge, "Bound hitbox edge color", Color_t(255, 255, 255, 255), VISUAL);
		CVar(BoundHitboxEdgeIgnoreZ, "Bound hitbox edge ignore Z color", Color_t(255, 255, 255, 0), VISUAL);
		CVar(BoundHitboxFace, "Bound hitbox face color", Color_t(255, 255, 255, 0), VISUAL);
		CVar(BoundHitboxFaceIgnoreZ, "Bound hitbox face ignore Z color", Color_t(255, 255, 255, 0), VISUAL);

		CVar(PlayerPath, "Player path color", Color_t(255, 255, 255, 0), VISUAL);
		CVar(PlayerPathIgnoreZ, "Player path ignore Z color", Color_t(255, 255, 255, 255), VISUAL);
		CVar(ProjectilePath, "Projectile path color", Color_t(255, 255, 255, 0), VISUAL);
		CVar(ProjectilePathIgnoreZ, "Projectile path ignore Z color", Color_t(255, 255, 255, 255), VISUAL);
		CVar(TrajectoryPath, "Trajectory path color", Color_t(255, 255, 255, 0), VISUAL);
		CVar(TrajectoryPathIgnoreZ, "Trajectory path ignore Z color", Color_t(255, 255, 255, 255), VISUAL);
		CVar(ShotPath, "Shot path color", Color_t(255, 255, 255, 0), VISUAL);
		CVar(ShotPathIgnoreZ, "Shot path ignore Z color", Color_t(255, 255, 255, 255), VISUAL);
		CVar(SplashRadius, "Splash radius color", Color_t(255, 255, 255, 0), VISUAL);
		CVar(SplashRadiusIgnoreZ, "Splash radius ignore Z color", Color_t(255, 255, 255, 255), VISUAL);
		CVar(RealPath, "Real path color", Color_t(255, 255, 255, 0), NOSAVE | DEBUGVAR);
		CVar(RealPathIgnoreZ, "Real path ignore Z color", Color_t(255, 255, 255, 255), NOSAVE | DEBUGVAR);
	NAMESPACE_END(Colors)

	NAMESPACE_BEGIN(Aimbot)
		NAMESPACE_BEGIN(General, Aimbot)
			CVarEnum(AimType, "Aim type", 0, NONE, nullptr,
				VA_LIST("Off", "Plain", "Smooth", "Silent", "Locking", "Assistive"),
				Off, Plain, Smooth, Silent, Locking, Assistive);
			CVarEnum(TargetSelection, "Target selection", 0, NONE, nullptr,
				VA_LIST("FOV", "Distance", "Hybrid"),
				FOV, Distance, Hybrid);
			CVarEnum(Target, "Target", 0b0000001, DROPDOWN_MULTI, nullptr,
				VA_LIST("Players", "Sentries", "Dispensers", "Teleporters", "Stickies", "NPCs", "Bombs"),
				Players = 1 << 0, Sentry = 1 << 1, Dispenser = 1 << 2, Teleporter = 1 << 3, Stickies = 1 << 4, NPCs = 1 << 5, Bombs = 1 << 6,
				All = Players | Sentry | Dispenser | Teleporter | Stickies | NPCs | Bombs, Building = Sentry | Dispenser | Teleporter);
			CVarEnum(Ignore, "Ignore", 0b00000001000, DROPDOWN_MULTI, nullptr,
				VA_LIST("Friends", "Party", "Unprioritized", "Invulnerable", "Invisible", "Unsimulated", "Dead ringer", "Vaccinator", "Disguised", "Taunting", "Team"),
				Friends = 1 << 0, Party = 1 << 1, Unprioritized = 1 << 2, Invulnerable = 1 << 3, Invisible = 1 << 4, Unsimulated = 1 << 5, DeadRinger = 1 << 6, Vaccinator = 1 << 7, Disguised = 1 << 8, Taunting = 1 << 9, Team = 1 << 10);
			CVar(AimFOV, "Aim FOV", 180.f, SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 180.f);
			CVar(MaxTargets, "Max targets", 2, SLIDER_MIN, 1, 6);
			CVar(IgnoreInvisible, "Ignore invisible", 50.f, SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 100.f, 10.f, "%g%%");
			CVar(AssistStrength, "Assist strength", 25.f, SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 100.f, 1.f, "%g%%");
			CVar(TickTolerance, "Tick tolerance", 4, SLIDER_CLAMP, 0, 21);
			CVar(AutoShoot, "Auto shoot", true);
			CVar(FOVCircle, "FOV Circle", true, VISUAL);
			CVar(LeadAndRestrict, "Lead and restrict", false, VISUAL);
			CVar(NoSpread, "No spread", false);

			CVarEnum(AimHoldsFire, "Aim holds fire", 2, NOSAVE | DEBUGVAR, nullptr,
				VA_LIST("False", "Minigun only", "Always"),
				False, MinigunOnly, Always);
			CVar(NoSpreadOffset, "No spread offset", 0.f, NOSAVE | DEBUGVAR | SLIDER_PRECISION, -1.f, 1.f, 0.1f);
			CVar(NoSpreadAverage, "No spread average", 5, NOSAVE | DEBUGVAR | SLIDER_MIN, 1, 25);
			CVar(NoSpreadInterval, "No spread interval", 0.1f, NOSAVE | DEBUGVAR | SLIDER_MIN, 0.05f, 5.f, 0.1f, "%gs");
			CVar(NoSpreadBackupInterval, "No spread backup interval", 2.f, NOSAVE | DEBUGVAR | SLIDER_MIN, 2.f, 10.f, 0.1f, "%gs");
		NAMESPACE_END(Global)

		NAMESPACE_BEGIN(Hitscan)
			CVarEnum(Hitboxes, VA_LIST("Hitboxes", "Hitscan hitboxes"), 0b000111, DROPDOWN_MULTI, nullptr,
				VA_LIST("Head", "Body", "Pelvis", "Arms", "Legs", "##Divider", "Bodyaim if lethal", "Headshot only"),
				Head = 1 << 0, Body = 1 << 1, Pelvis = 1 << 2, Arms = 1 << 3, Legs = 1 << 4, BodyaimIfLethal = 1 << 5, HeadshotOnly = 1 << 6);
			CVarValues(MultipointHitboxes, "Multipoint hitboxes", 0b00000, DROPDOWN_MULTI, "All",
				VA_LIST("Head", "Body", "Pelvis", "Arms", "Legs"));
			CVarEnum(Modifiers, VA_LIST("Modifiers", "Hitscan modifiers"), 0b0100000, DROPDOWN_MULTI, nullptr,
				VA_LIST("Tapfire", "Wait for headshot", "Wait for charge", "Scoped only", "Auto scope", "Auto rev minigun", "Extinguish team"),
				Tapfire = 1 << 0, WaitForHeadshot = 1 << 1, WaitForCharge = 1 << 2, ScopedOnly = 1 << 3, AutoScope = 1 << 4, AutoRev = 1 << 5, ExtinguishTeam = 1 << 6);
			CVar(MultipointScale, "Multipoint scale", 0.f, SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 100.f, 5.f, "%g%%");
			CVar(TapfireDistance, "Tapfire distance", 1000.f, SLIDER_MIN | SLIDER_PRECISION, 250.f, 1000.f, 50.f);

			CVarEnum(PeekCheck, "Peek check", 1, NOSAVE | DEBUGVAR, nullptr,
				VA_LIST("Off", "Doubletap only", "Always"),
				Off, DoubletapOnly, Always);
			CVar(PeekAmount, "Peek amount", 1, NOSAVE | DEBUGVAR, 0, 5);
			CVar(BoneSizeSubtract, "Bone size subtract", 1.f, NOSAVE | DEBUGVAR | SLIDER_MIN, 0.f, 4.f, 0.25f);
			CVar(BoneSizeMinimumScale, "Bone size minimum scale", 1.f, NOSAVE | DEBUGVAR | SLIDER_CLAMP, 0.f, 1.f, 0.1f);
		NAMESPACE_END(HITSCAN)

		NAMESPACE_BEGIN(Projectile)
			CVarEnum(StrafePrediction, VA_LIST("Predict", "Strafe prediction"), 0b11, DROPDOWN_MULTI, "Off",
				VA_LIST("Air strafing", "Ground strafing"),
				Air = 1 << 0, Ground = 1 << 1);
			CVarEnum(SplashPrediction, VA_LIST("Splash", "Splash prediction"), 0, NONE, nullptr,
				VA_LIST("Off", "Include", "Prefer", "Only"),
				Off, Include, Prefer, Only);
			CVarEnum(AutoDetonate, "Auto detonate", 0b00, DROPDOWN_MULTI, "Off",
				VA_LIST("Stickies", "Flares", "##Divider", "Prevent self damage", "Ignore invisible"),
				Stickies = 1 << 0, Flares = 1 << 1, PreventSelfDamage = 1 << 2, IgnoreInvisible = 1 << 3);
			CVarEnum(AutoAirblast, "Auto airblast", 0b000, DROPDOWN_MULTI, "Off",
				VA_LIST("Enabled", "##Divider", "Redirect", "Ignore FOV"),
				Enabled = 1 << 0, Redirect = 1 << 1, IgnoreFOV = 1 << 2);
			CVarEnum(Hitboxes, VA_LIST("Hitboxes", "Projectile hitboxes"), 0b001111, DROPDOWN_MULTI, nullptr,
				VA_LIST("Auto", "##Divider", "Head", "Body", "Feet", "##Divider", "Bodyaim if lethal", "Prioritize feet"),
				Auto = 1 << 0, Head = 1 << 1, Body = 1 << 2, Feet = 1 << 3, BodyaimIfLethal = 1 << 4, PrioritizeFeet = 1 << 5);
			CVarEnum(Modifiers, VA_LIST("Modifiers", "Projectile modifiers"), 0b0010, DROPDOWN_MULTI, nullptr,
				VA_LIST("Charge weapon", "Cancel charge", "Use arm time", "Air splash", "Lob angles", "Target dormant"),
				ChargeWeapon = 1 << 0, CancelCharge = 1 << 1, UseArmTime = 1 << 2, AirSplash = 1 << 3, LobAngles = 1 << 4, TargetDormant = 1 << 5);
			CVar(MaxSimulationTime, "Max simulation time", 1.f, SLIDER_MIN | SLIDER_PRECISION, 0.1f, 2.5f, 0.25f, "%gs");
			CVar(HitChance, "Hit chance", 0.f, SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 100.f, 10.f, "%g%%");
			CVar(AutodetRadius, "Autodet radius", 90.f, SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 100.f, 10.f, "%g%%");
			CVar(SplashRadius, "Splash radius", 90.f, SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 100.f, 10.f, "%g%%");
			CVar(AutoRelease, "Auto release", 0.f, SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 100.f, 5.f, "%g%%");

			CVar(GroundSamples, "Samples", 33, NOSAVE | DEBUGVAR, 3, 66);
			CVar(GroundStraightFuzzyValue, "Straight fuzzy value", 100.f, NOSAVE | DEBUGVAR | SLIDER_PRECISION, 0.f, 500.f, 25.f);
			CVar(GroundLowMinimumSamples, "Low min samples", 16, NOSAVE | DEBUGVAR, 3, 66);
			CVar(GroundHighMinimumSamples, "High min samples", 33, NOSAVE | DEBUGVAR, 3, 66);
			CVar(GroundLowMinimumDistance, "Low min distance", 0.f, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0.f, 2500.f, 100.f);
			CVar(GroundHighMinimumDistance, "High min distance", 1000.f, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0.f, 2500.f, 100.f);
			CVar(GroundMaxChanges, "Max changes", 0, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0, 5);
			CVar(GroundMaxChangeTime, "Max change time", 0, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0, 66);

			CVar(AirSamples, "Samples", 33, NOSAVE | DEBUGVAR, 3, 66);
			CVar(AirStraightFuzzyValue, "Straight fuzzy value", 0.f, NOSAVE | DEBUGVAR | SLIDER_PRECISION, 0.f, 500.f, 25.f);
			CVar(AirLowMinimumSamples, "Low min samples", 16, NOSAVE | DEBUGVAR, 3, 66);
			CVar(AirHighMinimumSamples, "High min samples", 16, NOSAVE | DEBUGVAR, 3, 66);
			CVar(AirLowMinimumDistance, "Low min distance", 100000.f, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0.f, 2500.f, 100.f);
			CVar(AirHighMinimumDistance, "High min distance", 100000.f, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0.f, 2500.f, 100.f);
			CVar(AirMaxChanges, "Max changes", 2, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0, 5);
			CVar(AirMaxChangeTime, "Max change time", 16, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0, 66);

			CVar(VelocityAverageCount, "Velocity average count", 5, NOSAVE | DEBUGVAR, 1, 10);
			CVar(VerticalShift, "Vertical shift", 5.f, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0.f, 10.f, 0.5f);
			CVar(DragOverride, "Drag override", 0.f, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0.f, 1.f, 0.01f);
			CVar(TimeOverride, "Time override", 0.f, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0.f, 2.f, 0.01f);
			CVar(LobAnglesUnderpredict, "Lob angles underpredict", true, NOSAVE | DEBUGVAR);
			CVar(HuntsmanLerp, "Huntsman lerp", 50.f, NOSAVE | DEBUGVAR | SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 100.f, 1.f, "%g%%");
			CVar(HuntsmanLerpLow, "Huntsman lerp low", 100.f, NOSAVE | DEBUGVAR | SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 100.f, 1.f, "%g%%");
			CVar(HuntsmanAdd, "Huntsman add", 0.f, NOSAVE | DEBUGVAR | SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 20.f);
			CVar(HuntsmanAddLow, "Huntsman add low", 0.f, NOSAVE | DEBUGVAR | SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 20.f);
			CVar(HuntsmanClamp, "Huntsman clamp", 5.f, NOSAVE | DEBUGVAR | SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 10.f, 0.5f);
			CVar(HuntsmanPullPoint, "Huntsman pull point", false, NOSAVE | DEBUGVAR);
			CVar(HuntsmanPullNoZ, "Pull no Z", false, NOSAVE | DEBUGVAR);

			CVarEnum(SplashMode, "Splash mode", 0, NOSAVE | DEBUGVAR, nullptr,
				VA_LIST("Trace", "Face"),
				Trace, Face);
			CVar(SplashAirCount, "Splash air count", 0, NOSAVE | DEBUGVAR | SLIDER_MIN, 0, 10);
			CVar(SplashPointsDirect, "Splash points direct", 100, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0, 400, 5);
			CVar(SplashPointsArc, "Splash points arc", 100, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0, 400, 5);
			CVar(SplashRotateX, "Splash Rx", -1.f, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, -1.f, 360.f);
			CVar(SplashRotateY, "Splash Ry", -1.f, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, -1.f, 360.f);
			CVar(SplashDensityDirect, "Splash density direct", 40.f, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0.f, 100.f, 1.f);
			CVar(SplashDensityArc, "Splash density arc", 40.f, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0.f, 100.f, 1.f);
			CVar(SplashSamplesCutoff, "Splash samples cutoff", 0.0000001f, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0.f, 0.000001f, 0.00000001f);
			CVar(SplashRestrictDirect, "Splash restrict direct", 100, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 1, 400, 5);
			CVar(SplashRestrictArc, "Splash restrict arc", 5, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 1, 400, 5);
			CVar(SplashRestrictFirst, "Splash restrict first", 25, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 1, 400, 5);
			CVar(DirectTraceInterval, "Direct trace interval", 1, NOSAVE | DEBUGVAR | SLIDER_MIN, 1, 20);
			CVar(SplashTraceInterval, "Splash trace interval", 10, NOSAVE | DEBUGVAR | SLIDER_MIN, 1, 20);
			CVar(LobTraceInterval, "Lob trace interval", 20, NOSAVE | DEBUGVAR | SLIDER_MIN, 1, 20);
			CVar(IntervalRetest, "Interval retest", true, NOSAVE | DEBUGVAR);

			CVar(DeltaCount, "Delta count", 5, NOSAVE | DEBUGVAR, 1, 5);
			CVarEnum(DeltaMode, "Delta mode", 0, NOSAVE | DEBUGVAR, nullptr,
				VA_LIST("Average", "Max"),
				Average, Max);
			CVarEnum(MovesimFrictionFlags, "Movesim friction flags", 0b01, NOSAVE | DEBUGVAR | DROPDOWN_MULTI, nullptr,
				VA_LIST("Run reduce", "Calculate increase"),
				RunReduce = 1 << 0, CalculateIncrease = 1 << 1);
		NAMESPACE_END(Projectile)

		NAMESPACE_BEGIN(Melee)
			CVar(AutoBackstab, "Auto backstab", true);
			CVar(IgnoreRazorback, "Ignore razorback", false);
			CVar(SwingPrediction, "Swing prediction", true);
			CVar(WhipTeam, "Whip team", false);

			CVar(SwingTicks, "Swing ticks", 13, NOSAVE | DEBUGVAR | SLIDER_MIN, 0, 14);
			CVar(SwingPredictLag, "Swing predict lag", true, NOSAVE | DEBUGVAR);
			CVarEnum(SwingValidateMode, "Swing validate mode", 0, NOSAVE | DEBUGVAR, nullptr,
				VA_LIST("Both", "Swing", "Simulated"),
				Both, Swing, Simulated);
			CVarEnum(BackstabFlags, "Backstab flags", 0b11, NOSAVE | DEBUGVAR | DROPDOWN_MULTI, nullptr,
				VA_LIST("Account ping", "Double test"),
				AccountPing = 1 << 0, DoubleTest = 1 << 1);
		NAMESPACE_END(Melee)

		NAMESPACE_BEGIN(Healing)
			CVarEnum(HealPriority, "Heal priority", 0, NONE, nullptr,
				VA_LIST("None", "Prioritize team", "Prioritize friends", "Friends only"),
				None, PrioritizeTeam, PrioritizeFriends, FriendsOnly);
			CVarEnum(DangerIgnore, "Danger ignore", 0b1000, DROPDOWN_MULTI, nullptr,
				VA_LIST("Friends", "Party", "Unprioritized", "Ignored"),
				Friends = 1 << 0, Party = 1 << 1, Unprioritized = 1 << 2, Ignored = 1 << 3,
				Shared = Friends | Party | Unprioritized);
			CVar(AutoHeal, "Auto heal", false);
			CVar(AutoArrow, "Auto arrow", false);
			CVar(AutoRepair, "Auto repair", false);
			CVar(AutoSandvich, "Auto sandvich", false);
			CVar(AutoVaccinator, "Auto vaccinator", false);
			CVar(ActivateOnVoice, "Activate on voice", false);

			CVar(AutoVaccinatorBulletScale, "Auto vaccinator bullet scale", 100.f, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0.f, 200.f, 10.f, "%g%%");
			CVar(AutoVaccinatorBlastScale, "Auto vaccinator blast scale", 100.f, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0.f, 200.f, 10.f, "%g%%");
			CVar(AutoVaccinatorFireScale, "Auto vaccinator fire scale", 100.f, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0.f, 200.f, 10.f, "%g%%");
			CVar(AutoVaccinatorFlamethrowerDamageOnly, "Auto vaccinator flamethrower damage only", false, NOSAVE | DEBUGVAR);
		NAMESPACE_END(Healing)
	NAMESPACE_END(Aimbot)
	
	NAMESPACE_BEGIN(CritHack, Crit Hack)
		CVar(ForceCrits, "Force crits", false);
		CVar(AvoidRandomCrits, "Avoid random crits", false);
		CVar(AlwaysMeleeCrit, "Always melee crit", false);
		CVar(CritEffects, "Crit effects", false);
	NAMESPACE_END(CritHack)

	NAMESPACE_BEGIN(Backtrack)
		CVar(Latency, "Fake latency", 0, SLIDER_CLAMP, 0, 1000, 5);
		CVar(Interp, "Fake interp", 0, SLIDER_CLAMP | SLIDER_PRECISION, 0, 1000, 5);
		CVar(Window, VA_LIST("Window", "Backtrack window"), 185, SLIDER_CLAMP | SLIDER_PRECISION, 0, 200, 5);
		CVar(PreferOnShot, "Prefer on shot", false);

		CVar(Offset, "Offset", 0, NOSAVE | DEBUGVAR, -1, 1);
	NAMESPACE_END(Backtrack)

	NAMESPACE_BEGIN(Doubletap)
		CVar(Doubletap, "Doubletap", false);
		CVar(Warp, "Warp", false);
		CVar(RechargeTicks, "Recharge ticks", false);
		CVar(AntiWarp, "Anti-warp", true);
		CVar(TickLimit, "Tick limit", 22, SLIDER_CLAMP, 2, 22);
		CVar(WarpRate, "Warp rate", 22, SLIDER_CLAMP, 2, 22);
		CVar(RechargeLimit, "Recharge limit", 24, SLIDER_MIN, 1, 24);
		CVar(PassiveRecharge, "Passive recharge", 0, SLIDER_CLAMP, 0, 67);
	NAMESPACE_END(DoubleTap)

	NAMESPACE_BEGIN(Fakelag)
		CVarEnum(Fakelag, "Fakelag", 0, NONE, nullptr,
			VA_LIST("Off", "Plain", "Random", "Adaptive"),
			Off, Plain, Random, Adaptive);
		CVarEnum(Options, VA_LIST("Options", "Fakelag options"), 0b000, DROPDOWN_MULTI, nullptr,
			VA_LIST("Only moving", "On unduck", "Not airborne"),
			OnlyMoving = 1 << 0, OnUnduck = 1 << 1, NotAirborne = 1 << 2);
		CVar(PlainTicks, "Plain ticks", 12, SLIDER_CLAMP, 1, 22);
		CVar(RandomTicks, "Random ticks", IntRange_t(14, 18), SLIDER_CLAMP, 1, 22, 1, "%i - %i");
		CVar(UnchokeOnAttack, "Unchoke on attack", true);
		CVar(RetainBlastJump, "Retain blastjump", false);

		CVar(RetainSoldierOnly, "Retain blastjump soldier only", true, NOSAVE | DEBUGVAR);
	NAMESPACE_END(FakeLag)

	NAMESPACE_BEGIN(AutoPeek, Auto Peek)
		CVar(Enabled, VA_LIST("Enabled", "Auto peek"), false);
	NAMESPACE_END(AutoPeek)

	NAMESPACE_BEGIN(Speedhack)
		CVar(Scale, VA_LIST("Scale", "SpeedHack scale"), 1, NONE, 1, 50);
	NAMESPACE_END(Speedhack)

	NAMESPACE_BEGIN(AntiAim, Antiaim)
		CVar(Enabled, VA_LIST("Enabled", "Antiaim enabled"), false);
		CVarEnum(PitchReal, "Real pitch", 0, NONE, nullptr,
			VA_LIST("None", "Up", "Down", "Zero", "Jitter", "Reverse jitter"),
			None, Up, Down, Zero, Jitter, ReverseJitter);
		CVarEnum(PitchFake, "Fake pitch", 0, NONE, nullptr,
			VA_LIST("None", "Up", "Down", "Jitter", "Reverse jitter"),
			None, Up, Down, Jitter, ReverseJitter);
		Enum(Yaw, Forward, Left, Right, Backwards, Edge, Jitter, Spin);
		CVarValues(YawReal, "Real yaw", 0, NONE, nullptr,
			"Forward", "Left", "Right", "Backwards", "Edge", "Jitter", "Spin");
		CVarValues(YawFake, "Fake yaw", 0, NONE, nullptr,
			"Forward", "Left", "Right", "Backwards", "Edge", "Jitter", "Spin");
		Enum(YawMode, View, Target);
		CVarValues(RealYawBase, "Real base", 0, NONE, nullptr,
			"View", "Target");
		CVarValues(FakeYawBase, "Fake base", 0, NONE, nullptr,
			"View", "Target");
		CVar(RealYawOffset, "Real offset", 0.f, SLIDER_CLAMP | SLIDER_PRECISION, -180.f, 180.f, 5.f);
		CVar(FakeYawOffset, "Fake offset", 0.f, SLIDER_CLAMP | SLIDER_PRECISION, -180.f, 180.f, 5.f);
		CVar(RealYawValue, "Real value", 90.f, SLIDER_CLAMP | SLIDER_PRECISION, -180.f, 180.f, 5.f);
		CVar(FakeYawValue, "Fake value", -90.f, SLIDER_CLAMP | SLIDER_PRECISION, -180.f, 180.f, 5.f);
		CVar(SpinSpeed, "Spin speed", 15.f, SLIDER_PRECISION, -30.f, 30.f);
		CVar(MinWalk, "Minwalk", true);
		CVar(HidePitchOnShot, "Hide pitch on shot", false);

		CVar(AntiAimLines, "Antiaim lines", false, NOSAVE);
	NAMESPACE_END(AntiAim)

	NAMESPACE_BEGIN(Resolver)
		CVar(Enabled, VA_LIST("Enabled", "Resolver enabled"), false);
		CVar(AutoResolve, "Auto resolve", false);
		CVar(AutoResolveCheatersOnly, "Auto resolve cheaters only", false);
		CVar(AutoResolveHeadshotOnly, "Auto resolve headshot only", false);
		CVar(AutoResolveYawAmount, "Auto resolve yaw", 90.f, SLIDER_CLAMP | SLIDER_PRECISION, -180.f, 180.f, 45.f);
		CVar(AutoResolvePitchAmount, "Auto resolve pitch", 90.f, SLIDER_CLAMP, -180.f, 180.f, 90.f);
		CVar(CycleYaw, "Cycle yaw", 0.f, SLIDER_CLAMP | SLIDER_PRECISION, -180.f, 180.f, 45.f);
		CVar(CyclePitch, "Cycle pitch", 0.f, SLIDER_CLAMP, -180.f, 180.f, 90.f);
		CVar(CycleView, "Cycle view", false);
		CVar(CycleMinwalk, "Cycle minwalk", false);
	NAMESPACE_END(Resolver)

	NAMESPACE_BEGIN(ESP)
		CVarValues(ActiveGroups, "Active groups", int(0b11111111111111111111111111111111), VISUAL | DROPDOWN_MULTI | DROPDOWN_NOSANITIZATION, nullptr);
	NAMESPACE_END(ESP)

	NAMESPACE_BEGIN(Visuals)
		NAMESPACE_BEGIN(UI)
			CVarEnum(StreamerMode, "Streamer mode", 0, VISUAL, nullptr,
				VA_LIST("Off", "Local", "Friends", "Party", "All"),
				Off, Local, Friends, Party, All);
			CVarEnum(ChatTags, "Chat tags", 0b000, VISUAL | DROPDOWN_MULTI, nullptr,
				VA_LIST("Local", "Friends", "Party", "Assigned"),
				Local = 1 << 0, Friends = 1 << 1, Party = 1 << 2, Assigned = 1 << 3);
			CVar(FieldOfView, "Field of view## FOV", 0.f, VISUAL | SLIDER_MIN | SLIDER_PRECISION, 0.f, 160.f, 5.f);
			CVar(ZoomFieldOfView, "Zoomed field of view## Zoomed FOV", 0.f, VISUAL | SLIDER_MIN | SLIDER_PRECISION, 0.f, 160.f, 5.f);
			CVar(AspectRatio, "Aspect ratio", 0.f, VISUAL | SLIDER_MIN | SLIDER_PRECISION, 0.f, 5.f, 0.05f);
			CVar(RevealScoreboard, "Reveal scoreboard", false, VISUAL);
			CVar(ScoreboardUtility, "Scoreboard utility", false);
			CVar(ScoreboardColors, "Scoreboard colors", false, VISUAL);
			CVar(CleanScreenshots, "Clean screenshots", true);
		NAMESPACE_END(UI)

		NAMESPACE_BEGIN(Thirdperson)
			CVar(Enabled, "Thirdperson", false, VISUAL);
			CVar(Crosshair, VA_LIST("Crosshair", "Thirdperson crosshair"), false, VISUAL);
			CVar(Distance, "Thirdperson distance", 150.f, VISUAL | SLIDER_PRECISION, 0.f, 400.f, 10.f);
			CVar(Right, "Thirdperson right", 0.f, VISUAL | SLIDER_PRECISION, -100.f, 100.f, 5.f);
			CVar(Up, "Thirdperson up", 0.f, VISUAL | SLIDER_PRECISION, -100.f, 100.f, 5.f);

			CVar(Scale, "Thirdperson scales", true, NOSAVE | DEBUGVAR);
			CVar(Collide, "Thirdperson collides", true, NOSAVE | DEBUGVAR);
		NAMESPACE_END(ThirdPerson)

		NAMESPACE_BEGIN(Removals)
			CVar(Interpolation, VA_LIST("Interpolation", "Remove interpolation"), false);
			CVar(Lerp, VA_LIST("Lerp", "Remove lerp"), true);
			CVar(Disguises, VA_LIST("Disguises", "Remove disguises"), false, VISUAL);
			CVar(Taunts, VA_LIST("Taunts", "Remove taunts"), false, VISUAL);
			CVar(Scope, VA_LIST("Scope", "Remove scope"), false, VISUAL);
			CVar(PostProcessing, VA_LIST("Post processing", "Remove post processing"), false, VISUAL);
			CVar(ScreenOverlays, VA_LIST("Screen overlays", "Remove screen overlays"), false, VISUAL);
			CVar(ScreenEffects, VA_LIST("Screen effects", "Remove screen effects"), false, VISUAL);
			CVar(ViewPunch, VA_LIST("View punch", "Remove view punch"), false, VISUAL);
			CVar(AngleForcing, VA_LIST("Angle forcing", "Remove angle forcing"), false, VISUAL);
			CVar(Ragdolls, VA_LIST("Ragdolls", "Remove ragdoll"), false, VISUAL);
			CVar(Gibs, VA_LIST("Gibs", "Remove gibs"), false, VISUAL);
			CVar(MOTD, VA_LIST("MOTD", "Remove MOTD"), false, VISUAL);
		NAMESPACE_END(Removals)

		NAMESPACE_BEGIN(Effects)
			CVarValues(BulletTracer, "Bullet tracer", std::string("Default"), VISUAL | DROPDOWN_CUSTOM, nullptr,
				"Default", "None", "Big nasty", "Distortion trail", "Machina", "Sniper rail", "Short circuit", "C.A.P.P.E.R", "Merasmus ZAP", "Merasmus ZAP 2", "Black ink", "Line", "Line ignore Z", "Beam");
			CVarValues(CritTracer, "Crit tracer", std::string("Default"), VISUAL | DROPDOWN_CUSTOM, nullptr,
				"Default", "None", "Big nasty", "Distortion trail", "Machina", "Sniper rail", "Short circuit", "C.A.P.P.E.R", "Merasmus ZAP", "Merasmus ZAP 2", "Black ink", "Line", "Line ignore Z", "Beam");
			CVarValues(MedigunBeam, "Medigun beam", std::string("Default"), VISUAL | DROPDOWN_CUSTOM, nullptr,
				"Default", "None", "Uber", "Dispenser", "Passtime", "Bombonomicon", "White", "Orange");
			CVarValues(MedigunCharge, "Medigun charge", std::string("Default"), VISUAL | DROPDOWN_CUSTOM, nullptr,
				"Default", "None", "Electrocuted", "Halloween", "Fireball", "Teleport", "Burning", "Scorching", "Purple energy", "Green energy", "Nebula", "Purple stars", "Green stars", "Sunbeams", "Spellbound", "Purple sparks", "Yellow sparks", "Green zap", "Yellow zap", "Plasma", "Frostbite", "Time warp", "Purple souls", "Green souls", "Bubbles", "Hearts");
			CVarValues(ProjectileTrail, "Projectile trail", std::string("Default"), VISUAL | DROPDOWN_CUSTOM, nullptr,
				"Default", "None", "Rocket", "Critical", "Energy", "Charged", "Ray", "Fireball", "Teleport", "Fire", "Flame", "Sparks", "Flare", "Trail", "Health", "Smoke", "Bubbles", "Halloween", "Monoculus", "Sparkles", "Rainbow");
			CVarEnum(SpellFootsteps, "Spell footsteps", 0, VISUAL, nullptr,
				VA_LIST("Off", "Color", "Team", "Halloween"),
				Off, Color, Team, Halloween);
			CVarEnum(RagdollEffects, "Ragdoll effects", 0b000000, VISUAL | DROPDOWN_MULTI, nullptr,
				VA_LIST("Burning", "Electrocuted", "Ash", "Dissolve", "##Divider", "Gold", "Ice"),
				Burning = 1 << 0, Electrocuted = 1 << 1, Ash = 1 << 2, Dissolve = 1 << 3, Gold = 1 << 4, Ice = 1 << 5);
			CVar(DrawIconsThroughWalls, "Draw icons through walls", false, VISUAL);
			CVar(DrawDamageNumbersThroughWalls, "Draw damage numbers through walls", false, VISUAL);
		NAMESPACE_END(Tracers)

		NAMESPACE_BEGIN(Viewmodel)
			CVar(CrosshairAim, "Crosshair aim", false, VISUAL);
			CVar(ViewmodelAim, "Viewmodel aim", false, VISUAL);
			CVar(OffsetX, VA_LIST("Offset X", "Viewmodel offset X"), 0.f, VISUAL | SLIDER_PRECISION, -45.f, 45.f, 5.f);
			CVar(OffsetY, VA_LIST("Offset Y", "Viewmodel offset Y"), 0.f, VISUAL | SLIDER_PRECISION, -45.f, 45.f, 5.f);
			CVar(OffsetZ, VA_LIST("Offset Z", "Viewmodel offset Z"), 0.f, VISUAL | SLIDER_PRECISION, -45.f, 45.f, 5.f);
			CVar(Pitch, VA_LIST("Pitch", "Viewmodel pitch"), 0.f, VISUAL | SLIDER_CLAMP | SLIDER_PRECISION, -180.f, 180.f, 5.f);
			CVar(Yaw, VA_LIST("Yaw", "Viewmodel yaw"), 0.f, VISUAL | SLIDER_CLAMP | SLIDER_PRECISION, -180.f, 180.f, 5.f);
			CVar(Roll, VA_LIST("Roll", "Viewmodel roll"), 0.f, VISUAL | SLIDER_CLAMP | SLIDER_PRECISION, -180.f, 180.f, 5.f);
			CVar(SwayScale, VA_LIST("Sway scale", "Viewmodel sway scale"), 0.f, VISUAL | SLIDER_MIN | SLIDER_PRECISION, 0.f, 5.f, 0.5f);
			CVar(SwayInterp, VA_LIST("Sway interp", "Viewmodel sway interp"), 0.f, VISUAL | SLIDER_MIN | SLIDER_PRECISION, 0.f, 1.f, 0.1f);
		NAMESPACE_END(Viewmodel)

		NAMESPACE_BEGIN(World)
			CVarEnum(Modulations, "Modulations", 0b00000, VISUAL | DROPDOWN_MULTI, nullptr,
				VA_LIST("World", "Sky", "Prop", "Particle", "Fog"),
				World = 1 << 0, Sky = 1 << 1, Prop = 1 << 2, Particle = 1 << 3, Fog = 1 << 4);
			CVarValues(SkyboxChanger, "Skybox changer", std::string("Off"), VISUAL | DROPDOWN_CUSTOM, nullptr,
				VA_LIST("Off"));
			CVarValues(WorldTexture, "World texture", std::string("Default"), VISUAL | DROPDOWN_CUSTOM, nullptr,
				"Default", "Dev", "Camo", "Black", "White", "Gray", "Flat");
			CVar(NearPropFade, "Near prop fade", false, VISUAL);
			CVar(NoPropFade, "No prop fade", false, VISUAL);
		NAMESPACE_END(World)

		NAMESPACE_BEGIN(Beams) // as of now, these will stay out of the menu
			CVar(Model, "Model", std::string("sprites/physbeam.vmt"), VISUAL);
			CVar(Life, "Life", 2.f, VISUAL | SLIDER_MIN | SLIDER_PRECISION, 0.f, 10.f);
			CVar(Width, "Width", 2.f, VISUAL | SLIDER_MIN | SLIDER_PRECISION, 0.f, 10.f);
			CVar(EndWidth, "End width", 2.f, VISUAL | SLIDER_MIN | SLIDER_PRECISION, 0.f, 10.f);
			CVar(FadeLength, "Fade length", 10.f, VISUAL | SLIDER_MIN | SLIDER_PRECISION, 0.f, 30.f);
			CVar(Amplitude, "Amplitude", 2.f, VISUAL | SLIDER_MIN | SLIDER_PRECISION, 0.f, 10.f);
			CVar(Brightness, "Brightness", 255.f, VISUAL | SLIDER_MIN | SLIDER_PRECISION, 0.f, 255.f);
			CVar(Speed, "Speed", 0.2f, VISUAL | SLIDER_MIN | SLIDER_PRECISION, 0.f, 5.f);
			CVar(Segments, "Segments", 2, VISUAL | SLIDER_MIN, 1, 10);
			CVar(Color, "Color", Color_t(255, 255, 255, 255), VISUAL);
			CVarEnum(Flags, "Flags", 0b10000000100000000, VISUAL | DROPDOWN_MULTI, nullptr,
				VA_LIST("Start entity", "End entity", "Fade in", "Fade out", "Sine noise", "Solid", "Shade in", "Shade out", "Only noise once", "No tile", "Use hitboxes", "Start visible", "End visible", "Is active", "Forever", "Halobeam", "Reverse"),
				StartEntity = 1 << 0, EndEntity = 1 << 1, FadeIn = 1 << 2, FadeOut = 1 << 3, SineNoise = 1 << 4, Solid = 1 << 5, ShadeIn = 1 << 6, ShadeOut = 1 << 7, OnlyNoiseOnce = 1 << 8, NoTile = 1 << 9, UseHitboxes = 1 << 10, StartVisible = 1 << 11, EndVisible = 1 << 12, IsActive = 1 << 13, Forever = 1 << 14, Halobeam = 1 << 15, Reverse = 1 << 16);
		NAMESPACE_END(Beams)

		NAMESPACE_BEGIN(Line)
			CVar(TracersEnabled, "Tracers enabled", false, VISUAL);
			CVar(DrawDuration, VA_LIST("Draw duration", "Line draw duration"), 5.f, VISUAL | SLIDER_MIN | SLIDER_PRECISION, 0.f, 10.f);
		NAMESPACE_END(Line)

		NAMESPACE_BEGIN(Hitbox)
			CVarEnum(BonesEnabled, VA_LIST("Bones enabled", "Hitbox bones enabled"), 0b00, VISUAL | DROPDOWN_MULTI, "Off",
				VA_LIST("On shot", "On hit"),
				OnShot = 1 << 0, OnHit = 1 << 1);
			CVarEnum(BoundsEnabled, VA_LIST("Bounds enabled", "Hitbox bounds enabled"), 0b000, VISUAL | DROPDOWN_MULTI, "Off",
				VA_LIST("On shot", "On hit", "Aim point"),
				OnShot = 1 << 0, OnHit = 1 << 1, AimPoint = 1 << 2);
			CVar(DrawDuration, VA_LIST("Draw duration", "Hitbox draw duration"), 5.f, VISUAL | SLIDER_MIN | SLIDER_PRECISION, 0.f, 10.f);
		NAMESPACE_END(Hitbox)

		NAMESPACE_BEGIN(Prediction)
			CVarValues(PlayerPath, "Player path", 0, VISUAL, nullptr,
				"Off", "Line", "Separators", "Spaced", "Arrows", "Boxes");
			CVarValues(ProjectilePath, "Projectile path", 0, VISUAL, nullptr,
				"Off", "Line", "Separators", "Spaced", "Arrows", "Boxes");
			CVar(SwingLines, "Swing lines", false, VISUAL);
			CVar(PlayerDrawDuration, VA_LIST("Draw duration", "Player path draw duration"), 5.f, VISUAL | SLIDER_MIN | SLIDER_PRECISION, 0.f, 10.f);
			CVar(ProjectileDrawDuration, VA_LIST("Draw duration", "Projectile path draw duration"), 5.f, VISUAL | SLIDER_MIN | SLIDER_PRECISION, 0.f, 10.f);

			CVarValues(RealPath, "Real path", 0, NOSAVE | DEBUGVAR, nullptr,
				"Off", "Line", "Separators", "Spaced", "Arrows", "Boxes");
		NAMESPACE_END(Prediction)

		NAMESPACE_BEGIN(Simulation)
			CVarValues(TrajectoryPath, "Trajectory path", 0, VISUAL, nullptr,
				"Off", "Line", "Separators", "Spaced", "Arrows", "Boxes");
			CVarValues(ShotPath, "Shot path", 0, VISUAL, nullptr,
				"Off", "Line", "Separators", "Spaced", "Arrows", "Boxes");
			CVarEnum(SplashRadius, "Splash radius", 0b0, VISUAL | DROPDOWN_MULTI, "Off",
				VA_LIST("Rockets", "Stickies", "Pipes", "Flares", "##Divider", "Trace", "Sphere"),
				Rockets = 1 << 0, Stickies = 1 << 1, Pipes = 1 << 2, Flares = 1 << 3, Trace = 1 << 4, Sphere = 1 << 5,
				Enabled = Rockets | Stickies | Pipes | Flares);
			CVar(ProjectileCamera, "Projectile camera", false, VISUAL);
			CVar(ProjectileWindow, "Projectile window", WindowBox_t(), VISUAL | NOBIND);
			CVar(Box, VA_LIST("Box", "Path box"), true, VISUAL);
		NAMESPACE_END(Simulation)

		NAMESPACE_BEGIN(Path)
			Enum(Style, Off, Line, Separators, Spaced, Arrows, Boxes);

			CVar(SeparatorSpacing, "Separator spacing", 4, NOSAVE | DEBUGVAR, 1, 16);
			CVar(SeparatorLength, "Separator length", 12.f, NOSAVE | DEBUGVAR, 2.f, 16.f);
		NAMESPACE_END(Path)

		NAMESPACE_BEGIN(Trajectory)
			CVar(Override, "Simulation override", false, NOSAVE | DEBUGVAR);
			CVar(Type, "Type", std::string("custom"), NOSAVE | DEBUGVAR);
			CVar(OffsetX, "Offset X", 23.5f, NOSAVE | DEBUGVAR | SLIDER_PRECISION, -25.f, 25.f, 0.5f);
			CVar(OffsetY, "Offset Y", 12.f, NOSAVE | DEBUGVAR | SLIDER_PRECISION, -25.f, 25.f, 0.5f);
			CVar(OffsetZ, "Offset Z", -3.f, NOSAVE | DEBUGVAR | SLIDER_PRECISION, -25.f, 25.f, 0.5f);
			CVar(ForwardRedirect, "Forward redirect", 2000.f, NOSAVE | DEBUGVAR | SLIDER_PRECISION, 0.f, 2000.f, 100.f);
			CVar(ForwardCutoff, "Forward cutoff", 0.1f, NOSAVE | DEBUGVAR | SLIDER_PRECISION, 0.f, 1.f, 0.1f);
			CVar(Hull, "Hull", 0.f, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0.f, 10.f, 0.5f);
			CVar(Speed, "Speed", 1100.f, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0.f, 5000.f, 50.f);
			CVar(Gravity, "Gravity", 0.f, NOSAVE | DEBUGVAR | SLIDER_PRECISION, 0.f, 800.f, 100.f);
			CVar(LifeTime, "Life time", 10.f, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0.f, 10.f, 0.1f);
			CVar(UpVelocity, "Up velocity", 0.f, NOSAVE | DEBUGVAR | SLIDER_PRECISION, 0.f, 1000.f, 50.f);
			CVar(AngularVelocityX, "Angular velocity X", 0.f, NOSAVE | DEBUGVAR | SLIDER_PRECISION, -1000.f, 1000.f, 50.f);
			CVar(AngularVelocityY, "Angular velocity Y", 0.f, NOSAVE | DEBUGVAR | SLIDER_PRECISION, -1000.f, 1000.f, 50.f);
			CVar(AngularVelocityZ, "Angular velocity Z", 0.f, NOSAVE | DEBUGVAR | SLIDER_PRECISION, -1000.f, 1000.f, 50.f);
			CVar(Drag, "Drag", 0.f, NOSAVE | DEBUGVAR | SLIDER_PRECISION, 0.f, 2.f, 0.1f);
			CVar(DragX, "Drag X", 0.f, NOSAVE | DEBUGVAR | SLIDER_PRECISION, 0.f, 0.1f, 0.01f, "%.15g");
			CVar(DragY, "Drag Y", 0.f, NOSAVE | DEBUGVAR | SLIDER_PRECISION, 0.f, 0.1f, 0.01f, "%.15g");
			CVar(DragZ, "Drag Z", 0.f, NOSAVE | DEBUGVAR | SLIDER_PRECISION, 0.f, 0.1f, 0.01f, "%.15g");
			CVar(AngularDragX, "Angular drag X", 0.f, NOSAVE | DEBUGVAR | SLIDER_PRECISION, 0.f, 0.1f, 0.01f, "%.15g");
			CVar(AngularDragY, "Angular drag Y", 0.f, NOSAVE | DEBUGVAR | SLIDER_PRECISION, 0.f, 0.1f, 0.01f, "%.15g");
			CVar(AngularDragZ, "Angular drag Z", 0.f, NOSAVE | DEBUGVAR | SLIDER_PRECISION, 0.f, 0.1f, 0.01f, "%.15g");
			CVar(MaxVelocity, "Max velocity", 0.f, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0.f, 4000.f, 50.f);
			CVar(MaxAngularVelocity, "Max angular velocity", 0.f, NOSAVE | DEBUGVAR | SLIDER_MIN | SLIDER_PRECISION, 0.f, 7200.f, 50.f);
		NAMESPACE_END(ProjectileTrajectory)
	NAMESPACE_END(Visuals)

	NAMESPACE_BEGIN(Misc)
		NAMESPACE_BEGIN(Movement)
			CVarEnum(AutoStrafe, "Auto strafe", 0, NONE, nullptr,
				VA_LIST("Off", "Legit", "Directional"),
				Off, Legit, Directional);
			CVar(AutoStrafeTurnScale, VA_LIST("Turn scale", "Auto strafe turn scale"), 0.5f, SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 1.f, 0.1f);
			CVar(AutoStrafeMaxDelta, VA_LIST("Max delta", "Auto strafe max delta"), 180.f, SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 180.f, 5.f);
			CVar(Bunnyhop, "Bunnyhop", false);
			CVar(EdgeJump, "Edge jump", false);
			CVar(AutoJumpbug, "Auto jumpbug", false);
			CVar(BreakJump, "Break jump", false);
			CVar(AutoRocketJump, "Auto rocket jump", false);
			CVar(AutoCTap, "Auto ctap", false);
			CVar(AutoFaNJump, "Auto FaN jump", false);
			CVar(AutoRevJump, "Auto rev jump", false);
			CVar(FastStop, "Fast stop", false);
			CVar(FastAccelerate, "Fast accelerate", false);
			CVar(DuckSpeed, "Duck speed", false);
			CVar(ShieldTurnRate, "Shield turn rate", false);
			CVar(NoPush, "No push", false);
			CVar(MovementLock, "Movement lock", false);

			CVar(AutoRocketJumpChokeGrounded, "Choke grounded", 1, NOSAVE | DEBUGVAR, 0, 3);
			CVar(AutoRocketJumpChokeAir, "Choke air", 1, NOSAVE | DEBUGVAR, 0, 3);
			CVar(AutoRocketJumpSkipGround, "Skip grounded", 0, NOSAVE | DEBUGVAR, 0, 3);
			CVar(AutoRocketJumpSkipAir, "Skip air", 1, NOSAVE | DEBUGVAR, 0, 3);
			CVar(AutoRocketJumpTimingOffset, "Timing offset", 0, NOSAVE | DEBUGVAR, 0, 3);
			CVar(AutoRocketJumpApplyAbove, "Apply offset above", 0, NOSAVE | DEBUGVAR, 0, 10);

			// ── Aletherium movement features ──────────────────────────────
			CVar(StrafeOptimizer, "Strafe optimizer", false);
			CVar(StrafeOptimizerGain, "Optimizer gain", 50.f, SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 100.f, 5.f);
			CVar(StrafeOptimizerMinSpeed, "Optimizer min speed", 100.f, SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 500.f, 10.f);

			// EdgeBug
			CVar(EdgeBug, "EdgeBug", false);
			CVar(EdgeBugLockTicks, "EB lock ticks", 10, SLIDER_MIN, 1, 128);
			CVar(EdgeBugAngleLimit, "EB angle limit", 90.f, SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 180.f);
			CVar(EdgeBugAdvancedSearch, "EB advanced search", true);
			CVar(EdgeBugScanRadius, "EB scan radius", 8, SLIDER_MIN, 1, 48);
			CVar(EdgeBugAutoStrafe, "EB auto strafe", true);
			CVar(EdgeBugChatPrint, "EB chat print", false);
			CVar(EdgeBugVisualize, "EB visualize path", false, VISUAL);
			CVar(EdgeBugMouseLock, "EB mouse lock", false);
			CVarEnum(EdgeBugMouseLockType, "EB lock type", 0, NONE, nullptr,
				VA_LIST("Classic", "Static", "Dynamic", "Predicted"),
				Classic, Static, Dynamic, Predicted);
			CVar(EdgeBugLockAmount, "EB lock amount", 80.f, SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 100.f, 5.f);
			CVarEnum(EdgeBugSound, "EB sound", 0, NONE, nullptr,
				VA_LIST("Off", "Panel open", "Menu focus", "Button click", "Button rollover", "Beep", "Blip", "Custom"),
				Off, PanelOpen, MenuFocus, ButtonClick, ButtonRollover, Beep, Blip, Custom);
			CVar(EdgeBugSoundCustom, "EB sound path", std::string(""));

			// LongJump / MiniJump
			CVar(LongJump, "Long jump", false);
			CVar(MiniJump, "Mini jump", false);
			CVar(MiniJumpHoldDuck, "Mini jump hold duck", true);
			CVar(MiniJumpQueue, "Mini jump queue", true);

			// AutoAlign
			CVar(AutoAlign, "Auto align", false);

			// PixelSurf
			CVar(PixelSurf, "Pixel surf", false);

			// TextureBug
			CVar(TextureBug, "Texture bug", false);
			CVar(TextureBugAutoCrouch, "TB auto crouch", true);
			CVar(TextureBugEdgeStop, "TB edge stop", true);
			CVar(TextureBugHoldTicks, "TB hold ticks", 4, SLIDER_MIN, 0, 16);
			CVar(TextureBugCatchEps, "TB catch eps", 2.f, SLIDER_CLAMP | SLIDER_PRECISION, 0.1f, 8.f, 0.1f);
			CVar(TextureBugReach, "TB reach", 2.f, SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 16.f, 0.5f);
			CVar(TextureBugScanStep, "TB scan step", 1.f, SLIDER_CLAMP | SLIDER_PRECISION, 0.5f, 4.f, 0.25f);
			CVar(TextureBugChokeTick, "TB choke tick", false);
			CVar(TextureBugChokeTicks, "TB choke ticks", 6, SLIDER_MIN, 1, 14);

			// HeadSurf (runs under TextureBug toggle)
			CVar(HeadSurf, "Head surf", false);

			// WallClimb
			CVar(WallClimb, "Wall climb", false);

			// AirStuck
			CVar(AirStuck, "Air stuck", false);
			CVar(AirStuckCatchEps, "AS catch eps", 2.f, SLIDER_CLAMP | SLIDER_PRECISION, 0.1f, 8.f, 0.1f);
			CVar(AirStuckReach, "AS reach", 24.f, SLIDER_CLAMP | SLIDER_PRECISION, 4.f, 64.f, 2.f);
			CVar(AirStuckFlushTol, "AS flush tol", 0.1f, SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 2.f, 0.05f);
			CVar(AirStuckDriftGain, "AS drift gain", 18.f, SLIDER_CLAMP | SLIDER_PRECISION, 1.f, 80.f, 1.f);
			CVar(AirStuckSimBudget, "AS sim budget", 64, SLIDER_MIN, 4, 128);

			// PixelFinder
			CVar(PixelFinder, "Pixel finder", false);
			CVar(PixelFinderScanMargin, "PF scan margin", 4.f, SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 32.f, 1.f);

			// PixelSurf line recorder
			CVar(PixelSurfLine, "Pixel surf line", false, VISUAL);
			CVar(PixelSurfLineColor, "Surf line color", Color_t(255, 200, 50, 200), VISUAL);

			// PixelSurfAssist
			CVar(PixelSurfAssist, "Pixel surf assist", false);
			CVar(PixelSurfAssistRadius, "PS assist radius", 300.f, SLIDER_CLAMP | SLIDER_PRECISION, 50.f, 1500.f, 25.f);
			CVar(PixelSurfAssistSnapDist, "PS snap dist", 32.f, SLIDER_CLAMP | SLIDER_PRECISION, 4.f, 128.f, 4.f);
			CVar(PixelSurfAssistRender, "PS assist render", true, VISUAL);
			CVar(PixelSurfAssistSteer, "PS assist steer", true);
			CVar(PixelSurfAssistJumpBox, "PS jump box", true, VISUAL);
			CVar(PixelSurfAssistSetPoint, "PS set point", false, NOBIND);
			CVar(PixelSurfAssistDeletePoint, "PS delete point", false, NOBIND);

			// MovementRecorder
			CVar(MovementRecorder, "Movement recorder", false);
			CVar(MovementRecorderRecord, "MR record", false, NOBIND);
			CVar(MovementRecorderPlay, "MR play", false, NOBIND);
			CVar(MovementRecorderStop, "MR stop", false, NOBIND);
			CVar(MovementRecorderSave, "MR save", false, NOBIND);
			CVar(MovementRecorderLoad, "MR load", false, NOBIND);
			CVar(MovementRecorderClear, "MR clear", false, NOBIND);
			CVar(MovementRecorderHud, "MR HUD", true, VISUAL);
			CVar(MovementRecorderShowPath, "MR show path", true, VISUAL);
			CVar(MovementRecorderShowRoutes, "MR show routes", true, VISUAL);
			CVar(MovementRecorderMoveToStart, "MR move to start", true);
			CVar(MovementRecorderMoveToStartDist, "MR start dist", 4.f, SLIDER_CLAMP | SLIDER_PRECISION, 0.5f, 32.f, 0.5f);
			CVar(MovementRecorderStartSpeed, "MR start speed", 16.f, SLIDER_CLAMP | SLIDER_PRECISION, 1.f, 80.f, 1.f);
			CVar(MovementRecorderStartTolerance, "MR start tol", 8.f, SLIDER_CLAMP | SLIDER_PRECISION, 1.f, 64.f, 1.f);
			CVar(MovementRecorderDriftCorrect, "MR drift correct", true);
			CVar(MovementRecorderDriftStrength, "MR drift strength", 0.5f, SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 1.f, 0.05f);
			CVar(MovementRecorderVerbatim, "MR verbatim", false);
			CVar(MovementRecorderLockView, "MR lock view", true);
			CVar(MovementRecorderRouteColor, "MR route color", Color_t(255, 200, 50, 180), VISUAL);
			CVar(MovementRecorderRoutePoints, "MR route points", 8, SLIDER_MIN, 3, 64);
			CVar(MovementRecorderRouteRadius, "MR route radius", 6.f, SLIDER_CLAMP | SLIDER_PRECISION, 1.f, 24.f, 1.f);
			CVar(MovementRecorderRouteSolid, "MR route solid", false, VISUAL);
			CVar(MovementRecorderShadowPlay, "MR shadow play", false);
			CVar(MovementRecorderShadowSave, "MR shadow save", false, NOBIND);
			CVar(MovementRecorderShadowSeconds, "MR shadow secs", 15.f, SLIDER_CLAMP | SLIDER_PRECISION, 5.f, 60.f, 5.f);

			// Checkpoints (KZ practice)
			CVar(Checkpoints, "Checkpoints", false);
			CVar(CheckpointSaveKey, "CP save key", 0, NOBIND);
			CVar(CheckpointTeleportKey, "CP teleport key", 0, NOBIND);
			CVar(CheckpointNoclipKey, "CP noclip key", 0, NOBIND);

			// JumpStats
			CVar(JumpStats, "Jump stats", false, VISUAL);
			// ──────────────────────────────────────────────────────────────
		NAMESPACE_END(Movement)

		NAMESPACE_BEGIN(Automation)
			CVarEnum(AntiBackstab, "Anti-backstab", 0, NONE, nullptr,
				VA_LIST("Off", "Yaw", "Pitch", "Fake"),
				Off, Yaw, Pitch, Fake);
			CVar(TauntControl, "Taunt control", false);
			CVar(KartControl, "Kart control", false);
			CVar(AntiAutobalance, "Anti-autobalance", false);
			CVar(AntiAFK, "Anti-AFK", false);
			CVar(AutoF2Ignored, "Auto F2 ignored", false);
			CVar(AutoF1Priority, "Auto F1 priority", false);
			CVar(AcceptItemDrops, "Auto accept item drops", false);
		NAMESPACE_END(Automation)

		NAMESPACE_BEGIN(Exploits)
			CVar(PureBypass, "Pure bypass", false);
			CVar(CheatsBypass, "Cheats bypass", false);
			CVar(UnlockCVars, "Unlock CVars", false);
			CVar(EquipRegionUnlock, "Equip region unlock", false);
			CVar(BackpackExpander, "Backpack expander", false);
			CVar(NoisemakerSpam, "Noisemaker spam", false);
			CVar(PingReducer, "Ping reducer", false);
			CVar(PingTarget, "cl_cmdrate", 1, SLIDER_CLAMP, 1, 66);
		NAMESPACE_END(Exploits)

		NAMESPACE_BEGIN(Game)
			CVar(NetworkFix, "Network fix", false);
			CVar(SetupBonesOptimization, "Bones optimization", false);
			CVar(AntiCheatCompatibility, "Anti-cheat compatibility", false);

			CVar(AntiCheatCritHack, "Anti-cheat crit hack", false, NOSAVE | DEBUGVAR);
		NAMESPACE_END(Game)

		NAMESPACE_BEGIN(Queueing)
			CVarEnum(ForceRegions, "Force regions", 0b0, DROPDOWN_MULTI, nullptr, // i'm not sure all of these are actually used for tf2 servers
				VA_LIST("Atlanta", "Chicago", "Dallas", "Los Angeles", "Seattle", "Virginia", "##Divider", "Amsterdam", "Falkenstein", "Frankfurt", "Helsinki", "London", "Madrid", "Paris", "Stockholm", "Vienna", "Warsaw", "##Divider", "Buenos Aires", "Lima", "Santiago", "Sao Paulo", "##Divider", "Chennai", "Dubai", "Hong Kong", "Mumbai", "Seoul", "Singapore", "Tokyo", "##Divider", "Sydney", "##Divider", "Johannesburg"),
				// North America
				ATL = 1 << 0, // Atlanta
				ORD = 1 << 1, // Chicago
				DFW = 1 << 2, // Dallas
				LAX = 1 << 3, // Los Angeles
				SEA = 1 << 4, // Seattle (+DC_EAT?)
				IAD = 1 << 5, // Virginia
				// Europe
				AMS = 1 << 6, // Amsterdam
				FSN = 1 << 7, // Falkenstein
				FRA = 1 << 8, // Frankfurt
				HEL = 1 << 9, // Helsinki
				LHR = 1 << 10, // London
				MAD = 1 << 11, // Madrid
				PAR = 1 << 12, // Paris
				STO = 1 << 13, // Stockholm
				VIE = 1 << 14, // Vienna
				WAW = 1 << 15, // Warsaw
				// South America
				EZE = 1 << 16, // Buenos Aires
				LIM = 1 << 17, // Lima
				SCL = 1 << 18, // Santiago
				GRU = 1 << 19, // Sao Paulo
				// Asia
				MAA = 1 << 20, // Chennai
				DXB = 1 << 21, // Dubai
				HKG = 1 << 22, // Hong Kong
				BOM = 1 << 23, // Mumbai
				SEO = 1 << 24, // Seoul
				SGP = 1 << 25, // Singapore
				TYO = 1 << 26, // Tokyo
				// Australia
				SYD = 1 << 27, // Sydney
				// Africa
				JNB = 1 << 28, // Johannesburg
			);
			CVar(ExtendQueue, "Extend queue", false);
			CVar(AutoCasualQueue, "Auto casual queue", false);
		NAMESPACE_END(Queueing)

		NAMESPACE_BEGIN(MannVsMachine, Mann vs. Machine)
			CVar(InstantRespawn, "Instant respawn", false);
			CVar(InstantRevive, "Instant revive", false);
			CVar(AllowInspect, "Allow inspect", false);
		NAMESPACE_END(Sound)

		NAMESPACE_BEGIN(Sound)
			CVarEnum(Block, VA_LIST("Block", "Sound block"), 0b0000, DROPDOWN_MULTI, nullptr,
				VA_LIST("Footsteps", "Noisemaker", "Frying pan", "Water"),
				Footsteps = 1 << 0, Noisemaker = 1 << 1, FryingPan = 1 << 2, Water = 1 << 3);
			CVar(HitsoundAlways, "Hitsound always", false);
			CVar(RemoveDSP, "Remove DSP", false);
			CVar(GiantWeaponSounds, "Giant weapon sounds", false);
		NAMESPACE_END(Sound)
	NAMESPACE_END(Misc)

	NAMESPACE_BEGIN(Logging)
		CVarEnum(Logs, "Logs", 0b0000011, DROPDOWN_MULTI, "Off",
			VA_LIST("Vote start", "Vote cast", "Class changes", "Damage", "Cheat detection", "Tags", "Aliases", "Resolver"),
			VoteStart = 1 << 0, VoteCast = 1 << 1, ClassChanges = 1 << 2, Damage = 1 << 3, CheatDetection = 1 << 4, Tags = 1 << 5, Aliases = 1 << 6, Resolver = 1 << 7);
		Enum(LogTo, Toasts = 1 << 0, Chat = 1 << 1, Party = 1 << 2, Console = 1 << 3, Menu = 1 << 4, Debug = 1 << 5);
		CVarEnum(NotificationPosition, "Notification position", 0, VISUAL, nullptr,
			VA_LIST("Top left", "Top right", "Bottom left", "Bottom right"),
			TopLeft, TopRight, BottomLeft, BottomRight);
		CVar(NotificationTime, "Notification time", 5.f, VISUAL, 0.5f, 5.f, 0.5f);
		CVar(MaxNotifications, "Max notifications", 10, VISUAL | SLIDER_MIN, 1, 10);

		NAMESPACE_BEGIN(VoteStart, Logging)
			CVarValues(LogTo, "Vote start log to", 0b000001, DROPDOWN_MULTI, nullptr,
				"Toasts", "Chat", "Party", "Console", "Menu", "Debug");
		NAMESPACE_END(VoteStart)

		NAMESPACE_BEGIN(VoteCast, Logging)
			CVarValues(LogTo, "Vote cast log to", 0b000001, DROPDOWN_MULTI, nullptr,
				"Toasts", "Chat", "Party", "Console", "Menu", "Debug");
		NAMESPACE_END(VoteCast)

		NAMESPACE_BEGIN(ClassChange, Logging)
			CVarValues(LogTo, "Class change log to", 0b000001, DROPDOWN_MULTI, nullptr,
				"Toasts", "Chat", "Party", "Console", "Menu", "Debug");
		NAMESPACE_END(ClassChange)

		NAMESPACE_BEGIN(Damage, Logging)
			CVarValues(LogTo, "Damage log to", 0b000001, DROPDOWN_MULTI, nullptr,
				"Toasts", "Chat", "Party", "Console", "Menu", "Debug");
		NAMESPACE_END(Damage)

		NAMESPACE_BEGIN(CheatDetection, Logging)
			CVarValues(LogTo, "Cheat detection log to", 0b000001, DROPDOWN_MULTI, nullptr,
				"Toasts", "Chat", "Party", "Console", "Menu", "Debug");
		NAMESPACE_END(CheatDetection)

		NAMESPACE_BEGIN(Tags, Logging)
			CVarValues(LogTo, "Tags log to", 0b000001, DROPDOWN_MULTI, nullptr,
				"Toasts", "Chat", "Party", "Console", "Menu", "Debug");
		NAMESPACE_END(Tags)

		NAMESPACE_BEGIN(Aliases, Logging)
			CVarValues(LogTo, "Aliases log to", 0b000001, DROPDOWN_MULTI, nullptr,
				"Toasts", "Chat", "Party", "Console", "Menu", "Debug");
		NAMESPACE_END(Aliases)

		NAMESPACE_BEGIN(Resolver, Logging)
			CVarValues(LogTo, "Resolver log to", 0b000001, DROPDOWN_MULTI, nullptr,
				"Toasts", "Chat", "Party", "Console", "Menu", "Debug");
		NAMESPACE_END(Resolver)
	NAMESPACE_END(Logging)

	NAMESPACE_BEGIN(CheatDetection, Cheat Detection)
		CVarEnum(Methods, "Detection methods", 0b0000, DROPDOWN_MULTI, nullptr,
			VA_LIST("Invalid pitch", "Packet choking", "Aim flicking", "Duck Speed"),
			InvalidPitch = 1 << 0, PacketChoking = 1 << 1, AimFlicking = 1 << 2, DuckSpeed = 1 << 3);
		CVar(DetectionsRequired, "Detections required", 10, SLIDER_MIN, 0, 50);
		CVar(MinChoking, "Min choking", 20, SLIDER_MIN, 4, 22);
		CVar(MinFlick, "Min flick angle", 20.f, SLIDER_PRECISION, 10.f, 30.f); // min flick size to suspect
		CVar(MaxNoise, "Max flick noise", 1.f, SLIDER_PRECISION, 1.f, 10.f); // max difference between angles before and after flick
	NAMESPACE_END(CheatDetection)

	NAMESPACE_BEGIN(Debug)
		CVar(Info, "Debug info", false, NOSAVE);
		CVar(Logging, "Debug logging", false, NOSAVE);
		CVar(Options, "Debug options", false, NOSAVE);
		CVar(CrashLogging, "Crash logging", true, NOBIND);

#ifdef DEBUG_TRACES
		CVar(VisualizeTraces, "Visualize traces", false, NOSAVE);
		CVar(VisualizeTraceHits, "Visualize trace hits", false, NOSAVE);
#endif

		CVar(DrawHitboxes, "Show hitboxes", false, NOSAVE);
	NAMESPACE_END(Debug)

	// ── Aletherium ─────────────────────────────────────────────────────────
	// Features ported from the Aletherium fork. The logic files are not yet
	// included; these vars declare the config surface so the menu page compiles
	// and configs save/load correctly. Wire feature implementations as needed.

	NAMESPACE_BEGIN(Aletherium, Aletherium)

		// Backstab (Aimbot > Knife)
		NAMESPACE_BEGIN(Backstab, Aletherium)
			CVar(AutoBackstab, "Auto backstab", false);
			CVarEnum(AimType, "Backstab aim type", 0, NONE, nullptr,
				VA_LIST("Silent", "Legit"),
				Silent, Legit);
			CVar(FOV, "Backstab FOV", 180.f, SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 180.f);
			CVar(AutoShoot, "Backstab auto shoot", false);
			CVarValues(Ignore, "Backstab ignore", 0b100000001000, DROPDOWN_MULTI, nullptr,
				VA_LIST("Friends", "Party", "Unprioritized", "Invulnerable", "Invisible",
				        "Unsimulated", "Dead ringer", "Vaccinator", "Disguised", "Taunting", "Team", "Razorback"));
			// Reliability
			CVar(AccountPing, "Backstab account ping", false);
			CVar(DoubleTest, "Backstab double test", false);
			CVar(RemoveInterp, "Backstab remove interp", false);
			CVar(StrictRange, "Backstab strict range", false);
			CVar(PredictMovement, "Backstab predict movement", false);
			CVar(Aggressive, "Backstab aggressive", false);
			CVar(MultiPoint, "Backstab multi point", false);
			CVar(ExtrapolateYaw, "Backstab extrapolate yaw", false);
			CVar(UnstableConnection, "Backstab unstable connection", false);
			CVar(UnstableMaxAngle, "Backstab unstable max angle", 25.f, SLIDER_CLAMP | SLIDER_PRECISION, 0.f, 60.f);
		NAMESPACE_END(Backstab)

		// Watermark HUD
		NAMESPACE_BEGIN(Watermark, Aletherium)
			CVar(Enabled, "Watermark", false, VISUAL);
			CVar(ShowFPS,  "Show FPS",  true,  VISUAL);
			CVar(ShowPing, "Show ping", false, VISUAL);
			CVar(ShowLoss, "Show loss", false, VISUAL);
			CVar(ShowTime, "Show time", false, VISUAL);
			CVar(Background, "Watermark background", true, VISUAL);
			CVarEnum(Position, "Watermark position", 2, VISUAL, nullptr,
				VA_LIST("Top left", "Top center", "Top right", "Bottom left", "Bottom center", "Bottom right"),
				TopLeft, TopCenter, TopRight, BottomLeft, BottomCenter, BottomRight);
		NAMESPACE_END(Watermark)

		// Weather / Fog
		NAMESPACE_BEGIN(Weather, Aletherium)
			CVar(Fog, "Fog override", false, VISUAL);
			CVar(FogStart, "Fog start", 0.f,   VISUAL | SLIDER_PRECISION, 0.f, 8192.f, 32.f, "%.0f");
			CVar(FogEnd,   "Fog end",   2048.f, VISUAL | SLIDER_PRECISION, 0.f, 8192.f, 32.f, "%.0f");
			CVar(FogDensity, "Fog density", 0.5f, VISUAL | SLIDER_PRECISION, 0.f, 1.f, 0.01f, "%.2f");
			CVar(FogLinkSkybox, "Link skybox fog", true, VISUAL);
			CVar(FogSkyStart,   "Sky fog start",   0.f,    VISUAL | SLIDER_PRECISION, 0.f, 8192.f, 32.f, "%.0f");
			CVar(FogSkyEnd,     "Sky fog end",     2048.f, VISUAL | SLIDER_PRECISION, 0.f, 8192.f, 32.f, "%.0f");
			CVar(FogSkyDensity, "Sky fog density", 0.5f,   VISUAL | SLIDER_PRECISION, 0.f, 1.f, 0.01f, "%.2f");
			CVar(FogColor, "Fog color", Color_t(200, 200, 200, 255), VISUAL);
			CVarEnum(Precipitation, "Precipitation", 0, VISUAL, nullptr,
				VA_LIST("Off", "Rain", "Snow"),
				Off, Rain, Snow);
			// Rain convar detail
			CVar(RainAlpha,   "Rain alpha",    0.5f,  VISUAL | SLIDER_PRECISION, 0.f, 1.f, 0.05f, "%.2f");
			CVar(RainSpeed,   "Rain speed",    1500.f,VISUAL | SLIDER_PRECISION, 100.f, 4000.f, 50.f, "%.0f");
			CVar(RainWidth,   "Rain width",    0.5f,  VISUAL | SLIDER_PRECISION, 0.1f, 4.f, 0.1f, "%.1f");
			CVar(RainLength,  "Rain length",   0.1f,  VISUAL | SLIDER_PRECISION, 0.01f, 1.f, 0.01f, "%.2f");
			CVar(RainRadius,  "Rain radius",   1500.f,VISUAL | SLIDER_PRECISION, 100.f, 4000.f, 50.f, "%.0f");
			CVar(RainSideVel, "Rain side vel", 130.f, VISUAL | SLIDER_PRECISION, 0.f, 500.f, 10.f, "%.0f");
			// Snow convar detail
			CVar(SnowAlpha,   "Snow alpha",   0.8f,  VISUAL | SLIDER_PRECISION, 0.f, 1.f, 0.05f, "%.2f");
			CVar(SnowSpeed,   "Snow speed",   150.f, VISUAL | SLIDER_PRECISION, 10.f, 1000.f, 10.f, "%.0f");
			CVar(SnowWidth,   "Snow width",   2.f,   VISUAL | SLIDER_PRECISION, 0.5f, 8.f, 0.25f, "%.2f");
			CVar(SnowLength,  "Snow length",  0.05f, VISUAL | SLIDER_PRECISION, 0.01f, 0.5f, 0.01f, "%.2f");
			CVar(SnowRadius,  "Snow radius",  1500.f,VISUAL | SLIDER_PRECISION, 100.f, 4000.f, 50.f, "%.0f");
			CVar(SnowSideVel, "Snow side vel",40.f,  VISUAL | SLIDER_PRECISION, 0.f, 300.f, 5.f, "%.0f");
			CVar(SnowDensity, "Snow density", 1.f,   VISUAL | SLIDER_PRECISION, 0.1f, 5.f, 0.1f, "%.1f");
			CVar(SnowSprite,  "Snow sprite",  std::string("particle/snow"), VISUAL);
			CVar(FlipWorld, "Flip world", false, VISUAL);
		NAMESPACE_END(Weather)

		// Indicators (velocity + keybind HUD)
		NAMESPACE_BEGIN(Indicators, Aletherium)
			// ── Velocity number ──────────────────────────────────────────
			CVar(VelocityEnabled, "Velocity indicator", false, VISUAL);
			CVar(VelocityPosX, "Velocity X", 0.5f, VISUAL | SLIDER_PRECISION, 0.f, 1.f, 0.005f, "%.3f");
			CVar(VelocityPosY, "Velocity Y", 0.85f, VISUAL | SLIDER_PRECISION, 0.f, 1.f, 0.005f, "%.3f");
			CVar(VelocityScale, "Velocity scale", 1.f, VISUAL | SLIDER_PRECISION, 0.5f, 3.f, 0.05f, "%.2f");
			CVar(VelocityColorBySpeed, "Color by speed", false, VISUAL);
			CVar(VelocityColorLow,  "Low speed color",  Color_t(255, 255, 255, 255), VISUAL);
			CVar(VelocityColorMid,  "Mid speed color",  Color_t(255, 200, 100, 255), VISUAL);
			CVar(VelocityColorHigh, "High speed color", Color_t(255, 100, 100, 255), VISUAL);
			CVar(VelocitySpeedLow,  "Low speed",  300.f, VISUAL | SLIDER_PRECISION, 0.f,    800.f, 10.f, "%.0f");
			CVar(VelocitySpeedHigh, "High speed", 600.f, VISUAL | SLIDER_PRECISION, 100.f, 1500.f, 10.f, "%.0f");
			CVar(VelocityFadeIn, "Fade in", true, VISUAL);
			CVar(VelocityFadeThreshold, "Fade threshold", 0.f, VISUAL | SLIDER_PRECISION, 0.f, 1000.f, 5.f, "%.0f");
			CVar(VelocityOutline, "Outline", false, VISUAL);
			CVar(VelocityOutlineThick, "Outline thick", 1.f, VISUAL | SLIDER_PRECISION, 1.f, 4.f, 0.5f, "%.1f");
			CVar(VelocityOutlineColor, "Outline color", Color_t(0, 0, 0, 200), VISUAL);
			CVar(VelocityShadow, "Shadow", false, VISUAL);
			CVar(VelocityShadowBlur, "Shadow blur", false, VISUAL);
			CVar(VelocityShadowColor, "Shadow color", Color_t(0, 0, 0, 180), VISUAL);
			CVar(VelocityShadowOffX, "Shadow X", 1.f, VISUAL | SLIDER_PRECISION, -5.f, 5.f, 0.5f, "%.1f");
			CVar(VelocityShadowOffY, "Shadow Y", 1.f, VISUAL | SLIDER_PRECISION, -5.f, 5.f, 0.5f, "%.1f");
			// ── Takeoff (pre-jump) number ────────────────────────────────
			CVar(VelocityTakeoff, "Takeoff speed", false, VISUAL);
			CVarEnum(VelocityTakeoffPos, "Takeoff position", 0, VISUAL, nullptr,
				VA_LIST("Beside", "Above", "Below"),
				Beside, Above, Below);
			CVarEnum(VelocityTakeoffAnim, "Takeoff animation", 0, VISUAL, nullptr,
				VA_LIST("Slide", "Expand", "Static"),
				Slide, Expand, Static);
			CVar(VelocityTakeoffHold, "Takeoff hold time", 1.2f, VISUAL | SLIDER_PRECISION, 0.1f, 5.f, 0.1f, "%.1f");
			CVar(VelocityTakeoffFade, "Takeoff fade time", 0.4f, VISUAL | SLIDER_PRECISION, 0.f, 2.f, 0.05f, "%.2f");
			// ── Velocity graph ───────────────────────────────────────────
			CVar(VelocityGraph, "Velocity graph", false, VISUAL);
			CVar(VelocityGraphPosX, "Graph X", 0.5f, VISUAL | SLIDER_PRECISION, 0.f, 1.f, 0.005f, "%.3f");
			CVar(VelocityGraphPosY, "Graph Y", 0.9f, VISUAL | SLIDER_PRECISION, 0.f, 1.f, 0.005f, "%.3f");
			CVar(VelocityGraphW, "Graph width", 200.f, VISUAL | SLIDER_PRECISION, 50.f, 600.f, 5.f, "%.0f");
			CVar(VelocityGraphH, "Graph height", 60.f, VISUAL | SLIDER_PRECISION, 10.f, 300.f, 2.f, "%.0f");
			CVar(VelocityGraphMaxSpeed, "Graph max speed", 500.f, VISUAL | SLIDER_PRECISION, 100.f, 3000.f, 50.f, "%.0f");
			CVar(VelocityGraphLineH, "Graph line height", 1.f, VISUAL | SLIDER_PRECISION, 1.f, 5.f, 0.5f, "%.1f");
			CVar(VelocityGraphColor, "Graph color", Color_t(255, 255, 255, 180), VISUAL);
			CVar(VelocityGraphFade, "Graph fade", 0, VISUAL);
			// ── Keybind indicator ────────────────────────────────────────
			CVar(KeybindEnabled, "Keybind indicator", false, VISUAL);
			CVar(KeybindMask, "Keybind indicators", 4095, VISUAL);
			CVar(KeybindPosX, "Keybind X", 0.5f, VISUAL | SLIDER_PRECISION, 0.f, 1.f, 0.005f, "%.3f");
			CVar(KeybindPosY, "Keybind Y", 0.9f, VISUAL | SLIDER_PRECISION, 0.f, 1.f, 0.005f, "%.3f");
			CVarEnum(KeybindEasing, "Keybind easing", 1, VISUAL, nullptr,
				VA_LIST("Linear", "Fast", "Smooth", "Bounce", "Overshoot"),
				Linear, Fast, Smooth, Bounce, Overshoot);
			CVar(KeybindFadeDuration, "Fade duration", 0.25f, VISUAL | SLIDER_PRECISION, 0.05f, 2.f, 0.05f, "%.2f");
			CVarEnum(KeybindLayout, "Layout", 0, VISUAL, nullptr,
				VA_LIST("Horizontal", "Vertical"),
				Horizontal, Vertical);
			CVarEnum(KeybindVerticalDir, "Vertical dir", 0, VISUAL, nullptr,
				VA_LIST("Down", "Up"),
				Down, Up);
			CVarEnum(KeybindEntryAnim, "Entry animation", 1, VISUAL, nullptr,
				VA_LIST("Static", "Slide right", "Slide left", "Slide center"),
				Static, SlideRight, SlideLeft, SlideCenter);
			CVar(KeybindEntryOffset, "Entry offset", 60.f, VISUAL | SLIDER_PRECISION, 0.f, 300.f, 5.f, "%.0f");
			CVar(KeybindTextColor, "Text color", Color_t(255, 255, 255, 255), VISUAL);
			CVar(KeybindShadow, "Shadow", false, VISUAL);
			CVar(KeybindShadowBlur, "Shadow blur", false, VISUAL);
			CVar(KeybindShadowColor, "Shadow color", Color_t(0, 0, 0, 180), VISUAL);
			CVar(KeybindShadowOffX, "Shadow X", 1.f, VISUAL | SLIDER_PRECISION, -5.f, 5.f, 0.5f, "%.1f");
			CVar(KeybindShadowOffY, "Shadow Y", 1.f, VISUAL | SLIDER_PRECISION, -5.f, 5.f, 0.5f, "%.1f");
			// detect effects
			CVar(KeybindDetectText, "Detect color text", true, VISUAL);
			CVar(KeybindDetectTextColor, "Detect text color", Color_t(255, 200, 50, 255), VISUAL);
			CVar(KeybindDetectColor, "Detect color", Color_t(255, 200, 50, 255), VISUAL);
			CVar(KeybindDetectGlow, "Detect glow", false, VISUAL);
			CVar(KeybindDetectGlowColor, "Glow color", Color_t(255, 200, 50, 200), VISUAL);
			CVar(KeybindDetectGlowBlur, "Glow blur", 4.f, VISUAL | SLIDER_PRECISION, 0.f, 20.f, 0.5f, "%.1f");
			CVar(KeybindDetectGlowStencil, "Glow stencil", 1, VISUAL);
			CVar(KeybindDetectEffectMask, "Keybind detection effects", 4095, VISUAL);
			CVar(KeybindDetectParticles, "Particles", false, VISUAL);
			CVar(KeybindDetectParticleColor, "Particle color", Color_t(255, 200, 50, 220), VISUAL);
			CVar(KeybindDetectParticleCount, "Particle count", 12, SLIDER_MIN, 1, 60);
			CVar(KeybindDetectParticleSpeed, "Particle speed", 180.f, VISUAL | SLIDER_PRECISION, 10.f, 800.f, 10.f, "%.0f");
			CVar(KeybindDetectParticleLifetime, "Particle lifetime", 0.8f, VISUAL | SLIDER_PRECISION, 0.1f, 3.f, 0.05f, "%.2f");
			CVar(KeybindDetectParticleScale, "Particle scale", 3.f, VISUAL | SLIDER_PRECISION, 1.f, 20.f, 0.5f, "%.1f");
			CVar(KeybindDetectParticleSpread, "Particle spread", 6.f, VISUAL | SLIDER_PRECISION, 0.f, 40.f, 1.f, "%.0f");
			CVar(KeybindDetectParticleGravity, "Particle gravity", 80.f, VISUAL | SLIDER_PRECISION, -500.f, 500.f, 10.f, "%.0f");
			CVar(KeybindDetectParticleAntiAlias, "Particle AA", true, VISUAL);
			CVarEnum(KeybindDetectParticleShape, "Particle shape", 0, VISUAL, nullptr,
				VA_LIST("Random", "Circle", "Square", "Triangle", "Diamond", "Star"),
				Random, Circle, Square, Triangle, Diamond, Star);
			CVar(KeybindDetectRipple, "Ripple", false, VISUAL);
			CVar(KeybindDetectRippleColor, "Ripple color", Color_t(255, 200, 50, 200), VISUAL);
			CVar(KeybindDetectRippleCount, "Ripple count", 3, SLIDER_MIN, 1, 8);
			CVar(KeybindDetectRippleDuration, "Ripple duration", 0.7f, VISUAL | SLIDER_PRECISION, 0.1f, 3.f, 0.05f, "%.2f");
			CVar(KeybindDetectRippleRadius, "Ripple radius", 22.f, VISUAL | SLIDER_PRECISION, 5.f, 80.f, 1.f, "%.0f");
			CVar(KeybindDetectRippleThickness, "Ripple thickness", 1.5f, VISUAL | SLIDER_PRECISION, 0.5f, 6.f, 0.25f, "%.2f");
			// Labels (customisable display names)
			CVar(KeybindLabelEdgeBug,       "EdgeBug label",       std::string("EdgeBug"));
			CVar(KeybindLabelLongJump,      "LongJump label",      std::string("LongJump"));
			CVar(KeybindLabelMiniJump,      "MiniJump label",      std::string("MiniJump"));
			CVar(KeybindLabelAutoAlign,     "AutoAlign label",     std::string("AutoAlign"));
			CVar(KeybindLabelPixelSurf,     "PixelSurf label",     std::string("PixelSurf"));
			CVar(KeybindLabelTextureBug,    "TextureBug label",    std::string("TextureBug"));
			CVar(KeybindLabelWallClimb,     "WallClimb label",     std::string("WallClimb"));
			CVar(KeybindLabelJumpBug,       "JumpBug label",       std::string("JumpBug"));
			CVar(KeybindLabelPixelFinder,   "PixelFinder label",   std::string("PixelFinder"));
			CVar(KeybindLabelPixelSurfAssist,"PixelSurfAssist label", std::string("PSAssist"));
			CVar(KeybindLabelAutoStrafe,    "AutoStrafe label",    std::string("AutoStrafe"));
			CVar(KeybindLabelAirStuck,      "AirStuck label",      std::string("AirStuck"));
		NAMESPACE_END(Indicators)

		// Discord RPC
		NAMESPACE_BEGIN(Discord, Aletherium)
			CVar(Enabled, "Discord RPC", false, NOBIND);
		NAMESPACE_END(Discord)

		// FakePOV (demo cam angle offset)
		NAMESPACE_BEGIN(FakePOV, Aletherium)
			CVar(Enabled, "Fake POV", false, VISUAL);
			CVarEnum(Mode, "Fake POV mode", 0, VISUAL, nullptr,
				VA_LIST("Right", "Left", "Up", "Bottom", "Backwards", "Spinning"),
				Right, Left, Up, Bottom, Backwards, Spinning);
			CVar(SmoothSpeed, "Smooth speed", 5.f, VISUAL | SLIDER_PRECISION, 0.1f, 30.f, 0.5f, "%.1f");
			CVar(SpinSpeed, "Spin speed", 90.f, VISUAL | SLIDER_PRECISION, 10.f, 720.f, 5.f, "%.0f");
			CVar(SnapView, "Snap view on disable", true, VISUAL);
			CVar(Arrow, "Show arrow", true, VISUAL);
			CVar(ArrowColor, "Arrow color", Color_t(255, 200, 50, 220), VISUAL);
			CVar(ArrowSize, "Arrow size", 12.f, VISUAL | SLIDER_PRECISION, 4.f, 40.f, 1.f, "%.0f");
			CVar(ArrowDist, "Arrow distance", 28.f, VISUAL | SLIDER_PRECISION, 0.f, 120.f, 2.f, "%.0f");
		NAMESPACE_END(FakePOV)

		// Cheater Detection (extended)
		NAMESPACE_BEGIN(CheaterDetection, Aletherium)
			CVar(PacketChoking, "Detect packet choking", true, NOBIND);
			CVar(AimFlicking, "Detect aim flicking", true, NOBIND);
			CVar(DuckSpeed, "Detect duck speed", true, NOBIND);
			CVar(MinimumChoking, "Minimum choke count", 20, SLIDER_MIN, 4, 22);
			CVar(MinimumFlick, "Minimum flick angle", 20.f, SLIDER_PRECISION, 10.f, 60.f);
			CVar(MaximumNoise, "Maximum flick noise", 1.f, SLIDER_PRECISION, 0.f, 10.f);
		NAMESPACE_END(CheaterDetection)

	NAMESPACE_END(Aletherium)
	// ───────────────────────────────────────────────────────────────────────

NAMESPACE_END(Vars)