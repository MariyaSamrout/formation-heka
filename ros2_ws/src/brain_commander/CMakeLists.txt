cmake_minimum_required(VERSION 3.8)
project(brain_commander)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(trajectory_interfaces REQUIRED)
find_package(turtlesim REQUIRED)

add_executable(commander_node src/commander_node.cpp)
ament_target_dependencies(commander_node rclcpp std_msgs trajectory_interfaces turtlesim)

install(TARGETS
  commander_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
