/**
 * \file construction_qt_user_functions.cpp
 *
 * \copyright 2020 John Harwell, All rights reserved.
 *
 * This file is part of SILICON.
 *
 * SILICON is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * SILICON is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * SILICON.  If not, see <http://www.gnu.org/licenses/
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "rcppsw/common/common.hpp"
RCPPSW_WARNING_DISABLE_PUSH()
RCPPSW_WARNING_DISABLE_OVERLOADED_VIRTUAL()
#include "silicon/support/construction_qt_user_functions.hpp"
RCPPSW_WARNING_DISABLE_POP()

#include <argos3/core/simulator/entity/controllable_entity.h>

#include "cosm/subsystem/saa_subsystemQ3D.hpp"
#include "cosm/vis/block_carry_visualizer.hpp"
#include "cosm/vis/polygon2D_visualizer.hpp"
#include "cosm/vis/steer2D_visualizer.hpp"
#include "cosm/ds/cell3D.hpp"

#include "silicon/controller/fcrw_bst_controller.hpp"
#include "silicon/controller/perception/builder_perception_subsystem.hpp"

/*******************************************************************************
 * Namespaces
 ******************************************************************************/
NS_START(silicon, support);

/*******************************************************************************
 * Constructors/Destructor
 ******************************************************************************/
construction_qt_user_functions::construction_qt_user_functions(void) {
  RegisterUserFunction<construction_qt_user_functions, chal::robot>(
      &construction_qt_user_functions::Draw);
}

/*******************************************************************************
 * Member Functions
 ******************************************************************************/
void construction_qt_user_functions::Draw(chal::robot& c_entity) {
  const auto* controller =
      dynamic_cast<const controller::constructing_controller*>(
          &c_entity.GetControllableEntity().GetController());

  if (controller->display_id()) {
    DrawText(argos::CVector3(0.0, 0.0, 0.5), c_entity.GetId());
  }

  if (controller->is_carrying_block()) {
    cvis::block_carry_visualizer(this, kBLOCK_VIS_OFFSET, kTEXT_VIS_OFFSET)
        .draw(controller->block(), controller->GetId().size());
  }
  if (controller->display_steer2D()) {
    auto steering = cvis::steer2D_visualizer(this, kTEXT_VIS_OFFSET);
    steering(controller->rpos3D(),
             c_entity.GetEmbodiedEntity().GetOriginAnchor().Orientation,
             controller->saa()->steer_force2D().tracker());
  }
  if (controller->display_los() && nullptr != controller->perception()->los()) {
    los_render(controller,
               c_entity.GetEmbodiedEntity().GetOriginAnchor().Orientation);
  }

  if (controller->display_nearest_ct() &&
      nullptr != controller->perception()->nearest_ct()) {
    nearest_ct_render(controller,
                      c_entity.GetEmbodiedEntity().GetOriginAnchor().Orientation);
  }
}

void construction_qt_user_functions::los_render(
    const controller::constructing_controller* controller,
    const argos::CQuaternion& orientation) {
  const auto* los = controller->perception()->los();
  const auto* ct = controller->perception()->nearest_ct();

  /*
   * The LOS rspan is relative to the virtual structure origin, and knows
   * nothing of where the structure is in the arena, so we fix that to get
   * proper rendering. Note we use translate() rather than recenter(), so that
   * the rendered LOS "snaps" to structure coordinates.
   */
  auto xspan = los->xrspan().translate(ct->voriginr().x());
  auto yspan = los->yrspan().translate(ct->voriginr().y());

  std::vector<rmath::vector2d> points = {
    {xspan.lb(), yspan.lb()},
    {xspan.lb(), yspan.ub()},
    {xspan.ub(), yspan.ub()},
    {xspan.ub(), yspan.lb()}
  };
  cvis::polygon2D_visualizer(this).abs_draw(
      controller->rpos3D(), orientation, points, rutils::color::kYELLOW);
} /* los_render() */

void construction_qt_user_functions::nearest_ct_render(
    const controller::constructing_controller* controller,
    const argos::CQuaternion& orientation) {
  const auto* ct = controller->perception()->nearest_ct();
  auto bbd = ct->bbd(true);
  size_t shell_size = ct->vshell_sized();
  double correction = ct->block_unit_dim();

  std::vector<rmath::vector2d> ct_rpoints = {
    ct->cell_loc_abs(rmath::vector3z(shell_size, shell_size, 0)).to_2D(),
    ct->cell_loc_abs(rmath::vector3z(shell_size, bbd.y() - shell_size, 0)).to_2D(),
    ct->cell_loc_abs(
          rmath::vector3z(bbd.x() - shell_size, bbd.y() - shell_size, 0))
        .to_2D(),
    ct->cell_loc_abs(rmath::vector3z(bbd.x() - shell_size, shell_size, 0)).to_2D(),
  };
  std::vector<rmath::vector2d> ct_vpoints = {
    ct->cell_loc_abs(rmath::vector3z(0, 0, 0)).to_2D(),
    ct->cell_loc_abs(rmath::vector3z(0, bbd.y() - 1, 0)).to_2D() +
        rmath::vector2d(0.0, correction),
    ct->cell_loc_abs(rmath::vector3z(bbd.x() - 1, bbd.y() - 1, 0)).to_2D() +
        rmath::vector2d(correction, correction),
    ct->cell_loc_abs(rmath::vector3z(bbd.x() - 1, 0, 0)).to_2D() +
        rmath::vector2d(correction, 0.0)
  };
  /* Draw BOTH virtual and real bounding boxes in 2D */
  cvis::polygon2D_visualizer v(this);
  v.abs_draw(controller->rpos3D(), orientation, ct_rpoints, rutils::color::kRED);
  v.abs_draw(
      controller->rpos3D(), orientation, ct_vpoints, rutils::color::kORANGE);
} /* nearest_ct_render() */

using namespace argos; // NOLINT

RCPPSW_WARNING_DISABLE_PUSH()
RCPPSW_WARNING_DISABLE_MISSING_VAR_DECL()
RCPPSW_WARNING_DISABLE_MISSING_PROTOTYPE()
RCPPSW_WARNING_DISABLE_GLOBAL_CTOR()
REGISTER_QTOPENGL_USER_FUNCTIONS(construction_qt_user_functions,
                                 "construction_qt_user_functions"); // NOLINT
RCPPSW_WARNING_DISABLE_POP()

NS_END(support, silicon);
