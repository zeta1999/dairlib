#include <chrono>
#include <gflags/gflags.h>

#include "examples/Cassie/cassie_fixed_point_solver.h"

DEFINE_double(height, 1, "Fixed height,");
DEFINE_double(mu, .5, "Coefficient of friction,");
DEFINE_double(min_normal_force, 50, "Minimum normal force per contact pont.");
DEFINE_bool(linear_friction_cone, true, "Use linear or nonlinear Lorentz cone,");

namespace dairlib {

int do_main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  srand((unsigned int) time(0));
  drake::logging::set_log_level("err");  // ignore warnings about joint limits

  std::string urdf = "examples/Cassie/urdf/cassie_fixed_springs.urdf";

  Eigen::VectorXd q, u, lambda;

  CassieFixedPointSolver(urdf, FLAGS_height, FLAGS_mu, FLAGS_min_normal_force,
      FLAGS_linear_friction_cone, &q, &u, &lambda, true);

  std::cout << "Positions" << std::endl;
  std::cout << q << std::endl;
  std::cout << "Inputs" << std::endl;
  std::cout << u << std::endl;
  std::cout << "Forces" << std::endl;
  std::cout << lambda << std::endl;
  return 0;
}

}  // namespace dairlib

int main(int argc, char* argv[]) { return dairlib::do_main(argc, argv); }