#include <chrono>
#include <cmath>
#include <functional>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/bool.hpp"
#include "trajectory_interfaces/msg/trajectory_command.hpp"
#include "turtlesim/msg/pose.hpp"

using namespace std::chrono_literals;

class CommanderNode : public rclcpp::Node
{
public:
  CommanderNode()
  : Node("commander_node")
  {
    publisher_ =
      this->create_publisher<trajectory_interfaces::msg::TrajectoryCommand>(
        "/trajectory_cmd", 10);

    timer_ = this->create_wall_timer(
      20ms, std::bind(&CommanderNode::timer_callback, this));

    pose_subscription_ = this->create_subscription<turtlesim::msg::Pose>(
      "/turtle1/pose", 10,
      std::bind(&CommanderNode::pose_callback, this, std::placeholders::_1));

    obstacle_subscription_ = this->create_subscription<std_msgs::msg::Bool>(
      "/obstacle_alert", 10,
      std::bind(&CommanderNode::obstacle_callback, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(), "commander_node demarre.");
  }

private:
  void pose_callback(const turtlesim::msg::Pose::SharedPtr message)
  {
    x_ = message->x;
    y_ = message->y;
    angle_ = message->theta;
    pose_received_ = true;

    // Calculate the rectangle's four fixed target corners once,
    // based on the turtle's starting position and direction.
    if (!waypoints_ready_)
    {
      const double forward_x = std::cos(angle_);
      const double forward_y = std::sin(angle_);
      const double left_x = -forward_y;
      const double left_y = forward_x;

      const double start_x = x_;
      const double start_y = y_;

      // Target 0: end of the long side.
      target_x_[0] = start_x + 3.0 * forward_x;
      target_y_[0] = start_y + 3.0 * forward_y;

      // Target 1: end of the first short side.
      target_x_[1] = target_x_[0] + 1.5 * left_x;
      target_y_[1] = target_y_[0] + 1.5 * left_y;

      // Target 2: end of the second long side.
      target_x_[2] = start_x + 1.5 * left_x;
      target_y_[2] = start_y + 1.5 * left_y;

      // Target 3: return to the starting corner.
      target_x_[3] = start_x;
      target_y_[3] = start_y;

      waypoints_ready_ = true;
      RCLCPP_INFO(this->get_logger(), "Les quatre coins du rectangle sont definis.");
    }
  }

  void obstacle_callback(const std_msgs::msg::Bool::SharedPtr message)
  {
    if (message->data != obstacle_detected_)
    {
      obstacle_detected_ = message->data;

      if (obstacle_detected_)
      {
        RCLCPP_INFO(this->get_logger(), "Obstacle detecte.");
      }
      else
      {
        RCLCPP_INFO(this->get_logger(), "Obstacle disparu.");
      }
    }
  }

  void timer_callback()
  {
    constexpr double pi = 3.14159265358979323846;
    constexpr double distance_tolerance = 0.03;
    constexpr double angle_tolerance = 0.03;

    trajectory_interfaces::msg::TrajectoryCommand message;
    message.linear_speed = 0.0;
    message.angular_speed = 0.0;
    message.avoid_obstacle = obstacle_detected_;

    // Stop until the first turtle pose has arrived and the corners are known.
    if (!pose_received_ || !waypoints_ready_ || obstacle_detected_)
    {
      publisher_->publish(message);
      return;
    }

    const double dx = target_x_[target_index_] - x_;
    const double dy = target_y_[target_index_] - y_;
    const double distance = std::sqrt(dx * dx + dy * dy);

    // When close enough to a corner, move on to the next fixed corner.
    if (distance < distance_tolerance)
    {
      target_index_ = (target_index_ + 1) % 4;
      publisher_->publish(message);
      return;
    }

    const double target_angle = std::atan2(dy, dx);
    double angle_error = target_angle - angle_;

    // Keep the angle error between -pi and pi.
    while (angle_error > pi)
    {
      angle_error -= 2.0 * pi;
    }

    while (angle_error < -pi)
    {
      angle_error += 2.0 * pi;
    }

    // Turn in place until the turtle faces the next corner.
    if (std::fabs(angle_error) > angle_tolerance)
    {
      message.linear_speed = 0.0;

      if (angle_error > 0.0)
      {
        message.angular_speed = 0.6;
      }
      else
      {
        message.angular_speed = -0.6;
      }
    }
    else
    {
      // Once aligned, move toward the corner.
      message.linear_speed = 0.5;
      message.angular_speed = 0.0;
    }

    publisher_->publish(message);
  }

  rclcpp::Publisher<trajectory_interfaces::msg::TrajectoryCommand>::SharedPtr
    publisher_;

  rclcpp::TimerBase::SharedPtr timer_;

  rclcpp::Subscription<turtlesim::msg::Pose>::SharedPtr
    pose_subscription_;

  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr
    obstacle_subscription_;

  double x_{0.0};
  double y_{0.0};
  double angle_{0.0};

  double target_x_[4]{};
  double target_y_[4]{};

  int target_index_{0};

  bool pose_received_{false};
  bool waypoints_ready_{false};
  bool obstacle_detected_{false};
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CommanderNode>());
  rclcpp::shutdown();
  return 0;
}