#include "Indicators.h"
#include "../../SDK/SDK.h"
#include "../Misc/Misc.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>

// Math constants missing from our SDK
#ifndef PI
static constexpr float PI = 3.14159265358979323846f;
#endif
#ifndef DEG2RAD
static constexpr float DEG2RAD_FACTOR = PI / 180.f;
#define DEG2RAD(x) ((x) * DEG2RAD_FACTOR)
#endif

// All movement vars and detector methods are now provided by the ported Misc.h / Misc.cpp.
// Forward the MiscStubs namespace to the real F::Misc methods so the keybind indicator
// wiring below compiles without changes.
namespace { namespace MiscStubs {
    inline bool IsEdgeBugDetected()      { return F::Misc.IsEdgeBugDetected(); }
    inline bool IsLongJumpDetected()     { return F::Misc.IsLongJumpDetected(); }
    inline bool IsMiniJumpDetected()     { return F::Misc.IsMiniJumpDetected(); }
    inline bool IsWallDetected()         { return F::Misc.IsWallDetected(); }
    inline bool IsPixelSurfing()         { return F::Misc.IsPixelSurfing(); }
    inline bool IsTextureBugHit()        { return F::Misc.IsTextureBugHit(); }
    inline bool IsWallClimbHit()         { return F::Misc.IsWallClimbHit(); }
    inline bool IsJumpBugHit()           { return F::Misc.IsJumpBugHit(); }
    inline bool IsPixelFinderActive()    { return F::Misc.IsPixelFinderActive(); }
    inline bool IsPixelSurfAssistActive(){ return F::Misc.IsPixelSurfAssistActive(); }
    inline bool IsAutoStrafeActive()     { return F::Misc.IsAutoStrafeActive(); }
    inline bool IsAirStuckHit()          { return F::Misc.IsAirStuckHit(); }
    inline int  EdgeBugSuccessCounter()  { return F::Misc.EdgeBugSuccessCounter(); }
}}
// ──────────────────────────────────────────────────────────────────────────

// Detection-particle RNG. Lives on the render thread only, so a plain file-local generator is fine and
// keeps us off the game's shared random stream (no prediction side effects, unlike SDK::RandomFloat).
namespace
{
    float ParticleRand(float flLo, float flHi)
    {
        static std::mt19937 s_gen(std::random_device{}());
        std::uniform_real_distribution<float> dist(flLo, flHi);
        return dist(s_gen);
    }
}

float CIndicators::Lerp(float flCurrent, float flTarget, float flSpeed)
{
    float flDelta = flTarget - flCurrent;
    float flStep = flDelta * flSpeed * I::GlobalVars->frametime;

    if (std::abs(flDelta) < 0.01f)
        return flTarget;

    return flCurrent + flStep;
}

// Frame-rate independent exponential smoothing (matches the feel).
float CIndicators::LerpSmooth(float flCurrent, float flTarget, float flSpeed)
{
    if (std::abs(flTarget - flCurrent) < 0.001f)
        return flTarget;

    return flCurrent + (flTarget - flCurrent) * (1.f - expf(-flSpeed * I::GlobalVars->frametime));
}

// - easing functions copied from AHEasing (public domain). p in [0,1].
namespace KeybindEase
{
    static inline float Linear(float p) { return p; }
    // Smooth = cubic ease in-out (slow start, slow end).
    static inline float Smooth(float p)
    {
        return p < 0.5f ? 4.f * p * p * p : 1.f - powf(-2.f * p + 2.f, 3.f) / 2.f;
    }
    // Fast = exponential ease out (snaps in almost instantly, settles softly).
    static inline float Fast(float p)
    {
        return p >= 1.f ? 1.f : 1.f - powf(2.f, -10.f * p);
    }
    // Bounce = ease-out bounce (drops onto the target and bounces a couple of times).
    static inline float Bounce(float p)
    {
        if (p < 4.f / 11.f)
            return (121.f * p * p) / 16.f;
        if (p < 8.f / 11.f)
            return (363.f / 40.f) * p * p - (99.f / 10.f) * p + 17.f / 5.f;
        if (p < 9.f / 10.f)
            return (4356.f / 361.f) * p * p - (35442.f / 1805.f) * p + 16061.f / 1805.f;
        return (54.f / 5.f) * p * p - (513.f / 25.f) * p + 268.f / 25.f;
    }
    // Overshoot = back ease out (slides past the target then pulls back). Output exceeds 1
    // mid-curve on purpose - clamp at the call site where it's used as an opacity.
    static inline float Overshoot(float p)
    {
        constexpr float c1 = 1.70158f, c3 = c1 + 1.f;
        const float f = p - 1.f;
        return 1.f + c3 * f * f * f + c1 * f * f;
    }
    static inline float Apply(int iMode, float p)
    {
        p = std::clamp(p, 0.f, 1.f);
        switch (iMode)
        {
        case Vars::Aletherium::Indicators::KeybindEasingEnum::Linear:    return Linear(p);
        case Vars::Aletherium::Indicators::KeybindEasingEnum::Fast:      return Fast(p);
        case Vars::Aletherium::Indicators::KeybindEasingEnum::Bounce:    return Bounce(p);
        case Vars::Aletherium::Indicators::KeybindEasingEnum::Overshoot: return Overshoot(p);
        default:                                              return Smooth(p);
        }
    }
}

void CIndicators::Draw(CTFPlayer* pLocal)
{
    if (!pLocal || !pLocal->IsAlive())
        return;

    if (Vars::Aletherium::Indicators::VelocityEnabled.Value || Vars::Aletherium::Indicators::VelocityGraph.Value)
        DrawVelocity(pLocal);

    if (Vars::Aletherium::Indicators::KeybindEnabled.Value)
        DrawKeybinds();
}

// Critically-damped spring.
void CIndicators::SpringToward(float& flVal, float& flVel, float flTarget, float flDt, float flOmega)
{
    constexpr float flZeta = 1.f;
    const float flErr = flVal - flTarget;
    const float flDamp = 2.f * flZeta * flOmega;
    flVel = flVel - (flDamp * flVel + flOmega * flOmega * flErr) * flDt;
    flVal = flVal + flVel * flDt;
    if (std::fabs(flVal - flTarget) < 0.15f && std::fabs(flVel) < 0.5f) { flVal = flTarget; flVel = 0.f; }
}

// Speed -> color gradient (slow/mid/fast) with master alpha multiplied in.
Color_t CIndicators::ResolveSpeedColor(float flSpeed, float flMasterAlpha)
{
    const float flA = std::clamp(flMasterAlpha, 0.f, 1.f);
    auto Apply = [&](Color_t c) { return c.Alpha(static_cast<byte>(c.a * flA)); };

    if (!Vars::Aletherium::Indicators::VelocityColorBySpeed.Value)
        return Apply(Vars::Aletherium::Indicators::VelocityColorLow.Value);

    float flLo = Vars::Aletherium::Indicators::VelocitySpeedLow.Value;
    float flHi = Vars::Aletherium::Indicators::VelocitySpeedHigh.Value;
    if (flHi <= flLo) flHi = flLo + 1.f;

    const Color_t cLow = Vars::Aletherium::Indicators::VelocityColorLow.Value;
    const Color_t cMid = Vars::Aletherium::Indicators::VelocityColorMid.Value;
    const Color_t cHigh = Vars::Aletherium::Indicators::VelocityColorHigh.Value;

    if (flSpeed <= flLo) return Apply(cLow);
    if (flSpeed >= flHi) return Apply(cHigh);

    const float t = (flSpeed - flLo) / (flHi - flLo);
    const Color_t c = (t < 0.5f) ? cLow.Lerp(cMid, t * 2.f) : cMid.Lerp(cHigh, (t - 0.5f) * 2.f);
    return Apply(c);
}

void CIndicators::DrawVelocity(CTFPlayer* pLocal)
{
    const float flDt = std::min(I::GlobalVars->frametime, 0.1f);
    const Vec3 vVelocity = pLocal->m_vecVelocity();
    const float flSpeed = vVelocity.Length2D();
    const float flVz = vVelocity.z;
    const bool bOnGround = pLocal->OnSolid();

    const bool bNumber = Vars::Aletherium::Indicators::VelocityEnabled.Value;
    const bool bGraph = Vars::Aletherium::Indicators::VelocityGraph.Value;
    const bool bShowPreOpt = Vars::Aletherium::Indicators::VelocityTakeoff.Value;
    const bool bFadeIn = Vars::Aletherium::Indicators::VelocityFadeIn.Value;
    // Pre-jump placement + animation mode. Slide/expand only make sense next to the number;
    // above/below (and Static) reduce to a pure alpha fade with no horizontal motion.
    const bool bPreBeside = Vars::Aletherium::Indicators::VelocityTakeoffPos.Value == Vars::Aletherium::Indicators::VelocityTakeoffPosEnum::Beside;
    const bool bPreAbove = Vars::Aletherium::Indicators::VelocityTakeoffPos.Value == Vars::Aletherium::Indicators::VelocityTakeoffPosEnum::Above;
    const bool bPreStatic = Vars::Aletherium::Indicators::VelocityTakeoffAnim.Value == Vars::Aletherium::Indicators::VelocityTakeoffAnimEnum::Static;
    const bool bAnimCenter = bPreBeside && !bPreStatic
        && Vars::Aletherium::Indicators::VelocityTakeoffAnim.Value == Vars::Aletherium::Indicators::VelocityTakeoffAnimEnum::Expand;

    const int iScreenW = H::Draw.m_nScreenW;
    const int iScreenH = H::Draw.m_nScreenH;
    // FONT_VELOCITY resolves to the custom family (if set) or the default / default-thin
    // family, and always applies the velocity size slider — so use it unconditionally.
    const EFonts eFontV = FONT_VELOCITY;
    const auto& tFont = H::Fonts.GetFont(eFontV);

    // ==== Ground/air + pre-jump capture (genuine jump only: vz > 10) ====
    if (m_bWasOnGround && !bOnGround && flVz > 10.f)
    {
        m_flPreVal = flSpeed;
        m_flPreTimer = Vars::Aletherium::Indicators::VelocityTakeoffHold.Value;
        m_flCenterDelay = 0.f;
        if (m_flPreAlpha < 0.05f)
        {
            m_flPreXOff = (bPreStatic || !bPreBeside) ? 0.f : (bAnimCenter ? -1.f : 150.f);
            m_flPreXOffV = 0.f;
            m_flPreSlotW = 0.f;
            m_flPreSlotWV = 0.f;
        }
    }
    m_bWasOnGround = bOnGround;

    // ==== Pre-jump alpha + slide/expand springs ====
    if (bShowPreOpt)
    {
        char szPreTmp[32];
        std::snprintf(szPreTmp, sizeof(szPreTmp), "(%d)", static_cast<int>(std::round(m_flPreVal)));
        const float flNaturalW = H::Draw.GetTextSize(szPreTmp, tFont).x + 6.f;

        const bool bHolding = m_flPreTimer > 0.f;
        if (bHolding)
        {
            m_flPreTimer -= flDt;
            if (m_flPreTimer <= 0.f) m_flCenterDelay = 0.2f;
        }

        if (bPreStatic)
        {
            // Static: no motion at all - pure alpha fade over the fade-time slider (0 = instant).
            const float flFade = Vars::Aletherium::Indicators::VelocityTakeoffFade.Value;
            const float flStep = flFade > 0.f ? flDt / flFade : 1.f;
            m_flPreAlpha = std::clamp(m_flPreAlpha + (bHolding ? flStep : -flStep), 0.f, 1.f);
            m_flPreAlphaV = 0.f;
            m_flPreXOff = 0.f; m_flPreXOffV = 0.f;
            // Slot snaps so the main number jumps over/back instantly (beside placement only).
            m_flPreSlotW = (bHolding || m_flPreAlpha > 0.004f) ? flNaturalW : 0.f;
            m_flPreSlotWV = 0.f;
            m_flCenterDelay = 0.f;
        }
        else if (!bPreBeside)
        {
            // Above/below: there's no horizontal docking, just the spring fade in place.
            SpringToward(m_flPreAlpha, m_flPreAlphaV, bHolding ? 1.f : 0.f, flDt);
            m_flPreAlpha = std::clamp(m_flPreAlpha, 0.f, 1.f);
            m_flPreXOff = 0.f; m_flPreXOffV = 0.f;
            m_flPreSlotW = 0.f; m_flPreSlotWV = 0.f;
        }
        else if (!bAnimCenter)
        {
            // Slide-from-right: XOff is pixels (large positive = off-screen, 0 = docked).
            if (bHolding)
            {
                SpringToward(m_flPreAlpha, m_flPreAlphaV, 1.f, flDt);
                SpringToward(m_flPreXOff, m_flPreXOffV, 0.f, flDt);
                SpringToward(m_flPreSlotW, m_flPreSlotWV, flNaturalW, flDt);
            }
            else
            {
                SpringToward(m_flPreAlpha, m_flPreAlphaV, 0.f, flDt);
                m_flPreXOff = 0.f; m_flPreXOffV = 0.f;
                if (m_flCenterDelay > 0.f) m_flCenterDelay -= flDt;
                else SpringToward(m_flPreSlotW, m_flPreSlotWV, 0.f, flDt);
            }
            m_flPreAlpha = std::clamp(m_flPreAlpha, 0.f, 1.f);
            m_flPreXOff = std::max(0.f, m_flPreXOff);
            m_flPreSlotW = std::max(0.f, m_flPreSlotW);
        }
        else
        {
            // Expand-from-center: XOff normalized [-1, 0] (-1 = collapsed, 0 = expanded).
            if (bHolding)
            {
                SpringToward(m_flPreAlpha, m_flPreAlphaV, 1.f, flDt);
                SpringToward(m_flPreXOff, m_flPreXOffV, 0.f, flDt);
                SpringToward(m_flPreSlotW, m_flPreSlotWV, flNaturalW, flDt);
            }
            else
            {
                SpringToward(m_flPreAlpha, m_flPreAlphaV, 0.f, flDt);
                SpringToward(m_flPreXOff, m_flPreXOffV, -1.f, flDt);
                if (m_flCenterDelay > 0.f) m_flCenterDelay -= flDt;
                else SpringToward(m_flPreSlotW, m_flPreSlotWV, 0.f, flDt);
            }
            m_flPreAlpha = std::clamp(m_flPreAlpha, 0.f, 1.f);
            m_flPreXOff = std::clamp(m_flPreXOff, -1.f, 0.f);
            m_flPreSlotW = std::max(0.f, m_flPreSlotW);
        }
    }
    else
    {
        m_flPreAlpha = 0.f; m_flPreAlphaV = 0.f;
        m_flPreXOff = 0.f; m_flPreXOffV = 0.f;
        m_flPreSlotW = 0.f; m_flPreSlotWV = 0.f;
        m_flCenterDelay = 0.f;
    }

    // ==== Graph history ====
    if (bGraph)
    {
        m_dqHistory.push_front(flSpeed);
        while (static_cast<int>(m_dqHistory.size()) > m_iHistMax)
            m_dqHistory.pop_back();
    }
    else if (!m_dqHistory.empty())
        m_dqHistory.clear();

    // ==== Number fade alpha ====
    const float flFadeThresh = Vars::Aletherium::Indicators::VelocityFadeThreshold.Value;
    const bool bNumberVisible = bNumber && (flFadeThresh <= 0.f || flSpeed >= flFadeThresh);
    const float flTargetA = bNumberVisible ? 1.f : 0.f;
    if (!bFadeIn) { m_flAlpha = flTargetA; m_flAlphaV = 0.f; }
    else { SpringToward(m_flAlpha, m_flAlphaV, flTargetA, flDt); m_flAlpha = std::clamp(m_flAlpha, 0.f, 1.f); }

    // ==== Graph fade alpha (independent) ====
    const float flTargetGA = bGraph ? 1.f : 0.f;
    if (!bFadeIn) { m_flGraphAlpha = flTargetGA; m_flGraphAlphaV = 0.f; }
    else { SpringToward(m_flGraphAlpha, m_flGraphAlphaV, flTargetGA, flDt); m_flGraphAlpha = std::clamp(m_flGraphAlpha, 0.f, 1.f); }

    // ==== Text draw helper: outline + shadow + body ====
    const bool bOutline = Vars::Aletherium::Indicators::VelocityOutline.Value;
    const bool bShadow = Vars::Aletherium::Indicators::VelocityShadow.Value;
    auto DrawWithEffects = [&](const char* szStr, int iX, int iY, Color_t tCol, float flMasterA)
    {
        if (bOutline)
        {
            const int iOt = std::max(1, static_cast<int>(std::round(Vars::Aletherium::Indicators::VelocityOutlineThick.Value)));
            const Color_t cO = Vars::Aletherium::Indicators::VelocityOutlineColor.Value;
            const Color_t oc = cO.Alpha(static_cast<byte>(cO.a * std::clamp(flMasterA, 0.f, 1.f)));
            H::Draw.String(tFont, iX - iOt, iY, oc, ALIGN_TOPLEFT, szStr);
            H::Draw.String(tFont, iX + iOt, iY, oc, ALIGN_TOPLEFT, szStr);
            H::Draw.String(tFont, iX, iY - iOt, oc, ALIGN_TOPLEFT, szStr);
            H::Draw.String(tFont, iX, iY + iOt, oc, ALIGN_TOPLEFT, szStr);
            H::Draw.String(tFont, iX - iOt, iY - iOt, oc, ALIGN_TOPLEFT, szStr);
            H::Draw.String(tFont, iX + iOt, iY - iOt, oc, ALIGN_TOPLEFT, szStr);
            H::Draw.String(tFont, iX - iOt, iY + iOt, oc, ALIGN_TOPLEFT, szStr);
            H::Draw.String(tFont, iX + iOt, iY + iOt, oc, ALIGN_TOPLEFT, szStr);
        }
        if (bShadow)
        {
            const Color_t cS = Vars::Aletherium::Indicators::VelocityShadowColor.Value;
            const int iOx = static_cast<int>(std::round(Vars::Aletherium::Indicators::VelocityShadowOffX.Value));
            const int iOy = static_cast<int>(std::round(Vars::Aletherium::Indicators::VelocityShadowOffY.Value));
            if (Vars::Aletherium::Indicators::VelocityShadowBlur.Value)
            {
                const Color_t sc = cS.Alpha(static_cast<byte>(cS.a * std::clamp(flMasterA, 0.f, 1.f) * 0.35f));
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++)
                        H::Draw.String(tFont, iX + iOx + dx, iY + iOy + dy, sc, ALIGN_TOPLEFT, szStr);
            }
            else
            {
                const Color_t sc = cS.Alpha(static_cast<byte>(cS.a * std::clamp(flMasterA, 0.f, 1.f)));
                H::Draw.String(tFont, iX + iOx, iY + iOy, sc, ALIGN_TOPLEFT, szStr);
            }
        }
        H::Draw.String(tFont, iX, iY, tCol, ALIGN_TOPLEFT, szStr);
    };

    // ==== Draw numbers ====
    if (bNumber && m_flAlpha > 0.004f)
    {
        if (m_flDisplayVal < 0.f) { m_flDisplayVal = flSpeed; m_flDisplayV = 0.f; }
        SpringToward(m_flDisplayVal, m_flDisplayV, flSpeed, flDt);
        m_flDisplayVal = std::max(0.f, m_flDisplayVal);

        const float flSx = iScreenW * Vars::Aletherium::Indicators::VelocityPosX.Value;
        const float flSy = iScreenH * Vars::Aletherium::Indicators::VelocityPosY.Value;
        const Color_t tTextCol = ResolveSpeedColor(flSpeed, m_flAlpha);

        char szMain[32];
        std::snprintf(szMain, sizeof(szMain), "%d", static_cast<int>(std::round(m_flDisplayVal)));
        const Vec2 vMainSz = H::Draw.GetTextSize(szMain, tFont);

        if (m_flMainSlotW < 1.f) { m_flMainSlotW = vMainSz.x; m_flMainSlotWV = 0.f; }
        SpringToward(m_flMainSlotW, m_flMainSlotWV, vMainSz.x, flDt);
        m_flMainSlotW = std::max(1.f, m_flMainSlotW);

        constexpr float flGap = 6.f;
        const bool bShowPre = bShowPreOpt && m_flPreAlpha > 0.004f;

        char szPre[32] = {};
        Vec2 vPreSz = {};
        if (bShowPre || m_flPreAlpha > 0.f)
        {
            std::snprintf(szPre, sizeof(szPre), "(%d)", static_cast<int>(std::round(m_flPreVal)));
            vPreSz = H::Draw.GetTextSize(szPre, tFont);
        }

        const float flTy = flSy - vMainSz.y * 0.5f;

        if (!bPreBeside)
        {
            // Above/below placement: main number stays centred on the position sliders, the
            // pre-jump number sits centred one row above or below it (no horizontal motion).
            const float flTargetMainX = flSx - m_flMainSlotW * 0.5f;
            if (m_flMainPosX < 0.f || bPreStatic) m_flMainPosX = flTargetMainX;
            else m_flMainPosX += (flTargetMainX - m_flMainPosX) * (1.f - std::exp(-12.f * flDt));
            const float flMainX = m_flMainPosX + (m_flMainSlotW - vMainSz.x) * 0.5f;
            DrawWithEffects(szMain, static_cast<int>(std::round(flMainX)), static_cast<int>(std::round(flTy)), tTextCol, m_flAlpha);
            if (bShowPre)
            {
                const float flRowGap = 2.f;
                const float flPreX = flSx - vPreSz.x * 0.5f;
                const float flPreY = bPreAbove ? flTy - vPreSz.y - flRowGap : flTy + vMainSz.y + flRowGap;
                const float flPa = m_flPreAlpha * m_flAlpha;
                const Color_t tPreCol = ResolveSpeedColor(m_flPreVal, flPa);
                DrawWithEffects(szPre, static_cast<int>(std::round(flPreX)), static_cast<int>(std::round(flPreY)), tPreCol, flPa);
            }
        }
        else if (!bAnimCenter)
        {
            // Slide-from-right (or static beside): main number springs to grouped/solo center position.
            const bool bGroupMode = bShowPre || (m_flCenterDelay > 0.f);
            const float flGroupW = m_flMainSlotW + (bGroupMode ? m_flPreSlotW : 0.f);
            const float flTargetMainX = flSx - flGroupW * 0.5f;
            if (m_flMainPosX < 0.f || bPreStatic) m_flMainPosX = flTargetMainX; // static: no recenter animation
            else m_flMainPosX += (flTargetMainX - m_flMainPosX) * (1.f - std::exp(-12.f * flDt));
            const float flTx = m_flMainPosX;
            const float flMainX = flTx + (m_flMainSlotW - vMainSz.x) * 0.5f;
            DrawWithEffects(szMain, static_cast<int>(std::round(flMainX)), static_cast<int>(std::round(flTy)), tTextCol, m_flAlpha);
            if (bShowPre)
            {
                const float flPreX = flTx + m_flMainSlotW + flGap + m_flPreXOff;
                const float flPa = m_flPreAlpha * m_flAlpha;
                const Color_t tPreCol = ResolveSpeedColor(m_flPreVal, flPa);
                DrawWithEffects(szPre, static_cast<int>(std::round(flPreX)), static_cast<int>(std::round(flTy)), tPreCol, flPa);
            }
        }
        else
        {
            // Expand-from-center: expansion = slotW * (1 + XOff), XOff in [-1, 0].
            const float flExpansion = m_flPreSlotW * (1.f + m_flPreXOff);
            const float flTotalW = bShowPre ? m_flMainSlotW + flExpansion : m_flMainSlotW;
            const float flTx = flSx - flTotalW * 0.5f;
            const float flMainX = flTx + (m_flMainSlotW - vMainSz.x) * 0.5f;
            DrawWithEffects(szMain, static_cast<int>(std::round(flMainX)), static_cast<int>(std::round(flTy)), tTextCol, m_flAlpha);
            if (bShowPre)
            {
                const float flPreX = flTx + m_flMainSlotW + flExpansion - vPreSz.x;
                const float flPa = m_flPreAlpha * m_flAlpha;
                const Color_t tPreCol = ResolveSpeedColor(m_flPreVal, flPa);
                DrawWithEffects(szPre, static_cast<int>(std::round(flPreX)), static_cast<int>(std::round(flTy)), tPreCol, flPa);
            }
        }
    }

    // ==== Draw graph ====
    if (bGraph && m_flGraphAlpha > 0.004f && static_cast<int>(m_dqHistory.size()) >= 2)
    {
        const int N = static_cast<int>(m_dqHistory.size());
        const float flScale = Vars::Aletherium::Indicators::VelocityScale.Value;
        const float flGw = Vars::Aletherium::Indicators::VelocityGraphW.Value * flScale;
        const float flGh = Vars::Aletherium::Indicators::VelocityGraphH.Value * flScale;
        const float flGsx = iScreenW * Vars::Aletherium::Indicators::VelocityGraphPosX.Value;
        const float flGsy = iScreenH * Vars::Aletherium::Indicators::VelocityGraphPosY.Value;
        const float flLeft = flGsx - flGw * 0.5f;
        const float flBot = flGsy;
        const float flMaxSpd = std::max(1.f, Vars::Aletherium::Indicators::VelocityGraphMaxSpeed.Value);
        const float flLineW = flGw / static_cast<float>(m_iHistMax - 1);
        const Color_t cGraph = Vars::Aletherium::Indicators::VelocityGraphColor.Value;
        const int iFadeMode = Vars::Aletherium::Indicators::VelocityGraphFade.Value;
        const int iThick = std::max(1, static_cast<int>(std::round(Vars::Aletherium::Indicators::VelocityGraphLineH.Value)));

        for (int i = 0; i < N - 1; i++)
        {
            const float flCur = m_dqHistory[i];
            const float flNext = m_dqHistory[i + 1];

            int iFadeMult = 255;
            switch (iFadeMode)
            {
            case 1: // both ends
                if (i < 60) iFadeMult = static_cast<int>(((i + 1) / 60.f) * 255.f);
                else if (i > N - 61) iFadeMult = static_cast<int>(((N - i) / 60.f) * 255.f);
                break;
            case 2: // both ends + by velocity
                if (i < 60) iFadeMult = static_cast<int>(((i + 1) / 60.f) * 255.f);
                else if (i > N - 61) iFadeMult = static_cast<int>(((N - i) / 60.f) * 255.f);
                iFadeMult = static_cast<int>(iFadeMult * std::clamp(((flCur + flNext) * 0.5f) / flMaxSpd, 0.f, 1.f));
                break;
            case 3: // end only
                if (i > N - 61) iFadeMult = static_cast<int>(((N - i) / 60.f) * 255.f);
                break;
            case 4: // start only
                if (i < 60) iFadeMult = static_cast<int>(((i + 1) / 60.f) * 255.f);
                break;
            default: break;
            }
            iFadeMult = std::clamp(iFadeMult, 0, 255);

            const float flFa = (cGraph.a / 255.f) * m_flGraphAlpha * (iFadeMult / 255.f);
            const Color_t lc = cGraph.Alpha(static_cast<byte>(std::clamp(flFa, 0.f, 1.f) * 255.f));

            const int iX1 = static_cast<int>(std::round(flLeft + flGw - static_cast<float>(i) * flLineW));
            const int iX2 = static_cast<int>(std::round(flLeft + flGw - static_cast<float>(i + 1) * flLineW));
            const int iY1 = static_cast<int>(std::round(flBot - std::clamp(flCur / flMaxSpd, 0.f, 1.f) * flGh));
            const int iY2 = static_cast<int>(std::round(flBot - std::clamp(flNext / flMaxSpd, 0.f, 1.f) * flGh));

            // Surface lines are 1px; stack copies to emulate thickness.
            for (int k = 0; k < iThick; k++)
                H::Draw.Line(iX1, iY1 + k, iX2, iY2 + k, lc);
        }
    }
}

void CIndicators::DrawKeybinds()
{
    // Animation tuning (frame-rate independent).
    constexpr float flPosSpeed = 20.f;
    constexpr float flDetectSpeed = 14.f;
    constexpr float flSpacing = 8.f;

    // Movement features that own an indicator: stable id + (customizable) label + enabled +
    // detection state + whether the text uses the accent color (the edge-bug "name").
    struct Source_t { int m_iId; std::string m_sLabel; bool m_bEnabled; bool m_bDetect; bool m_bAccent; };
    const Source_t aSources[] =
    {
        { 0, Vars::Aletherium::Indicators::KeybindLabelEdgeBug.Value,          Vars::Misc::Movement::EdgeBug.Value,         MiscStubs::IsEdgeBugDetected(),      false },
        { 1, Vars::Aletherium::Indicators::KeybindLabelLongJump.Value,         Vars::Misc::Movement::LongJump.Value,        MiscStubs::IsLongJumpDetected(),     false },
        { 2, Vars::Aletherium::Indicators::KeybindLabelMiniJump.Value,         Vars::Misc::Movement::MiniJump.Value,        MiscStubs::IsMiniJumpDetected(),     false },
        { 3, Vars::Aletherium::Indicators::KeybindLabelAutoAlign.Value,        Vars::Misc::Movement::AutoAlign.Value,       MiscStubs::IsWallDetected(),         false },
        { 4, Vars::Aletherium::Indicators::KeybindLabelPixelSurf.Value,        Vars::Misc::Movement::PixelSurf.Value,       MiscStubs::IsPixelSurfing(),         false },
        { 5, Vars::Aletherium::Indicators::KeybindLabelTextureBug.Value,       Vars::Misc::Movement::TextureBug.Value,      MiscStubs::IsTextureBugHit(),        false },
        { 6, Vars::Aletherium::Indicators::KeybindLabelWallClimb.Value,        Vars::Misc::Movement::WallClimb.Value,       MiscStubs::IsWallClimbHit(),         false },
        { 7, Vars::Aletherium::Indicators::KeybindLabelJumpBug.Value,          Vars::Misc::Movement::AutoJumpbug.Value,     MiscStubs::IsJumpBugHit(),           false },
        { 8, Vars::Aletherium::Indicators::KeybindLabelPixelFinder.Value,      Vars::Misc::Movement::PixelFinder.Value,     MiscStubs::IsPixelFinderActive(),    false },
        { 9, Vars::Aletherium::Indicators::KeybindLabelPixelSurfAssist.Value,  Vars::Misc::Movement::PixelSurfAssist.Value, MiscStubs::IsPixelSurfAssistActive(),false },
        { 10, Vars::Aletherium::Indicators::KeybindLabelAutoStrafe.Value,      (bool)Vars::Misc::Movement::AutoStrafe.Value,      MiscStubs::IsAutoStrafeActive(),     false },
        { 11, Vars::Aletherium::Indicators::KeybindLabelAirStuck.Value,        Vars::Misc::Movement::AirStuck.Value,        MiscStubs::IsAirStuckHit(),          false },
    };

    // Detection effects fire when the action is actually *performed*. For most features the rising edge
    const int iEbCur = MiscStubs::EdgeBugSuccessCounter();
    const bool bEdgeBugFired = (m_iLastEdgeBugSuccess >= 0 && iEbCur != m_iLastEdgeBugSuccess);
    m_iLastEdgeBugSuccess = iEbCur;

    const bool bEffectsOn = Vars::Aletherium::Indicators::KeybindDetectParticles.Value || Vars::Aletherium::Indicators::KeybindDetectRipple.Value;

    // Reconcile sources with live indicators: add new, refresh detect, retire disabled.
    for (auto& ind : m_vKeybindIndicators)
        ind.m_bRemoving = true; // assume gone until matched below

    for (const auto& src : aSources)
    {
        if (!src.m_bEnabled)
            continue;

        // Per-indicator visibility toggle (Visuals -> keybind indicators multi-select).
        if (!(Vars::Aletherium::Indicators::KeybindMask.Value & (1 << src.m_iId)))
            continue;

        auto it = std::find_if(m_vKeybindIndicators.begin(), m_vKeybindIndicators.end(),
            [&](const MovementIndicator_t& ind) { return ind.m_iId == src.m_iId; });

        if (it != m_vKeybindIndicators.end())
        {
            it->m_sLabel = src.m_sLabel; // pick up live label edits
            it->m_bAccent = src.m_bAccent;
            it->m_bActive = true;
            it->m_bRemoving = false;
            it->m_flDetect = LerpSmooth(it->m_flDetect, src.m_bDetect ? 1.f : 0.f, flDetectSpeed);
            // Request a burst once the action fires, if this feature is enabled in the effect mask.
            const bool bMasked = (Vars::Aletherium::Indicators::KeybindDetectEffectMask.Value & (1 << src.m_iId)) != 0;
            const bool bFired = (src.m_iId == 0) ? bEdgeBugFired             // eb: actual completion
                                                 : (src.m_bDetect && !it->m_bWasDetected); // others: rising edge
            if (bEffectsOn && bMasked && bFired)
                it->m_bSpawnBurst = true; // consumed once the label is positioned
            it->m_bWasDetected = src.m_bDetect;
        }
        else
        {
            MovementIndicator_t ind;
            ind.m_iId = src.m_iId;
            ind.m_sLabel = src.m_sLabel;
            ind.m_bAccent = src.m_bAccent;
            ind.m_bActive = true;
            ind.m_bRemoving = false;
            ind.m_flEnter = 0.f; // entry animation starts here
            ind.m_flAlpha = 0.f;
            ind.m_flDetect = src.m_bDetect ? 1.f : 0.f;
            ind.m_flPosX = -1.f; // -1 = snap to computed position on first frame
            ind.m_flPosY = -1.f;
            ind.m_bWasDetected = src.m_bDetect; // seed edge state so appearing-while-detected doesn't burst
            m_vKeybindIndicators.push_back(ind);
        }
    }

    // - advance raw entry/exit progress over the configurable fade duration, then derive the
    // eased alpha. Drop indicators that have fully retracted.
    const float flDt = std::min(I::GlobalVars->frametime, 0.1f);
    const int iEasing = Vars::Aletherium::Indicators::KeybindEasing.Value;
    const float flDuration = std::max(0.05f, Vars::Aletherium::Indicators::KeybindFadeDuration.Value);
    for (auto it = m_vKeybindIndicators.begin(); it != m_vKeybindIndicators.end();)
    {
        it->m_flEnter = std::clamp(it->m_flEnter + (it->m_bRemoving ? -1.f : 1.f) * flDt / flDuration, 0.f, 1.f);
        it->m_flAlpha = KeybindEase::Apply(iEasing, it->m_flEnter);

        if (it->m_bRemoving && it->m_flEnter <= 0.f)
            it = m_vKeybindIndicators.erase(it);
        else
            ++it;
    }

    // Keep going while effects are still alive even if every label has retracted, so a burst finishes.
    if (m_vKeybindIndicators.empty())
    {
        UpdateDrawEffects();
        return;
    }

    const int iScreenW = H::Draw.m_nScreenW;
    const int iScreenH = H::Draw.m_nScreenH;
    // FONT_KEYBIND already resolves to the custom family (if set) or the default /
    const EFonts eFontK = FONT_KEYBIND;
    const auto& tFont = H::Fonts.GetFont(eFontK);

    // Anchor from the X/Y position sliders (screen fractions, like the velocity indicator).
    const float flAnchorX = iScreenW * Vars::Aletherium::Indicators::KeybindPosX.Value;
    const float flAnchorY = iScreenH * Vars::Aletherium::Indicators::KeybindPosY.Value;

    const int iLayout = Vars::Aletherium::Indicators::KeybindLayout.Value;
    const bool bVertical = iLayout == Vars::Aletherium::Indicators::KeybindLayoutEnum::Vertical;
    const int iRowH = tFont.m_nTall + H::Draw.Scale(4);

    // Static = labels just appear in place: no motion of any kind. Each alive entry reserves its full
    // slot (weight 1) so neighbours never slide while one fades in/out - the only thing that animates
    const bool bStatic = Vars::Aletherium::Indicators::KeybindEntryAnim.Value == Vars::Aletherium::Indicators::KeybindEntryAnimEnum::Static;

    // Measure labels; alive entries contribute their (alpha-weighted) extent so the list reflows.
    // Overshoot easing exceeds 1 mid-curve, so clamp where the progress acts as a visibility weight.
    std::vector<float> vWidths;
    vWidths.reserve(m_vKeybindIndicators.size());
    float flMaxW = 0.f, flTotalW = 0.f, flTotalH = 0.f;
    for (auto& ind : m_vKeybindIndicators)
    {
        const float flW = H::Draw.GetTextSize(ind.m_sLabel.c_str(), tFont).x;
        const float flVis = bStatic ? 1.f : std::clamp(ind.m_flAlpha, 0.f, 1.f);
        vWidths.push_back(flW);
        flMaxW = std::max(flMaxW, flW);
        flTotalW += (flW + flSpacing) * flVis;
        flTotalH += iRowH * flVis;
    }
    if (flTotalW > 0.f) flTotalW -= flSpacing;

    // Lay out target positions: horizontal row centred on the anchor, or a vertical column.
    if (!bVertical)
    {
        float flCurrentX = flAnchorX - flTotalW / 2.f;
        for (size_t i = 0; i < m_vKeybindIndicators.size(); i++)
        {
            auto& ind = m_vKeybindIndicators[i];
            ind.m_flTargetX = flCurrentX;
            ind.m_flTargetY = flAnchorY;
            flCurrentX += (vWidths[i] + flSpacing) * (bStatic ? 1.f : std::clamp(ind.m_flAlpha, 0.f, 1.f));
        }
    }
    else
    {
        // Grow from the anchor in the chosen direction (no vertical centering,
        // desktop list): Down puts the anchor at the top edge, Up stacks the list above it.
        const bool bUp = Vars::Aletherium::Indicators::KeybindVerticalDir.Value == Vars::Aletherium::Indicators::KeybindVerticalDirEnum::Up;
        float flCurrentY = bUp ? flAnchorY - iRowH : flAnchorY;
        for (size_t i = 0; i < m_vKeybindIndicators.size(); i++)
        {
            auto& ind = m_vKeybindIndicators[i];
            ind.m_flTargetX = flAnchorX - vWidths[i] * 0.5f; // column centred on the anchor
            ind.m_flTargetY = flCurrentY;
            flCurrentY += (bUp ? -1.f : 1.f) * iRowH * (bStatic ? 1.f : std::clamp(ind.m_flAlpha, 0.f, 1.f));
        }
    }

    // Entry/exit slide vector (applied as (1 - eased) * delta off the target).
    const int iAnim = Vars::Aletherium::Indicators::KeybindEntryAnim.Value;
    const float flOffset = Vars::Aletherium::Indicators::KeybindEntryOffset.Value;

    // Shadow / colour settings.
    const bool bShadow = Vars::Aletherium::Indicators::KeybindShadow.Value;
    const bool bShadowBlur = Vars::Aletherium::Indicators::KeybindShadowBlur.Value;
    const int iShadowOx = static_cast<int>(std::round(Vars::Aletherium::Indicators::KeybindShadowOffX.Value));
    const int iShadowOy = static_cast<int>(std::round(Vars::Aletherium::Indicators::KeybindShadowOffY.Value));
    const Color_t cShadowBase = Vars::Aletherium::Indicators::KeybindShadowColor.Value;
    const Color_t cDetect = Vars::Aletherium::Indicators::KeybindDetectColor.Value;
    const Color_t cText = Vars::Aletherium::Indicators::KeybindTextColor.Value;
    const bool bDetectText = Vars::Aletherium::Indicators::KeybindDetectText.Value;
    const Color_t cDetectText = Vars::Aletherium::Indicators::KeybindDetectTextColor.Value;
    const Color_t cAccent = Vars::Menu::Theme::Accent.Value;
    const bool bGlow = Vars::Aletherium::Indicators::KeybindDetectGlow.Value;
    const Color_t cGlow = Vars::Aletherium::Indicators::KeybindDetectGlowColor.Value;

    for (size_t i = 0; i < m_vKeybindIndicators.size(); i++)
    {
        auto& ind = m_vKeybindIndicators[i];
        const float eased = ind.m_flAlpha;
        // Visibility weight for opacities/layout: overshoot easing exceeds 1 mid-curve.
        const float flVis = std::clamp(eased, 0.f, 1.f);

        // Entry slide delta off the target (horizontal for row layouts, along the stacking axis for
        // center entry in vertical layouts).
        float dx = 0.f, dy = 0.f;
        switch (iAnim)
        {
        case Vars::Aletherium::Indicators::KeybindEntryAnimEnum::SlideRight: dx = flOffset; break;
        case Vars::Aletherium::Indicators::KeybindEntryAnimEnum::SlideLeft:  dx = -flOffset; break;
        case Vars::Aletherium::Indicators::KeybindEntryAnimEnum::SlideCenter:
            if (bVertical) dy = flAnchorY - ind.m_flTargetY;
            else           dx = flAnchorX - ind.m_flTargetX;
            break;
        default: break; // Static: pure fade
        }

        // Smooth only the reflow (resting target) motion. The entry slide is then added on top
        if (bStatic)
        {
            ind.m_flPosX = ind.m_flTargetX;
            ind.m_flPosY = ind.m_flTargetY;
        }
        else
        {
            ind.m_flPosX = ind.m_flPosX < 0.f ? ind.m_flTargetX : LerpSmooth(ind.m_flPosX, ind.m_flTargetX, flPosSpeed);
            ind.m_flPosY = ind.m_flPosY < 0.f ? ind.m_flTargetY : LerpSmooth(ind.m_flPosY, ind.m_flTargetY, flPosSpeed);
        }

        if (flVis < 0.01f)
            continue;

        const float d = ind.m_flDetect;
        const int iPosX = static_cast<int>(std::round(ind.m_flPosX + (1.f - eased) * dx));
        const int iPosY = static_cast<int>(std::round(ind.m_flPosY + (1.f - eased) * dy));

        // Detection burst: fire effects from the centre of this label now that we know where it sits.
        if (ind.m_bSpawnBurst)
        {
            ind.m_bSpawnBurst = false;
            const float flCx = iPosX + vWidths[i] * 0.5f;
            const float flCy = iPosY + tFont.m_nTall * 0.5f;
            if (Vars::Aletherium::Indicators::KeybindDetectParticles.Value)
                SpawnParticleBurst(flCx, flCy, Vars::Aletherium::Indicators::KeybindDetectParticleColor.Value);
            if (Vars::Aletherium::Indicators::KeybindDetectRipple.Value)
                SpawnRippleBurst(flCx, flCy, Vars::Aletherium::Indicators::KeybindDetectRippleColor.Value);
        }

        // Text colour: custom edge-bug "name" accent, else the configurable label colour.
        Color_t clrText = ind.m_bAccent ? cAccent : cText;
        // Foreground detection tint: fade the label toward the detect colour while detected.
        if (bDetectText && d > 0.f)
            clrText = clrText.Lerp(cDetectText, d);
        clrText.a = static_cast<byte>(clrText.a * flVis);

        // Detection glow: a solid outline of the label.
        if (bGlow && d > 0.01f)
        {
            const Color_t cg = cGlow;
            const float flBaseA = (cg.a / 255.f) * d * flVis;        // solid-core (peak) halo strength
            const float flBlur = H::Draw.Scale(Vars::Aletherium::Indicators::KeybindDetectGlowBlur.Value, Scale_Round);
            const float flBold = H::Draw.Scale(static_cast<float>(Vars::Aletherium::Indicators::KeybindDetectGlowStencil.Value), Scale_Round);
            const float sigma = flBlur * 0.5f + 0.5f;
            const float reach = flBold + sigma * 3.f;                // run the gaussian fully out (~3s) so
                                                                     // the halo fades smoothly, no hard ring
            if (flBaseA > 0.003f && reach >= 0.5f)
            {
                // Bounded disc sampling: ~1px taps for small glows, coarser for big soft ones (where the
                // coarseness is invisible anyway). Capped at 17x17 taps so cost stays fixed per indicator.
                const int iHalf = std::clamp(static_cast<int>(std::ceil(reach)), 2, 8);
                const float step = reach / iHalf;
                const float r2max = reach * reach;                   // smooth disc -> round halo, no fudge
                // H::Draw.String alpha-composites (over), not additive: N overlapping taps at alpha a give
                // 1-(1-a)^N. So pick the per-tap core alpha that composites back to flBaseA over the ~N
                const float normR = flBold + sigma;
                const float flN = std::max(3.14159265f * normR * normR / (step * step), 1.f);
                const float aPeak = 1.f - powf(1.f - std::clamp(flBaseA, 0.f, 0.999f), 1.f / flN);
                for (int gy = -iHalf; gy <= iHalf; gy++)
                {
                    for (int gx = -iHalf; gx <= iHalf; gx++)
                    {
                        if (gx == 0 && gy == 0) continue;            // centre is the label itself
                        const float ox = gx * step, oy = gy * step;
                        const float dist2 = ox * ox + oy * oy;
                        if (dist2 > r2max) continue;
                        const float edge = std::max(0.f, sqrtf(dist2) - flBold); // 0 inside the solid core
                        const float w = expf(-0.5f * edge * edge / (sigma * sigma));
                        const float a = aPeak * w;
                        if (a < 0.004f) continue;
                        Color_t cc = cg.Alpha(static_cast<byte>(std::clamp(a, 0.f, 1.f) * 255.f));
                        H::Draw.String(tFont, iPosX + static_cast<int>(std::round(ox)), iPosY + static_cast<int>(std::round(oy)), cc, ALIGN_TOPLEFT, ind.m_sLabel.c_str());
                    }
                }
            }
        }

        if (bShadow)
        {
            // Detection colour drives the shadow: base shadow lerps toward the detect colour.
            Color_t clrShadow = cShadowBase.Lerp(cDetect, d);
            if (bShadowBlur)
            {
                // Soft shadow: spread the label across a 3x3 disc at reduced alpha (same as velocity).
                clrShadow.a = static_cast<byte>(clrShadow.a * flVis * 0.35f);
                for (int dy = -1; dy <= 1; dy++)
                    for (int dx = -1; dx <= 1; dx++)
                        H::Draw.String(tFont, iPosX + iShadowOx + dx, iPosY + iShadowOy + dy, clrShadow, ALIGN_TOPLEFT, ind.m_sLabel.c_str());
            }
            else
            {
                clrShadow.a = static_cast<byte>(clrShadow.a * flVis);
                H::Draw.String(tFont, iPosX + iShadowOx, iPosY + iShadowOy, clrShadow, ALIGN_TOPLEFT, ind.m_sLabel.c_str());
            }
        }

        H::Draw.String(tFont, iPosX, iPosY, clrText, ALIGN_TOPLEFT, ind.m_sLabel.c_str());
    }

    // Advance + draw any live detection effects after the labels so they float on top.
    UpdateDrawEffects();
}

// Launch a burst of shapes from (flCenterX, flCenterY) in random directions. Each gets a randomised
// speed, lifetime, scale and spin around the slider values so the burst looks organic, not uniform.
void CIndicators::SpawnParticleBurst(float flCenterX, float flCenterY, Color_t cColor)
{
    const int iCount = Vars::Aletherium::Indicators::KeybindDetectParticleCount.Value;
    const float flSpeed = Vars::Aletherium::Indicators::KeybindDetectParticleSpeed.Value;
    const float flLife = Vars::Aletherium::Indicators::KeybindDetectParticleLifetime.Value;
    const float flScale = H::Draw.Scale(Vars::Aletherium::Indicators::KeybindDetectParticleScale.Value);
    const float flSpread = H::Draw.Scale(Vars::Aletherium::Indicators::KeybindDetectParticleSpread.Value);
    const int iShapeMode = Vars::Aletherium::Indicators::KeybindDetectParticleShape.Value;

    // Cap the live pool so repeated detections can't pile up unbounded.
    constexpr size_t kMaxParticles = 600;
    if (m_vParticles.size() > kMaxParticles - iCount)
        m_vParticles.erase(m_vParticles.begin(), m_vParticles.begin() + (m_vParticles.size() - (kMaxParticles - iCount)));

    for (int i = 0; i < iCount; i++)
    {
        IndicatorParticle_t p;
        const float flAng = ParticleRand(0.f, 2.f * static_cast<float>(PI));
        const float flMag = flSpeed * ParticleRand(0.45f, 1.f); // spread of velocities for a fuller burst
        // Initial scatter: start somewhere within a disc of radius "spread" around the label centre.
        const float flOffAng = ParticleRand(0.f, 2.f * static_cast<float>(PI));
        const float flOffR = flSpread * std::sqrt(ParticleRand(0.f, 1.f)); // sqrt -> uniform over the disc
        p.m_flX = flCenterX + std::cos(flOffAng) * flOffR;
        p.m_flY = flCenterY + std::sin(flOffAng) * flOffR;
        p.m_flVelX = std::cos(flAng) * flMag;
        p.m_flVelY = std::sin(flAng) * flMag;
        p.m_flMaxLife = p.m_flLife = flLife * ParticleRand(0.7f, 1.f);
        p.m_flScale = std::max(1.f, flScale * ParticleRand(0.7f, 1.3f));
        p.m_flRot = ParticleRand(0.f, 2.f * static_cast<float>(PI));
        p.m_flRotSpeed = ParticleRand(-5.f, 5.f);
        p.m_iShape = (iShapeMode == Vars::Aletherium::Indicators::KeybindDetectParticleShapeEnum::Random)
            ? SDK::StdRandomInt(Vars::Aletherium::Indicators::KeybindDetectParticleShapeEnum::Square,
                                Vars::Aletherium::Indicators::KeybindDetectParticleShapeEnum::Star)
            : iShapeMode;
        p.m_cColor = cColor;
        m_vParticles.push_back(p);
    }
}

// Integrate every live particle (linear drift + a touch of gravity), fade it over its lifetime, draw it,
// and retire the dead ones.
void CIndicators::UpdateDrawParticles()
{
    if (m_vParticles.empty())
        return;

    const float flDt = std::min(I::GlobalVars->frametime, 0.1f);
    const float flGravity = Vars::Aletherium::Indicators::KeybindDetectParticleGravity.Value; // px/s^2 (0 = floaty, <0 = rise)

    for (auto it = m_vParticles.begin(); it != m_vParticles.end();)
    {
        it->m_flLife -= flDt;
        if (it->m_flLife <= 0.f)
        {
            it = m_vParticles.erase(it);
            continue;
        }

        it->m_flVelY += flGravity * flDt;
        it->m_flX += it->m_flVelX * flDt;
        it->m_flY += it->m_flVelY * flDt;
        it->m_flRot += it->m_flRotSpeed * flDt;

        const float flT = std::clamp(it->m_flLife / std::max(it->m_flMaxLife, 0.001f), 0.f, 1.f); // 1 -> 0
        Color_t c = it->m_cColor;
        c.a = static_cast<byte>(c.a * flT);
        DrawParticleShape(it->m_iShape, it->m_flX, it->m_flY, it->m_flScale, it->m_flRot, c);
        ++it;
    }
}

// Draw one filled shape centred on (flX, flY). Polygons are built as fans the surface can triangulate
// (convex shapes from the perimeter; the star from its centre so the concave points still fill).
void CIndicators::DrawParticleShape(int iShape, float flX, float flY, float flScale, float flRot, Color_t cColor)
{
    const float rBase = std::max(flScale, 1.f);

    // Fill the shape at an arbitrary radius/colour (one draw call). Captured by the AA pass loop below.
    auto fnFill = [&](float r, Color_t c)
    {
        const int ix = static_cast<int>(std::round(flX));
        const int iy = static_cast<int>(std::round(flY));
        switch (iShape)
        {
        case Vars::Aletherium::Indicators::KeybindDetectParticleShapeEnum::Circle:
            H::Draw.FillCircle(ix, iy, r, 22, c);
            break;
        case Vars::Aletherium::Indicators::KeybindDetectParticleShapeEnum::Triangle:
        {
            std::vector<Vertex_t> v;
            for (int k = 0; k < 3; k++)
            {
                const float a = flRot - static_cast<float>(PI) * 0.5f + k * (2.f * static_cast<float>(PI) / 3.f);
                v.emplace_back(Vector2D{ flX + std::cos(a) * r, flY + std::sin(a) * r });
            }
            H::Draw.FillPolygon(v, c);
            break;
        }
        case Vars::Aletherium::Indicators::KeybindDetectParticleShapeEnum::Diamond:
        {
            std::vector<Vertex_t> v;
            for (int k = 0; k < 4; k++)
            {
                const float a = flRot + k * (static_cast<float>(PI) * 0.5f); // points up/right/down/left
                v.emplace_back(Vector2D{ flX + std::cos(a) * r, flY + std::sin(a) * r });
            }
            H::Draw.FillPolygon(v, c);
            break;
        }
        case Vars::Aletherium::Indicators::KeybindDetectParticleShapeEnum::Star:
        {
            std::vector<Vertex_t> v;
            v.emplace_back(Vector2D{ flX, flY }); // centre anchor -> fan covers the concave points
            for (int k = 0; k <= 10; k++)
            {
                const float a = flRot - static_cast<float>(PI) * 0.5f + k * (static_cast<float>(PI) / 5.f);
                const float rr = (k % 2 == 0) ? r : r * 0.45f;
                v.emplace_back(Vector2D{ flX + std::cos(a) * rr, flY + std::sin(a) * rr });
            }
            H::Draw.FillPolygon(v, c);
            break;
        }
        default: // Square (rotated)
        {
            std::vector<Vertex_t> v;
            for (int k = 0; k < 4; k++)
            {
                const float a = flRot + static_cast<float>(PI) * 0.25f + k * (static_cast<float>(PI) * 0.5f);
                v.emplace_back(Vector2D{ flX + std::cos(a) * r, flY + std::sin(a) * r });
            }
            H::Draw.FillPolygon(v, c);
            break;
        }
        }
    };

    if (Vars::Aletherium::Indicators::KeybindDetectParticleAntiAlias.Value)
    {
        // Faint expanded copies first (drawn behind), then the solid shape on top.
        const float aaDr[2] = { 1.5f, 0.75f };
        const float aaAm[2] = { 0.12f, 0.34f };
        for (int p = 0; p < 2; p++)
        {
            Color_t c = cColor;
            c.a = static_cast<byte>(c.a * aaAm[p]);
            if (c.a > 0)
                fnFill(rBase + aaDr[p], c);
        }
    }
    fnFill(rBase, cColor);
}

// Launch a small cluster of expanding rings from (flCenterX, flCenterY). Staggered starts make them read
// as a few successive pulses ("a couple times") rather than one ring.
void CIndicators::SpawnRippleBurst(float flCenterX, float flCenterY, Color_t cColor)
{
    const int iCount = Vars::Aletherium::Indicators::KeybindDetectRippleCount.Value;
    const float flDuration = Vars::Aletherium::Indicators::KeybindDetectRippleDuration.Value;
    const float flRadius = H::Draw.Scale(Vars::Aletherium::Indicators::KeybindDetectRippleRadius.Value);
    const float flThick = H::Draw.Scale(Vars::Aletherium::Indicators::KeybindDetectRippleThickness.Value);
    const float flStagger = flDuration * 0.33f; // overlap successive rings so it pulses a few times

    constexpr size_t kMaxRipples = 96;
    if (m_vRipples.size() > kMaxRipples - iCount)
        m_vRipples.erase(m_vRipples.begin(), m_vRipples.begin() + (m_vRipples.size() - (kMaxRipples - iCount)));

    for (int i = 0; i < iCount; i++)
    {
        IndicatorRipple_t rp;
        rp.m_flX = flCenterX;
        rp.m_flY = flCenterY;
        rp.m_flAge = -i * flStagger; // negative = wait this long before this ring starts expanding
        rp.m_flDuration = std::max(0.05f, flDuration);
        rp.m_flMaxRadius = std::max(2.f, flRadius);
        rp.m_flThickness = std::max(1.f, flThick);
        rp.m_cColor = cColor;
        m_vRipples.push_back(rp);
    }
}

// Grow each ring from 0 to its max radius while fading out, then retire it.
void CIndicators::UpdateDrawRipples()
{
    if (m_vRipples.empty())
        return;

    const float flDt = std::min(I::GlobalVars->frametime, 0.1f);
    for (auto it = m_vRipples.begin(); it != m_vRipples.end();)
    {
        it->m_flAge += flDt;
        if (it->m_flAge >= it->m_flDuration)
        {
            it = m_vRipples.erase(it);
            continue;
        }
        if (it->m_flAge < 0.f) { ++it; continue; } // still staggered out, not visible yet

        const float t = std::clamp(it->m_flAge / it->m_flDuration, 0.f, 1.f);
        const float r = std::max(1.f, t * it->m_flMaxRadius);
        Color_t c = it->m_cColor;
        c.a = static_cast<byte>(c.a * (1.f - t)); // fade as it expands

        const int ix = static_cast<int>(std::round(it->m_flX));
        const int iy = static_cast<int>(std::round(it->m_flY));
        const int iThick = std::max(1, static_cast<int>(std::round(it->m_flThickness)));
        // Surface circles are 1px; stack outlined circles to fake thickness (also softens the edge a touch).
        for (int k = 0; k < iThick; k++)
            H::Draw.LineCircle(ix, iy, r + static_cast<float>(k), 32, c);
        ++it;
    }
}

void CIndicators::UpdateDrawEffects()
{
    UpdateDrawParticles();
    UpdateDrawRipples();
}
