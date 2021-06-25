/**
 * \file validate_placement.cpp
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
#include "silicon/structure/operations/validate_placement.hpp"

#include "cosm/repr/cube_block3D.hpp"
#include "cosm/repr/ramp_block3D.hpp"

#include "silicon/structure/structure3D.hpp"

/*******************************************************************************
 * Namespaces/Decls
 ******************************************************************************/
NS_START(silicon, structure, operations);

/*******************************************************************************
 * Class Definitions
 ******************************************************************************/
bool validate_placement::operator()(const crepr::cube_block3D* block) const {
  ER_CHECK(validate_common(),
           "Common validation for cube block%d with intent %s failed",
           block->id().v(),
           rcppsw::to_string(mc_intent).c_str());

  /* @todo check if structure invariants are violated by adding this block */
  return true;

error:
  ER_ERR("Cube block%d placement with intent %s failed validation",
         block->id().v(),
         rcppsw::to_string(mc_intent).c_str());
  return false;
} /* operator() */

bool validate_placement::operator()(const crepr::ramp_block3D* block) const {
  ER_CHECK(validate_common(),
           "Common validation for ramp block%d with intent %s failed",
           block->id().v(),
           rcppsw::to_string(mc_intent).c_str());

  if (rmath::radians::kZERO == mc_intent.z_rot()) {
    /* @todo check if structure invariants are violated by adding this block */
  } else if (rmath::radians::kPI == mc_intent.z_rot()) {
    /* @todo check if structure invariants are violated by adding this block */
  } else {
    ER_FATAL_SENTINEL("Bad orientation: %s",
                      rcppsw::to_string(mc_intent.z_rot()).c_str());
  }
error:
  ER_ERR("Ramp block%d placement with intent %s failed validation",
         block->id().v(),
         rcppsw::to_string(mc_intent).c_str());
  return false;
} /* operator() */

bool validate_placement::validate_common(void) const {
  /* all accesses into 3D array must be relative to virtual origin */
  ER_ASSERT(ssds::ct_coord::relativity::ekVORIGIN ==
                mc_intent.site().relative_to(),
            "Placement coordinates not relative to virtual origin");

  auto vcoord = mc_intent.site();
  const auto& cell = mc_structure->access(vcoord.offset());

  /* all cell spec requests must be relative to real origin */
  auto rcoord = mc_intent.site().to_real();

  /* Cell can't already have block */
  ER_CHECK(!cell.fsm().state_has_block(),
           "Cell@%s already in ekST_HAS_BLOCK",
           rcppsw::to_string(cell.loc()).c_str());

  /* Cell can't already be part of a block extent */
  ER_CHECK(!cell.fsm().state_in_block_extent(),
           "Cell@%s already in ekST_BLOCK_EXTENT",
           rcppsw::to_string(cell.loc()).c_str());

  /* All valid placements must correspond to a cell spec */
  ER_CHECK(nullptr != mc_structure->cell_spec_retrieve(rcoord),
           "Cell@%s has no spec",
           rcppsw::to_string(rcoord).c_str());

  /* check rotation */
  ER_CHECK(rmath::radians::kZERO == mc_intent.z_rot() ||
           rmath::radians::kPI_OVER_TWO == mc_intent.z_rot() ||
           rmath::radians::kPI == mc_intent.z_rot() ||
           rmath::radians::kTHREE_PI_OVER_TWO == mc_intent.z_rot(),
           "Bad rotation %s: must be %s or %s",
           rcppsw::to_string(mc_intent.z_rot()).c_str(),
           rcppsw::to_string(rmath::radians::kZERO).c_str(),
           rcppsw::to_string(rmath::radians::kPI_OVER_TWO).c_str());

  /*
   * @todo check if the embodiment for this block would overlap with any other
   * blocks already placed on the structure.
   */

  return true;

error:
  return false;
} /* validate_common() */

NS_END(operations, structure, silicon);
