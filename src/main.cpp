#include "main.hpp"
#include "VivifyRuntime.hpp"
#include <algorithm>
#include <string_view>
#include "HMUI/ViewController.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Transform.hpp"
#include "bsml/shared/BSML-Lite/Creation/Layout.hpp"
#include "bsml/shared/BSML-Lite/Creation/Settings.hpp"
#include "bsml/shared/BSML/Settings/BSMLSettings.hpp"
#include "custom-types/shared/register.hpp"
#include "scotland2/shared/modloader.h"

static modloader::ModInfo modInfo{MOD_ID, VERSION, 0};

namespace {
constexpr std::string_view kMultipassRenderingConfigKey = "multipassRendering";
constexpr std::string_view kVivifyDebugLoggingConfigKey = "vivifyDebugLogging";
constexpr std::string_view kDisableBeat0FilmgrainBlitConfigKey = "disableBeat0FilmgrainBlit";
constexpr std::string_view kDisableAllBlitsConfigKey = "disableAllBlits";
constexpr std::string_view kDisableCreateCameraDepthConfigKey = "disableCreateCameraDepth";
constexpr std::string_view kDisableVRCenterAdjustConfigKey = "disableVRCenterAdjust";
constexpr std::string_view kDisableCustomNoteVisualsConfigKey = "disableCustomNoteVisuals";
constexpr std::string_view kDisableVisualsInMultiplayerConfigKey = "disableVisualsInMultiplayer";
constexpr std::string_view kSafeModeLimitsConfigKey = "safeModeLimits";
constexpr std::string_view kMaxActivePrefabsConfigKey = "maxActivePrefabs";
constexpr std::string_view kMaxNoteVisualFragmentsConfigKey = "maxNoteVisualFragments";
constexpr std::string_view kMaxTotalPixelsConfigKey = "maxTotalRenderTexturePixels";
constexpr std::string_view kAllowWindowsBundleFallbackConfigKey = "allowWindowsBundleFallback";
constexpr std::string_view kHideVanillaSabersConfigKey = "hideVanillaSabersOnCustomSaberMaps";
constexpr std::string_view kAssetCullingConfigKey = "assetCulling";
constexpr std::string_view kAssetCullingScaleConfigKey = "assetCullingScale";
constexpr std::string_view kFakeAudioLinkConfigKey = "fakeAudioLink";
constexpr std::string_view kPerformanceModeConfigKey = "performanceMode";
constexpr std::string_view kMirror30FPSConfigKey = "mirror30FPS";
constexpr std::string_view kDynamicEffectResolutionConfigKey = "dynamicEffectResolution";
bool gMultipassRenderingEnabled = true;
bool gVivifyDebugLogging = false;
bool gDisableBeat0FilmgrainBlit = false;
bool gDisableAllBlits = false;
bool gDisableCreateCameraDepth = false;
bool gDisableVRCenterAdjust = false;
bool gDisableCustomNoteVisuals = false;
bool gDisableVisualsInMultiplayer = true;
// Resource safety limits: heavy Vivify maps (mirrors + several cameras + dozens
// of prefabs) can push the Quest into a GPU driver reset / OOM kill. These caps
// make the mod fail soft — it degrades visuals instead of crashing the game.
bool gSafeModeLimits = true;
bool gAllowWindowsBundleFallback = false;
// Hide vanilla red/blue sabers when a Vivify map supplies its own saber model
// (previously hardcoded off because it broke CustomSabers — now config-gated
// and only applied when the map actually assigns a saber asset).
bool gHideVanillaSabers = true;
// Asset culling: shrink Vivify objects that are behind the player so the GPU
// spends fewer vertices/pixels on unseen geometry, restoring full detail when
// the player turns to face them. Auto-enabled; can be turned off in settings.
bool gAssetCulling = true;
// AudioLink is a SongCore capability, not a required standalone mod on Quest.
// Keep the compatibility shim enabled by default so AudioLink maps remain playable.
bool gFakeAudioLink = true;
bool gPerformanceMode = true;
bool gMirror30FPS = true;
bool gDynamicEffectResolution = true;
float gAssetCullingScale = 0.55f;
int gMaxActivePrefabs = 96;
int gMaxNoteVisualFragments = 12;
// 24M pixels keeps normal Quest mirror + screen-effect chains alive while the
// runtime hard cap remains 32M. The old 8M default rejected valid full-screen
// effects on a single 1680x1760 eye buffer plus one mirror.
int gMaxTotalPixels = 24 * 1024 * 1024;

void EnsureConfigObject() {
  auto& doc = getConfig().config;
  if (!doc.IsObject()) {
    doc.SetObject();
  }
}

bool EnsureBoolConfigValue(std::string_view key, bool defaultValue, bool& value) {
  auto& doc = getConfig().config;
  auto it = doc.FindMember(key.data());
  if (it != doc.MemberEnd() && it->value.IsBool()) {
    value = it->value.GetBool();
    return false;
  }

  auto& allocator = doc.GetAllocator();
  value = defaultValue;
  if (it == doc.MemberEnd()) {
    doc.AddMember(rapidjson::Value(key.data(), allocator), rapidjson::Value(defaultValue), allocator);
  } else {
    it->value.SetBool(defaultValue);
  }
  return true;
}

void SetBoolConfigValue(std::string_view key, bool enabled, bool& value) {
  auto& config = getConfig();
  auto& doc = config.config;
  EnsureConfigObject();
  auto& allocator = doc.GetAllocator();
  auto it = doc.FindMember(key.data());
  bool needsWrite = true;
  if (it == doc.MemberEnd()) {
    doc.AddMember(rapidjson::Value(key.data(), allocator), rapidjson::Value(enabled), allocator);
  } else if (it->value.IsBool() && it->value.GetBool() == enabled) {
    needsWrite = false;
  } else {
    it->value.SetBool(enabled);
  }
  value = enabled;
  if (needsWrite) {
    config.Write();
  }
}

void SetVivifyDebugLogging(bool enabled) {
  SetBoolConfigValue(kVivifyDebugLoggingConfigKey, enabled, gVivifyDebugLogging);
}

void SetDisableBeat0FilmgrainBlit(bool enabled) {
  SetBoolConfigValue(kDisableBeat0FilmgrainBlitConfigKey, enabled, gDisableBeat0FilmgrainBlit);
  Vivify::RefreshIsolationSettings();
}

void SetDisableAllBlits(bool enabled) {
  SetBoolConfigValue(kDisableAllBlitsConfigKey, enabled, gDisableAllBlits);
  Vivify::RefreshIsolationSettings();
}

void SetDisableCreateCameraDepth(bool enabled) {
  SetBoolConfigValue(kDisableCreateCameraDepthConfigKey, enabled, gDisableCreateCameraDepth);
  Vivify::RefreshIsolationSettings();
}

void SetDisableVRCenterAdjust(bool enabled) {
  SetBoolConfigValue(kDisableVRCenterAdjustConfigKey, enabled, gDisableVRCenterAdjust);
  Vivify::RefreshIsolationSettings();
}

void RegisterModSettings() {
  BSML::BSMLSettings::get_instance()->TryAddSettingsMenu(
      [](HMUI::ViewController* viewController, bool firstActivation, bool, bool) {
        if (!firstActivation || viewController == nullptr) return;
        auto* container = BSML::Lite::CreateScrollableSettingsContainer(viewController->get_transform());
        if (container == nullptr) return;
        BSML::Lite::CreateToggle(container->get_transform(), u"Multipass Rendering", GetMultipassRenderingEnabled(),
                                 [](bool value) {
                                   SetMultipassRenderingEnabled(value);
                                   Vivify::RefreshMultipassRendering();
                                 });
        BSML::Lite::CreateToggle(container->get_transform(), u"Vivify Debug Logging", GetVivifyDebugLogging(),
                                 [](bool value) { SetVivifyDebugLogging(value); });
        BSML::Lite::CreateToggle(container->get_transform(), u"Disable Beat 0 Filmgrain Blit",
                                 GetDisableBeat0FilmgrainBlit(),
                                 [](bool value) { SetDisableBeat0FilmgrainBlit(value); });
        BSML::Lite::CreateToggle(container->get_transform(), u"Disable All Blits", GetDisableAllBlits(),
                                 [](bool value) { SetDisableAllBlits(value); });
        BSML::Lite::CreateToggle(container->get_transform(), u"Disable Custom Vivify Note Visuals",
                                 GetDisableCustomNoteVisuals(),
                                 [](bool value) { SetDisableCustomNoteVisuals(value); });
        BSML::Lite::CreateToggle(container->get_transform(), u"Safe Mode Limits (prevent crashes on heavy maps)",
                                 GetSafeModeLimits(),
                                 [](bool value) { SetSafeModeLimits(value); });
        BSML::Lite::CreateToggle(container->get_transform(), u"Hide Vanilla Sabers on Custom-Saber Maps",
                                 GetHideVanillaSabers(),
                                 [](bool value) { SetHideVanillaSabers(value); });
        BSML::Lite::CreateToggle(container->get_transform(), u"Asset Culling (perf: shrink objects behind you)",
                                 GetAssetCullingEnabled(),
                                 [](bool value) { SetAssetCullingEnabled(value); });
        BSML::Lite::CreateToggle(container->get_transform(), u"Fake AudioLink (allow AudioLink songs)",
                                 GetFakeAudioLinkEnabled(),
                                 [](bool value) { SetFakeAudioLinkEnabled(value); });
        BSML::Lite::CreateToggle(container->get_transform(), u"Performance Mode (recommended)",
                                 GetPerformanceModeEnabled(),
                                 [](bool value) { SetPerformanceModeEnabled(value); });
        BSML::Lite::CreateToggle(container->get_transform(), u"Mirrors at 30 FPS",
                                 GetMirror30FPSEnabled(),
                                 [](bool value) { SetMirror30FPSEnabled(value); });
        BSML::Lite::CreateToggle(container->get_transform(), u"Dynamic Effect Resolution",
                                 GetDynamicEffectResolutionEnabled(),
                                 [](bool value) { SetDynamicEffectResolutionEnabled(value); });
        BSML::Lite::CreateToggle(container->get_transform(), u"Allow Windows bundles (risky fallback)",
                                 GetAllowWindowsBundleFallback(),
                                 [](bool value) { SetAllowWindowsBundleFallback(value); });
        BSML::Lite::CreateToggle(container->get_transform(), u"Disable Vivify Visuals In Multiplayer",
                                 GetDisableVisualsInMultiplayer(),
                                 [](bool value) { SetDisableVisualsInMultiplayer(value); });
        BSML::Lite::CreateToggle(container->get_transform(), u"Disable CreateCamera/Depth",
                                 GetDisableCreateCameraDepth(),
                                 [](bool value) { SetDisableCreateCameraDepth(value); });
        BSML::Lite::CreateToggle(container->get_transform(), u"Disable VR Center Adjust",
                                 GetDisableVRCenterAdjust(),
                                 [](bool value) { SetDisableVRCenterAdjust(value); });
      },
      "Vivify", false);
}
}  // namespace

Configuration &getConfig() {
  static Configuration config(modInfo);
  return config;
}

bool GetMultipassRenderingEnabled() {
  return gMultipassRenderingEnabled;
}

bool GetVivifyDebugLogging() {
  return gVivifyDebugLogging;
}

bool GetDisableBeat0FilmgrainBlit() {
  return gDisableBeat0FilmgrainBlit;
}

bool GetDisableAllBlits() {
  return gDisableAllBlits;
}

bool GetDisableCreateCameraDepth() {
  return gDisableCreateCameraDepth;
}

bool GetDisableVRCenterAdjust() {
  return gDisableVRCenterAdjust;
}

bool GetDisableVisualsInMultiplayer() {
  return gDisableVisualsInMultiplayer;
}

void SetDisableVisualsInMultiplayer(bool enabled) {
  SetBoolConfigValue(kDisableVisualsInMultiplayerConfigKey, enabled, gDisableVisualsInMultiplayer);
}

bool GetDisableCustomNoteVisuals() {
  return gDisableCustomNoteVisuals;
}

bool GetSafeModeLimits() {
  return gSafeModeLimits;
}

bool GetHideVanillaSabers() {
  return gHideVanillaSabers;
}

bool GetAssetCullingEnabled() {
  return gAssetCulling;
}

bool GetFakeAudioLinkEnabled() {
  return gFakeAudioLink;
}

bool GetPerformanceModeEnabled() {
  return gPerformanceMode;
}

bool GetMirror30FPSEnabled() {
  return gMirror30FPS;
}

bool GetDynamicEffectResolutionEnabled() {
  return gDynamicEffectResolution;
}

float GetAssetCullingScale() {
  return gAssetCullingScale;
}

bool GetAllowWindowsBundleFallback() {
  return gAllowWindowsBundleFallback;
}

int GetMaxActivePrefabs() {
  return gMaxActivePrefabs;
}

int GetMaxNoteVisualFragments() {
  return gMaxNoteVisualFragments;
}

int GetMaxTotalPixels() {
  return gMaxTotalPixels;
}

void SetMultipassRenderingEnabled(bool enabled) {
  SetBoolConfigValue(kMultipassRenderingConfigKey, enabled, gMultipassRenderingEnabled);
}

void SetSafeModeLimits(bool enabled) {
  SetBoolConfigValue(kSafeModeLimitsConfigKey, enabled, gSafeModeLimits);
}

void SetAllowWindowsBundleFallback(bool enabled) {
  SetBoolConfigValue(kAllowWindowsBundleFallbackConfigKey, enabled, gAllowWindowsBundleFallback);
}

void SetDisableCustomNoteVisuals(bool enabled) {
  SetBoolConfigValue(kDisableCustomNoteVisualsConfigKey, enabled, gDisableCustomNoteVisuals);
}

void SetHideVanillaSabers(bool enabled) {
  SetBoolConfigValue(kHideVanillaSabersConfigKey, enabled, gHideVanillaSabers);
  Vivify::RefreshIsolationSettings();
}

void SetAssetCullingEnabled(bool enabled) {
  SetBoolConfigValue(kAssetCullingConfigKey, enabled, gAssetCulling);
  Vivify::RefreshIsolationSettings();
}

void SetPerformanceModeEnabled(bool enabled) {
  SetBoolConfigValue(kPerformanceModeConfigKey, enabled, gPerformanceMode);
  Vivify::RefreshIsolationSettings();
}

void SetMirror30FPSEnabled(bool enabled) {
  SetBoolConfigValue(kMirror30FPSConfigKey, enabled, gMirror30FPS);
  Vivify::RefreshIsolationSettings();
}

void SetDynamicEffectResolutionEnabled(bool enabled) {
  SetBoolConfigValue(kDynamicEffectResolutionConfigKey, enabled, gDynamicEffectResolution);
  Vivify::RefreshIsolationSettings();
}

void SetFakeAudioLinkEnabled(bool enabled) {
  // SongCore capabilities are registered once during late-load. The option is
  // persisted for compatibility/debugging; disabling it takes effect on the
  // next game launch, while the current process remains safely registered.
  SetBoolConfigValue(kFakeAudioLinkConfigKey, enabled, gFakeAudioLink);
}

void SetMaxActivePrefabs(int limit) {
  gMaxActivePrefabs = std::clamp(limit, 8, 512);
  auto& config = getConfig();
  auto& doc = config.config;
  auto it = doc.FindMember(kMaxActivePrefabsConfigKey.data());
  if (it != doc.MemberEnd()) it->value.SetInt(gMaxActivePrefabs);
  config.Write();
}

void SetMaxNoteVisualFragments(int limit) {
  gMaxNoteVisualFragments = std::clamp(limit, 1, 64);
  auto& config = getConfig();
  auto& doc = config.config;
  auto it = doc.FindMember(kMaxNoteVisualFragmentsConfigKey.data());
  if (it != doc.MemberEnd()) it->value.SetInt(gMaxNoteVisualFragments);
  config.Write();
}

void SetMaxTotalPixels(int limit) {
  gMaxTotalPixels = std::clamp(limit, 512 * 1024, 64 * 1024 * 1024);
  auto& config = getConfig();
  auto& doc = config.config;
  auto it = doc.FindMember(kMaxTotalPixelsConfigKey.data());
  if (it != doc.MemberEnd()) it->value.SetInt(gMaxTotalPixels);
  config.Write();
}

void EnsureConfigDefaults() {
  auto& config = getConfig();
  auto& doc = config.config;
  EnsureConfigObject();
  bool needsWrite = false;
  needsWrite |= EnsureBoolConfigValue(kMultipassRenderingConfigKey, true, gMultipassRenderingEnabled);
  needsWrite |= EnsureBoolConfigValue(kVivifyDebugLoggingConfigKey, false, gVivifyDebugLogging);
  needsWrite |= EnsureBoolConfigValue(kDisableBeat0FilmgrainBlitConfigKey, false, gDisableBeat0FilmgrainBlit);
  needsWrite |= EnsureBoolConfigValue(kDisableAllBlitsConfigKey, false, gDisableAllBlits);
  needsWrite |= EnsureBoolConfigValue(kDisableCustomNoteVisualsConfigKey, false, gDisableCustomNoteVisuals);
  needsWrite |= EnsureBoolConfigValue(kDisableVisualsInMultiplayerConfigKey, true, gDisableVisualsInMultiplayer);
  needsWrite |= EnsureBoolConfigValue(kDisableCreateCameraDepthConfigKey, false, gDisableCreateCameraDepth);
  needsWrite |= EnsureBoolConfigValue(kDisableVRCenterAdjustConfigKey, false, gDisableVRCenterAdjust);
  needsWrite |= EnsureBoolConfigValue(kSafeModeLimitsConfigKey, true, gSafeModeLimits);
  needsWrite |= EnsureBoolConfigValue(kAllowWindowsBundleFallbackConfigKey, false, gAllowWindowsBundleFallback);
  needsWrite |= EnsureBoolConfigValue(kHideVanillaSabersConfigKey, true, gHideVanillaSabers);
  needsWrite |= EnsureBoolConfigValue(kAssetCullingConfigKey, true, gAssetCulling);
  needsWrite |= EnsureBoolConfigValue(kFakeAudioLinkConfigKey, true, gFakeAudioLink);
  needsWrite |= EnsureBoolConfigValue(kPerformanceModeConfigKey, true, gPerformanceMode);
  needsWrite |= EnsureBoolConfigValue(kMirror30FPSConfigKey, true, gMirror30FPS);
  needsWrite |= EnsureBoolConfigValue(kDynamicEffectResolutionConfigKey, true, gDynamicEffectResolution);
  auto it = doc.FindMember(kAssetCullingScaleConfigKey.data());
  if (it != doc.MemberEnd() && it->value.IsFloat()) {
    gAssetCullingScale = std::clamp(it->value.GetFloat(), 0.1f, 1.0f);
  } else {
    auto& allocator = doc.GetAllocator();
    if (it == doc.MemberEnd()) {
      doc.AddMember(rapidjson::Value(kAssetCullingScaleConfigKey.data(), allocator),
                    rapidjson::Value(gAssetCullingScale), allocator);
    } else {
      it->value.SetFloat(gAssetCullingScale);
    }
    needsWrite = true;
  }
  // Int limits: clamp into range, write back if the stored value was missing/out of range.
  auto ensureIntConfigValue = [&](std::string_view key, int defaultValue, int& value, int min, int max) {
    auto it = doc.FindMember(key.data());
    if (it != doc.MemberEnd() && it->value.IsInt()) {
      int stored = it->value.GetInt();
      // Migrate the original 8M default. That value was too small for a normal
      // Quest full-screen effect plus a mirror, so Safe Mode silently rejected
      // valid images. Explicit values other than 8M remain untouched.
      if (key == kMaxTotalPixelsConfigKey && stored == 8 * 1024 * 1024) {
        stored = 24 * 1024 * 1024;
        it->value.SetInt(stored);
        needsWrite = true;
      }
      value = std::clamp(stored, min, max);
      if (value != stored) {
        it->value.SetInt(value);
        needsWrite = true;
      }
      return;
    }
    value = defaultValue;
    auto& allocator = doc.GetAllocator();
    if (it == doc.MemberEnd()) {
      doc.AddMember(rapidjson::Value(key.data(), allocator), rapidjson::Value(defaultValue), allocator);
    } else {
      it->value.SetInt(defaultValue);
    }
    needsWrite = true;
  };
  ensureIntConfigValue(kMaxActivePrefabsConfigKey, gMaxActivePrefabs, gMaxActivePrefabs, 8, 512);
  ensureIntConfigValue(kMaxNoteVisualFragmentsConfigKey, gMaxNoteVisualFragments, gMaxNoteVisualFragments, 1, 64);
  ensureIntConfigValue(kMaxTotalPixelsConfigKey, gMaxTotalPixels, gMaxTotalPixels, 512 * 1024, 64 * 1024 * 1024);
  if (needsWrite) {
    config.Write();
  }
}

MOD_EXTERN_FUNC void setup(CModInfo *info) noexcept {
  *info = modInfo.to_c();
  getConfig().Load();
  EnsureConfigDefaults();
}
MOD_EXTERN_FUNC void late_load() noexcept {
  il2cpp_functions::Init();
  custom_types::Register::AutoRegister();
  RegisterModSettings();
  Vivify::LateLoad();
}
