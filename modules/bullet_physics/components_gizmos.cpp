#include "components_gizmos.h"

#include "../godot/nodes/entity.h"

void BtShapeComponentsGizmo::init() {
	const Color gizmo_color = EDITOR_DEF("editors/3d_gizmos/gizmo_colors/shape", Color(0.5, 0.7, 1));
	create_material("shape_material", gizmo_color);

	const float gizmo_value = gizmo_color.get_v();
	const Color gizmo_color_disabled = Color(gizmo_value, gizmo_value, gizmo_value, 0.65);
	create_material("shape_material_disabled", gizmo_color_disabled);

	create_handle_material("handles");
}

void BtShapeComponentsGizmo::redraw(EditorNode3DGizmo *p_gizmo) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(entity == nullptr);

	if (entity->has_component(box_component_name)) {
		const Ref<Material> material = get_material("shape_material", p_gizmo);
		//const Ref<Material> material_disabled = get_material("shape_material_disabled", p_gizmo);
		Ref<Material> handles_material = get_material("handles");

		Vector3 half_extents = entity->get_component_value(box_component_name, "half_extents", Space::GLOBAL);

		Vector<Vector3> lines;
		AABB aabb;
		aabb.position = -half_extents;
		aabb.size = half_extents * 2.0;

		for (int i = 0; i < 12; i++) {
			Vector3 a, b;
			aabb.get_edge(i, a, b);
			lines.push_back(a);
			lines.push_back(b);
		}

		Vector<Vector3> handles;

		for (int i = 0; i < 3; i++) {
			Vector3 ax;
			ax[i] = half_extents[i];
			handles.push_back(ax);
		}

		p_gizmo->add_lines(lines, material);
		p_gizmo->add_collision_segments(lines);
		p_gizmo->add_handles(handles, handles_material);
	}
}

int BtShapeComponentsGizmo::get_handle_count() const {
	return 3;
}

String BtShapeComponentsGizmo::get_handle_name(const EditorNode3DGizmo *p_gizmo, int p_idx) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND_V(entity == nullptr, "");
	ERR_FAIL_COND_V(entity->has_component(box_component_name) == false, "");

	if (p_idx == 0) {
		return "half_extents_x";
	} else if (p_idx == 1) {
		return "half_extents_y";
	} else {
		return "half_extents_z";
	}
}

Variant BtShapeComponentsGizmo::get_handle_value(EditorNode3DGizmo *p_gizmo, int p_idx) const {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND_V(entity == nullptr, Variant());
	ERR_FAIL_COND_V(entity->has_component(box_component_name) == false, Variant());

	const Vector3 half_extents = entity->get_component_value(box_component_name, half_extents_name, Space::GLOBAL);
	ERR_FAIL_COND_V_MSG((p_idx < 0) || p_idx > 2, Variant(), "The index " + itos(p_idx) + "is invalid. It should go from 0 to 2 (inclusive).");
	return half_extents[p_idx];
}

void BtShapeComponentsGizmo::set_handle(EditorNode3DGizmo *p_gizmo, int p_idx, Camera3D *p_camera, const Point2 &p_point) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(box_component_name) == false);

	Transform gt = entity->get_global_transform();
	Transform gi = gt.affine_inverse();

	Vector3 ray_from = p_camera->project_ray_origin(p_point);
	Vector3 ray_dir = p_camera->project_ray_normal(p_point);
	Vector3 sg[2] = { gi.xform(ray_from), gi.xform(ray_from + ray_dir * 4096) };

	// Extract the Handle value
	Vector3 axis;
	axis[p_idx] = 1.0;
	Vector3 ra, rb;
	Geometry3D::get_closest_points_between_segments(Vector3(), axis * 4096, sg[0], sg[1], ra, rb);
	float d = ra[p_idx];
	if (Node3DEditor::get_singleton()->is_snap_enabled()) {
		d = Math::snapped(d, Node3DEditor::get_singleton()->get_translate_snap());
	}

	if (d < 0.001) {
		d = 0.001;
	}

	Vector3 half_extents = entity->get_component_value(box_component_name, "half_extents", Space::GLOBAL);
	half_extents[p_idx] = d;

	entity->set_component_value(box_component_name, half_extents_name, half_extents, Space::GLOBAL);
}

void BtShapeComponentsGizmo::commit_handle(EditorNode3DGizmo *p_gizmo, int p_idx, const Variant &p_restore, bool p_cancel) {
	Entity3D *entity = static_cast<Entity3D *>(p_gizmo->get_spatial_node());
	ERR_FAIL_COND(entity == nullptr);
	ERR_FAIL_COND(entity->has_component(box_component_name) == false);

	Vector3 half_extents = entity->get_component_value(box_component_name, half_extents_name, Space::GLOBAL);

	if (p_cancel) {
		entity->set_component_value(box_component_name, half_extents_name, p_restore, Space::GLOBAL);
	} else {
		UndoRedo *ur = Node3DEditor::get_singleton()->get_undo_redo();
		ur->create_action(TTR("Change Shape Box Half Extent"));
		ur->add_do_method(entity, "set_component_value", box_component_name, half_extents_name, half_extents, Space::GLOBAL);
		ur->add_undo_method(entity, "set_component_value", box_component_name, half_extents_name, p_restore, Space::GLOBAL);
		ur->commit_action();
	}
}
