/**
 * \file silicon_metrics_aggregator.cpp
 *
 * \copyright 2018 John Harwell, All rights reserved.
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
#include "silicon/metrics/silicon_metrics_aggregator.hpp"

#include <boost/mpl/for_each.hpp>

#include "rcppsw/mpl/typelist.hpp"
#include "rcppsw/utils/maskable_enum.hpp"

#include "cosm/convergence/convergence_calculator.hpp"
#include "cosm/metrics/collector_registerer.hpp"

#include "silicon/controller/fcrw_bst_controller.hpp"
#include "silicon/fsm/fcrw_bst_fsm.hpp"
#include "silicon/lane_alloc/metrics/lane_alloc_metrics_collector.hpp"
#include "silicon/metrics/blocks/manipulation_metrics_collector.hpp"
#include "silicon/structure/ct_manager.hpp"
#include "silicon/structure/metrics/structure_progress_metrics_collector.hpp"
#include "silicon/structure/metrics/structure_state_metrics_collector.hpp"
#include "silicon/structure/metrics/subtargets_metrics_collector.hpp"
#include "silicon/structure/structure3D.hpp"
#include "silicon/structure/subtarget.hpp"
#include "silicon/support/tv/metrics/env_dynamics_metrics.hpp"
#include "silicon/support/tv/metrics/env_dynamics_metrics_collector.hpp"

/*******************************************************************************
 * Namespaces
 ******************************************************************************/
NS_START(silicon, metrics);

/*******************************************************************************
 * Constructors/Destructors
 ******************************************************************************/
silicon_metrics_aggregator::silicon_metrics_aggregator(
    const cmconfig::metrics_config* const mconfig,
    const cdconfig::grid2D_config* const gconfig,
    const std::string& output_root,
    const ds::ct_vectorno& targets)
    : ER_CLIENT_INIT("silicon.metrics.aggregator"),
      base_metrics_aggregator(mconfig, output_root) {
  /* register collectors from base class  */
  rmath::vector2z dims2D =
      rmath::dvec2zvec(gconfig->dims, gconfig->resolution.v());
  size_t max_height = 0;
  for (auto* target : targets) {
    max_height = std::max(target->zdsize(), max_height);
  } /* for(*target..) */

  /* robots can be on top of structures, so give a little padding in Z */
  rmath::vector3z dims3D =
      rmath::vector3z(dims2D.x(), dims2D.y(), max_height + 2);

  register_with_arena_dims2D(mconfig, dims2D);
  register_with_arena_dims3D(mconfig, dims3D);

  /* register SILICON collectors */
  register_standard_non_target(mconfig);
  for (auto* target : targets) {
    register_standard_target(mconfig, target);
    register_with_target_lanes(mconfig, target);
    register_with_target_lanes_and_id(mconfig, target);
    register_with_target_dims(mconfig, target);
  } /* for(*target..) */

  reset_all();
}

/*******************************************************************************
 * Member Functions
 ******************************************************************************/
void silicon_metrics_aggregator::collect_from_tv(
    const support::tv::tv_manager* const tvm) {
  collect("tv::environment", *tvm->dynamics<ctv::dynamics_type::ekENVIRONMENT>());

  if (nullptr != tvm->dynamics<ctv::dynamics_type::ekPOPULATION>()) {
    collect("tv::population", *tvm->dynamics<ctv::dynamics_type::ekPOPULATION>());
  }
} /* collect_from_tv() */

void silicon_metrics_aggregator::collect_from_ct(
    const structure::ct_manager* manager) {
  for (auto* target : manager->targetsro()) {
    collect("structure" + rcppsw::to_string(target->id()) + "::state", *target);
    collect("structure" + rcppsw::to_string(target->id()) + "::progress",
            *target);
    for (auto* st : target->subtargets()) {
      collect("structure" + rcppsw::to_string(target->id()) + "::subtargets",
              *st);
    } /* for(*t..) */
  } /* for(*target..) */
} /* collect_from_ct() */

template <typename TController>
void silicon_metrics_aggregator::collect_from_controller(
    const TController* c,
    const rtypes::type_uuid& structure_id) {
  base_metrics_aggregator::collect_from_controller(c);

  if (rtypes::constants::kNoUUID != structure_id) {
    collect("structure" + rcppsw::to_string(structure_id) + "::lane_alloc",
            *c->fsm()->lane_allocator());
  }
  collect("blocks::manipulation", *c->block_manip_recorder());
  collect("blocks::acq_locs2D", *c);
  collect("blocks::acq_explore_locs2D", *c);
  collect("blocks::acq_vector_locs2D", *c);
  collect("blocks::acq_counts", *c);
  collect("fsm::interference_counts", *c->fsm());
} /* collect_from_controller() */

void silicon_metrics_aggregator::register_standard_non_target(
    const cmconfig::metrics_config* mconfig) {
  using collector_typelist = rmpl::typelist<
      rmpl::identity<blocks::manipulation_metrics_collector>,
      rmpl::identity<support::tv::metrics::env_dynamics_metrics_collector> >;
  cmetrics::collector_registerer<>::creatable_set creatable_set = {
    { typeid(blocks::manipulation_metrics_collector),
      "block_manipulation",
      "blocks::manipulation",
      rmetrics::output_mode::ekAPPEND },
    { typeid(support::tv::metrics::env_dynamics_metrics_collector),
      "tv_environment",
      "tv::environment",
      rmetrics::output_mode::ekAPPEND }
  };
  cmetrics::collector_registerer<> registerer(mconfig, creatable_set, this);
  boost::mpl::for_each<collector_typelist>(registerer);
} /* register_standard_non_target() */

void silicon_metrics_aggregator::register_standard_target(
    const cmconfig::metrics_config* mconfig,
    const sstructure::structure3D* structure) {
  using collector_typelist = rmpl::typelist<
      rmpl::identity<structure::metrics::structure_progress_metrics_collector> >;
  cmetrics::collector_registerer<>::creatable_set creatable_set = {
    { typeid(structure::metrics::structure_progress_metrics_collector),
      "structure" + rcppsw::to_string(structure->id()) + "_progress",
      "structure" + rcppsw::to_string(structure->id()) + "::progress",
      rmetrics::output_mode::ekAPPEND },
  };
  cmetrics::collector_registerer<> registerer(mconfig, creatable_set, this);
  boost::mpl::for_each<collector_typelist>(registerer);
} /* register_standard_target() */

void silicon_metrics_aggregator::register_with_target_lanes(
    const cmconfig::metrics_config* mconfig,
    const sstructure::structure3D* structure) {
  using extra_args_type = std::tuple<size_t>;
  using collector_typelist = rmpl::typelist<
      rmpl::identity<structure::metrics::subtargets_metrics_collector> >;

  cmetrics::collector_registerer<extra_args_type>::creatable_set creatable_set = {
    { typeid(structure::metrics::subtargets_metrics_collector),
      "structure" + rcppsw::to_string(structure->id()) + "_subtargets",
      "structure" + rcppsw::to_string(structure->id()) + "::subtargets",
      rmetrics::output_mode::ekAPPEND },
  };
  auto extra_args = std::make_tuple(structure->subtargets().size());
  cmetrics::collector_registerer<extra_args_type> registerer(
      mconfig, creatable_set, this, extra_args);
  boost::mpl::for_each<collector_typelist>(registerer);
} /* register_with_target_lanes() */

void silicon_metrics_aggregator::register_with_target_lanes_and_id(
    const cmconfig::metrics_config* mconfig,
    const sstructure::structure3D* structure) {
  using extra_args_type = std::tuple<rtypes::type_uuid, size_t>;
  using collector_typelist =
      rmpl::typelist<rmpl::identity<slametrics::lane_alloc_metrics_collector> >;

  cmetrics::collector_registerer<extra_args_type>::creatable_set creatable_set = {
    { typeid(slametrics::lane_alloc_metrics_collector),
      "structure" + rcppsw::to_string(structure->id()) + "_lane_alloc",
      "structure" + rcppsw::to_string(structure->id()) + "::lane_alloc",
      rmetrics::output_mode::ekAPPEND }
  };
  auto extra_args =
      std::make_tuple(structure->id(), structure->subtargets().size());
  cmetrics::collector_registerer<extra_args_type> registerer(
      mconfig, creatable_set, this, extra_args);
  boost::mpl::for_each<collector_typelist>(registerer);
} /* register_with_target_lanes_and_id() */

void silicon_metrics_aggregator::register_with_target_dims(
    const cmconfig::metrics_config* mconfig,
    const sstructure::structure3D* structure) {
  using extra_args_type = std::tuple<rmath::vector3z>;
  using collector_typelist = rmpl::typelist<
      rmpl::identity<structure::metrics::structure_state_metrics_collector> >;
  cmetrics::collector_registerer<extra_args_type>::creatable_set creatable_set = {
    { typeid(structure::metrics::structure_state_metrics_collector),
      "structure" + rcppsw::to_string(structure->id()) + "_state",
      "structure" + rcppsw::to_string(structure->id()) + "::state",
      rmetrics::output_mode::ekCREATE | rmetrics::output_mode::ekTRUNCATE },
  };

  auto extra_args = std::make_tuple(structure->dimsd());
  cmetrics::collector_registerer<extra_args_type> registerer(
      mconfig, creatable_set, this, extra_args);
  boost::mpl::for_each<collector_typelist>(registerer);
} /* register_with_target_dims() */

/*******************************************************************************
 * Template Instantiations
 ******************************************************************************/
template void silicon_metrics_aggregator::collect_from_controller(
    const controller::fcrw_bst_controller*,
    const rtypes::type_uuid&);

NS_END(metrics, silicon);
