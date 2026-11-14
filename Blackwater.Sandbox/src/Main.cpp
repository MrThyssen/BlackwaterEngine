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
        XMFLOAT3 normal;
        XMFLOAT4 colour;
    };

    // PORT(M5): the one D3D type left in game-side code. Becomes an engine
    // vertex-layout description that each backend translates.
    const D3D11_INPUT_ELEMENT_DESC kInputLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,
          offsetof(Vertex, position), D3D11_INPUT_PER_VERTEX_DATA, 0 },

        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0,
          offsetof(Vertex, normal),   D3D11_INPUT_PER_VERTEX_DATA, 0 },

        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
          offsetof(Vertex, colour),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    // ----------------------------------------------------------------------
    // Constant buffer payloads. Both are multiples of 16 bytes, which
    // ConstantBuffer<T> asserts on.
    // ----------------------------------------------------------------------

    // 128 bytes. World alone is needed to turn normals into world space;
    // the combined matrix still does the positions.
    struct Transforms
    {
        XMFLOAT4X4 worldViewProjection;
        XMFLOAT4X4 world;
    };

    // 16 bytes: HLSL packs a float3 and a following float into one 16-byte
    // row, so this struct matches the cbuffer with no padding.
    struct Lighting
    {
        XMFLOAT3 directionToLight;   // unit vector, pointing at the sun
        float    ambient;
    };

    // The camera looks from the -X, +Y, -Z side, so the top, -X and -Z faces
    // are the visible ones. A sun high up and towards -X lights them roughly
    // 100% / 70% / 50% -- three clearly different shades.
    constexpr XMFLOAT3 kDirectionToLight = { -0.6f, 1.0f, -0.35f };   // normalised at upload
    constexpr float    kAmbient          = 0.2f;

    const XMFLOAT4 kCubeColour = { 0.85f, 0.50f, 0.32f, 1.0f };

    //
    // A cube spanning -1..+1 with 24 vertices: 4 per face.
    //
    // A corner is shared by three faces, and each of those faces needs its own
    // normal there. A vertex holds one normal, so each corner is stored three
    // times.
    //
    bw::Mesh MakeCube(ID3D11Device* device)
    {
        std::vector<Vertex>   vertices;
        std::vector<uint32_t> indices;

        // One face: its outward normal n, plus two edge directions u and v
        // chosen so that cross(u, v) == n. Corners a, b, c, d walk the
        // square in an order that makes cross(b - a, c - a) point along n --
        // which is clockwise when viewed from outside, D3D11's front face.
        // Get it backwards and back-face culling makes the face vanish.
        const auto addFace = [&](XMFLOAT3 n, XMFLOAT3 u, XMFLOAT3 v)
        {
            const auto corner = [&](float su, float sv)
            {
                return XMFLOAT3{ n.x + su * u.x + sv * v.x,
                                 n.y + su * u.y + sv * v.y,
                                 n.z + su * u.z + sv * v.z };
            };

            const auto first = static_cast<uint32_t>(vertices.size());

            vertices.push_back({ corner(-1.0f, -1.0f), n, kCubeColour });   // a
            vertices.push_back({ corner( 1.0f, -1.0f), n, kCubeColour });   // b
            vertices.push_back({ corner( 1.0f,  1.0f), n, kCubeColour });   // c
            vertices.push_back({ corner(-1.0f,  1.0f), n, kCubeColour });   // d

            // Two triangles: a-b-c and a-c-d.
            indices.insert(indices.end(), { first, first + 1, first + 2,
                                            first, first + 2, first + 3 });
        };

        constexpr XMFLOAT3 x{ 1, 0, 0 }, y{ 0, 1, 0 }, z{ 0, 0, 1 };
        constexpr XMFLOAT3 nx{ -1, 0, 0 }, ny{ 0, -1, 0 }, nz{ 0, 0, -1 };

        addFace(y,  z, x);   // top     cross(z, x) =  y
        addFace(ny, x, z);   // bottom  cross(x, z) = -y
        addFace(x,  y, z);   // right   cross(y, z) =  x
        addFace(nx, z, y);   // left    cross(z, y) = -x
        addFace(z,  x, y);   // back    cross(x, y) =  z
        addFace(nz, y, x);   // front   cross(y, x) = -z

        return bw::Mesh(device,
                        vertices.data(), vertices.size(), sizeof(Vertex),
                        indices);
    }

    constexpr float kWalkSpeed = 12.0f;   // world units per second

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

        // Lines have no surface, so the normal is a placeholder: the grid is
        // drawn with the unlit pixel shader, which ignores it.
        constexpr XMFLOAT3 up{ 0.0f, 1.0f, 0.0f };

        const auto addLine = [&](XMFLOAT3 from, XMFLOAT3 to, XMFLOAT4 colour)
        {
            indices.push_back(static_cast<uint32_t>(vertices.size()));
            vertices.push_back({ from, up, colour });
            indices.push_back(static_cast<uint32_t>(vertices.size()));
            vertices.push_back({ to, up, colour });
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
            //
            // Two shaders from one file: same vertex shader, different pixel
            // shader. LearnOpenGL does the same for its lamp cube.
            , _litShader(GetGraphics().Device(), L"shaders\\Basic.hlsl",
                          "VSMain", "PSLit", kInputLayout)
            , _unlitShader(GetGraphics().Device(), L"shaders\\Basic.hlsl",
                            "VSMain", "PSUnlit", kInputLayout)
            , _cube(MakeCube(GetGraphics().Device()))
            , _grid(MakeGrid(GetGraphics().Device()))
            , _transforms(GetGraphics().Device())
            , _lighting(GetGraphics().Device())
        {
            // The light never changes, so upload it once. Normalised here
            // rather than in the shader: once on the CPU beats once per pixel.
            Lighting lighting{};
            XMStoreFloat3(&lighting.directionToLight,
                          XMVector3Normalize(XMLoadFloat3(&kDirectionToLight)));
            lighting.ambient = kAmbient;

            _lighting.Update(GetGraphics().Context(), lighting);
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

            // Shared by every draw below. Only the transforms buffer's
            // *contents* change per draw.
            auto* context = graphics.Context();
            _transforms.BindToVertexStage(context, 0);
            _lighting.BindToPixelStage(context, 1);

            // Ground: unlit. Its vertices are already in world space.
            _unlitShader.Bind(context);
            DrawMesh(_grid, XMMatrixIdentity(), viewProjection);

            // Everything else is lit.
            _litShader.Bind(context);

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
            XMStoreFloat4x4(&transforms.world, XMMatrixTranspose(world));

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
        bw::Shader                     _litShader;
        bw::Shader                     _unlitShader;
        bw::Mesh                       _cube;
        bw::Mesh                       _grid;
        bw::ConstantBuffer<Transforms> _transforms;
        bw::ConstantBuffer<Lighting>   _lighting;

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
