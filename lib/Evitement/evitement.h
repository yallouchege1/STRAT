#ifndef __EVITEMENT_H
#define __EVITEMENT_H
#include "instruction.h"

typedef enum
{
    NO_POINT,
    DANGER_MV,//mouvement
    DANGER_ST,//statique
    OBSTACLE_SUR_TRAJECTOIRE,
    NO_DANGER,
}Evitement_etat;


class Evitement {
//protected:


public:
   // Evitement();
    void trameCan(const CANMessage *msg);
    int lidar_danger(short x_obstacle, short y_obstacle, signed short angle_obstacle, int distance);
    void lidar_end_danger(Instruction* instruction, S_Dodge_queue* dodgeq, signed short local_target_x_robot, signed short local_target_y_robot, signed short local_target_theta_robot);
};

#endif