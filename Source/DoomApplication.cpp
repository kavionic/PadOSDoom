// This file is part of PadOSDoom.
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <new>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <App/Application.h>
#include <GUI/Bitmap.h>
#include <GUI/View.h>
#include <PadOS/Time.h>
#include <System/AppDefinition.h>
#include <System/ExceptionHandling.h>
#include <Threads/EventTimer.h>

#include "DoomSession.h"
#include "doomgeneric.h"
#include "doomkeys.h"
#include "doomfeatures.h"

extern "C"
{
#include "i_system.h"
}

#ifdef FEATURE_SOUND
#error The initial PadOS port does not support FEATURE_SOUND.
#endif

namespace PadOSDoom
{

static constexpr float AUTO_DISPLAY_SCALE = 0.0f;

class DoomView : public PView
{
public:
    DoomView(Ptr<PBitmap> bitmap, float displayScale);
    void OnPaint(const PRect& updateRect) override;
    void OnPointerDown(
        PPointerID pointerID,
        const PPoint& position,
        const PPointerEvent& pointerEvent,
        PEventPhase phase) override;
    void OnKeyDown(PKeyCodes keyCode, const PString& text, const PKeyEvent& keyEvent) override;
    void OnKeyUp(PKeyCodes keyCode, const PString& text, const PKeyEvent& keyEvent) override;
    void OnKeyboardFocusChanged(bool hasFocus) override;
    void Present();
    int GetKey(int* pressed, unsigned char* key);

private:
    static unsigned char MapKey(PKeyCodes keyCode);
    void QueueKey(unsigned char key, bool pressed);
    PRect GetDisplayFrame() const;
    void DrawFrame();

    Ptr<PBitmap> m_Bitmap;
    std::deque<uint16_t> m_KeyEvents;
    float m_DisplayScale;
    std::array<bool, 256> m_KeyStates{};
    bool m_HasFrame = false;
};

class DoomApplication : public PApplication
{
public:
    DoomApplication(int argc, char** argv, float displayScale);
    ~DoomApplication() override;
    static DoomApplication* GetCurrent();

    void ThreadStarted() override;
    void ThreadStopping() override;
    void InitializeDisplay();
    void Present();
    void WaitForEvents(uint32_t milliseconds);
    uint32_t GetTicks() const;
    int GetKey(int* pressed, unsigned char* key);

private:
    void SlotFrame();

    TimeValNanos m_StartTime;
    PEventTimer m_FrameTimer;
    PEventTimer m_WakeupTimer;
    Ptr<DoomView> m_View;
    int m_Argc;
    char** m_Argv;
    float m_DisplayScale;
};

DoomView::DoomView(Ptr<PBitmap> bitmap, float displayScale)
    : PView("Doom", nullptr, PViewFlags::WillDraw | PViewFlags::FullUpdateOnResize),
      m_Bitmap(bitmap), m_DisplayScale(displayScale)
{
    SetEraseColor(0, 0, 0);
}

void DoomView::OnPaint(const PRect& updateRect)
{
    EraseRect(updateRect);
    DrawFrame();
}

void DoomView::OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& pointerEvent, PEventPhase phase)
{
    if (phase == PEventPhase::Target) {
        SetKeyboardFocus();
    }
}

void DoomView::OnKeyDown(PKeyCodes keyCode, const PString& text, const PKeyEvent& keyEvent)
{
    QueueKey(MapKey(keyCode), true);
}

void DoomView::OnKeyUp(PKeyCodes keyCode, const PString& text, const PKeyEvent& keyEvent)
{
    QueueKey(MapKey(keyCode), false);
}

void DoomView::OnKeyboardFocusChanged(bool hasFocus)
{
    if (!hasFocus) {
        for (size_t i = 0; i < m_KeyStates.size(); ++i) {
            QueueKey(static_cast<unsigned char>(i), false);
        }
    }
}

void DoomView::Present()
{
    m_HasFrame = true;
    DrawFrame();
}

int DoomView::GetKey(int* pressed, unsigned char* key)
{
    if (!m_KeyEvents.empty())
    {
        const uint16_t event = m_KeyEvents.front();
        m_KeyEvents.pop_front();
        *pressed = event >> 8;
        *key = static_cast<unsigned char>(event);
        return 1;
    }
    return 0;
}

unsigned char DoomView::MapKey(PKeyCodes keyCode)
{
    switch (keyCode)
    {
        case PKeyCodes::CURSOR_LEFT: return KEY_LEFTARROW;
        case PKeyCodes::CURSOR_RIGHT: return KEY_RIGHTARROW;
        case PKeyCodes::CURSOR_UP: return KEY_UPARROW;
        case PKeyCodes::CURSOR_DOWN: return KEY_DOWNARROW;
        case PKeyCodes::HOME: return KEY_HOME;
        case PKeyCodes::END: return KEY_END;
        case PKeyCodes::DELETE: return KEY_DEL;
        case PKeyCodes::BACKSPACE: return KEY_BACKSPACE;
        case PKeyCodes::TAB: return KEY_TAB;
        case PKeyCodes::ENTER: return KEY_ENTER;
        case PKeyCodes::SHIFT: return KEY_RSHIFT;
        case PKeyCodes::CTRL: return KEY_FIRE;
        case PKeyCodes::ALT: return KEY_RALT;
        case PKeyCodes::ESCAPE: return KEY_ESCAPE;
        case PKeyCodes::SPACE: return KEY_USE;
        default: break;
    }
    uint32_t character = std::to_underlying(keyCode);
    if (character >= 'A' && character <= 'Z') {
        character += 'a' - 'A';
    }
    return (character >= '!' && character <= '~') ? static_cast<unsigned char>(character) : 0;
}

void DoomView::QueueKey(unsigned char key, bool pressed)
{
    if (key != 0 && m_KeyStates[key] != pressed)
    {
        m_KeyEvents.push_back(static_cast<uint16_t>((uint16_t(pressed) << 8) | key));
        m_KeyStates[key] = pressed;
    }
}

PRect DoomView::GetDisplayFrame() const
{
    const PRect bounds = GetBounds();
    float scale = std::min(bounds.Width() / DOOMGENERIC_RESX, bounds.Height() / DOOMGENERIC_RESY);
    if (m_DisplayScale != AUTO_DISPLAY_SCALE) {
        scale = std::min(scale, m_DisplayScale);
    }
    const PPoint size = (PPoint(DOOMGENERIC_RESX, DOOMGENERIC_RESY) * scale).GetRounded();
    const PPoint position = ((bounds.Size() - size) * 0.5f).GetRounded() + bounds.TopLeft();
    return PRect(position, position + size);
}

void DoomView::DrawFrame()
{
    if (m_HasFrame)
    {
        const PRect destination = GetDisplayFrame();
        if (destination.IsValid())
        {
            const PRect source(0, 0, DOOMGENERIC_RESX, DOOMGENERIC_RESY);
            if (destination.Size() == source.Size()) {
                DrawBitmap(m_Bitmap, source, destination.TopLeft());
            } else {
                DrawBitmap(m_Bitmap, source, destination);
            }
            // Repainting also borrows the engine buffer until the server completes the draw.
            Sync();
        }
    }
}

DoomApplication::DoomApplication(int argc, char** argv, float displayScale)
    : PApplication("Doom"), m_StartTime(get_monotonic_time()), m_Argc(argc), m_Argv(argv), m_DisplayScale(displayScale)
{
    SetDeleteOnExit(false);
}

DoomApplication::~DoomApplication()
{
    CRITICAL_SCOPE(GetMutex());
    m_FrameTimer.Stop();
    m_WakeupTimer.Stop();
    if (m_View != nullptr)
    {
        RemoveView(m_View);
        m_View = nullptr;
    }
    // Complete server access to the borrowed raster before releasing it.
    Sync();
}

DoomApplication* DoomApplication::GetCurrent()
{
    return static_cast<DoomApplication*>(PThread::GetCurrentThread());
}

void DoomApplication::ThreadStarted()
{
    try
    {
        doomgeneric_Create(m_Argc, m_Argv);
        m_FrameTimer.Set(TimeValNanos::FromNanoseconds(1));
        m_FrameTimer.SignalTrigged.Connect(this, &DoomApplication::SlotFrame);
        if (!AddTimer(&m_FrameTimer, true)) {
            throw std::bad_alloc();
        }
    }
    catch (const std::exception& exception)
    {
        std::fprintf(stderr, "Doom: %s\n", exception.what());
        std::exit(EXIT_FAILURE);
    }
}

void DoomApplication::ThreadStopping()
{
    m_FrameTimer.Stop();
    I_Quit();
}

void DoomApplication::InitializeDisplay()
{
    if (DG_ScreenBuffer == nullptr) {
        throw std::bad_alloc();
    }
    Ptr<PBitmap> bitmap = ptr_new<PBitmap>(
        DOOMGENERIC_RESX,
        DOOMGENERIC_RESY,
        PEColorSpace::RGB16,
        DG_ScreenBuffer,
        DOOMGENERIC_RESX * sizeof(pixel_t),
        0,
        this);
    if (!bitmap->IsValid()) {
        throw std::runtime_error("Unable to create the Doom bitmap.");
    }
    m_View = ptr_new<DoomView>(bitmap, m_DisplayScale);
    m_View->SetFrame(PRect(0, 0, DOOMGENERIC_RESX, DOOMGENERIC_RESY));
    if (!AddView(m_View, PViewDockType::DockedWindow)) {
        throw std::runtime_error("Unable to attach the Doom view.");
    }
    m_View->SetKeyboardFocus();
    Sync();
}

void DoomApplication::Present()
{
    m_View->Present();
}

void DoomApplication::WaitForEvents(uint32_t milliseconds)
{
    m_WakeupTimer.Set(TimeValNanos::FromMilliseconds(milliseconds), true);
    m_WakeupTimer.Start(true, this);
    do {
        thread_testcancel();
        if (!Tick()) {
            I_Quit();
        }
    } while (m_WakeupTimer.IsRunning());
}

uint32_t DoomApplication::GetTicks() const
{
    return static_cast<uint32_t>((get_monotonic_time() - m_StartTime).AsMilliseconds());
}

int DoomApplication::GetKey(int* pressed, unsigned char* key)
{
    return m_View->GetKey(pressed, key);
}

void DoomApplication::SlotFrame()
{
    try
    {
        thread_testcancel();
        // The single-shot timer stays disarmed while Doom processes events inside DG_SleepMs().
        doomgeneric_Tick();
        if (!AddTimer(&m_FrameTimer, true)) {
            throw std::bad_alloc();
        }
    }
    catch (const std::exception& exception)
    {
        std::fprintf(stderr, "Doom: %s\n", exception.what());
        std::exit(EXIT_FAILURE);
    }
}

static bool ParseDisplayScale(int argc, char* argv[], float& displayScale)
{
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "-scale") == 0)
        {
            ++i;
            const std::string_view value = (i < argc) ? argv[i] : "";
            if (value == "auto")
            {
                displayScale = AUTO_DISPLAY_SCALE;
            }
            else
            {
                // Exclude NaN and infinity without relying on floating-point checks under -ffast-math.
                const bool hasNumericPrefix = !value.empty() &&
                    ((value.front() >= '0' && value.front() <= '9') || value.front() == '.');
                const std::from_chars_result result = std::from_chars(value.data(), value.data() + value.size(), displayScale);
                if (!hasNumericPrefix || result.ec != std::errc() || result.ptr != value.data() + value.size() ||
                    displayScale <= AUTO_DISPLAY_SCALE)
                {
                    std::fprintf(stderr, "Doom: -scale expects 'auto' or a positive number.\n");
                    return false;
                }
            }
        }
    }
    return true;
}

static int Main(int argc, char* argv[])
{
    float displayScale = AUTO_DISPLAY_SCALE;
    if (!ParseDisplayScale(argc, argv, displayScale)) {
        return EXIT_FAILURE;
    }
    static std::atomic_flag isRunning = ATOMIC_FLAG_INIT;
    if (isRunning.test_and_set(std::memory_order_acquire))
    {
        std::fprintf(stderr, "Doom is already running. Quit the current session before launching another.\n");
        return EXIT_FAILURE;
    }
    PScopeExit releaseSession([]() { isRunning.clear(std::memory_order_release); });
    DoomSession session;
    DoomApplication application(argc, argv, displayScale);
    application.Adopt();
    return EXIT_FAILURE;
}

static constexpr size_t APP_STACK_SIZE = 64 * 1024;
static PAppDefinition g_AppDefinition("doom", "Doom for PadOS (without sound)", Main, APP_STACK_SIZE);

}

void DG_Init()
{
    PadOSDoom::DoomApplication::GetCurrent()->InitializeDisplay();
}

void DG_DrawFrame()
{
    PadOSDoom::DoomApplication::GetCurrent()->Present();
}

void DG_SleepMs(uint32_t milliseconds)
{
    PadOSDoom::DoomApplication::GetCurrent()->WaitForEvents(milliseconds);
}

uint32_t DG_GetTicksMs()
{
    return PadOSDoom::DoomApplication::GetCurrent()->GetTicks();
}

int DG_GetKey(int* pressed, unsigned char* key)
{
    return PadOSDoom::DoomApplication::GetCurrent()->GetKey(pressed, key);
}

void DG_SetWindowTitle(const char* title)
{
    // PadOS currently exposes no window-title update in the application protocol.
}
