#pragma once

//
// The fixed isometric camera: a locked angle and an orthographic projection.
// No rotation, no zoom -- so nothing to interpolate.
//

// PORT(M5): DirectXMath supports ARM, but needs verifying with clang on macOS.
// If it fights us there, swap for a portable math library -- Camera's API uses
// plain floats and XMFLOAT3 storage, so the blast radius is small.
#include <DirectXMath.h>

namespace bw
{
    /// A half-line in world space. direction is unit length.
    struct Ray
    {
        DirectX::XMFLOAT3 origin;
        DirectX::XMFLOAT3 direction;
    };

    /// Intersects a ray with the horizontal plane y = height.
    ///
    /// Returns false when the ray runs parallel to the plane or points away
    /// from it -- clicking the sky, in practice.
    [[nodiscard]] bool IntersectGroundPlane(const Ray& ray, float height,
                                            DirectX::XMFLOAT3& hit) noexcept;

    class Camera final
    {
    public:
        struct Settings
        {
            /// 45 degrees looks diagonally across the grid -- the classic
            /// isometric diamond.
            float yawRadians   = DirectX::XMConvertToRadians(45.0f);

            /// Downward tilt. True isometric is 35.26 degrees; games often use
            /// ~30, which flattens the view slightly. Tuned by eye.
            float pitchRadians = DirectX::XMConvertToRadians(30.0f);

            /// How much of the world is visible vertically, in world units.
            /// Visible width follows from the window's aspect ratio, so a wider
            /// window shows more world instead of stretching it.
            float viewHeight   = 18.0f;

            /// Orthographic, so distance changes nothing on screen -- it only
            /// has to keep the scene between the near and far planes.
            float distance     = 50.0f;
            float nearPlane    = 0.1f;
            float farPlane     = 200.0f;
        };

        explicit Camera(const Settings& settings = Settings{});

        /// XM_CALLCONV selects __vectorcall on x64, so XMMATRIX values travel
        /// in SIMD registers instead of through memory.
        [[nodiscard]] DirectX::XMMATRIX XM_CALLCONV View() const noexcept;
        [[nodiscard]] DirectX::XMMATRIX XM_CALLCONV Projection(float aspectRatio) const noexcept;

        /// The world-space ray under a pixel. With an orthographic projection
        /// every such ray points the same way -- only its origin moves.
        [[nodiscard]] Ray ScreenToWorldRay(float pixelX, float pixelY,
                                           float viewportWidth,
                                           float viewportHeight) const noexcept;

    private:
        Settings _settings;

        // Storage type, not a SIMD type: XMFLOAT3 is three plain floats,
        // XMVECTOR is a 16-byte-aligned register type for computation.
        DirectX::XMFLOAT3 _target{ 0.0f, 0.0f, 0.0f };
    };
}
