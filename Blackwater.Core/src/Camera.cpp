#include "Blackwater/Camera.h"

#include <cmath>

using namespace DirectX;   // fine in a .cpp -- never in a header

namespace bw
{
    bool IntersectGroundPlane(const Ray& ray, float height, XMFLOAT3& hit) noexcept
    {
        // A point on the ray is origin + t * direction. Solve for the t at
        // which its y equals the plane height:
        //
        //     t = (height - origin.y) / direction.y
        //
        // A near-zero direction.y means the ray is (almost) parallel to the
        // ground and would hit it absurdly far away, or never.
        constexpr float kParallelEpsilon = 1e-6f;

        if (std::fabs(ray.direction.y) < kParallelEpsilon)
        {
            return false;
        }

        const float t = (height - ray.origin.y) / ray.direction.y;

        // Negative t means the plane is *behind* the ray's origin.
        if (t < 0.0f)
        {
            return false;
        }

        hit.x = ray.origin.x + t * ray.direction.x;
        hit.y = height;
        hit.z = ray.origin.z + t * ray.direction.z;
        return true;
    }

    Camera::Camera(const Settings& settings)
        : _settings(settings)
    {
    }

    XMMATRIX XM_CALLCONV Camera::View() const noexcept
    {
        // Spherical coordinates around the target, left-handed (+Z forward).
        // Yaw 0 would put the eye on the -Z side; 45 degrees swings it to the
        // diagonal, so the grid appears as a diamond.
        const float yaw        = _settings.yawRadians;
        const float pitch      = _settings.pitchRadians;
        const float distance   = _settings.distance;
        const float horizontal = distance * std::cos(pitch);

        const XMVECTOR target = XMLoadFloat3(&_target);

        const XMVECTOR eye = XMVectorAdd(target, XMVectorSet(
            -horizontal * std::sin(yaw),
             distance   * std::sin(pitch),
            -horizontal * std::cos(yaw),
             0.0f));

        return XMMatrixLookAtLH(eye, target, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
    }

    XMMATRIX XM_CALLCONV Camera::Projection(float aspectRatio) const noexcept
    {
        // Orthographic: a box, not a pyramid. Size is in world units, so an
        // object's on-screen size no longer depends on its distance -- which
        // is what makes isometric look isometric.
        return XMMatrixOrthographicLH(_settings.viewHeight * aspectRatio,
                                      _settings.viewHeight,
                                      _settings.nearPlane,
                                      _settings.farPlane);
    }

    Ray Camera::ScreenToWorldRay(float pixelX, float pixelY,
                                 float viewportWidth,
                                 float viewportHeight) const noexcept
    {
        const XMMATRIX view       = View();
        const XMMATRIX projection = Projection(viewportWidth / viewportHeight);

        // Unproject runs the rendering transform backwards: pixel -> NDC ->
        // inverse projection -> inverse view. Doing it at depth 0 (near plane)
        // and depth 1 (far plane) gives two world points; everything under
        // this pixel lies on the line between them.
        //
        // The same code works for perspective and orthographic projections:
        // with orthographic the near and far points sit directly "behind" each
        // other, so every ray comes out parallel.
        const auto unproject = [&](float depth)
        {
            return XMVector3Unproject(
                XMVectorSet(pixelX, pixelY, depth, 0.0f),
                0.0f, 0.0f, viewportWidth, viewportHeight,
                0.0f, 1.0f,
                projection, view, XMMatrixIdentity());
        };

        const XMVECTOR nearPoint = unproject(0.0f);
        const XMVECTOR farPoint  = unproject(1.0f);

        Ray ray{};
        XMStoreFloat3(&ray.origin, nearPoint);
        XMStoreFloat3(&ray.direction, XMVector3Normalize(XMVectorSubtract(farPoint, nearPoint)));
        return ray;
    }
}
