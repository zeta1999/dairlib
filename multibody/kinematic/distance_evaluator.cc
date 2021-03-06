#include "multibody/kinematic/distance_evaluator.h"

#include "drake/math/orthonormal_basis.h"

using drake::multibody::Frame;
using drake::multibody::MultibodyPlant;
using drake::systems::Context;
using drake::MatrixX;
using drake::VectorX;
using Eigen::Vector3d;

namespace dairlib {
namespace multibody {

template <typename T>
DistanceEvaluator<T>::DistanceEvaluator(
    const MultibodyPlant<T>& plant, const Vector3d pt_A,
    const Frame<T>& frame_A, const Vector3d pt_B, const Frame<T>& frame_B,
    double distance)
    : KinematicEvaluator<T>(plant, 1),
      pt_A_(pt_A),
      frame_A_(frame_A),
      pt_B_(pt_B),
      frame_B_(frame_B),
      distance_(distance) {}

template <typename T>
VectorX<T> DistanceEvaluator<T>::EvalFull(
    const Context<T>& context) const {
  // Transform point A to frame B, and compute norm
  VectorX<T> pt_A_B(3);

  plant().CalcPointsPositions(context, frame_A_,
      pt_A_.template cast<T>(), frame_B_, &pt_A_B);  
  auto rel_pos = pt_A_B - pt_B_;
  VectorX<T> difference(1);
  difference << rel_pos.norm() - distance_;

  return difference;
}

template <typename T>
MatrixX<T> DistanceEvaluator<T>::EvalFullJacobian(
    const Context<T>& context) const {
  /// Jacobian of ||pt_A - pt_B||, evaluated all in frame B, is
  ///   (pt_A - pt_B)^T * (J_A - J_B) / ||pt_A - pt_B||

  VectorX<T> pt_A_B(3);

  plant().CalcPointsPositions(context, frame_A_,
      pt_A_.template cast<T>(), frame_B_, &pt_A_B);   
  auto rel_pos = pt_A_B - pt_B_;

  MatrixX<T> J_A(3, plant().num_velocities());

  // .template cast<T> converts pt_A_, as a double, into type T
  plant().CalcJacobianTranslationalVelocity(
    context, drake::multibody::JacobianWrtVariable::kV,
    frame_A_, pt_A_.template cast<T>(), frame_B_, frame_B_, &J_A);

  return (rel_pos.transpose() * J_A) / rel_pos.norm();
}

template <typename T>
VectorX<T> DistanceEvaluator<T>::EvalFullJacobianDotTimesV(
    const Context<T>& context) const {
  // From applying the chain rule to Jacobian, where here we must use
  // the world frame. Jdot * v is
  //
  // ||(J_A - J_B) * v||^2/phi ...
  //   + (pt_A - pt_B)^T * (J_A_dot * v  -J_B_dot * v) / phi ...
  //   - phidot * (pt_A - pt_B)^T (J_A - J_B) *v / phi^2
  const drake::multibody::Frame<T>& world = plant().world_frame();

  VectorX<T> pt_A_world(3);
  VectorX<T> pt_B_world(3);

  auto pt_A_cast = pt_A_.template cast<T>();
  auto pt_B_cast = pt_B_.template cast<T>();


  // Perform all kinematic calculations, finding A, B in world frame,
  // Jacobians J_A and J_B, and Jdotv for both A and B
  plant().CalcPointsPositions(context, frame_A_,
      pt_A_cast, world, &pt_A_world);
  plant().CalcPointsPositions(context, frame_B_,
      pt_B_cast, world, &pt_B_world);
  auto rel_pos = pt_A_world - pt_B_world;

  MatrixX<T> J_A(3, plant().num_velocities());
  MatrixX<T> J_B(3, plant().num_velocities());

  plant().CalcJacobianTranslationalVelocity(
    context, drake::multibody::JacobianWrtVariable::kV,
    frame_A_, pt_A_cast, world, world, &J_A);
  plant().CalcJacobianTranslationalVelocity(
    context, drake::multibody::JacobianWrtVariable::kV,
    frame_B_, pt_B_cast, world, world, &J_B);
  auto J_rel = J_A - J_B;

  VectorX<T> J_A_dot_times_v = plant().CalcBiasSpatialAcceleration(
      context, drake::multibody::JacobianWrtVariable::kV, frame_A_,
      pt_A_cast, world, world).translational();
  VectorX<T> J_B_dot_times_v = plant().CalcBiasSpatialAcceleration(
      context, drake::multibody::JacobianWrtVariable::kV, frame_B_,
      pt_B_cast, world, world).translational();
  auto J_rel_dot_times_v = J_A_dot_times_v - J_B_dot_times_v;
  T phi = rel_pos.norm();

  // Jacobian for total constraint -- this result will match the version 
  // calculated using the B frame only in EvalFullJacobian 
  auto J = (rel_pos.transpose() * J_rel) / phi;

  T phidot = J.dot(plant().GetVelocities(context));

  // Compute (J_A - J_B) * v, as this is used multiple times
  VectorX<T> J_rel_v(3);
  J_rel_v << J_rel * plant().GetVelocities(context);

  // Compute all terms as scalars using dot products
  VectorX<T> J_dot_times_v(1);
  J_dot_times_v << (J_rel_v).squaredNorm() / phi
      + rel_pos.dot(J_rel_dot_times_v) / phi
      - phidot * rel_pos.dot(J_rel_v) / (phi * phi);
  return J_dot_times_v;
}

DRAKE_DEFINE_CLASS_TEMPLATE_INSTANTIATIONS_ON_DEFAULT_NONSYMBOLIC_SCALARS(
    class ::dairlib::multibody::DistanceEvaluator)

}  // namespace multibody
}  // namespace dairlib