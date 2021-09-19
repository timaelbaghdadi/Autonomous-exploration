#include <termios.h>
#include <stdio.h>
#include <ros/ros.h>
#include <vector>
#include <algorithm>

#include "nav_msgs/OccupancyGrid.h"
#include "geometry_msgs/PoseStamped.h"
#include <visualization_msgs/Marker.h>
#include <std_msgs/Float32.h>

#include <move_base_msgs/MoveBaseAction.h>
#include <actionlib/client/simple_action_client.h>

#include <tf/transform_datatypes.h>
#include <tf/transform_listener.h>

// // frontiers
#include <boost/pending/disjoint_sets.hpp>
#include <numeric>
#include <algorithm>

#include <bits/stdc++.h>
#include <cmath>

using namespace std;

actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> * ac_;
int robot_status_=1; //0: moving, 1: goal reached, 2: couldn't reach goal
tf::TransformListener * listener_; //used to fetch transforms between frame_id's
geometry_msgs::Pose robot_pose_; //current robot position
geometry_msgs::Pose prev_robot_pose_; //last robot pose, used to compute travelled distance
nav_msgs::OccupancyGrid map_; //map received from gmapping
nav_msgs::OccupancyGrid frontier_map_; //map with frontiers painted as obstacles
bool map_received=false;
int num_map_cells_=0; //number of map cells (height*width)
int number_of_goals_sent=0; //total sent goals
int number_of_goals_reached=0; //succesfully reached goals
double wheel_travelled_distance=0.0; //stores distance travelled by the robot wheels in meters
ros::Time exploration_time; //used to calculate elapsed time
int explored_cells; //number of explored (!unknown) cells in the map
bool exploration_finished=false; //triggers end of exploration
geometry_msgs::Pose target_goal_; //last sent goal
int min_frontier_size=10; //frontier size threshold

ros::Subscriber sub_map;
ros::Publisher  pub_cand_mark;
ros::Publisher  pub_text_mark;
ros::Publisher  pub_frontiers_mark;
ros::Publisher  pub_frontiers_map;

// list of all frontiers in the occupancy grid
vector<int> labels_;

typedef struct
{
  int id;
  unsigned int size; // frontier size
  geometry_msgs::Point center_point;  // position of the center cell
  int center_cell;
  geometry_msgs::Point free_center_point;  // position of the nearest free cell to the center cell
  int free_center_cell;
  vector<int> frontier_cells; // points in grid cells
  vector<geometry_msgs::Point> frontier_points; // points in grid coordinates
} frontier;
vector<frontier> frontiers_;

//////////// Function headers ////////////

bool moveRobot(const geometry_msgs::Pose& goal);
void move_baseDone(const actionlib::SimpleClientGoalState& state,  const move_base_msgs::MoveBaseResultConstPtr& result);
void move_baseActive();
void publishMarker(uint i, const float & x, const float & y,int color,int type,const float & yaw);
void occupancy_gridCallback(const nav_msgs::OccupancyGrid::ConstPtr& msg);
bool refreshRobotPosition();
void clearMarkers();
geometry_msgs::Pose getRandomPose(float radius);
bool isValidPoint(const geometry_msgs::Point & point);
void finish();
void publishLabels(const std::vector<int> & v);
std_msgs::ColorRGBA colorets(int n);
void findFrontiers();
bool isFrontier(int point);
void twoPassLabeling();
int point2cell(const geometry_msgs::Point & point);
geometry_msgs::Point cell2point(const int & cell);
void getStraightPoints(int point, int points[]);
void getAdjacentPoints(int point, int points[]);
int left(int point);
int upleft(int point);
int up(int point);
int upright(int point);
int right(int point);
int downright(int point);
int down(int point);
int downleft(int point);
int floor0(float value);

//////////////////////////////////////////////////////////////////////
// TODO 1: if you need your own variables and functions
//         define them HERE
//////////////////////////////////////////////////////////////////////

#include <iostream>
#include <fstream>
std:: ofstream MyFile;

bool in_goal_vecinity = false;
ros::Time Tstart;

int frontierId=0;

bool detectFrontierChange(bool replan){
  if (replan){
    return replan;
  }else{
    //// Frontier has changed significantly, so replan and adapt to it //////
    geometry_msgs::Point goal_vs_frontier;
    goal_vs_frontier.x = target_goal_.position.x - frontiers_[frontierId].free_center_point.x;
    goal_vs_frontier.y = target_goal_.position.y - frontiers_[frontierId].free_center_point.y;
    float goal_to_frontier_distance = hypot(goal_vs_frontier.x, goal_vs_frontier.y);
    float dist_thresh = 0.1;
    if (goal_to_frontier_distance > dist_thresh){
      ROS_INFO("Frontier has moved, replaning to find new goal");
      replan = true;
    }
  }
    return replan;
  /////////////////////////////////////////////////////////////////////////
}

typedef struct
{
  int priority;
  double cost;
} info_frontier;
vector<info_frontier> transit_zones;

// returns distance between 2 cells
double radius_f(){
  double radius=0;
  geometry_msgs::Point point_1=cell2point(1);
  geometry_msgs::Point point_2=cell2point(2);
  if((point_1.x-point_2.x) == 0){
    radius=std::sqrt(std::pow(point_1.y-point_2.y,2));
  }
  else{
    radius=std::sqrt(std::pow(point_1.x-point_2.x,2));
  }
  return radius;
}

// returns a vector contining the neighbourhood
vector<int> return_neigh(geometry_msgs::Point central_point){
  double radius=radius_f();
  vector<int> vec;
  geometry_msgs::Point point1;
  geometry_msgs::Point point2;
  geometry_msgs::Point point3;
  geometry_msgs::Point point4;
  geometry_msgs::Point point6;
  geometry_msgs::Point point7;
  geometry_msgs::Point point8;
  geometry_msgs::Point point9;
  geometry_msgs::Point point10;
  geometry_msgs::Point point11;
  geometry_msgs::Point point12;
  geometry_msgs::Point point13;
  geometry_msgs::Point point14;
  geometry_msgs::Point point15;
  geometry_msgs::Point point16;
  geometry_msgs::Point point17;
  geometry_msgs::Point point18;
  geometry_msgs::Point point19;
  geometry_msgs::Point point20;
  geometry_msgs::Point point21;
  geometry_msgs::Point point22;
  geometry_msgs::Point point23;
  geometry_msgs::Point point24;
  geometry_msgs::Point point25;

  point1.x=central_point.x-radius;
  point1.y=central_point.y+radius;
  vec.push_back(point2cell(point1));

  point2.x=central_point.x;
  point2.y=central_point.y+radius;
  vec.push_back(point2cell(point2));

  point3.x=central_point.x+radius;
  point3.y=central_point.y+radius;
  vec.push_back(point2cell(point3));

  point4.x=central_point.x-radius;
  point4.y=central_point.y;
  vec.push_back(point2cell(point4));

  point6.x=central_point.x+radius;
  point6.y=central_point.y;
  vec.push_back(point2cell(point6));

  point7.x=central_point.x-radius;
  point7.y=central_point.y-radius;
  vec.push_back(point2cell(point7));

  point8.x=central_point.x;
  point8.y=central_point.y-radius;
  vec.push_back(point2cell(point8));

  point9.x=central_point.x+radius;
  point9.y=central_point.y-radius;
  vec.push_back(point2cell(point9));

  point10.x=central_point.x-2*radius;
  point10.y=central_point.y+2*radius;
  vec.push_back(point2cell(point10));

  point11.x=central_point.x-radius;
  point11.y=central_point.y+2*radius;
  vec.push_back(point2cell(point11));

  point12.x=central_point.x;
  point12.y=central_point.y+2*radius;
  vec.push_back(point2cell(point12));

  point13.x=central_point.x+radius;
  point13.y=central_point.y+2*radius;
  vec.push_back(point2cell(point13));

  point14.x=central_point.x+2*radius;
  point14.y=central_point.y+2*radius;
  vec.push_back(point2cell(point14));

  point15.x=central_point.x-2*radius;
  point15.y=central_point.y+radius;
  vec.push_back(point2cell(point15));

  point16.x=central_point.x+2*radius;
  point16.y=central_point.y+radius;
  vec.push_back(point2cell(point16));

  point17.x=central_point.x-2*radius;
  point17.y=central_point.y;
  vec.push_back(point2cell(point17));

  point18.x=central_point.x+2*radius;
  point18.y=central_point.y;
  vec.push_back(point2cell(point18));

  point19.x=central_point.x-2*radius;
  point19.y=central_point.y-radius;
  vec.push_back(point2cell(point14));

  point20.x=central_point.x+2*radius;
  point20.y=central_point.y-radius;
  vec.push_back(point2cell(point20));

  point21.x=central_point.x-2*radius;
  point21.y=central_point.y-2*radius;
  vec.push_back(point2cell(point21));

  point22.x=central_point.x-radius;
  point22.y=central_point.y-2*radius;
  vec.push_back(point2cell(point22));

  point23.x=central_point.x;
  point23.y=central_point.y-2*radius;
  vec.push_back(point2cell(point23));

  point24.x=central_point.x+radius;
  point24.y=central_point.y-2*radius;
  vec.push_back(point2cell(point24));

  point25.x=central_point.x+2*radius;
  point25.y=central_point.y-2*radius;
  vec.push_back(point2cell(point25));


  return vec;
}

// returns the value that a concrete frontier has
double cost_function(int priority_zone, vector<frontier> frontiers, int id_frontier, geometry_msgs::Pose robotpose){
  int a=frontiers[id_frontier].size;
  int s=priority_zone;
  double c=hypot(robotpose.position.x - frontiers[id_frontier].free_center_point.x,
    robotpose.position.y - frontiers[id_frontier].free_center_point.y);
  ROS_INFO("FUNCTION: Distance to frontier %f",c);
  ROS_INFO("FUNCTION: priority %d",s);
  ROS_INFO("FUNCTION: frontier size %d",a);
  double f=(double)a * (double)s * exp(1.0/c);
  return f;
}

float getFrontierNormal(int frontierId){
  //////////////////////////////////////////////////////////////////////
  /* getFrontierNormal returns the angle of the orthogonal vector to the
  line from the beginning of the frontier to its end */
  //////////////////////////////////////////////////////////////////////

  // First point (firstp) of the frontier in map coordinates
  geometry_msgs::Point firstp = frontiers_[frontierId].frontier_points[0];
  // Last point (lastp) of the frontier in map coordinates
  geometry_msgs::Point lastp = frontiers_[frontierId].frontier_points[
    frontiers_[frontierId].size-1];
  // Free center point (fcp) and center point (cp)
  geometry_msgs::Point free_centerp = frontiers_[frontierId].free_center_point;
  geometry_msgs::Point centerp = frontiers_[frontierId].center_point;
  // Find vector from first pont of the frontier to last point of the frontier
  geometry_msgs::Point lastp_firstp_vec;
  lastp_firstp_vec.x = lastp.x - firstp.x;
  lastp_firstp_vec.y = lastp.y - firstp.y;
  lastp_firstp_vec.z = lastp.z - firstp.z;
  // Find the normal to vector lastp_firstp_vec in both directions
  geometry_msgs::Point normal1;
  geometry_msgs::Point normal2;
  normal1.x = lastp_firstp_vec.y;
  normal1.y = -lastp_firstp_vec.x;
  normal1.z = lastp_firstp_vec.z;
  normal2.x = -lastp_firstp_vec.y;
  normal2.y = lastp_firstp_vec.x;
  normal2.z = lastp_firstp_vec.z;
  // Find normal at center point
  geometry_msgs::Point normalcp;
  normalcp.x = free_centerp.x - centerp.x;
  normalcp.y = free_centerp.y - centerp.y;
  normalcp.z = free_centerp.z - centerp.z;
  // Check if the normal at center point has more or less the same orientation
  // as normal1 (vector arrows aiming in the same orientation). To do so, compute
  // the dot product and check its sign.
  float dotprod1 = normal1.x*normalcp.x + normal1.y*normalcp.y;
  float dotprod2 = normal2.x*normalcp.x + normal2.y*normalcp.y;
  geometry_msgs::Point orient;
  if(dotprod1 > 0){
    // Then normal1 and normalcp have the same orientation
    orient = normal2;
  } else {
    orient = normal1;
  }

  float yaw = atan2(orient.y,orient.x);;
  return yaw;
}





//////////////////////////////////////////////////////////////////////
// TODO 1 END
//////////////////////////////////////////////////////////////////////

bool replan()
{
  bool replan = false;
  ////////////////////////////////////////////////////////////////////
  // TODO 2: replan
  ////////////////////////////////////////////////////////////////////

  //// If the robot is wandering around the goal, replan //////
    float thresh_goal_dist = 0.5; // Exact value can be tweaked
    ros::Duration thresh_duration = ros::Duration(5.0); // method of ros::Duration to compare
    // Check how far from the goal the robot is
    float goal_dist = hypot(robot_pose_.position.x-target_goal_.position.x, robot_pose_.position.y-target_goal_.position.y);
    // If goal_dist is below a threshold then start counting. If it spends too
    // much time there wandering around replan will be set to true
    if(goal_dist < thresh_goal_dist){
      if(in_goal_vecinity){
        if(ros::Time::now() - Tstart > thresh_duration){
          ROS_INFO("Robot wandering around the goal too much time. Replanning...");
          replan = true;
        }
      }else{
        // If it wasn't in the vecinity of the goal, now it is, so set it to true
        // and start counting time
        ROS_INFO("Robot close to goal...");
        Tstart = ros::Time::now();
        in_goal_vecinity = true;
      }
    }else{
      // It is not in the vecinity of the goal, so set the boolean variable to false
      in_goal_vecinity = false;
    }
  //////////////////////////////////////////////////////////////////
  replan = detectFrontierChange(replan);

  // If the robot is otherwise not moving, replan
  if(robot_status_!=0) // 0 means moving
    replan = true;

  ROS_INFO("Replan? %d",replan);
  ROS_INFO("Robot status? %d",robot_status_);
  ////////////////////////////////////////////////////////////////////
  // TODO 2 END
  ////////////////////////////////////////////////////////////////////
  return replan;
}

geometry_msgs::Pose decideGoal()
{
  geometry_msgs::Pose g;
  g.orientation.w=1.0; //valid quaternion
  ////////////////////////////////////////////////////////////////////
  // TODO 3: decideGoal
  ////////////////////////////////////////////////////////////////////

  int max_frontier_size = 0;
  bool at_least_one_validPoint = false;
  geometry_msgs::Pose g2;
  /* Once all frontiers are scanned go for the biggest one, that is,
  loop for all frontiers and take the biggest one and return it as g */
  // ROS_INFO("yaw to quaternion %f",2*M_PI * (rand()%100)/100.0);
  //vector<info_frontier> transit_zones(frontiers_.size(),2);
  for(unsigned int i = 0; i<frontiers_.size(); i++){
    transit_zones.resize(frontiers_.size());
    bool trafic_area_1=false;
    bool trafic_area_2=false;
    int frontier_size=frontiers_[i].frontier_points.size();
    geometry_msgs::Point central_begin=frontiers_[i].frontier_points[0];
    geometry_msgs::Point central_end=frontiers_[i].frontier_points[frontier_size-1];
    vector<int> vec_neigh_b = return_neigh(central_begin);
    vector<int> vec_neigh_e = return_neigh(central_end);

    // CHECKING THAT THE BEGINING OF THE FRONTIER HAS A CONNECTION WITH THE WALL
    for(unsigned int j = 0; j<vec_neigh_b.size(); j++){
      if(map_.data[vec_neigh_b[j]]==100){
        trafic_area_1=true;
        break;
      }
    }
    // CHECKING THAT THE END OF THE FRONTIER HAS A CONNECTION WITH THE WALL
    for(unsigned int j = 0; j<vec_neigh_e.size(); j++){
      if(map_.data[vec_neigh_e[j]]==100){
        trafic_area_2=true;
        break;
      }
    }
    ROS_INFO("ID FRONTIER %d",frontiers_[i].id);
    ROS_INFO("Trafic area1: %d",trafic_area_1);
    ROS_INFO("Trafic area2: %d",trafic_area_2);
    float dist_front=std::sqrt(std::pow(robot_pose_.position.x-frontiers_[i].free_center_point.x,2)+std::pow(robot_pose_.position.y-frontiers_[i].free_center_point.y,2));
    ROS_INFO("Distance to frontier: %f",dist_front);
    double f_cost=0.0;
    if(trafic_area_1==true || trafic_area_2==true){
      ROS_INFO("transit area");
      f_cost=cost_function(80, frontiers_, i,robot_pose_);
      //transit_zones.push_back({val_frontier,f_cost});
      transit_zones[i].priority=80;
      transit_zones[i].cost=f_cost;
    }
    else if(trafic_area_1==false && trafic_area_2==false){
      ROS_INFO("free area");
      f_cost=cost_function(1, frontiers_, i,robot_pose_);
      //transit_zones.push_back({val_frontier,f_cost});
      transit_zones[i].priority=1;
      transit_zones[i].cost=f_cost;
    }
    ROS_INFO("Cost %f",f_cost);
  }
  //FINDING THE FRONTIER WITH THE MAXIMUM COST
  double max=0;
  int index_max=0;
  for(unsigned int i = 0; i<transit_zones.size(); i++){
    if(transit_zones[i].cost > max){
      max=transit_zones[i].cost;
      index_max=i;
    }
  }
  //SENDING THE ROBOT TO THE POSITION OF THE DECIDED FRONTIER
  float yaw = getFrontierNormal(index_max);
  g2.orientation = tf::createQuaternionMsgFromYaw(yaw);//2*M_PI * (rand()%100)/100.0);
  g2.position.x = frontiers_[index_max].free_center_point.x;
  g2.position.y = frontiers_[index_max].free_center_point.y;
  if(isValidPoint(g2.position)){
    g = g2;
    at_least_one_validPoint = true;
  }

  // There is no validPoint, so rotate a bit the robot...
  if(at_least_one_validPoint == false){
    ROS_INFO("No valid points, rotating robot a bit...");
    // Turn a bit the robot because it is not moving right now
    g.orientation = tf::createQuaternionMsgFromYaw(1*M_PI);
  }

  publishMarker(0,g.position.x,g.position.y,4,1,tf::getYaw(g.orientation));
  ROS_INFO("Returning goal");
  ////////////////////////////////////////////////////////////////////
  // TODO 3 END
  ////////////////////////////////////////////////////////////////////
  ROS_INFO("ID FRONTIER MAX %d",frontiers_[index_max].id);
  frontierId=index_max;
  return g;
}


//////////////////GIVEN FUNCTIONS////////////////////////////
//            Do not modify the following code             //
/////////////////////////////////////////////////////////////

geometry_msgs::Pose getRandomPose(float radius)
{
  geometry_msgs::Pose goal_pose;

  if (radius <= 0)
  {
      ROS_WARN("getRandomPose: radius must be > 0. Changed to 1");
      radius = 1;
  }

  // gives a pose between +-radius limits. If you add it to robot_pose_ coordinates, you have a random pose around the robot
  float rand_yaw = 2*M_PI * (rand()%100)/100.0;
  float rand_r = radius*(rand()%100)/100.0;

  goal_pose.position.x = rand_r * cos(rand_yaw);
  goal_pose.position.y = rand_r * sin(rand_yaw);
  goal_pose.orientation = tf::createQuaternionMsgFromYaw(rand_yaw);

  return goal_pose;
}

//Tells if given point could be a valid goal for the robot: inside map limits, without obstacles around
bool isValidPoint(const geometry_msgs::Point & point)
{
  int N = ceil(0.2/map_.info.resolution); // radius (in cells) of the robot footprint
  int index = point2cell(point); // cell corresponding to the point

  if(index==-1 || map_.data[index]!=0)
    return false; // not valid if the point is out of the map or if the cell is not free

  // Check if no obstacles cells are in the robot footprint
  for(int j=-N; j<=N; j++)
  {
    for(int i=-N; i<=N; i++)
    {
      int cell_i = i*map_.info.width+j+index;
      if(cell_i >0 && cell_i < signed(map_.data.size())) // check if cell_i is in the map_.data size
        if(map_.data[cell_i]==100)
          return false; //if the cell_i cell is an obstacle the goal is not valid
    }
  }
  // otherwise the goal is valid
  return true;
}

void spin()
{
  bool first=true;
  ros::spinOnce();
  ros::NodeHandle n;
  sub_map = n.subscribe("/map", 1, occupancy_gridCallback);
  pub_cand_mark = n.advertise<visualization_msgs::Marker>("candidate_marker", 100);
  pub_text_mark = n.advertise<visualization_msgs::Marker>("text_marker", 100);
  pub_frontiers_mark = n.advertise<visualization_msgs::Marker>("frontiers_marker", 200);
  pub_frontiers_map = n.advertise<nav_msgs::OccupancyGrid>("frontiers_map", 1);
  ros::Rate loop_rate(1);

  while (ros::ok())
  {
    if(map_received)
    {
      map_received=false;
      ROS_INFO_ONCE("First map received!");

      if(first)
      {
        exploration_time = ros::Time::now();
        first=false;
      }

      if(exploration_finished)
        finish();
      else
      {
        if(refreshRobotPosition())
        {
          if(replan())
          {
            target_goal_ = decideGoal();
            moveRobot(target_goal_);
          }
        }
        else
          ROS_WARN("Couldn't get robot position!");
      }
    }
    else
      ROS_INFO_ONCE("Waiting for first map");

    ros::spinOnce();
    loop_rate.sleep();
  }
}

int main(int argc, char** argv)
{
  ROS_INFO("Exploration Node");
  ros::init(argc, argv, "exploration");
  listener_ = new tf::TransformListener();
  ac_ = new actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction>("move_base",true);
  spin();
  delete listener_;
  return 0;
}

// Cancels the goal, prints the results of the exploration and kills the node
void finish()
{
    if (robot_status_==0)
        ac_->cancelGoal();
    ros::Duration t = ros::Time::now() - exploration_time;
    int elapsed_time_minutes = int(t.toSec())/60;
    int elapsed_time_seconds = int(t.toSec())%60;
    ROS_INFO_ONCE("END OF EXPLORATION");
    ROS_INFO_ONCE("--- Sent %d goals (reached %d).", number_of_goals_sent, number_of_goals_reached);
    ROS_INFO_ONCE("--- Wheel travelled %.2f meters.", wheel_travelled_distance);

    ROS_INFO_ONCE("--- Took %2.2i:%2.2i minutes", elapsed_time_minutes, elapsed_time_seconds);
    ROS_INFO_ONCE("--- Explored %.2f m^2 (%d cells).", explored_cells*map_.info.resolution*map_.info.resolution, explored_cells);
    ROS_INFO_ONCE("If you want to save the current map, remember to do: $rosrun map_server map_saver -f my_map_name");

    ros::shutdown();
}

// Occupancy Grid Callback: called each time a map message is received
void occupancy_gridCallback(const nav_msgs::OccupancyGrid::ConstPtr& msg)
{
  //ROS_INFO("Occupancy Grid data received: resolution %f cells, width %d cells, height %d cells",msg->info.resolution, msg->info.width, msg->info.height);
  map_=*msg;
  num_map_cells_=map_.info.width*map_.info.height;
  frontier_map_=*msg;
  map_received=true;
  findFrontiers();
}

// NAVIGATION FUNCTIONS ////////////////////////////////////////////
//moveRobot: sends goal to the robot
bool moveRobot(const geometry_msgs::Pose& goal_pose)
{
  //tell the action client that we want to spin a thread by default
  //MoveBaseClient ac("move_base", true);

  //wait for the action server to come up
  if(!ac_->waitForServer(ros::Duration(5.0)))
  {
    ROS_INFO("moveRobot: Waiting for the move_base action server to come up");
    return false;
  }

  // overwrite the frame_id and stamp
  move_base_msgs::MoveBaseGoal goal;
  goal.target_pose.header.frame_id = "/map";
  goal.target_pose.header.stamp = ros::Time::now();
  goal.target_pose.pose = goal_pose;

  number_of_goals_sent++;
  ROS_INFO("moveRobot: Sending Goal #%d: x=%4.2f, y=%4.2f, yaw=%4.2f in frame=%s",
           number_of_goals_sent,
           goal.target_pose.pose.position.x,
           goal.target_pose.pose.position.y,
           tf::getYaw(goal.target_pose.pose.orientation) ,
           goal.target_pose.header.frame_id.c_str());

  ros::Duration t = ros::Time::now() - exploration_time;
  int elapsed_time_minutes = int(t.toSec())/60;
  int elapsed_time_seconds = int(t.toSec())%60;
  MyFile.open("cost.txt", std::ofstream::out | std::ofstream::app);
  MyFile<< wheel_travelled_distance;
  MyFile<< " ";
  MyFile<< (elapsed_time_minutes*60 + elapsed_time_seconds);
  MyFile<< " ";
  MyFile<<  explored_cells*map_.info.resolution*map_.info.resolution;
  MyFile<< "\n";
  MyFile.close();
  ROS_INFO("Exploration status: Sent %d goals (reached %d). Wheel travelled %.2f meters. Elapsed %2.2i:%2.2i min. Explored %.2f m^2 (%d cells)",
           number_of_goals_sent,
           number_of_goals_reached,
           wheel_travelled_distance,
           elapsed_time_minutes, elapsed_time_seconds,
           explored_cells*map_.info.resolution*map_.info.resolution,
           explored_cells);

  ac_->sendGoal(goal,
            boost::bind(&move_baseDone, _1, _2),
            boost::bind(&move_baseActive));
  robot_status_=0; //moving
  return true;
}

//move_baseDone: called when the robot reaches the goal, or navigations finishes for any reason
void move_baseDone(const actionlib::SimpleClientGoalState& state,  const move_base_msgs::MoveBaseResultConstPtr& result)
{
  //ROS_INFO("move_baseDone");
  if(state==actionlib::SimpleClientGoalState::SUCCEEDED)
  {
    robot_status_=1; //succedeed
    number_of_goals_reached++;
  }
  else
    robot_status_=2; //error
}

//move_baseActive: called when navigations receives the sent goal
void move_baseActive()
{
  //ROS_INFO("move_baseActive");
  robot_status_ = 0; //moving
}

// asks TF for the current position of the robot in map coordinates
bool refreshRobotPosition()
{
  static bool first=true;
  tf::StampedTransform transform;
  ros::Time target_time = ros::Time(0); //ros::Time::now();
  std::string source_frame = "/map";
  std::string target_frame = "/base_footprint";
  try
  {
    if(listener_->waitForTransform(source_frame, target_frame, target_time, ros::Duration(5.0)))
        listener_->lookupTransform(source_frame, target_frame, target_time, transform);
    else
    {
      ROS_ERROR("refreshRobotPosition: no transform between frames %s and %s", source_frame.c_str(), target_frame.c_str());
      return false;
    }
  }
  catch(tf::TransformException ex)
  {
    ROS_ERROR("%s",ex.what());
    return false;
  }
  robot_pose_.position.x = transform.getOrigin().x();
  robot_pose_.position.y = transform.getOrigin().y();
  robot_pose_.position.z = 0.0;
  robot_pose_.orientation = tf::createQuaternionMsgFromYaw(tf::getYaw(transform.getRotation()));
  //ROS_INFO("base_link|map: x:%fm y:%fm th:%fº", robot_pose_.position.x, robot_pose_.position.y, tf::getYaw(robot_pose_.orientation));

  if(first)
  {
    prev_robot_pose_=robot_pose_;
    first=false;
  }

  float dist_incr=std::sqrt(std::pow(prev_robot_pose_.position.x-robot_pose_.position.x,2)+std::pow(prev_robot_pose_.position.y-robot_pose_.position.y,2));
  float angle_incr=fabs(tf::getYaw(prev_robot_pose_.orientation)-tf::getYaw(robot_pose_.orientation));
  float d=0.115; //half of the distance between wheels
  wheel_travelled_distance += (fabs(dist_incr+d*angle_incr)+fabs(dist_incr-d*angle_incr))/2.0f;

  prev_robot_pose_=robot_pose_;
  return true;
}

// MARKERS MANAGING FUNCITONS /////////////////////////////////////////////////
void publishMarker(uint id, const float & x, const float & y,int color,int type,const float & yaw)
{
  visualization_msgs::Marker marker, marker_text;
  // Set the frame ID and timestamp.  See the TF tutorials for information on these.
  marker.header.frame_id      = "/map";
  marker.header.stamp         = ros::Time::now();
  marker_text.header.frame_id = "/map";
  marker_text.header.stamp    = ros::Time::now();

  // Set the namespace and id for this marker.  This serves to create a unique ID
  // Any marker sent with the same namespace and id will overwrite the old one
  marker.ns = "candidates";
  marker_text.ns = "candidates_text";
  marker.id = marker_text.id = id;
  stringstream ss;
  ss << id;
  marker_text.text = ss.str();

  // Set the marker type.  Initially this is CUBE, and cycles between that and SPHERE, ARROW, and CYLINDER
  // Set the scale of the marker -- 1x1x1 here means 1m on a side
  switch(type)
  {
    default:
    case 0:
      marker.type = visualization_msgs::Marker::CYLINDER;
      marker.scale.x = marker.scale.y = 0.10; marker.scale.z = 0.5;
      break;
    case 1:
      marker.type = visualization_msgs::Marker::ARROW;
      marker.scale.x = 0.5;
      marker.scale.y = 0.2;
      marker.scale.z = 0.1;
      break;
  }
  marker_text.type = visualization_msgs::Marker::TEXT_VIEW_FACING;
  marker_text.scale.x = marker_text.scale.y = 1;
  marker_text.scale.z = 0.5;

  // Set the marker action.  Options are ADD and DELETE
  marker.action = visualization_msgs::Marker::ADD;
   marker_text.action = visualization_msgs::Marker::ADD;

  // Set the pose of the marker.  This is a full 6DOF pose relative to the frame/time specified in the header
  marker.pose.position.x = marker_text.pose.position.x = x;
  marker.pose.position.y = marker_text.pose.position.y = y;
  marker.pose.position.z = 0;
  marker_text.pose.position.z = 0.5;
  tf::quaternionTFToMsg (tf::createQuaternionFromRPY(0.0,0.0,yaw), marker.pose.orientation);
  tf::quaternionTFToMsg (tf::createQuaternionFromRPY(0.0,0.0,yaw), marker_text.pose.orientation);

  // Set the lifetime of the marker
  marker.lifetime = ros::Duration(0);
  marker_text.lifetime = ros::Duration(0);

  // Set the color -- be sure to set alpha to something non-zero!
  marker.color = colorets(color);
  marker_text.color = marker.color;

  // Publish the marker
  pub_cand_mark.publish(marker);
  pub_text_mark.publish(marker_text);
}

// publishes the frontiers using only a marker
void publishLabels(const std::vector<int> & v)
{
  // From labels to x-y-z-r-g-b points
  // Each label will have a color
  std::vector<geometry_msgs::Point> p;
  std::vector<std_msgs::ColorRGBA> c;
  p.reserve(v.size());
  c.reserve(v.size());
  for(unsigned int i = 0; i < v.size(); ++i)
  {
    if(v[i]!=0)
    {
      geometry_msgs::Point d = cell2point(i);
      p.push_back(d);
      c.push_back(colorets(v[i]));
    }
  }
  visualization_msgs::Marker marker;
  marker.header.frame_id = "/map";
  marker.header.stamp = ros::Time::now();
  marker.ns = "frontiers";
  marker.id = 1;
  marker.type = visualization_msgs::Marker::POINTS;
  marker.action = visualization_msgs::Marker::ADD;
  marker.pose.position.x = marker.pose.position.y = 0.0; marker.pose.position.z = 0;
  marker.pose.orientation.x = marker.pose.orientation.y = marker.pose.orientation.z = 0; marker.pose.orientation.w = 1;
  marker.scale.x = marker.scale.y = marker.scale.z = map_.info.resolution;
  marker.lifetime = ros::Duration();
  marker.points = p;
  marker.colors = c;

  pub_frontiers_mark.publish(marker);
}

// clears frontier centroids markers
void clearMarkers()
{
  visualization_msgs::Marker marker;
  visualization_msgs::Marker marker_text;
  marker.ns = "candidates";
  marker_text.ns = "candidates_text";
  marker.action = marker_text.action = visualization_msgs::Marker::DELETE;
  for(unsigned int i = 0; i < frontiers_.size(); ++i)
  {
    marker.id = frontiers_[i].id;
    marker_text.id = frontiers_[i].id;
    marker.header.frame_id = "/map";
    marker_text.header.frame_id = "/map";
    pub_cand_mark.publish(marker);
    pub_text_mark.publish(marker_text);
  }
}

// assigns colours in a kind of rainbow basis
std_msgs::ColorRGBA colorets(int n)
{
  std_msgs::ColorRGBA c;
  c.a = 1.0;
  c.r = sin(n*0.3);
  c.g = sin(n*0.3 + 2*M_PI/3);
  c.b = sin(n*0.3 + 4*M_PI/3);
  return c;
}

// FRONTIER MANAGING FUNCITONS /////////////////////////////////////////////////
void findFrontiers()
{
  frontier_map_.data.resize(num_map_cells_);
  frontier_map_.data.assign(frontier_map_.data.size(), 0);
  clearMarkers();
  explored_cells = 0;
  //check for all cells in the occupancy grid whether or not they are frontier cells
  for(unsigned int i = 0; i < frontier_map_.data.size(); ++i)
  {
    if(isFrontier(i))
      frontier_map_.data[i] = 100;
    else
      frontier_map_.data[i] = 0;
    // count the discovered cells
    if (map_.data[i] != -1)
        explored_cells++;
  }

  // publish frontier map
  ros::NodeHandle n;
  ros::Publisher  pub2 = n.advertise<nav_msgs::OccupancyGrid>("frontiers_map", 1);
  pub2.publish(frontier_map_);

  // Label frontiers (connected cells)
  twoPassLabeling();

  // Create frontiers structs
  frontiers_.clear();
  for (unsigned int i = 0; i < labels_.size(); ++i)
  {
    if(labels_[i]!=0) //frontier labeled cell
    {
      // search for existing frontier
      bool new_label = true;
      for (unsigned int j = 0; j < frontiers_.size(); j++)
      {
        // found
        if (frontiers_[j].id == labels_[i])
        {
          frontiers_[j].frontier_cells.push_back(i);
          frontiers_[j].frontier_points.push_back(cell2point(i));
          new_label = false;
          break;
        }
      }
      // not found
      if (new_label)
      {
        frontiers_.resize(frontiers_.size()+1);
        frontiers_.back().frontier_cells.push_back(i);
        frontiers_.back().frontier_points.push_back(cell2point(i));
        frontiers_.back().id = labels_[i];
      }
    }
  }
  // Search middle cell
  for(unsigned int i = 0; i < frontiers_.size(); ++i)
  {
    frontiers_[i].size = frontiers_[i].frontier_cells.size();
    int label = frontiers_[i].id;

    // order the frontier cells
    std::deque<int> ordered_cells(0);
    ordered_cells.push_back(frontiers_[i].frontier_cells.front());
    while (ordered_cells.size() < frontiers_[i].size)
    {
        int initial_size = ordered_cells.size();

        // connect cells to first cell
        int frontAdjacentPoints[8];
        getAdjacentPoints(ordered_cells.front(), frontAdjacentPoints);
        for (unsigned int k = 0; k<8; k++)
        {
            if (frontAdjacentPoints[k] != -1 && labels_[frontAdjacentPoints[k]] == label && std::find(ordered_cells.begin(), ordered_cells.end(), frontAdjacentPoints[k]) == ordered_cells.end() )
            {
                ordered_cells.push_front(frontAdjacentPoints[k]);
                break;
            }
        }

        // connect cells to last cell
        int backAdjacentPoints[8];
        getAdjacentPoints(ordered_cells.back(), backAdjacentPoints);
        for (unsigned int k = 0; k<8; k++)
        {
            if (backAdjacentPoints[k] != -1 && labels_[backAdjacentPoints[k]] == label && std::find(ordered_cells.begin(), ordered_cells.end(), backAdjacentPoints[k]) == ordered_cells.end() )
            {
                ordered_cells.push_back(backAdjacentPoints[k]);
                break;
            }
        }

        if (initial_size == ordered_cells.size() && ordered_cells.size() < frontiers_[i].size)
            break;
    }
    // center cell
    frontiers_[i].center_cell = ordered_cells[ordered_cells.size() / 2];

    // find the close free cell of the middle frontier cell
    int adjacentPoints[8];
    getAdjacentPoints(frontiers_[i].center_cell, adjacentPoints);
    for (unsigned int k = 0; k<8; k++)
    {
        if (map_.data[adjacentPoints[k]] == 0) // free neighbor cell
        {
          frontiers_[i].free_center_cell = adjacentPoints[k];
          break;
        }
        if (k == 7)
          ROS_ERROR("findFrontiers: No free cell close to the center frontier cell!");
    }
    frontiers_[i].center_point = cell2point(frontiers_[i].center_cell);
    frontiers_[i].free_center_point = cell2point(frontiers_[i].free_center_cell);
    publishMarker(frontiers_[i].id, frontiers_[i].free_center_point.x, frontiers_[i].free_center_point.y, frontiers_[i].id, 0, 0);
  }
  publishLabels(labels_);

  //if all frontiers are smaller than 'min_frontier_size', we will end the exploration
  if(frontiers_.size()>0)
  {
    bool all_frontiers_are_small=true;
    for(unsigned int i = 0; i < frontiers_.size(); ++i)
    {
      if(frontiers_[i].size > min_frontier_size)
      {
        all_frontiers_are_small=false;
        break;
      }
    }

    if(all_frontiers_are_small)
    {
      ROS_WARN("All frontiers found are smaller than threshold (size %d).",min_frontier_size);
      exploration_finished=true;
    }
  }
  else
  {
    ROS_WARN("No frontiers found.");
    exploration_finished=true;
  }
}

// Check if a cell is frontier
bool isFrontier(int cell)
{
  if(map_.data[cell] == -1) //check if it is unknown
  {
    int straightPoints[4];
    getStraightPoints(cell,straightPoints);
    for(int i = 0; i < 4; ++i)
      if(straightPoints[i] != -1 && map_.data[straightPoints[i]] == 0) //check if any neigbor is free space
        return true;
  }
  // If it is obstacle or free can not be frontier
  return false;
}

// Two pass labeling to label frontiers [http://en.wikipedia.org/wiki/Connected-component_labeling]
void twoPassLabeling()
{
  labels_.assign(frontier_map_.data.begin(), frontier_map_.data.end());
  vector<int> neigh_labels;
  vector<int> rank(1000);
  vector<int> parent(1000);
  boost::disjoint_sets<int*,int*> dj_set(&rank[0], &parent[0]);
  int current_label_=1;

  // 1ST PASS: Assign temporary labels to frontiers and establish relationships
  for(unsigned int i = 0; i < frontier_map_.data.size(); i++)
  {
    if( frontier_map_.data[i] != 0)
    {
      neigh_labels.clear();
      // Find 8-connectivity neighbours already labeled
      if(upleft(i)  != -1 && labels_[upleft(i)]  != 0) neigh_labels.push_back(labels_[upleft(i)]);
      if(up(i)      != -1 && labels_[up(i)]      != 0) neigh_labels.push_back(labels_[up(i)]);
      if(upright(i) != -1 && labels_[upright(i)] != 0) neigh_labels.push_back(labels_[upright(i)]);
      if(left(i)    != -1 && labels_[left(i)]    != 0) neigh_labels.push_back(labels_[left(i)]);

      if(neigh_labels.empty())                                                  // case: No neighbours
      {
        dj_set.make_set(current_label_);                                        //   create new set of labels
        labels_[i] = current_label_;                                            //   update cell's label
        current_label_++;                                                       //   update label
      }
      else                                                                      // case: With neighbours
      {
        labels_[i] = *std::min_element(neigh_labels.begin(), neigh_labels.end());//   choose minimum label of the neighbours
        for(unsigned int j = 0; j < neigh_labels.size(); ++j)                   //   update neighbours sets
          dj_set.union_set(labels_[i],neigh_labels[j]);                          //   unite sets minimum label with the others
      }
    }
  }

  // 2ND PASS: Assign final label
  dj_set.compress_sets(labels_.begin(), labels_.end());
  // compress sets for efficiency
  for(unsigned int i = 0; i < frontier_map_.data.size(); i++)
    if( labels_[i] != 0)
      labels_[i] = dj_set.find_set(labels_[i]);                                 // relabel each element with the lowest equivalent label
}

/*
 These functions calculate the index of an adjacent point
 (left,upleft,up,upright,right,downright,down,downleft) to the given point.
 If there is no such point (meaning the point would cause the index to be out of bounds),
 -1 is returned.
 */
int point2cell(const geometry_msgs::Point & point)
{
  if(point.x <= map_.info.origin.position.x || point.x >= map_.info.width*map_.info.resolution + map_.info.origin.position.x  ||
     point.y <= map_.info.origin.position.y || point.y >= map_.info.height*map_.info.resolution+ map_.info.origin.position.y)
  {
    return -1;
  }

  int x_cell = floor0((point.x - map_.info.origin.position.x)/map_.info.resolution);
  int y_cell = floor0((point.y - map_.info.origin.position.y)/map_.info.resolution);
  int cell = x_cell + (y_cell)*map_.info.width;
  return cell;
}
geometry_msgs::Point cell2point(const int & cell)
{
  geometry_msgs::Point point;
  point.x = (cell % map_.info.width)*map_.info.resolution + map_.info.origin.position.x + map_.info.resolution / 2;
  point.y = floor(cell/map_.info.width)*map_.info.resolution + map_.info.origin.position.y + map_.info.resolution / 2;
  return point;
}

void getStraightPoints(int cell, int cells[])
{
  cells[0] = left(cell);
  cells[1] = up(cell);
  cells[2] = right(cell);
  cells[3] = down(cell);
}
void getAdjacentPoints(int cell, int cells[])
{
  cells[0] = left(cell);
  cells[1] = up(cell);
  cells[2] = right(cell);
  cells[3] = down(cell);
  cells[4] = upleft(cell);
  cells[5] = upright(cell);
  cells[6] = downright(cell);
  cells[7] = downleft(cell);
}
int right(int cell)
{
  // only go left if no index error and if current cell is not already on the left boundary
  if((cell % map_.info.width != 0))
    return cell+1;

  return -1;
}
int upright(int cell)
{
  if((cell % map_.info.width != 0) && (cell >= (int)map_.info.width))
    return cell-map_.info.width+1;

  return -1;
}
int up(int cell)
{
  if(cell >= (int)map_.info.width)
    return cell-map_.info.width;

  return -1;
}
int upleft(int cell)
{
  if((cell >= (int)map_.info.width) && ((cell + 1) % (int)map_.info.width != 0))
    return cell-map_.info.width-1;

  return -1;
}
int left(int cell)
{
  if((cell + 1) % map_.info.width != 0)
    return cell-1;

  return -1;
}
int downleft(int cell)
{
  if(((cell + 1) % map_.info.width != 0) && ((cell/map_.info.width) < (map_.info.height-1)))
    return cell+map_.info.width-1;

  return -1;
}
int down(int cell)
{
  if((cell/map_.info.width) < (map_.info.height-1))
    return cell+map_.info.width;

  return -1;
}
int downright(int cell)
{
  if(((cell/map_.info.width) < (map_.info.height-1)) && (cell % map_.info.width != 0))
    return cell+map_.info.width+1;

  return -1;
}
int floor0(float value)
{
  if (value < 0.0)
    return ceil( value );
  else
    return floor( value );
}
