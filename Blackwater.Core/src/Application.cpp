#include "Blackwater/Application.h"

#include <chrono>

namespace
{
    /// Longest frame the accumulator will accept, in seconds.
    ///
    /// Anything slower than this is treated as a stall rather than as real
    /// elapsed time. Without the clamp, pausing on a breakpoint for ten
    /// seconds would demand 600 catch-up updates in a single frame; those
    /// take longer than a frame to run, so the next frame is worse, and the
    /// application never recovers. That is the "spiral of death", and this
    /// single line is the whole defence against it.
    constexpr double kMaxFrameSeconds = 0.25;
}

namespace bw
{
    Application::Application(const Config& config)
        : _config(config)
        , _window(config.title, config.width, config.height)
        , _graphics(_window.Handle(), _window.Width(), _window.Height())
    {
        // Note this reads from `config`, the parameter, rather than from
        // _config. Both work here, but depending on _config would mean
        // depending on it having been initialised first -- a fragile thing to
        // rely on when someone later reorders the members.
    }

    int Application::Run()
    {
        // steady_clock, not system_clock: it is monotonic, so changing the
        // system time (or a DST rollover) cannot make it run backwards and
        // hand us a negative frame time. MSVC implements it on
        // QueryPerformanceCounter, so the precision is the same as doing that
        // by hand, without the boilerplate.
        using Clock = std::chrono::steady_clock;

        const double fixedDelta = 1.0 / _config.updatesPerSecond;

        double accumulator = 0.0;
        auto   previous    = Clock::now();

        while (_window.PumpMessages())
        {
            const auto current = Clock::now();

            // duration<double> converts the clock's integer ticks into
            // seconds without the truncation that duration_cast would apply.
            double frameSeconds =
                std::chrono::duration<double>(current - previous).count();

            previous = current;

            if (frameSeconds > kMaxFrameSeconds)
            {
                frameSeconds = kMaxFrameSeconds;
            }

            // -------------------------------------------------------------
            // Simulate.
            //
            // Real elapsed time goes into the accumulator; whole fixed steps
            // come out. Depending on how long the last frame took this runs
            // zero times (fast frame), once (typical), or several times
            // (after a hitch) -- but every call sees exactly the same delta.
            // -------------------------------------------------------------
            accumulator += frameSeconds;

            while (accumulator >= fixedDelta)
            {
                OnUpdate(fixedDelta);

                // Clear "pressed this step" flags and deltas *per update*, not
                // per frame. A frame that runs zero updates keeps them pending
                // for the next one, so no click is lost; a frame that runs
                // several hands the click to the first update only.
                _window.GetInput().EndUpdate();

                accumulator -= fixedDelta;
            }

            // Resize between simulation and rendering, so the frame we are
            // about to draw already targets the new backbuffer size.
            if (_window.ConsumeResized())
            {
                _graphics.Resize(_window.Width(), _window.Height());
            }

            // A minimised window has no drawable surface. Keep simulating,
            // but skip presenting and yield rather than spinning: with vsync
            // gone there is nothing left to pace the loop.
            if (_window.IsMinimized())
            {
                ::Sleep(1);   // PORT(M5): std::this_thread::sleep_for -- Win32-only today
                continue;
            }

            // -------------------------------------------------------------
            // Render.
            //
            // Whatever time is left in the accumulator is the fraction of a
            // step we are standing past the last simulation state. Rendering
            // positions interpolated by alpha is what stops a 60 Hz
            // simulation from looking stepped on a 144 Hz display.
            // -------------------------------------------------------------
            const double alpha = accumulator / fixedDelta;

            _graphics.Clear(_config.clearColour);
            OnRender(alpha);
            _graphics.Present(_config.vsync);
        }

        // PumpMessages returned false, meaning WM_QUIT. Everything unwinds
        // from here: _graphics then _window, in reverse declaration order,
        // each releasing what it owns.
        return 0;
    }
}
