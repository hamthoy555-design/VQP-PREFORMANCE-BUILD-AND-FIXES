#pragma once
#include "scotland2/shared/modloader.h"
#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "paper2_scotland2/shared/logger.hpp"
#include "_config.hpp"
Configuration &getConfig();
bool GetMultipassRenderingEnabled();
bool GetVivifyDebugLogging();
bool GetDisableBeat0FilmgrainBlit();
bool GetDisableAllBlits();
bool GetDisableCreateCameraDepth();
bool GetDisableVRCenterAdjust();
bool GetDisableCustomNoteVisuals();
void SetDisableCustomNoteVisuals(bool enabled);
bool GetDisableVisualsInMultiplayer();
bool GetSafeModeLimits();
bool GetAllowWindowsBundleFallback();
int GetMaxActivePrefabs();
int GetMaxNoteVisualFragments();
int GetMaxTotalPixels();
void SetSafeModeLimits(bool enabled);
void SetAllowWindowsBundleFallback(bool enabled);
void SetDisableVisualsInMultiplayer(bool enabled);
void SetMultipassRenderingEnabled(bool enabled);
void EnsureConfigDefaults();
