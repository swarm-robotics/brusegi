/**
 * \file structure_egress_fsm.hpp
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

#ifndef INCLUDE_SILICON_FSM_STRUCTURE_EGRESS_FSM_HPP_
#define INCLUDE_SILICON_FSM_STRUCTURE_EGRESS_FSM_HPP_

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include <vector>

#include "cosm/steer2D/ds/path_state.hpp"

#include "silicon/fsm/builder_util_fsm.hpp"
#include "silicon/fsm/calculators/lane_alignment.hpp"

/*******************************************************************************
 * Namespaces
 ******************************************************************************/
NS_START(silicon, fsm);

/*******************************************************************************
 * Class Definitions
 ******************************************************************************/
/**
 * \class structure_egress_fsm
 * \ingroup fsm
 *
 * \brief The FSM for exiting the in-progress structure once a robot has placed
 * its block.
 */
class structure_egress_fsm : public builder_util_fsm,
                             public rer::client<structure_egress_fsm> {
 public:
  structure_egress_fsm(
      const scperception::builder_perception_subsystem* perception,
      csubsystem::saa_subsystemQ3D* saa,
      rmath::rng* rng);

  ~structure_egress_fsm(void) override = default;

  /* not copy constructible or copy assignable by default */
  structure_egress_fsm(const structure_egress_fsm&) = delete;
  structure_egress_fsm& operator=(const structure_egress_fsm&) = delete;

  /* taskable overrides */
  void task_reset(void) override { init(); }
  bool task_running(void) const override {
    return current_state() != ekST_START && current_state() != ekST_FINISHED;
  }
  void task_execute(void) override;
  void task_start(cta::taskable_argument* c_arg) override;
  bool task_finished(void) const override {
    return current_state() == ekST_FINISHED;
  }

  /* HFSM overrides */
  void init(void) override;

 private:
  enum fsm_state {
    ekST_START,
    /**
     * The robot has placed its block and is moving from the cell it placed the
     * block FROM to the egress lane (it may already be there).
     */
    ekST_ACQUIRE_EGRESS_LANE,

    /**
     * The robot is leaving the structure via the egress lane.
     */
    ekST_STRUCTURE_EGRESS,

    /**
     * The robot is waiting for the robot in front of it to move so it can
     * continue.
     */
    ekST_WAIT_FOR_ROBOT,

    /**
     * The robot has left the structure and returned to the arena.
     */
    ekST_FINISHED,

    ekST_MAX_STATES
  };

  /* inherited states */
  RCPPSW_HFSM_ENTRY_INHERIT_ND(util_hfsm, entry_wait_for_signal);
  RCPPSW_HFSM_STATE_INHERIT(builder_util_fsm,
                            wait_for_robot,
                            const robot_wait_data);

  /* builder states */
  RCPPSW_HFSM_STATE_DECLARE_ND(structure_egress_fsm, start);
  RCPPSW_HFSM_STATE_DECLARE(structure_egress_fsm,
                            acquire_egress_lane,
                            csteer2D::ds::path_state);
  RCPPSW_HFSM_STATE_DECLARE(structure_egress_fsm,
                            structure_egress,
                            csteer2D::ds::path_state);
  RCPPSW_HFSM_EXIT_DECLARE(structure_egress_fsm, exit_structure_egress);
  RCPPSW_HFSM_ENTRY_DECLARE_ND(structure_egress_fsm, entry_structure_egress);

  RCPPSW_HFSM_STATE_DECLARE_ND(structure_egress_fsm, finished);

  RCPPSW_HFSM_DEFINE_STATE_MAP_ACCESSOR(state_map_ex, index) override {
    return &mc_state_map[index];
  }

  RCPPSW_HFSM_DECLARE_STATE_MAP(state_map_ex, mc_state_map, ekST_MAX_STATES);

  /* clang-format off */
  calculators::lane_alignment m_alignment_calc;
  /* clang-format on */
};

NS_END(fsm, silicon);

#endif /* INCLUDE_SILICON_FSM_STRUCTURE_EGRESS_FSM_HPP_ */
