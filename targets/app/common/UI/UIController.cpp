#include "UIController.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <algorithm>
#include <memory>
#include <utility>

#include "app/common/Audio/SoundEngine.h"
#include "app/common/DLC/DLCManager.h"
#include "app/common/Iggy/include/iggy.h"
#include "app/common/Network/GameNetworkManager.h"
#include "app/common/UI/All Platforms/UIEnums.h"
#include "app/common/UI/All Platforms/UIStructs.h"
#include "app/common/UI/Components/UIComponent_DebugUIConsole.h"
#include "app/common/UI/Components/UIComponent_DebugUIMarketingGuide.h"
#include "app/common/UI/Components/UIComponent_PressStartToPlay.h"
#include "app/common/UI/Components/UIComponent_Tooltips.h"
#include "app/common/UI/Components/UIComponent_TutorialPopup.h"
#include "app/common/UI/Components/UIScene_HUD.h"
#include "app/common/UI/UIBitmapFont.h"
#include "app/common/UI/UIGroup.h"
#include "app/common/UI/UIScene.h"
#include "app/common/UI/UIString.h"
#include "app/common/UI/UITTFFont.h"
#include "minecraft/GameEnums.h"
#include "platform/input/input.h"
#include "platform/profile/profile.h"
#ifndef _ENABLEIGGY
#include "app/common/Iggy/iggy_stubs.h"
#endif
#include "UIFontData.h"
#include "app/common/Game.h"
#include "app/common/UI/ConsoleUIController.h"
#include "java/System.h"
#include "minecraft/client/BufferedImage.h"
#include "minecraft/client/Minecraft.h"
#include "minecraft/client/multiplayer/MultiPlayerLocalPlayer.h"
#include "minecraft/client/renderer/Textures.h"
#include "minecraft/client/skins/DLCTexturePack.h"
#include "minecraft/client/skins/TexturePack.h"
#include "minecraft/client/skins/TexturePackRepository.h"
#include "minecraft/client/title/TitleScreen.h"
#include "platform/XboxStubs.h"
#include "platform/thread/C4JThread.h"
#include "strings.h"
#include "util/StringHelpers.h"
#include "util/Timer.h"

class Tutorial;

std::mutex UIController::ms_reloadSkinCS;
bool UIController::ms_bReloadSkinCSInitialised = false;

std::uint32_t UIController::m_dwTrialTimerLimitSecs = 2400;

static void RADLINK WarningCallback(void* user_callback_data, Iggy* player,
                                    IggyResult code, const char* message) {
    if (message != nullptr) {
        if (strstr(message, "LabelGamertag") != nullptr ||
            strstr(message, "Method SetSafeZone was not a function") !=
                nullptr) {
            return;
        }
    }

    switch (code) {
        case IGGY_RESULT_Warning_CannotSustainFrameRate:
            break;
        default:
            app.DebugPrintf(app.USER_SR, "[Iggy] ");
            app.DebugPrintf(app.USER_SR, message);
            app.DebugPrintf(app.USER_SR, "\n");
            break;
    };
}

static void RADLINK TraceCallback(void* user_callback_data, Iggy* player,
                                  char const* utf8_string,
                                  S32 length_in_bytes) {
    app.DebugPrintf(app.USER_UI, (char*)utf8_string);
}

int64_t UIController::iggyAllocCount = 0;
static std::unordered_map<void*, size_t> allocations;
static void* RADLINK AllocateFunction(void* alloc_callback_user_data,
                                      size_t size_requested,
                                      size_t* size_returned) {
    UIController* controller = (UIController*)alloc_callback_user_data;
    std::lock_guard<std::mutex> lock(controller->m_Allocatorlock);
    void* alloc = malloc(size_requested);
    *size_returned = size_requested;
    UIController::iggyAllocCount += size_requested;
    allocations[alloc] = size_requested;
    return alloc;
}

static void RADLINK DeallocateFunction(void* alloc_callback_user_data,
                                       void* ptr) {
    UIController* controller = (UIController*)alloc_callback_user_data;
    std::lock_guard<std::mutex> lock(controller->m_Allocatorlock);
    size_t size = allocations[ptr];
    UIController::iggyAllocCount -= size;
    allocations.erase(ptr);
    free(ptr);
}

UIController::UIController() {
    m_uiDebugConsole = nullptr;
    m_reloadSkinThread = nullptr;

    m_navigateToHomeOnReload = false;

    m_bCleanupOnReload = false;
    m_mcTTFFont = nullptr;
    m_moj7 = nullptr;
    m_moj11 = nullptr;

    m_eCurrentFont = m_eTargetFont = eFont_NotLoaded;

    m_fScreenWidth = 1920.0f;
    m_fScreenHeight = 1080.0f;
    m_bScreenWidthSetup = true;

    for (unsigned int i = 0; i < eLibrary_Count; ++i) {
        m_iggyLibraries[i] = IGGY_INVALID_LIBRARY;
    }

    for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
        m_bMenuDisplayed[i] = false;
        m_iCountDown[i] = 0;
        m_bMenuToBeClosed[i] = false;

        for (unsigned int key = 0; key <= ACTION_MAX_MENU; ++key) {
            m_actionRepeatTimer[i][key] = {};
        }
    }

    for (unsigned int i = 0; i < eUIGroup_COUNT; ++i) {
        m_bCloseAllScenes[i] = false;
    }

    m_iPressStartQuadrantsMask = 0;

    m_currentRenderViewport = IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN;
    m_bCustomRenderPosition = false;
    m_winUserIndex = 0;
    m_accumulatedTicks = 0;
    m_lastUiSfx = 0;

    m_bSystemUIShowing = false;

    if (!ms_bReloadSkinCSInitialised) {
        ms_bReloadSkinCSInitialised = true;
    }
}

void UIController::SetSysUIShowing(bool bVal) {
    if (bVal)
        app.DebugPrintf("System UI showing\n");
    else
        app.DebugPrintf("System UI stopped showing\n");
    m_bSystemUIShowing = bVal;
}

void UIController::SetSystemUIShowing(void* lpParam, bool bVal) {
    UIController* pClass = (UIController*)lpParam;
    pClass->SetSysUIShowing(bVal);
}

void UIController::preInit(S32 width, S32 height) {
    m_fScreenWidth = width;
    m_fScreenHeight = height;
    m_bScreenWidthSetup = true;

    IggyInit(0);

    IggySetWarningCallback(WarningCallback, 0);
    IggySetTraceCallbackUTF8(TraceCallback, 0);

    setFontCachingCalculationBuffer(-1);
}

void UIController::postInit() {
    IggySetCustomDrawCallback(&UIController::CustomDrawCallback, this);
    IggySetAS3ExternalFunctionCallbackUTF16(
        &UIController::ExternalFunctionCallback, this);
    IggySetTextureSubstitutionCallbacks(
        &UIController::TextureSubstitutionCreateCallback,
        &UIController::TextureSubstitutionDestroyCallback, this);

    SetupFont();
    loadSkins();

    for (unsigned int i = 0; i < eUIGroup_COUNT; ++i) {
        m_groups[i] = new UIGroup((EUIGroup)i, i - 1);
    }

    NavigateToScene(0, eUIScene_Intro);
}

UIController::EFont UIController::getFontForLanguage(int language) {
    switch (language) {
        case XC_LANGUAGE_JAPANESE:
            return eFont_Japanese;
        case XC_LANGUAGE_TCHINESE:
            return eFont_TradChinese;
        case XC_LANGUAGE_KOREAN:
            return eFont_Korean;
        default:
            return eFont_Bitmap;
    }
}

UITTFFont* UIController::createFont(EFont fontLanguage) {
    switch (fontLanguage) {
        case eFont_Japanese:
            return new UITTFFont(
                "Mojangles_TTF_jaJP",
                "app/common/Media/font/JPN/DFGMaruGothic-Md.ttf",
                0x2022);
        case eFont_TradChinese:
            return new UITTFFont("Mojangles_TTF_cnTD",
                                 "app/common/Media/font/CHT/DFHeiMedium-B5.ttf",
                                 0x2022);
        case eFont_Korean:
            return new UITTFFont("Mojangles_TTF_koKR",
                                 "app/common/Media/font/KOR/BOKMSD.ttf",
                                 0x2022);
        default:
            return nullptr;
    }
}

void UIController::SetupFont() {
    if ((m_eCurrentFont != m_eTargetFont) || !UIString::setCurrentLanguage())
        return;

    uint32_t nextLanguage = UIString::getCurrentLanguage();
    m_eTargetFont = getFontForLanguage(nextLanguage);

    app.m_dlcManager.LanguageChanged();
    app.loadStringTable();

    if (m_eTargetFont == m_eCurrentFont) {
        if (app.GetGameStarted())
            app.SetAction(PlatformProfile.GetPrimaryPad(),
                          eAppAction_ReloadFont);
        return;
    }

    if (m_eCurrentFont != eFont_NotLoaded)
        app.DebugPrintf(
            "[UIController] Font switch required for language transition to "
            "%i.\n",
            nextLanguage);
    else
        app.DebugPrintf("[UIController] Initialising font for language %i.\n",
                        nextLanguage);

    if (m_mcTTFFont != nullptr) {
        delete m_mcTTFFont;
        m_mcTTFFont = nullptr;
    }

    if (m_eTargetFont == eFont_Bitmap) {
        if (m_moj7 == nullptr)
            m_moj7 = new UIBitmapFont(SFontData::Mojangles_7);
        if (m_moj11 == nullptr)
            m_moj11 = new UIBitmapFont(SFontData::Mojangles_11);

        m_moj7->registerFont();
        m_moj11->registerFont();
    } else if (m_eTargetFont != eFont_NotLoaded) {
        m_mcTTFFont = createFont(m_eTargetFont);

        app.DebugPrintf("[Iggy] Set font indirect to '%hs'.\n",
                        m_mcTTFFont->getFontName().c_str());
        IggyFontSetIndirectUTF8("Mojangles7", -1, IGGY_FONTFLAG_all,
                                m_mcTTFFont->getFontName().c_str(), -1,
                                IGGY_FONTFLAG_none);
        IggyFontSetIndirectUTF8("Mojangles11", -1, IGGY_FONTFLAG_all,
                                m_mcTTFFont->getFontName().c_str(), -1,
                                IGGY_FONTFLAG_none);
    } else {
        assert(false);
    }

    if (m_eCurrentFont != eFont_NotLoaded) {
        app.SetAction(PlatformProfile.GetPrimaryPad(), eAppAction_ReloadFont);
    } else {
        updateCurrentFont();
    }
}

bool UIController::PendingFontChange() {
    return getFontForLanguage(XGetLanguage()) != m_eCurrentFont;
}

void UIController::setCleanupOnReload() { m_bCleanupOnReload = true; }

void UIController::updateCurrentFont() { m_eCurrentFont = m_eTargetFont; }

bool UIController::UsingBitmapFont() { return m_eCurrentFont == eFont_Bitmap; }

void UIController::tick() {
    SetupFont();

    if ((m_navigateToHomeOnReload || m_bCleanupOnReload) &&
        !ui.IsReloadingSkin()) {
        ui.CleanUpSkinReload();

        if (m_navigateToHomeOnReload || !g_NetworkManager.IsInSession()) {
            ui.NavigateToScene(PlatformProfile.GetPrimaryPad(),
                               eUIScene_MainMenu);
        } else {
            ui.CloseAllPlayersScenes();
        }

        updateCurrentFont();

        m_navigateToHomeOnReload = false;
        m_bCleanupOnReload = false;
    }

    for (unsigned int i = 0; i < eUIGroup_COUNT; ++i) {
        if (m_bCloseAllScenes[i]) {
            m_groups[i]->closeAllScenes();
            m_groups[i]->getTooltips()->SetTooltips(-1);
            m_bCloseAllScenes[i] = false;
        }
    }

    if (m_accumulatedTicks == 0) tickInput();
    m_accumulatedTicks = 0;

    for (unsigned int i = 0; i < eUIGroup_COUNT; ++i) {
        m_groups[i]->tick();
    }

    int64_t currentTime = System::currentTimeMillis();
    for (auto it = m_cachedMovieData.begin(); it != m_cachedMovieData.end();) {
        if (it->second.m_expiry < currentTime) {
            it = m_cachedMovieData.erase(it);
        } else {
            ++it;
        }
    }
}

void UIController::loadSkins() {
    std::string platformSkinPath = "";

#if 1
    if (m_fScreenHeight == 1080.0f) {
        platformSkinPath = "skinHDWin.swf";
    } else {
        platformSkinPath = "skinWin.swf";
    }
#endif
    if (m_fScreenHeight == 1080.0f) {
        m_iggyLibraries[eLibrary_Platform] =
            loadSkin(platformSkinPath, "platformskinHD.swf");
    } else {
        m_iggyLibraries[eLibrary_Platform] =
            loadSkin(platformSkinPath, "platformskin.swf");
    }

#if 1
    m_iggyLibraries[eLibrary_GraphicsDefault] =
        loadSkin("skinHDGraphics.swf", "skinHDGraphics.swf");
    m_iggyLibraries[eLibrary_GraphicsHUD] =
        loadSkin("skinHDGraphicsHud.swf", "skinHDGraphicsHud.swf");
    m_iggyLibraries[eLibrary_GraphicsInGame] =
        loadSkin("skinHDGraphicsInGame.swf", "skinHDGraphicsInGame.swf");
    m_iggyLibraries[eLibrary_GraphicsTooltips] =
        loadSkin("skinHDGraphicsTooltips.swf", "skinHDGraphicsTooltips.swf");
    m_iggyLibraries[eLibrary_GraphicsLabels] =
        loadSkin("skinHDGraphicsLabels.swf", "skinHDGraphicsLabels.swf");
    m_iggyLibraries[eLibrary_Labels] =
        loadSkin("skinHDLabels.swf", "skinHDLabels.swf");
    m_iggyLibraries[eLibrary_InGame] =
        loadSkin("skinHDInGame.swf", "skinHDInGame.swf");
    m_iggyLibraries[eLibrary_HUD] = loadSkin("skinHDHud.swf", "skinHDHud.swf");
    m_iggyLibraries[eLibrary_Tooltips] =
        loadSkin("skinHDTooltips.swf", "skinHDTooltips.swf");
    m_iggyLibraries[eLibrary_Default] = loadSkin("skinHD.swf", "skinHD.swf");
#endif
}

IggyLibrary UIController::loadSkin(const std::string& skinPath,
                                   const std::string& skinName) {
    IggyLibrary lib = IGGY_INVALID_LIBRARY;
    if (!skinPath.empty() && app.hasArchiveFile(skinPath)) {
        // КРИТИЧЕСКИЙ ФИКС: Кэшируем буферы и строки, чтобы Iggy не читал удаленную из стека память
        static std::unordered_map<std::string, std::vector<uint8_t>> skinDataCache;
        static std::unordered_map<std::string, std::u16string> skinNameCache;

        if (skinDataCache.find(skinPath) == skinDataCache.end()) {
            skinDataCache[skinPath] = app.getArchiveFile(skinPath);
        }
        if (skinNameCache.find(skinName) == skinNameCache.end()) {
            skinNameCache[skinName] = string_to_u16string(skinName);
        }

        std::vector<uint8_t>& baFile = skinDataCache[skinPath];
        std::u16string& convSkinName = skinNameCache[skinName];

        lib = IggyLibraryCreateFromMemoryUTF16(
            convSkinName.data(), (void*)baFile.data(), baFile.size(), nullptr);
    }
    return lib;
}

void UIController::ReloadSkin() {
    for (unsigned int i = 0; i < eUIGroup_COUNT; ++i) {
        m_groups[i]->DestroyAll();
    }

    for (int i = eLibrary_Count - 1; i >= 0; --i) {
        if (m_iggyLibraries[i] != IGGY_INVALID_LIBRARY)
            IggyLibraryDestroy(m_iggyLibraries[i]);
        m_iggyLibraries[i] = IGGY_INVALID_LIBRARY;
    }

    reloadSkinThreadProc(this);
}

void UIController::StartReloadSkinThread() {
    if (m_reloadSkinThread) m_reloadSkinThread->run();
}

int UIController::reloadSkinThreadProc(void* lpParam) {
    {
        std::lock_guard<std::mutex> lock(ms_reloadSkinCS);
        UIController* controller = (UIController*)lpParam;
        controller->loadSkins();

        for (int i = eUIGroup_Player1; i < eUIGroup_COUNT; ++i) {
            controller->m_groups[i]->ReloadAll();
        }

        controller->m_groups[eUIGroup_Fullscreen]->ReloadAll();
    }
    return 0;
}

bool UIController::IsReloadingSkin() {
    return m_reloadSkinThread && (!m_reloadSkinThread->hasStarted() ||
                                  m_reloadSkinThread->isRunning());
}

bool UIController::IsExpectingOrReloadingSkin() {
    return Minecraft::GetInstance()->skins->getSelected()->isLoadingData() ||
           Minecraft::GetInstance()->skins->needsUIUpdate() ||
           IsReloadingSkin() || PendingFontChange();
}

void UIController::CleanUpSkinReload() {
    delete m_reloadSkinThread;
    m_reloadSkinThread = nullptr;

    if (!Minecraft::GetInstance()->skins->isUsingDefaultSkin()) {
        if (!Minecraft::GetInstance()->skins->getSelected()->hasAudio()) {
            const unsigned int result =
                PlatformStorage.UnmountInstalledDLC("TPACK");
        }
    }

    for (auto it = m_queuedMessageBoxData.begin();
         it != m_queuedMessageBoxData.end(); ++it) {
        QueuedMessageBoxData* queuedData = *it;
        ui.NavigateToScene(queuedData->iPad, eUIScene_MessageBox,
                           &queuedData->info, queuedData->layer,
                           eUIGroup_Fullscreen);
        delete queuedData->info.uiOptionA;
        delete queuedData;
    }
    m_queuedMessageBoxData.clear();
}

std::vector<uint8_t> UIController::getMovieData(const std::string& filename) {
    int64_t targetTime = System::currentTimeMillis() + (1000LL * 60);
    auto it = m_cachedMovieData.find(filename);
    if (it == m_cachedMovieData.end()) {
        std::vector<uint8_t> baFile = app.getArchiveFile(filename);
        CachedMovieData cmd;
        cmd.m_ba = baFile;
        cmd.m_expiry = targetTime;
        m_cachedMovieData[filename] = cmd;
        return baFile;
    } else {
        it->second.m_expiry = targetTime;
        return it->second.m_ba;
    }
}

void UIController::tickInput() {
    if (!m_bSystemUIShowing) {
        handleInput();
        ++m_accumulatedTicks;
    }
}

void UIController::handleInput() {
    for (unsigned int iPad = 0; iPad < XUSER_MAX_COUNT; ++iPad) {
        for (unsigned int key = 0; key <= ACTION_MAX_MENU; ++key) {
            handleKeyPress(iPad, key);
        }
    }
}

void UIController::handleKeyPress(unsigned int iPad, unsigned int key) {
    bool down = PlatformInput.ButtonDown(iPad, key);
    bool pressed = PlatformInput.ButtonPressed(iPad, key);
    bool released = PlatformInput.ButtonReleased(iPad, key);
    bool repeat = false;

    if (pressed) {
        m_actionRepeatTimer[iPad][key] =
            time_util::clock::now() +
            std::chrono::milliseconds(UI_REPEAT_KEY_DELAY_MS);
    } else if (released) {
        m_actionRepeatTimer[iPad][key] = {};
    } else if (down) {
        auto now = time_util::clock::now();
        if (m_actionRepeatTimer[iPad][key] != time_util::time_point{} &&
            now > m_actionRepeatTimer[iPad][key]) {
            repeat = true;
            pressed = true;
            m_actionRepeatTimer[iPad][key] =
                now + std::chrono::milliseconds(UI_REPEAT_KEY_REPEAT_RATE_MS);
        }
    }

    if (repeat || pressed || released) {
        bool handled = false;
        m_groups[(int)eUIGroup_Fullscreen]->handleInput(
            iPad, key, repeat, pressed, released, handled);
        if (!handled) {
            m_groups[(iPad + 1)]->handleInput(iPad, key, repeat, pressed,
                                              released, handled);
        }
    }
}

rrbool RADLINK
UIController::ExternalFunctionCallback(void* user_callback_data, Iggy* player,
                                       IggyExternalFunctionCallUTF16* call) {
    UIScene* scene = (UIScene*)IggyPlayerGetUserdata(player);
    if (scene != nullptr) {
        scene->externalCallback(call);
    }
    return true;
}

void UIController::renderScenes() {
    if (app.GetGameStarted() &&
        !m_groups[eUIGroup_Fullscreen]->hidesLowerScenes()) {
        for (int i = eUIGroup_Player1; i < eUIGroup_COUNT; ++i) {
            m_groups[i]->render();
        }
    }
    m_groups[eUIGroup_Fullscreen]->render();
}

void UIController::getRenderDimensions(
    IPlatformRenderer::eViewportType viewport, S32& width, S32& height) {
    switch (viewport) {
        case IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN:
            width = (S32)(getScreenWidth());
            height = (S32)(getScreenHeight());
            break;
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_TOP:
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_BOTTOM:
            width = (S32)(getScreenWidth() / 2);
            height = (S32)(getScreenHeight() / 2);
            break;
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_LEFT:
        case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_RIGHT:
            width = (S32)(getScreenWidth() / 2);
            height = (S32)(getScreenHeight() / 2);
            break;
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_LEFT:
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT:
        case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
            width = (S32)(getScreenWidth() / 2);
            height = (S32)(getScreenHeight() / 2);
            break;
        default:
            break;
    }
}

void UIController::setupRenderPosition(
    IPlatformRenderer::eViewportType viewport) {
    if (m_bCustomRenderPosition || m_currentRenderViewport != viewport) {
        m_currentRenderViewport = viewport;
        m_bCustomRenderPosition = false;
        S32 xPos = 0;
        S32 yPos = 0;
        switch (viewport) {
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_TOP:
                xPos = (S32)(getScreenWidth() / 4);
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_BOTTOM:
                xPos = (S32)(getScreenWidth() / 4);
                yPos = (S32)(getScreenHeight() / 2);
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_LEFT:
                yPos = (S32)(getScreenHeight() / 4);
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_SPLIT_RIGHT:
                xPos = (S32)(getScreenWidth() / 2);
                yPos = (S32)(getScreenHeight() / 4);
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_LEFT:
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
                xPos = (S32)(getScreenWidth() / 2);
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT:
                yPos = (S32)(getScreenHeight() / 2);
                break;
            case IPlatformRenderer::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
                xPos = (S32)(getScreenWidth() / 2);
                yPos = (S32)(getScreenHeight() / 2);
                break;
            default:
                break;
        }
        m_tileOriginX = xPos;
        m_tileOriginY = yPos;
        setTileOrigin(xPos, yPos);
    }
}

void UIController::setupRenderPosition(S32 xOrigin, S32 yOrigin) {
    m_bCustomRenderPosition = true;
    m_tileOriginX = xOrigin;
    m_tileOriginY = yOrigin;
    setTileOrigin(xOrigin, yOrigin);
}

void UIController::setupCustomDrawGameState() {
    m_customRenderingClearRect.left = LONG_MAX;
    m_customRenderingClearRect.right = LONG_MIN;
    m_customRenderingClearRect.top = LONG_MAX;
    m_customRenderingClearRect.bottom = LONG_MIN;

    PlatformRenderer.StartFrame();
    PlatformRenderer.Set_matrixDirty();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, m_fScreenWidth, m_fScreenHeight, 0, 1000, 3000);
    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(true);
}

void UIController::setupCustomDrawMatrices(UIScene* scene,
                                           CustomDrawData* customDrawRegion) {
    float sceneWidth = (float)scene->getRenderWidth();
    float sceneHeight = (float)scene->getRenderHeight();

    int32_t left, right, top, bottom;
    left = m_tileOriginX +
           (sceneWidth + customDrawRegion->mat[(0 * 4) + 3] * sceneWidth) / 2;
    right = left + ((sceneWidth * customDrawRegion->mat[0]) / 2) *
                       customDrawRegion->x1;

    top = m_tileOriginY +
          (sceneHeight - customDrawRegion->mat[(1 * 4) + 3] * sceneHeight) / 2;
    bottom = top + (sceneHeight * -customDrawRegion->mat[(1 * 4) + 1]) / 2 *
                       customDrawRegion->y1;

    m_customRenderingClearRect.left =
        std::min<long>(m_customRenderingClearRect.left, left);
    m_customRenderingClearRect.right =
        std::max<long>(m_customRenderingClearRect.right, right);
    m_customRenderingClearRect.top =
        std::min<long>(m_customRenderingClearRect.top, top);
    m_customRenderingClearRect.bottom =
        std::max<long>(m_customRenderingClearRect.bottom, bottom);

    if (!m_bScreenWidthSetup) {
        Minecraft* pMinecraft = Minecraft::GetInstance();
        if (pMinecraft != nullptr) {
            m_fScreenWidth = (float)pMinecraft->width_phys;
            m_fScreenHeight = (float)pMinecraft->height_phys;
            m_bScreenWidthSetup = true;
        }
    }

    glLoadIdentity();
    glTranslatef(0, 0, -2000);
    glTranslatef(
        (m_fScreenWidth + customDrawRegion->mat[(0 * 4) + 3] * m_fScreenWidth) /
            2,
        (m_fScreenHeight -
         customDrawRegion->mat[(1 * 4) + 3] * m_fScreenHeight) /
            2,
        0);
    glScalef((m_fScreenWidth * customDrawRegion->mat[0]) / 2,
             (m_fScreenHeight * -customDrawRegion->mat[(1 * 4) + 1]) / 2, 1.0f);
}

void UIController::setupCustomDrawGameStateAndMatrices(
    UIScene* scene, CustomDrawData* customDrawRegion) {
    setupCustomDrawGameState();
    setupCustomDrawMatrices(scene, customDrawRegion);
}

void UIController::endCustomDrawGameState() {
    PlatformRenderer.Clear(GL_DEPTH_BUFFER_BIT);
    glDepthMask(false);
    glDisable(GL_ALPHA_TEST);
}

void UIController::endCustomDrawMatrices() {}

void UIController::endCustomDrawGameStateAndMatrices() {
    endCustomDrawMatrices();
    endCustomDrawGameState();
}

void RADLINK
UIController::CustomDrawCallback(void* user_callback_data, Iggy* player,
                                 IggyCustomDrawCallbackRegion* region) {
    UIScene* scene = (UIScene*)IggyPlayerGetUserdata(player);
    if (scene != nullptr) {
        scene->customDraw(region);
    }
}

GDrawTexture* RADLINK UIController::TextureSubstitutionCreateCallback(
    void* user_callback_data, IggyUTF16* texture_name, S32* width, S32* height,
    void** destroy_callback_data) {
    UIController* uiController = (UIController*)user_callback_data;
    auto it = uiController->m_substitutionTextures.find((char*)texture_name);

    if (it != uiController->m_substitutionTextures.end()) {
        BufferedImage image(it->second.data(), it->second.size());
        if (image.getData() != nullptr) {
            image.preMultiplyAlpha();
            Textures* t = Minecraft::GetInstance()->textures;
            int id = t->getTexture(
                &image, IPlatformRenderer::TEXTURE_FORMAT_RxGyBzAw, false);

            *width = 64;
            *height = 64;

            *destroy_callback_data = (void*)(intptr_t)id;
            return ui.getSubstitutionTexture(id);
        } else {
            return nullptr;
        }
    } else {
        return nullptr;
    }
}

void RADLINK UIController::TextureSubstitutionDestroyCallback(
    void* user_callback_data, void* destroy_callback_data,
    GDrawTexture* handle) {
    int64_t llVal = (int64_t)destroy_callback_data;
    int id = (int)llVal;

    ui.destroySubstitutionTexture(user_callback_data, handle);

    Textures* t = Minecraft::GetInstance()->textures;
    t->releaseTexture(id);
}

void UIController::registerSubstitutionTexture(const std::string& textureName,
                                               std::uint8_t* pbData,
                                               unsigned int dwLength) {
    unregisterSubstitutionTexture(textureName, false);
    m_substitutionTextures[textureName] =
        std::vector<uint8_t>(pbData, pbData + dwLength);
}

void UIController::unregisterSubstitutionTexture(const std::string& textureName,
                                                 bool deleteData) {
    auto it = m_substitutionTextures.find(textureName);
    if (it != m_substitutionTextures.end()) {
        m_substitutionTextures.erase(it);
    }
}

bool UIController::NavigateToScene(int iPad, EUIScene scene, void* initData,
                                   EUILayer layer, EUIGroup group) {
    static bool bSeenUpdateTextThisSession = false;
    if ((scene == eUIScene_LoadOrJoinMenu) &&
        (bSeenUpdateTextThisSession == false) &&
        (app.GetGameSettings(PlatformProfile.GetPrimaryPad(),
                             eGameSetting_DisplayUpdateMessage) != 0)) {
        scene = eUIScene_NewUpdateMessage;
        bSeenUpdateTextThisSession = true;
    }

    if (GetMenuDisplayed(iPad)) {
        switch (scene) {
            case eUIScene_PauseMenu:
            case eUIScene_Crafting2x2Menu:
            case eUIScene_Crafting3x3Menu:
            case eUIScene_FurnaceMenu:
            case eUIScene_ContainerMenu:
            case eUIScene_LargeContainerMenu:
            case eUIScene_InventoryMenu:
            case eUIScene_CreativeMenu:
            case eUIScene_DispenserMenu:
            case eUIScene_SignEntryMenu:
            case eUIScene_InGameInfoMenu:
            case eUIScene_EnchantingMenu:
            case eUIScene_BrewingStandMenu:
            case eUIScene_AnvilMenu:
            case eUIScene_TradingMenu:
            case eUIScene_BeaconMenu:
            case eUIScene_HorseMenu:
                return false;
                break;
            default:
                break;
        }
    }

    switch (scene) {
        case eUIScene_FullscreenProgress: {
            layer = eUILayer_Fullscreen;
            group = eUIGroup_Fullscreen;
        } break;
        case eUIScene_ConnectingProgress: {
            layer = eUILayer_Fullscreen;
        } break;
        case eUIScene_EndPoem: {
            group = eUIGroup_Fullscreen;
            layer = eUILayer_Scene;
        } break;
        default:
            break;
    };
    int menuDisplayedPad = XUSER_INDEX_ANY;
    if (group == eUIGroup_PAD) {
        if (app.GetGameStarted()) {
            if ((iPad != 255) && (iPad >= 0)) {
                menuDisplayedPad = iPad;
                group = (EUIGroup)(iPad + 1);
            } else
                group = eUIGroup_Fullscreen;
        } else {
            layer = eUILayer_Fullscreen;
            group = eUIGroup_Fullscreen;
        }
    }

    bool success;
    {
        std::lock_guard<std::mutex> lock(m_navigationLock);
        SetMenuDisplayed(menuDisplayedPad, true);
        success =
            m_groups[(int)group]->NavigateToScene(iPad, scene, initData, layer);
        if (success && group == eUIGroup_Fullscreen)
            setFullscreenMenuDisplayed(true);
    }

    return success;
}

bool UIController::NavigateBack(int iPad, bool forceUsePad, EUIScene eScene,
                                EUILayer eLayer) {
    bool navComplete = false;
    if (app.GetGameStarted()) {
        bool navComplete = m_groups[(int)eUIGroup_Fullscreen]->NavigateBack(
            iPad, eScene, eLayer);

        if (!navComplete && (iPad != 255) && (iPad >= 0)) {
            EUIGroup group = (EUIGroup)(iPad + 1);
            navComplete =
                m_groups[(int)group]->NavigateBack(iPad, eScene, eLayer);
            if (!m_groups[(int)group]->GetMenuDisplayed())
                SetMenuDisplayed(iPad, false);
        } else {
            if (!m_groups[(int)eUIGroup_Fullscreen]->GetMenuDisplayed()) {
                setFullscreenMenuDisplayed(false);
                for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
                    SetMenuDisplayed(i, m_groups[i + 1]->GetMenuDisplayed());
                }
            }
        }
    } else {
        navComplete = m_groups[(int)eUIGroup_Fullscreen]->NavigateBack(
            iPad, eScene, eLayer);
        if (!m_groups[(int)eUIGroup_Fullscreen]->GetMenuDisplayed())
            SetMenuDisplayed(XUSER_INDEX_ANY, false);
    }
    return navComplete;
}

void UIController::NavigateToHomeMenu() {
    ui.CloseAllPlayersScenes();
    app.SetLiveLinkRequired(false);
    Minecraft* pMinecraft = Minecraft::GetInstance();

    TexturePack* pTexPack = Minecraft::GetInstance()->skins->getSelected();
    DLCTexturePack* pDLCTexPack = nullptr;
    if (pTexPack->hasAudio()) {
        pDLCTexPack = (DLCTexturePack*)pTexPack;
    }

    pMinecraft->skins->selectTexturePackById(
        TexturePackRepository::DEFAULT_TEXTURE_PACK_ID);

    if (pTexPack->hasAudio()) {
        static_cast<SoundEngine*>(pMinecraft->soundEngine)
            ->SetStreamingSounds(eStream_Overworld_Calm1,
                                 eStream_Overworld_piano3, eStream_Nether1,
                                 eStream_Nether4, eStream_end_dragon,
                                 eStream_end_end, eStream_CD_1);
        pMinecraft->soundEngine->playStreaming("", 0, 0, 0, 1, 1);
        const unsigned int result =
            PlatformStorage.UnmountInstalledDLC("TPACK");
    }

    g_NetworkManager.ForceFriendsSessionRefresh();

    if (pMinecraft->skins->needsUIUpdate()) {
        m_navigateToHomeOnReload = true;
    } else {
        ui.NavigateToScene(PlatformProfile.GetPrimaryPad(), eUIScene_MainMenu);
#if defined(ENABLE_JAVA_GUIS)
        pMinecraft->setScreen(new TitleScreen());
#endif
    }
}

UIScene* UIController::GetTopScene(int iPad, EUILayer layer, EUIGroup group) {
    if (group == eUIGroup_PAD) {
        if (app.GetGameStarted()) {
            if ((iPad != 255) && (iPad >= 0)) {
                group = (EUIGroup)(iPad + 1);
            } else
                group = eUIGroup_Fullscreen;
        } else {
            layer = eUILayer_Fullscreen;
            group = eUIGroup_Fullscreen;
        }
    }
    return m_groups[(int)group]->GetTopScene(layer);
}

size_t UIController::RegisterForCallbackId(UIScene* scene) {
    std::lock_guard<std::mutex> lock(m_registeredCallbackScenesCS);
    static std::atomic<std::uint32_t> s_nextId{1};
    size_t newId = s_nextId.fetch_add(1, std::memory_order_relaxed) & 0xFFFFFF;
    newId |= (scene->getSceneType() << 24);
    m_registeredCallbackScenes[newId] = scene;
    return newId;
}

void UIController::UnregisterCallbackId(size_t id) {
    std::lock_guard<std::mutex> lock(m_registeredCallbackScenesCS);
    auto it = m_registeredCallbackScenes.find(id);
    if (it != m_registeredCallbackScenes.end()) {
        m_registeredCallbackScenes.erase(it);
    }
}

UIScene* UIController::GetSceneFromCallbackId(size_t id) {
    UIScene* scene = nullptr;
    auto it = m_registeredCallbackScenes.find(id);
    if (it != m_registeredCallbackScenes.end()) {
        scene = it->second;
    }
    return scene;
}

void UIController::lockCallbackScenes() { m_registeredCallbackScenesCS.lock(); }

void UIController::unlockCallbackScenes() {
    m_registeredCallbackScenesCS.unlock();
}

void UIController::CloseAllPlayersScenes() {
    m_groups[(int)eUIGroup_Fullscreen]->getTooltips()->SetTooltips(-1);
    for (unsigned int i = 0; i < eUIGroup_COUNT; ++i) {
        m_groups[i]->closeAllScenes();
        m_groups[i]->getTooltips()->SetTooltips(-1);
    }

    if (!m_groups[eUIGroup_Fullscreen]->GetMenuDisplayed()) {
        for (unsigned int i = 0; i < XUSER_MAX_COUNT; ++i) {
            SetMenuDisplayed(i, false);
        }
    }
    setFullscreenMenuDisplayed(false);
}

void UIController::CloseUIScenes(int iPad, bool forceIPad) {
    EUIGroup group;
    if (app.GetGameStarted() || forceIPad) {
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }

    m_groups[(int)group]->closeAllScenes();
    m_groups[(int)group]->getTooltips()->SetTooltips(-1);

    TutorialPopupInfo popupInfo;
    if (m_groups[(int)group]->getTutorialPopup())
        m_groups[(int)group]->getTutorialPopup()->SetTutorialDescription(
            &popupInfo);

    if (group == eUIGroup_Fullscreen) setFullscreenMenuDisplayed(false);

    SetMenuDisplayed((group == eUIGroup_Fullscreen ? XUSER_INDEX_ANY : iPad),
                     m_groups[(int)group]->GetMenuDisplayed());
}

void UIController::setFullscreenMenuDisplayed(bool displayed) {
    m_groups[(int)eUIGroup_Fullscreen]->showComponent(
        PlatformProfile.GetPrimaryPad(), eUIComponent_Tooltips,
        eUILayer_Tooltips, displayed);

    for (unsigned int i = (eUIGroup_Fullscreen + 1); i < eUIGroup_COUNT; ++i) {
        m_groups[i]->showComponent(i, eUIComponent_Tooltips, eUILayer_Tooltips,
                                   !displayed);
    }
}

bool UIController::IsPauseMenuDisplayed(int iPad) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    return m_groups[(int)group]->IsPauseMenuDisplayed();
}

bool UIController::IsContainerMenuDisplayed(int iPad) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    return m_groups[(int)group]->IsContainerMenuDisplayed();
}

bool UIController::IsIgnorePlayerJoinMenuDisplayed(int iPad) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    return m_groups[(int)group]->IsIgnorePlayerJoinMenuDisplayed();
}

bool UIController::IsIgnoreAutosaveMenuDisplayed(int iPad) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    return m_groups[(int)eUIGroup_Fullscreen]
               ->IsIgnoreAutosaveMenuDisplayed() ||
           (group != eUIGroup_Fullscreen &&
            m_groups[(int)group]->IsIgnoreAutosaveMenuDisplayed());
}

void UIController::SetIgnoreAutosaveMenuDisplayed(int iPad, bool displayed) {
}

bool UIController::IsSceneInStack(int iPad, EUIScene eScene) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    return m_groups[(int)group]->IsSceneInStack(eScene);
}

bool UIController::GetMenuDisplayed(int iPad) { return m_bMenuDisplayed[iPad]; }

void UIController::SetMenuDisplayed(int iPad, bool bVal) {
    if (bVal) {
        if (iPad == XUSER_INDEX_ANY) {
            for (int i = 0; i < XUSER_MAX_COUNT; i++) {
                PlatformInput.SetMenuDisplayed(i, true);
                m_bMenuDisplayed[i] = true;
                m_bMenuToBeClosed[i] = false;
            }
        } else {
            PlatformInput.SetMenuDisplayed(iPad, true);
            m_bMenuDisplayed[iPad] = true;
            m_bMenuToBeClosed[iPad] = false;
        }
    } else {
        if (iPad == XUSER_INDEX_ANY) {
            for (int i = 0; i < XUSER_MAX_COUNT; i++) {
                m_bMenuToBeClosed[i] = true;
                m_iCountDown[i] = 10;
            }
        } else {
            m_bMenuToBeClosed[iPad] = true;
            m_iCountDown[iPad] = 10;
        }
    }
}

void UIController::CheckMenuDisplayed() {
    for (int iPad = 0; iPad < XUSER_MAX_COUNT; iPad++) {
        if (m_bMenuToBeClosed[iPad]) {
            if (m_iCountDown[iPad] != 0) {
                m_iCountDown[iPad]--;
            } else {
                m_bMenuToBeClosed[iPad] = false;
                m_bMenuDisplayed[iPad] = false;
                PlatformInput.SetMenuDisplayed(iPad, false);
            }
        }
    }
}

void UIController::SetTooltipText(unsigned int iPad, unsigned int tooltip,
                                  int iTextID) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        if ((iPad != 255))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    if (m_groups[(int)group]->getTooltips())
        m_groups[(int)group]->getTooltips()->SetTooltipText(tooltip, iTextID);
}

void UIController::SetEnableTooltips(unsigned int iPad, bool bVal) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        if ((iPad != 255))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    if (m_groups[(int)group]->getTooltips())
        m_groups[(int)group]->getTooltips()->SetEnableTooltips(bVal);
}

void UIController::ShowTooltip(unsigned int iPad, unsigned int tooltip,
                               bool show) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        if ((iPad != 255))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    if (m_groups[(int)group]->getTooltips())
        m_groups[(int)group]->getTooltips()->ShowTooltip(tooltip, show);
}

void UIController::SetTooltips(unsigned int iPad, int iA, int iB, int iX,
                               int iY, int iLT, int iRT, int iLB, int iRB,
                               int iLS, int iRS, int iBack, bool forceUpdate) {
    EUIGroup group;

    if (iX == IDS_TOOLTIPS_SELECTDEVICE) iX = -1;
    if (iX == IDS_TOOLTIPS_CHANGEDEVICE) iX = -1;

    if (app.GetGameStarted()) {
        if ((iPad != 255))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    if (m_groups[(int)group]->getTooltips())
        m_groups[(int)group]->getTooltips()->SetTooltips(
            iA, iB, iX, iY, iLT, iRT, iLB, iRB, iLS, iRS, iBack, forceUpdate);
}

void UIController::EnableTooltip(unsigned int iPad, unsigned int tooltip,
                                 bool enable) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        if ((iPad != 255))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    if (m_groups[(int)group]->getTooltips())
        m_groups[(int)group]->getTooltips()->EnableTooltip(tooltip, enable);
}

void UIController::RefreshTooltips(unsigned int iPad) {}

void UIController::AnimateKeyPress(int iPad, int iAction, bool bRepeat,
                                   bool bPressed, bool bReleased) {
    EUIGroup group;
    if (bPressed == false) {
        return;
    }
    if (app.GetGameStarted()) {
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    bool handled = false;
    if (m_groups[(int)group]->getTooltips())
        m_groups[(int)group]->getTooltips()->handleInput(
            iPad, iAction, bRepeat, bPressed, bReleased, handled);
}

void UIController::OverrideSFX(int iPad, int iAction, bool bVal) {
    EUIGroup group;

    if (app.GetGameStarted()) {
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    bool handled = false;
    if (m_groups[(int)group]->getTooltips())
        m_groups[(int)group]->getTooltips()->overrideSFX(iPad, iAction, bVal);
}

void UIController::PlayUISFX(ESoundEffect eSound) {
    uint64_t time = System::currentTimeMillis();
    if (time - m_lastUiSfx < 10) {
        return;
    }
    m_lastUiSfx = time;

    Minecraft::GetInstance()->soundEngine->playUI(eSound, 1.0f, 1.0f);
}

void UIController::DisplayGamertag(unsigned int iPad, bool show) {
    if (app.GetGameSettings(PlatformProfile.GetPrimaryPad(),
                            eGameSetting_DisplaySplitscreenGamertags) == 0) {
        show = false;
    }
    EUIGroup group = (EUIGroup)(iPad + 1);
    if (m_groups[(int)group]->getHUD())
        m_groups[(int)group]->getHUD()->ShowDisplayName(show);

    if (app.GetLocalPlayerCount() > 1 &&
        m_groups[(int)group]->getTutorialPopup() &&
        !m_groups[(int)group]->IsContainerMenuDisplayed()) {
        m_groups[(int)group]->getTutorialPopup()->UpdateTutorialPopup();
    }
}

void UIController::SetSelectedItem(unsigned int iPad, const std::string& name) {
    EUIGroup group;

    if (app.GetGameStarted()) {
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    bool handled = false;
    if (m_groups[(int)group]->getHUD())
        m_groups[(int)group]->getHUD()->SetSelectedLabel(name);
}

void UIController::UpdateSelectedItemPos(unsigned int iPad) {}

void UIController::HandleDLCMountingComplete() {
    for (unsigned int i = 0; i < eUIGroup_COUNT; ++i) {
        m_groups[i]->HandleDLCMountingComplete();
    }
}

void UIController::HandleDLCInstalled(int iPad) {
    for (unsigned int i = 0; i < eUIGroup_COUNT; ++i) {
        m_groups[i]->HandleDLCInstalled();
    }
}

void UIController::HandleTMSDLCFileRetrieved(int iPad) {}

void UIController::HandleTMSBanFileRetrieved(int iPad) {}

void UIController::HandleInventoryUpdated(int iPad) {
    EUIGroup group = eUIGroup_Fullscreen;
    if (app.GetGameStarted() && (iPad != 255) && (iPad >= 0)) {
        group = (EUIGroup)(iPad + 1);
    }

    m_groups[group]->HandleMessage(eUIMessage_InventoryUpdated, nullptr);
}

void UIController::HandleGameTick() {
    tickInput();

    for (unsigned int i = 0; i < eUIGroup_COUNT; ++i) {
        if (m_groups[i]->getHUD()) m_groups[i]->getHUD()->handleGameTick();
    }
}

void UIController::SetTutorial(int iPad, Tutorial* tutorial) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    if (m_groups[(int)group]->getTutorialPopup())
        m_groups[(int)group]->getTutorialPopup()->SetTutorial(tutorial);
}

void UIController::SetTutorialDescription(int iPad, TutorialPopupInfo* info) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }

    if (m_groups[(int)group]->getTutorialPopup()) {
        m_groups[(int)group]->getTutorialPopup()->SetContainerMenuVisible(
            m_groups[(int)group]->IsContainerMenuDisplayed());
        m_groups[(int)group]->getTutorialPopup()->SetTutorialDescription(info);
    }
}

void UIController::RemoveInteractSceneReference(int iPad, UIScene* scene) {
    EUIGroup group;
    if ((iPad != 255) && (iPad >= 0))
        group = (EUIGroup)(iPad + 1);
    else
        group = eUIGroup_Fullscreen;
    if (m_groups[(int)group]->getTutorialPopup())
        m_groups[(int)group]->getTutorialPopup()->RemoveInteractSceneReference(
            scene);
}

void UIController::SetTutorialVisible(int iPad, bool visible) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    if (m_groups[(int)group]->getTutorialPopup())
        m_groups[(int)group]->getTutorialPopup()->SetVisible(visible);
}

bool UIController::IsTutorialVisible(int iPad) {
    EUIGroup group;
    if (app.GetGameStarted()) {
        if ((iPad != 255) && (iPad >= 0))
            group = (EUIGroup)(iPad + 1);
        else
            group = eUIGroup_Fullscreen;
    } else {
        group = eUIGroup_Fullscreen;
    }
    bool visible = false;
    if (m_groups[(int)group]->getTutorialPopup())
        visible = m_groups[(int)group]->getTutorialPopup()->IsVisible();
    return visible;
}

void UIController::UpdatePlayerBasePositions() {
    Minecraft* pMinecraft = Minecraft::GetInstance();

    for (int idx = 0; idx < XUSER_MAX_COUNT; ++idx) {
        if (pMinecraft->localplayers[idx] != nullptr) {
            if (pMinecraft->localplayers[idx]->m_iScreenSection ==
                IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN) {
                DisplayGamertag(idx, false);
            } else {
                DisplayGamertag(idx, true);
            }
            m_groups[idx + 1]->SetViewportType(
                (IPlatformRenderer::eViewportType)pMinecraft->localplayers[idx]
                    ->m_iScreenSection);
        } else {
            m_groups[idx + 1]->SetViewportType(
                IPlatformRenderer::VIEWPORT_TYPE_FULLSCREEN);
            DisplayGamertag(idx, false);
        }
    }
}

void UIController::SetEmptyQuadrantLogo(int iSection) {}

void UIController::HideAllGameUIElements() {}

void UIController::ShowOtherPlayersBaseScene(unsigned int iPad, bool show) {}

void UIController::ShowTrialTimer(bool show) {
    if (m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay())
        m_groups[(int)eUIGroup_Fullscreen]
            ->getPressStartToPlay()
            ->showTrialTimer(show);
}

void UIController::SetTrialTimerLimitSecs(unsigned int uiSeconds) {
    UIController::m_dwTrialTimerLimitSecs = uiSeconds;
}

void UIController::UpdateTrialTimer(unsigned int iPad) {
    char wcTime[20];

    std::uint32_t timeTicks = (std::uint32_t)app.getTrialTimer();

    if (timeTicks > m_dwTrialTimerLimitSecs) {
        timeTicks = m_dwTrialTimerLimitSecs;
    }

    timeTicks = m_dwTrialTimerLimitSecs - timeTicks;

#if !defined(_CONTENT_PACKAGE)
    if (true)
#else
    if (timeTicks < 180)
#endif
    {
        int iMins = timeTicks / 60;
        int iSeconds = timeTicks % 60;
        snprintf(wcTime, 20, "%d:%02d", iMins, iSeconds);
        if (m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay())
            m_groups[(int)eUIGroup_Fullscreen]
                ->getPressStartToPlay()
                ->setTrialTimer(wcTime);
    } else {
        if (m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay())
            m_groups[(int)eUIGroup_Fullscreen]
                ->getPressStartToPlay()
                ->setTrialTimer("");
    }

    if (timeTicks == 0) {
        if (!ui.GetMenuDisplayed(iPad)) {
            ui.NavigateToScene(iPad, eUIScene_PauseMenu, nullptr,
                               eUILayer_Scene);

            app.SetAction(iPad, eAppAction_TrialOver);
        }
    }
}

void UIController::ReduceTrialTimerValue() {
    std::uint32_t timeTicks = (std::uint32_t)app.getTrialTimer();

    if (timeTicks > m_dwTrialTimerLimitSecs) {
        timeTicks = m_dwTrialTimerLimitSecs;
    }

    m_dwTrialTimerLimitSecs -= timeTicks;
}

void UIController::ShowAutosaveCountdownTimer(bool show) {
    if (m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay())
        m_groups[(int)eUIGroup_Fullscreen]
            ->getPressStartToPlay()
            ->showTrialTimer(show);
}

void UIController::UpdateAutosaveCountdownTimer(unsigned int uiSeconds) {
    char wcAutosaveCountdown[100];
    snprintf(wcAutosaveCountdown, 100, app.GetString(IDS_AUTOSAVE_COUNTDOWN),
             uiSeconds);
    if (m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay())
        m_groups[(int)eUIGroup_Fullscreen]
            ->getPressStartToPlay()
            ->setTrialTimer(wcAutosaveCountdown);
}

void UIController::ShowSavingMessage(unsigned int iPad,
                                     IPlatformStorage::ESavingMessage eVal) {
    bool show = false;
    switch (eVal) {
        case IPlatformStorage::ESavingMessage_None:
            show = false;
            break;
        case IPlatformStorage::ESavingMessage_Short:
        case IPlatformStorage::ESavingMessage_Long:
            show = true;
            break;
    }
    if (m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay())
        m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay()->showSaveIcon(
            show);
}

void UIController::ShowPlayerDisplayname(bool show) {
    if (m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay())
        m_groups[(int)eUIGroup_Fullscreen]
            ->getPressStartToPlay()
            ->showPlayerDisplayName(show);
}

void UIController::SetWinUserIndex(unsigned int iPad) { m_winUserIndex = iPad; }

unsigned int UIController::GetWinUserIndex() { return m_winUserIndex; }

void UIController::ShowUIDebugConsole(bool show) {
#if !defined(_CONTENT_PACKAGE)

    if (show) {
        m_uiDebugConsole =
            (UIComponent_DebugUIConsole*)m_groups[eUIGroup_Fullscreen]
                ->addComponent(0, eUIComponent_DebugUIConsole, eUILayer_Debug);
    } else {
        m_groups[eUIGroup_Fullscreen]->removeComponent(
            eUIComponent_DebugUIConsole, eUILayer_Debug);
        m_uiDebugConsole = nullptr;
    }
#endif
}

void UIController::ShowUIDebugMarketingGuide(bool show) {
#if !defined(_CONTENT_PACKAGE)

    if (show) {
        m_uiDebugMarketingGuide =
            (UIComponent_DebugUIMarketingGuide*)m_groups[eUIGroup_Fullscreen]
                ->addComponent(0, eUIComponent_DebugUIMarketingGuide,
                               eUILayer_Debug);
    } else {
        m_groups[eUIGroup_Fullscreen]->removeComponent(
            eUIComponent_DebugUIMarketingGuide, eUILayer_Debug);
        m_uiDebugMarketingGuide = nullptr;
    }
#endif
}

void UIController::logDebugString(const std::string& text) {
    if (m_uiDebugConsole) m_uiDebugConsole->addText(text);
}

bool UIController::PressStartPlaying(unsigned int iPad) {
    return m_iPressStartQuadrantsMask & (1 << iPad) ? true : false;
}

void UIController::ShowPressStart(unsigned int iPad) {
    m_iPressStartQuadrantsMask |= 1 << iPad;
    if (m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay())
        m_groups[(int)eUIGroup_Fullscreen]
            ->getPressStartToPlay()
            ->showPressStart(iPad, true);
}

void UIController::HidePressStart() {
    ClearPressStart();
    if (m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay())
        m_groups[(int)eUIGroup_Fullscreen]
            ->getPressStartToPlay()
            ->showPressStart(0, false);
}

void UIController::ClearPressStart() { m_iPressStartQuadrantsMask = 0; }

IPlatformStorage::EMessageResult UIController::RequestAlertMessage(
    unsigned int uiTitle, unsigned int uiText, unsigned int* uiOptionA,
    unsigned int uiOptionC, unsigned int dwPad,
    int (*Func)(void*, int, const IPlatformStorage::EMessageResult),
    void* lpParam, char* pwchFormatString) {
    return RequestMessageBox(uiTitle, uiText, uiOptionA, uiOptionC, dwPad, Func,
                             lpParam, pwchFormatString, 0, false);
}

IPlatformStorage::EMessageResult UIController::RequestErrorMessage(
    unsigned int uiTitle, unsigned int uiText, unsigned int* uiOptionA,
    unsigned int uiOptionC, unsigned int dwPad,
    int (*Func)(void*, int, const IPlatformStorage::EMessageResult),
    void* lpParam, char* pwchFormatString) {
    return RequestMessageBox(uiTitle, uiText, uiOptionA, uiOptionC, dwPad, Func,
                             lpParam, pwchFormatString, 0, true);
}

IPlatformStorage::EMessageResult UIController::RequestMessageBox(
    unsigned int uiTitle, unsigned int uiText, unsigned int* uiOptionA,
    unsigned int uiOptionC, unsigned int dwPad,
    int (*Func)(void*, int, const IPlatformStorage::EMessageResult),
    void* lpParam, char* pwchFormatString, unsigned int dwFocusButton,
    bool bIsError)

{
    MessageBoxInfo param;
    param.uiTitle = uiTitle;
    param.uiText = uiText;
    param.uiOptionA = uiOptionA;
    param.uiOptionC = uiOptionC;
    param.dwPad = dwPad;
    param.Func = Func;
    param.lpParam = lpParam;
    param.pwchFormatString = pwchFormatString;
    param.dwFocusButton = dwFocusButton;

    EUILayer layer = bIsError ? eUILayer_Error : eUILayer_Alert;

    bool completed = false;
    if (ui.IsReloadingSkin()) {
        QueuedMessageBoxData* queuedData = new QueuedMessageBoxData();
        queuedData->info = param;
        queuedData->info.uiOptionA = new unsigned int[param.uiOptionC];
        memcpy(queuedData->info.uiOptionA, param.uiOptionA,
               param.uiOptionC * sizeof(unsigned int));
        queuedData->iPad = dwPad;
        queuedData->layer =
            eUILayer_Error; 
        m_queuedMessageBoxData.push_back(queuedData);
    } else {
        completed = ui.NavigateToScene(dwPad, eUIScene_MessageBox, &param,
                                       layer, eUIGroup_Fullscreen);
    }

    if (completed) {
        return IPlatformStorage::EMessage_Pending;
    } else {
        return IPlatformStorage::EMessage_Busy;
    }
}

IPlatformStorage::EMessageResult UIController::RequestUGCMessageBox(
    int title /* = -1 */, int message /* = -1 */, int iPad /* = -1*/,
    int (*Func)(void*, int,
                const IPlatformStorage::EMessageResult) /* = nullptr*/,
    void* lpParam /* = nullptr*/) {
    if (title == -1) {
        title = IDS_FAILED_TO_CREATE_GAME_TITLE;
    }

    if (message == -1) {
        message = IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_CREATE;
    }

    if (iPad == -1) iPad = PlatformProfile.GetPrimaryPad();

    unsigned int uiIDA[1];
    uiIDA[0] = IDS_CONFIRM_OK;
    return ui.RequestAlertMessage(title, message, uiIDA, 1, iPad, Func,
                                  lpParam);
}

IPlatformStorage::EMessageResult
UIController::RequestContentRestrictedMessageBox(
    int title /* = -1 */, int message /* = -1 */, int iPad /* = -1*/,
    int (*Func)(void*, int,
                const IPlatformStorage::EMessageResult) /* = nullptr*/,
    void* lpParam /* = nullptr*/) {
    if (title == -1) {
        title = IDS_FAILED_TO_CREATE_GAME_TITLE;
    }

    if (message == -1) {
#if 1
        message = IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_CREATE;
#else
        message = IDS_CONTENT_RESTRICTION;
#endif
    }

    if (iPad == -1) iPad = PlatformProfile.GetPrimaryPad();

    unsigned int uiIDA[1];
    uiIDA[0] = IDS_CONFIRM_OK;
    return ui.RequestAlertMessage(title, message, uiIDA, 1, iPad, Func,
                                  lpParam);
}

void UIController::setFontCachingCalculationBuffer(int length) {
#if INTPTR_MAX == INT64_MAX
    static const int CHAR_SIZE = 24;
#else
    static const int CHAR_SIZE = 16;
#endif

    if (m_tempBuffer != nullptr) delete[] m_tempBuffer;
    if (length < 0) {
        if (m_defaultBuffer == nullptr) {
            m_defaultBuffer = new char[CHAR_SIZE * 5000];
            memset(m_defaultBuffer, 0, CHAR_SIZE * 5000);
        }
        IggySetFontCachingCalculationBuffer(5000, m_defaultBuffer,
                                            CHAR_SIZE * 5000);
    } else {
        m_tempBuffer = new char[CHAR_SIZE * length];
        memset(m_tempBuffer, 0, CHAR_SIZE * length);
        IggySetFontCachingCalculationBuffer(length, m_tempBuffer,
                                            CHAR_SIZE * length);
    }
}

UIScene* UIController::FindScene(EUIScene sceneType) {
    UIScene* pScene = nullptr;

    for (int i = 0; i < eUIGroup_COUNT; i++) {
        pScene = m_groups[i]->FindScene(sceneType);
        if (pScene != nullptr) return pScene;
    }

    return pScene;
}