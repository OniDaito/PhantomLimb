/*
* @brief PhantomLimb Physics Header
* @file physics.hpp
* @author Benjamin Blundell <oni@section9.co.uk>
* @date 14/01/2014
*
*/

#ifndef PHANTOM_PHYSICS_HPP
#define PHANTOM_PHYSICS_HPP

#include "s9/common.hpp"

#include <LinearMath/btAlignedObjectArray.h>
#include <btBulletDynamicsCommon.h>

#include <mutex>

namespace s9 {

  /**
   * A class that deals with BulletPhysics to create a set of balls that the user can hit
   */

  class PhantomPhysics  {
    
  public:

    PhantomPhysics() {}

    PhantomPhysics(float_t gravity, float_t hand_radius);

    void Reset();

    void Update(double dt);

    void AddBall (float_t radius, glm::vec3 pos, glm::vec3 velocity);

    void MoveLeftHand(glm::vec3 pos);

    void MoveRightHand(glm::vec3 pos);

    std::vector<glm::mat4>& ball_orients () { CXSHARED return obj_->ball_orients; }


  private:

    struct SharedObject {
      SharedObject(float_t gravity, float_t hand_radius);
      ~SharedObject();

      void InitPhysics();
      void ExitPhysics();

      btVector3 gravity_vector;
       //keep the collision shapes, for deletion/cleanup
      btAlignedObjectArray<btCollisionShape*>   collision_shapes;
      btAlignedObjectArray<btRigidBody*>        balls;
      btBroadphaseInterface*                    broadphase;
      btCollisionDispatcher*                    dispatcher;
      btConstraintSolver*                       solver;
      btDefaultCollisionConfiguration*          collision_configuration;
      btDiscreteDynamicsWorld*                  dynamics_world;

      btRigidBody *left_hand, *right_hand, *ground;

      float_t  hand_radius;

      std::vector<glm::mat4> ball_orients;

      bool running_ = false;

      std::mutex update_mutex;

    };
   
    std::shared_ptr<SharedObject> obj_;

  public:
    bool operator == (const PhantomPhysics &ref) const { return this->obj_ == ref.obj_; }
    typedef std::shared_ptr<SharedObject> PhantomPhysics::*unspecified_bool_type;
    operator unspecified_bool_type() const { return ( obj_.get() == 0 ) ? 0 : &PhantomPhysics::obj_; }
    void reset() { obj_.reset(); }

  };

}

#endif