#include <Blackwater/Application.h>
#include <Blackwater/Camera.h>
#include <Blackwater/Check.h>
#include <Blackwater/ConstantBuffer.h>
#include <Blackwater/Mesh.h>
#include <Blackwater/Shader.h>

#include <DirectXMath.h>
#include <windows.h>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <iterator>
#include <vector>

namespace
{
    using namespace DirectX;

    // ----------------------------------------------------------------------
    // Vertex format
    //
    // This struct and kInputLayout below must agree exactly. If they drift,
    // CreateInputLayout fails at startup rather than drawing nonsense -- one
    // of the few mistakes D3D11 catches for you.
    // ----------------------------------------------------------------------
    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT4 colour;
    };

    // PORT(M5): the one D3D type left in game-side code. Becomes an engine
    // vertex-layout description that each backend translates.
    const D3D11_INPUT_ELEMENT_DESC kInputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,
          offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },

        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
          offsetof(Vertex, colour),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    // ----------------------------------------------------------------------
    // Constant buffer payload
    //
    // 64 bytes -- one 4x4 matrix -- so it already satisfies the multiple-of-16
    // rule that ConstantBuffer<T> asserts on.
    // ----------------------------------------------------------------------
    struct Transforms
    {
        XMFLOAT4X4 worldViewProjection;
    };

    // ----------------------------------------------------------------------
    // A unit cube: eight corners, each a different colour, so the rasterizer's
    // interpolation is obvious.
    //
    //   0..3 = top    (y = +1)
    //   4..7 = bottom (y = -1)
    // ----------------------------------------------------------------------
    const Vertex kVertices[] =
    {
        { { -1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
        { {  1.0f,  1.0f, -1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
        { {  1.0f,  1.0f,  1.0f }, { 0.0f, 1.0f, 1.0f, 1.0f } },
        { { -1.0f,  1.0f,  1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
        { { -1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f, 1.0f, 1.0f } },
        { {  1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f, 0.0f, 1.0f } },
        { {  1.0f, -1.0f,  1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
        { { -1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f, 0.0f, 1.0f } },
    };

    // Winding order matters. D3D11's default rasterizer culls back faces and
    // treats CLOCKWISE (in screen space) as front-facing. Reverse a triangle
    // here and that face simply vanishes -- a "hole" in the cube that looks
    // like a depth bug but is not.
    const uint32_t kIndices[] =
    {
        3, 1, 0,   2, 1, 3,     // top
        0, 5, 4,   1, 5, 0,     // front
        3, 4, 7,   0, 4, 3,     // left
        1, 6, 5,   2, 6, 1,     // right
        2, 7, 6,   3, 7, 2,     // back
        6, 4, 5,   7, 4, 6,     // bottom
    };

    constexpr float kWalkSpeed = 4.0f;   // world units per second

    // ----------------------------------------------------------------------
    // Ground grid
    // ----------------------------------------------------------------------
    constexpr int   kGridHalfExtent = 10;   // lines from -10 to +10
    constexpr float kGridY          = 0.0f;

    const XMFLOAT4 kGridColour  = { 0.22f, 0.26f, 0.30f, 1.0f };
    const XMFLOAT4 kAxisXColour = { 0.60f, 0.22f, 0.22f, 1.0f };   // reddish: the X axis
    const XMFLOAT4 kAxisZColour = { 0.22f, 0.32f, 0.60f, 1.0f };   // bluish:  the Z axis

    //
    // Builds the grid as a LINELIST: every two indices are one segment.
    //
    // Returns a Mesh by value even though Mesh is non-copyable and
    // non-movable. Since C++17, `return Mesh(...)` does not create a temporary
    // to be copied or moved -- the object is constructed directly in the
    // caller's storage ("guaranteed copy elision"). That is what lets a
    // factory function produce an immovable type.
    //
    bw::Mesh MakeGrid(ID3D11Device* device)
    {
        std::vector<Vertex>   vertices;
        std::vector<uint32_t> indices;

        const auto addLine = [&](XMFLOAT3 from, XMFLOAT3 to, XMFLOAT4 colour)
        {
            indices.push_back(static_cast<uint32_t>(vertices.size()));
            vertices.push_back({ from, colour });
            indices.push_back(static_cast<uint32_t>(vertices.size()));
            vertices.push_back({ to, colour });
        };

        const auto extent = static_cast<float>(kGridHalfExtent);

        for (int i = -kGridHalfExtent; i <= kGridHalfExtent; ++i)
        {
            const auto offset = static_cast<float>(i);

            // Line running along X, at z = offset. The one through the origin
            // is the X axis.
            addLine({ -extent, kGridY, offset }, { extent, kGridY, offset },
                    i == 0 ? kAxisXColour : kGridColour);

            // Line running along Z, at x = offset. The one through the origin
            // is the Z axis.
            addLine({ offset, kGridY, -extent }, { offset, kGridY, extent },
                    i == 0 ? kAxisZColour : kGridColour);
        }

        // The vectors die when this function returns. That is fine: Mesh
        // copied their contents into IMMUTABLE GPU buffers in its constructor.
        return bw::Mesh(device,
                        vertices.data(), vertices.size(), sizeof(Vertex),
                        indices,
                        D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    }

    constexpr float kCubeHalfSize   = 1.0f;    // the cube mesh spans -1..+1
    constexpr float kMarkerScale    = 0.25f;

    //
    // M3: a fixed isometric camera over a ground grid. Right-click the ground and
    // the cube walks there; the marker shows where it is going.
    //
    class SandboxApp final : public bw::Application
    {
    public:
        explicit SandboxApp(const Config& config)
            : bw::Application(config)
            // Base classes are fully constructed before derived members are,
            // so GetGraphics() is already valid here. That is what lets these
            // be plain members rather than unique_ptrs, despite being
            // non-movable.
            , _shader(GetGraphics().Device(), L"shaders\\Basic.hlsl",
                       "VSMain", "PSMain", kInputLayout)
            , _cube(GetGraphics().Device(),
                     kVertices, std::size(kVertices), sizeof(Vertex),
                     kIndices)
            , _grid(MakeGrid(GetGraphics().Device()))
            , _transforms(GetGraphics().Device())
        {
        }

    protected:
        void OnUpdate(double fixedDeltaSeconds) override
        {
            // Click first, then walk: a new click redirects the cube in the
            // same step it arrives.
            SetTargetOnRightClick();
            Walk(static_cast<float>(fixedDeltaSeconds));

            ++_updates;
            _secondAccumulator += fixedDeltaSeconds;

            if (_secondAccumulator >= 1.0)
            {
                wchar_t title[160];
                ::swprintf_s(title,
                             L"Blackwater - M3   |   %u fps   |   %u ups   |   debug layer: %s",
                             _frames, _updates,
                             GetGraphics().IsDebugLayerActive() ? L"ON" : L"off");

                GetWindow().SetTitle(title);

                _secondAccumulator -= 1.0;
                _frames  = 0;
                _updates = 0;
            }
        }

        void OnRender(double alpha) override
        {
            ++_frames;

            // Draw the cube *between* its last two simulation positions, so
            // walking stays smooth at any refresh rate. Drawing _position
            // directly would judder. This interpolated value is for drawing
            // only -- the simulation never reads it.
            const auto t = static_cast<float>(alpha);
            const float drawX = _previousPosition.x + (_position.x - _previousPosition.x) * t;
            const float drawZ = _previousPosition.z + (_position.z - _previousPosition.z) * t;

            auto& graphics = GetGraphics();

            const float aspect =
                static_cast<float>(graphics.Width()) / static_cast<float>(graphics.Height());

            // The camera never moves, so it needs no alpha. View and projection
            // are the same for every object this frame, so combine them once.
            const XMMATRIX viewProjection = _camera.View() * _camera.Projection(aspect);

            // Shared by every draw below: one shader, one constant buffer
            // slot. Only the buffer's *contents* change per draw.
            auto* context = graphics.Context();
            _shader.Bind(context);
            _transforms.BindToVertexStage(context, 0);

            // Ground: the grid's vertices are already in world space.
            DrawMesh(_grid, XMMatrixIdentity(), viewProjection);

            // Cube: lifted by its half-height so it rests on the ground.
            DrawMesh(_cube, XMMatrixTranslation(drawX, kCubeHalfSize, drawZ), viewProjection);

            // Marker: the same cube mesh, scaled down, at the destination.
            // It doesn't move continuously -- it jumps -- so it needs no alpha.
            if (_walking)
            {
                DrawMesh(_cube,
                         XMMatrixScaling(kMarkerScale, kMarkerScale, kMarkerScale) *
                         XMMatrixTranslation(_target.x,
                                             _target.y + kCubeHalfSize * kMarkerScale,
                                             _target.z),
                         viewProjection);
            }
        }

    private:
        //
        // Uploads one object's transform and draws it.
        //
        // Rewriting the same constant buffer before every draw is safe because
        // ConstantBuffer::Update maps with WRITE_DISCARD: each Map hands back
        // fresh memory ("renaming"), so a draw already queued keeps reading
        // the matrix it was issued with.
        //
        // FXMMATRIX / CXMMATRIX are DirectXMath's parameter types for
        // XM_CALLCONV functions: the first matrix travels in registers, later
        // ones by reference. Using them is the idiom; the compiler would
        // accept plain `const XMMATRIX&` too.
        //
        void XM_CALLCONV DrawMesh(const bw::Mesh& mesh, FXMMATRIX world, CXMMATRIX viewProjection)
        {
            // Row-vector convention: vertex -> world -> view -> clip, so the
            // matrices multiply in that order. Transposed because HLSL packs
            // float4x4 column-major while DirectXMath stores row-major.
            Transforms transforms{};
            XMStoreFloat4x4(&transforms.worldViewProjection,
                            XMMatrixTranspose(world * viewProjection));

            auto* context = GetGraphics().Context();
            _transforms.Update(context, transforms);
            mesh.Draw(context);
        }

        //
        // Right-click: cast a ray from the cursor; where it meets the ground
        // becomes the cube's new destination.
        //
        void SetTargetOnRightClick()
        {
            const bw::Input& input = GetWindow().GetInput();

            // WasButtonPressed, not IsButtonDown: one click, one placement.
            // Holding the button should not re-place it every step.
            if (!input.WasButtonPressed(bw::MouseButton::Right))
            {
                return;
            }

            const auto& graphics = GetGraphics();

            // Mouse coordinates are client-area pixels and the viewport covers
            // the whole client area, so the two line up directly.
            const bw::Ray ray = _camera.ScreenToWorldRay(
                static_cast<float>(input.MouseX()),
                static_cast<float>(input.MouseY()),
                static_cast<float>(graphics.Width()),
                static_cast<float>(graphics.Height()));

            XMFLOAT3 hit{};
            if (bw::IntersectGroundPlane(ray, kGridY, hit))
            {
                _target  = hit;
                _walking = true;
            }
            // No hit -- the ray points at the sky. Keep doing whatever we were.
        }

        //
        // One simulation step of walking: a straight line at constant speed.
        //
        void Walk(float deltaSeconds)
        {
            // Every step, even when standing still. Skipping it while idle
            // would leave a stale previous position, and the first frame after
            // stopping would interpolate from it -- a visible twitch backwards.
            _previousPosition = _position;

            if (!_walking)
            {
                return;
            }

            const float dx       = _target.x - _position.x;
            const float dz       = _target.z - _position.z;
            const float distance = std::sqrt(dx * dx + dz * dz);
            const float step     = kWalkSpeed * deltaSeconds;

            // Closer than one step: snap onto the target and stop. Without the
            // snap the cube would overshoot, turn back, and jitter around the
            // target forever.
            if (distance <= step)
            {
                _position = _target;
                _walking  = false;
                return;
            }

            // dx/distance, dz/distance is the unit direction; times step is
            // how far one simulation step moves.
            _position.x += dx / distance * step;
            _position.z += dz / distance * step;
        }

        bw::Camera                     _camera;   // fixed isometric view
        bw::Shader                     _shader;
        bw::Mesh                       _cube;
        bw::Mesh                       _grid;
        bw::ConstantBuffer<Transforms> _transforms;

        // Simulation state, on the ground (y = 0). OnRender draws between
        // _previousPosition and _position; nothing else reads them that way.
        XMFLOAT3 _position{};
        XMFLOAT3 _previousPosition{};
        XMFLOAT3 _target{};
        bool     _walking = false;

        double   _secondAccumulator = 0.0;
        uint32_t _frames            = 0;
        uint32_t _updates           = 0;
    };
}

// PORT(M5): wWinMain and MessageBoxA are Win32. SDL3 supplies a portable main()
// and SDL_ShowSimpleMessageBox for the error reports below.
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    try
    {
        SandboxApp app(bw::Application::Config{
            .title  = L"Blackwater - M3",
            .width  = 1280,
            .height = 720,
        });

        return app.Run();
    }
    catch (const bw::HResultError& e)
    {
        ::MessageBoxA(nullptr, e.what(), "Blackwater - HRESULT failure",
                      MB_OK | MB_ICONERROR);
        return 1;
    }
    catch (const std::exception& e)
    {
        // HLSL compile errors arrive here, carrying the compiler's own
        // diagnostics with line numbers.
        ::MessageBoxA(nullptr, e.what(), "Blackwater - fatal error",
                      MB_OK | MB_ICONERROR);
        return 1;
    }
}
