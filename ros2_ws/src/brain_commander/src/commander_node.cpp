/*
 * commander_node.cpp
 *
 * Node "cerveau" du projet : calcule la commande de deplacement de la tortue
 * et la publie sur /trajectory_cmd. Le node "turtle_executor" (Python) se
 * charge de transformer cette commande en Twist pour turtlesim.
 */

#include <chrono>
#include <memory>
#include <functional>
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/bool.hpp"
#include "trajectory_interfaces/msg/trajectory_command.hpp"

using namespace std::chrono_literals;

class CommanderNode : public rclcpp::Node
{
public:
  CommanderNode()
  : Node("commander_node")
  {
    publisher_ = this->create_publisher<trajectory_interfaces::msg::TrajectoryCommand>(
      "/trajectory_cmd", 10
    );
    timer_= this->create_wall_timer(
      100ms,
      std::bind(&CommanderNode::timer_callback, this));
    

    // Un noeud externe pour publier sur /obstacle_alert afin de simuler la detection d'un obstacle.
    //
     obstacle_subscription_ = this->create_subscription<std_msgs::msg::Bool>(
     "/obstacle_alert", 10,
     std::bind(&CommanderNode::obstacle_callback, this, std::placeholders::_1));

    RCLCPP_INFO(this->get_logger(), "commander_node demarre.");
  }

private:
enum Etat
{
  GRAND_COTE,
  TOURNE_APRES_GRAND_COTE,
  PETIT_COTE,
  TOURNE_APRES_PETIT_COTE
};

void timer_callback()
{
  trajectory_interfaces::msg::TrajectoryCommand message;
  if (obstacle_detected_)
{
  message.linear_speed = 0.0;
  message.angular_speed = 0.0;
  message.avoid_obstacle = true;

  publisher_->publish(message);
  return;
}
  compteur_= compteur_+ 1;
  if (etat_ == GRAND_COTE)
  {
    message.linear_speed = 2.0;
    message.angular_speed = 0.0;

    if (compteur_ >= 20)
    {
      etat_ = TOURNE_APRES_GRAND_COTE;
      compteur_ = 0;
    }
  }
  else if (etat_ == TOURNE_APRES_GRAND_COTE)
  {
    message.linear_speed = 0.0;
    message.angular_speed = 1.57;

    if (compteur_ >= 10)
    {
      etat_ = PETIT_COTE;
      compteur_ = 0;
    }
  }
  else if (etat_ == PETIT_COTE)
  {
    message.linear_speed = 2.0;
    message.angular_speed = 0.0;

    if (compteur_ >= 10)
    {
      etat_ = TOURNE_APRES_PETIT_COTE;
      compteur_ = 0;
    }
  }
  else if (etat_ == TOURNE_APRES_PETIT_COTE)
  {
    message.linear_speed = 0.0;
    message.angular_speed = 1.57;

    if (compteur_ >= 10)
    {
      etat_ = GRAND_COTE;
      compteur_ = 0;
    }
  }

  message.avoid_obstacle = false;

  publisher_->publish(message);
}

rclcpp::Publisher<trajectory_interfaces::msg::TrajectoryCommand>::SharedPtr publisher_;
rclcpp::TimerBase::SharedPtr timer_;
rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr obstacle_subscription_;
bool obstacle_detected_ = false;

Etat etat_ = GRAND_COTE;
int compteur_ = 0;
void obstacle_callback(const std_msgs::msg::Bool::SharedPtr message)
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

  // -----------------------------------------------------------------
  // TODO 1 : Implementer une trajectoire non triviale.
  //
  // Objectif : faire dessiner a la tortue une trajectoire definie
  // (par exemple un carre, un cercle, ou une suite de points).
  //
  // Suggestion d'approche pour un carre :
  //   - definir un etat interne (ex: enum { AVANCE, TOURNE })
  //   - avancer pendant N secondes, puis tourner de 90 degres,
  //     puis repeter
  //   - vous aurez besoin d'un ou plusieurs membres prives pour
  //     suivre le temps ecoule / l'etat courant (voir section
  //     "membres prives" plus bas)
  //
  // Pour l'instant, ce squelette avance tout droit en continu.
  // C'est a vous de le faire evoluer.
  // -----------------------------------------------------------------

  // -----------------------------------------------------------------
  // TODO 2 : Reagir a la detection d'un obstacle.
  //
  // La variable obstacle_detected_ est mise a jour automatiquement
  // par obstacle_callback() ci-dessus des qu'un message arrive sur
  // /obstacle_alert (testable avec :
  //   ros2 topic pub /obstacle_alert std_msgs/msg/Bool "{data: true}"
  // ).
  //
  // A vous de decider ce que la tortue doit faire quand un obstacle
  // est detecte (s'arreter ? reculer ? tourner ?), et de l'implementer
  // ici. Pensez a mettre message.avoid_obstacle a jour en consequence.
  // -----------------------------------------------------------------
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CommanderNode>());
  rclcpp::shutdown();
  return 0;
}
