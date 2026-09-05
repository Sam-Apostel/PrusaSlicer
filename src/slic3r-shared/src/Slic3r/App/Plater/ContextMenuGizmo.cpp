#include "Slic3r/App/Plater/ContextMenuGizmo.hpp"
#include "Slic3r/App/Scene/BedNodeTag.hpp"
#include "Slic3r/App/Scene/SceneNodeTag.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Algorithms/Point.hpp"
#include "Slic3r/App/Scene/ISceneProvider.hpp"

#include <tracy/Tracy.hpp>

namespace Slic3r::App::Plater {

ContextMenuGizmo::ContextMenuGizmo(
    Biz::ProjectInteractor& project_interactor,
    Scene::ISceneProvider& scene_provider
) :
    m_project_interactor(project_interactor),
    m_scene_provider(scene_provider)
{}

Scene::GizmoActivationState
ContextMenuGizmo::on_mouse(Scene::GizmoEventContext& ctx, bool only_active)
{
    ZoneScoped;

    // ignore bed picking when the camera is below the bed
    const Scene::Camera& camera{m_scene_provider.scene().camera()};
    const Scene::CameraProjectionType camera_type{camera.cam_projection().type()};
    const bool pick_disabled{
        (camera_type == Scene::CameraProjectionType::Perspective) ? camera.position().z() < 0.0 :
                                                                    camera.forward().z() >= 0.0
    };

    if (pick_disabled)
        return Scene::GizmoActivationState::Inactive;

    const Platform::MouseEvent& evt{ctx.mouse_event()};
    const Platform::MouseEvent::Type type{evt.type()};

    if (evt.button() != Platform::MouseButton::Right) {
        // Returning Inactive makes GizmoManager erase this gizmo, and the erase
        // lasts for the rest of the interaction -- so the ButtonUp that should
        // open the menu would never arrive. Motion carries no button, and a
        // trackpad interleaves motion and scroll into even a stationary
        // two-finger click, so that erase is the common case rather than a rare
        // one. Stay Probing until the press this gizmo is following ends.
        //
        // Probing does not keep the menu alive through a real drag: once the
        // camera gizmo passes its movement threshold it reports Active, which
        // claims the cycle and evicts this gizmo -- which is what should happen,
        // since a drag is not a click.
        return m_right_down ? Scene::GizmoActivationState::Probing
                            : Scene::GizmoActivationState::Inactive;
    }

    if (type == Platform::MouseEvent::Type::ButtonDown) {
        m_right_down = true;
        return Scene::GizmoActivationState::Probing;
    }
    auto& timer_queue = Biz::Platform::PlatformServices::instance().timer_queue();

    if (type == Platform::MouseEvent::Type::DoubleClick) {
        if (timer_queue.is_timer_running(m_timer_id)) {
            timer_queue.cancel_timer(m_timer_id);
        }
        m_double_click_detected = true;
        return Scene::GizmoActivationState::Done;
    }

    if (type == Platform::MouseEvent::Type::ButtonUp) {
        m_right_down = false;
        if (m_double_click_detected) {
            m_double_click_detected = false;
            return Scene::GizmoActivationState::Inactive;
        }

        Domain::Vec2f pos{evt.x(), evt.y()};

        if (ctx.pick_results().empty()) {
            invoke_show_context_menu(ContextMenuType::Scene, pos);
            return Scene::GizmoActivationState::Inactive;
        }

        Scene::Node* node = ctx.pick_results().front().node;
        if (const Scene::BedNodeTag* bed_tag{node->tag_of_type<Scene::BedNodeTag>()}) {
            const Domain::BedRef instance{bed_tag->config_container_id, bed_tag->instance_id};

            if (m_project_interactor.scene_interactor().bed_selection().is_selected(instance)) {
                invoke_show_context_menu(ContextMenuType::Bed, pos);
            }
        } else if (const Scene::SceneNodeTag* tag{node->tag_of_type<Scene::SceneNodeTag>()};
                   tag && !tag->is_wipe_tower())
        {
            // ButtonUp id postponed a bit from QuickSelectGizmo, so
            // we need to postpone its processing here slightly too to correct process selection before
            m_timer_id = timer_queue.set_timer(
                std::chrono::milliseconds(150),
                [this, pos]()
                {
                    const Biz::Scene::ObjectSelection& selection =
                        m_project_interactor.scene_interactor().object_selection();
                    if (!selection.empty()) {
                        Domain::Project& project = m_project_interactor.selected_project();
                        if (selection.mode == Slic3r::Biz::Scene::SelectionMode::Volume) {
                            bool is_svg_or_text = selection.elements.size() == 1;
                            if (is_svg_or_text) {
                                Domain::ModelVolume* volume = project.find_volume_by_id(
                                    selection.elements.front().object_id,
                                    selection.elements.front().volume_id
                                );
                                is_svg_or_text = volume->is_text() || volume->is_svg();
                            }

                            invoke_show_context_menu(
                                is_svg_or_text ? ContextMenuType::SvgOrText :
                                                 ContextMenuType::Volume,
                                pos
                            );
                        } else {
                            invoke_show_context_menu(
                                selection.only_single_object() ? ContextMenuType::Object :
                                                                 ContextMenuType::MultiObjects,
                                pos
                            );
                        }
                    }
                }
            );
            return Scene::GizmoActivationState::Inactive;
        }
    }

    return Scene::GizmoActivationState::Inactive;
}

void ContextMenuGizmo::on_cycle_prepare()
{
    // A new cycle means a new interaction, so no press can still be in flight.
    //
    // Without this the flag could stick on: a drag that passes the camera's
    // movement threshold makes the camera claim the cycle, which evicts this
    // gizmo before its ButtonUp arrives. A stuck flag is not merely cosmetic --
    // it makes on_mouse() answer Probing forever, so in_cycle_gizmos never
    // empties, GizmoManager never starts a new cycle, and every gizmo erased in
    // the meantime stays erased. The symptom is that all clicking dies until
    // some later right ButtonUp happens to reach here and clear it.
    m_right_down = false;
}

void ContextMenuGizmo::invoke_show_context_menu(ContextMenuType type, Domain::Vec2f mouse_position)
{
    invoke_listeners<IShowContextMenuListener>([&](IShowContextMenuListener* l)
                                               { l->on_show_context_menu(type, mouse_position); });
}

} // namespace Slic3r::App::Plater
