#pragma once

#include <Slic3r/App/Render/Geometry.hpp>

#define ENABLED_DEBUG_CAMERA 0

namespace Slic3r::App::Platform {
struct CameraSynchData;
} // namespace Slic3r::App::Platform

#include <Slic3r/Domain/Types.hpp>

namespace Slic3r::Domain {
class Project;
struct BedRef;
} // namespace Slic3r::Domain

namespace Slic3r::App::Platform {
class AnimationManager;
} // namespace Slic3r::App::Platform

namespace Slic3r::App::Scene {

class Camera;
class CameraTrackballController;

void zoom_to_box(Camera& camera, const Eigen::AlignedBox3d& aabb);

/**
 * @brief Slide the camera so what it was showing sits clear of a panel.
 *
 * The settings panel is an overlay: the scene keeps the whole canvas and the
 * panel is painted on top of it, over the plate. The loop that costs the user
 * is changing a setting, not being able to see what it did, panning the model
 * out from under the panel, and then having to put the camera back afterwards
 * -- so the camera moves itself instead, and moves back when the panel closes.
 *
 * The shift is measured in the scene, not in pixels: the world distance
 * between the middle of the canvas and the middle of what the panel leaves
 * visible, on the plane through the orbit target. That is the same plane
 * dragging pans along and zooming anchors to, so the three agree.
 *
 * @param occluded_left  Left edge of the panel, in the same coordinates as the
 *                       camera's viewport. Everything from here rightwards is
 *                       taken as covered.
 * @return The shift applied, to be passed back negated when the panel closes.
 *         Zero when the panel covers everything or nothing.
 */
Domain::Vec3d pan_clear_of_panel(
    const Camera& camera,
    CameraTrackballController& trackball,
    double occluded_left
);

/// Undo a shift from pan_clear_of_panel().
void unpan_clear_of_panel(CameraTrackballController& trackball, const Domain::Vec3d& shift);
void synchronize_camera(const Platform::CameraSynchData& data, Camera& camera, CameraTrackballController& trackball);
void center_camera_on_bed(const Domain::Project& project, const Domain::BedRef& bed_ref, CameraTrackballController& trackball);
void animated_center_camera_on_bed(const Domain::Project& project, const Domain::BedRef& bed_ref, CameraTrackballController& trackball,
    Platform::AnimationManager& animation_manager, double duration_in_sec = 0.25);

#if ENABLED_DEBUG_CAMERA
void render_imgui_debug_camera(const Camera& camera, const CameraTrackballController& trackball);
#endif // ENABLED_DEBUG_CAMERA

} // namespace Slic3r::App::Scene
