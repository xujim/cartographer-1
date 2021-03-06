
#include "cartographer/mapping/sparse_pose_graph.h"

#include "cartographer/mapping/sparse_pose_graph/constraint_builder.h"
#include "cartographer/mapping/sparse_pose_graph/optimization_problem_options.h"
#include "cartographer/transform/transform.h"
#include "glog/logging.h"

namespace cartographer {
namespace mapping {

proto::SparsePoseGraph::Constraint::Tag ToProto(
    const SparsePoseGraph::Constraint::Tag& tag) {
  switch (tag) {
    case SparsePoseGraph::Constraint::Tag::INTRA_SUBMAP:
      return proto::SparsePoseGraph::Constraint::INTRA_SUBMAP;
    case SparsePoseGraph::Constraint::Tag::INTER_SUBMAP:
      return proto::SparsePoseGraph::Constraint::INTER_SUBMAP;
  }
  LOG(FATAL) << "Unsupported tag.";
}

//sparse_pose_graph.lua
proto::SparsePoseGraphOptions CreateSparsePoseGraphOptions(
    common::LuaParameterDictionary* const parameter_dictionary) {
  proto::SparsePoseGraphOptions options;
  options.set_optimize_every_n_scans(
      parameter_dictionary->GetInt("optimize_every_n_scans"));
  *options.mutable_constraint_builder_options() =
      sparse_pose_graph::CreateConstraintBuilderOptions(
          parameter_dictionary->GetDictionary("constraint_builder").get());
  options.set_matcher_translation_weight(
      parameter_dictionary->GetDouble("matcher_translation_weight"));//5e2
  options.set_matcher_rotation_weight(
      parameter_dictionary->GetDouble("matcher_rotation_weight"));// 1.6e3
  *options.mutable_optimization_problem_options() =
      sparse_pose_graph::CreateOptimizationProblemOptions(
          parameter_dictionary->GetDictionary("optimization_problem").get());
  options.set_max_num_final_iterations(
      parameter_dictionary->GetNonNegativeInt("max_num_final_iterations"));
      //200
  
  CHECK_GT(options.max_num_final_iterations(), 0);
  options.set_global_sampling_ratio(
      parameter_dictionary->GetDouble("global_sampling_ratio"));//0.003
  return options;
}

proto::SparsePoseGraph SparsePoseGraph::ToProto() {
  proto::SparsePoseGraph proto;

  for (const auto& constraint : constraints()) {
    auto* const constraint_proto = proto.add_constraint();
    *constraint_proto->mutable_relative_pose() =
        transform::ToProto(constraint.pose.zbar_ij);
    constraint_proto->set_translation_weight(
        constraint.pose.translation_weight);
    constraint_proto->set_rotation_weight(constraint.pose.rotation_weight);
    constraint_proto->mutable_submap_id()->set_trajectory_id(
        constraint.submap_id.trajectory_id);
    constraint_proto->mutable_submap_id()->set_submap_index(
        constraint.submap_id.submap_index);

    constraint_proto->mutable_scan_id()->set_trajectory_id(
        constraint.node_id.trajectory_id);
    constraint_proto->mutable_scan_id()->set_scan_index(
        constraint.node_id.node_index);

    constraint_proto->set_tag(mapping::ToProto(constraint.tag));
  }

  for (const auto& trajectory_nodes : GetTrajectoryNodes()) {
    auto* trajectory_proto = proto.add_trajectory();
    for (const auto& node : trajectory_nodes) {
      auto* node_proto = trajectory_proto->add_node();
      node_proto->set_timestamp(common::ToUniversal(node.constant_data->time));
      *node_proto->mutable_pose() =
          transform::ToProto(node.pose * node.constant_data->tracking_to_pose);
    }

    if (!trajectory_nodes.empty()) {
      for (const auto& transform : GetSubmapTransforms(
               trajectory_nodes[0].constant_data->trajectory_id)) {
        *trajectory_proto->add_submap()->mutable_pose() =
            transform::ToProto(transform);
      }
    }
  }

  return proto;
}

}  // namespace mapping
}  // namespace cartographer
