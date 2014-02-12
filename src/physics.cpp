/**
* @brief Physics Class for Bullet3
* @file physics.cpp
* @author Benjamin Blundell <oni@section9.co.uk>
* @date 14/01/2014
*
*/

#include "physics.hpp"

#include "btBulletDynamicsCommon.h"


using namespace std;
using namespace s9;


/// Phantom Physics main constructor
PhantomPhysics::PhantomPhysics(float_t gravity, float_t hand_radius) 
  : obj_ (std::shared_ptr<SharedObject>(new SharedObject(gravity, hand_radius))) {

}

void PhantomPhysics::Reset() {
  CXSHARED
  obj_->ExitPhysics();
  obj_->InitPhysics();
}

void PhantomPhysics::AddBall(float_t radius, glm::vec3 pos, glm::vec3 velocity){

  CXSHARED

  if ( obj_->balls.size() > 20)
    Reset();

  //create a few dynamic rigidbodies
  // Re-using the same collision is better for memory usage and performance

  btCollisionShape* colShape = new btSphereShape(btScalar(radius));
  obj_->collision_shapes.push_back(colShape);

  /// Create Dynamic Objects
  btTransform startTransform;
  startTransform.setIdentity();

  btScalar  mass(1.f);

  //rigidbody is dynamic if and only if mass is non zero, otherwise static
  bool isDynamic = (mass != 0.f);

  btVector3 localInertia(velocity.x, velocity.y, velocity.z);
  if (isDynamic)
    colShape->calculateLocalInertia(mass,localInertia);

  startTransform.setOrigin(btVector3( btScalar(pos.x), btScalar(pos.y), btScalar(pos.z)));

    
  //using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
  btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
  btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,colShape,localInertia);
  btRigidBody* body = new btRigidBody(rbInfo);

  body->setLinearVelocity( btVector3(velocity.x, velocity.y, velocity.z) );

  obj_->dynamics_world->addRigidBody(body);

  // Keep a tally of the balls in flight and a useful orientation matrix
  obj_->balls.push_back(body);
  obj_->ball_orients.push_back(glm::translate(glm::mat4(1.0f), pos ));
  
}


// Update with dt in seconds passed
void PhantomPhysics::Update(double dt){
  CXSHARED

  obj_->update_mutex.lock();
  if (obj_->dynamics_world && obj_->running_) {
    obj_->dynamics_world->stepSimulation(dt, 10);
    
    // Update handy ball matrices
    for (size_t i = 0; i < obj_->balls.size(); ++i) {
      btRigidBody* ball_body = obj_->balls[i];
      btTransform trans;
      ball_body->getMotionState()->getWorldTransform(trans);
      trans.getOpenGLMatrix(  glm::value_ptr(obj_->ball_orients[i]) );
    }
  }
  obj_->update_mutex.unlock();

  // Cheeky little reset in game
 
}

void PhantomPhysics::MoveLeftHand(glm::vec3 pos) {
  CXSHARED
  //obj_->left_hand->translate( btVector3( pos.x, pos.y, pos.z ));
  btTransform newTrans;

  obj_->left_hand->getMotionState()->getWorldTransform(newTrans);
  newTrans.setOrigin( btVector3( pos.x, pos.y, pos.z ));
  obj_->left_hand->setActivationState(4);
  obj_->left_hand->getMotionState()->setWorldTransform(newTrans);

}

void PhantomPhysics::MoveRightHand(glm::vec3 pos){
  CXSHARED
  //obj_->right_hand->translate( btVector3( pos.x, pos.y, pos.z ));
  btTransform newTrans;
  
  obj_->right_hand->getMotionState()->getWorldTransform(newTrans);
  newTrans.setOrigin( btVector3( pos.x, pos.y, pos.z ));
  obj_->right_hand->setActivationState(4);
  obj_->right_hand->getMotionState()->setWorldTransform(newTrans);

 // ms.setWorldTransform(trans)
 // body.setMotionState(ms)

}


PhantomPhysics::SharedObject::SharedObject(float_t gravity, float_t hand_radius) {
  gravity_vector = btVector3(0,gravity,0);
  this->hand_radius = hand_radius;
  InitPhysics();
}

PhantomPhysics::SharedObject::~SharedObject() {
  ///\todo will need some cleanup I suspect?
  //ExitPhysics();
}

void PhantomPhysics::SharedObject::InitPhysics() {

  ///collision configuration contains default setup for memory, collision setup
  collision_configuration = new btDefaultCollisionConfiguration();
  //m_collisionConfiguration->setConvexConvexMultipointIterations();

  ///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
  dispatcher = new  btCollisionDispatcher(collision_configuration);
  broadphase = new btDbvtBroadphase();

  ///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
  btSequentialImpulseConstraintSolver* sol = new btSequentialImpulseConstraintSolver;
  solver = sol;

  dynamics_world = new btDiscreteDynamicsWorld(dispatcher,broadphase,solver,collision_configuration);
  dynamics_world->setGravity(gravity_vector);

  ///create a few basic rigid bodies
  btBoxShape* groundShape = new btBoxShape(btVector3(btScalar(50.),btScalar(50.),btScalar(50.)));

  collision_shapes.push_back(groundShape);

  btTransform groundTransform;
  groundTransform.setIdentity();
  groundTransform.setOrigin(btVector3(0,-50,0));

  //We can also use DemoApplication::localCreateRigidBody, but for clarity it is provided here:
  {
    btScalar mass(0.);

    //rigidbody is dynamic if and only if mass is non zero, otherwise static
    bool isDynamic = (mass != 0.f);

    btVector3 localInertia(0,0,0);
    if (isDynamic)
      groundShape->calculateLocalInertia(mass,localInertia);

    //using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
    btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass,myMotionState,groundShape,localInertia);
    ground = new btRigidBody(rbInfo);

    //add the body to the dynamics world
    dynamics_world->addRigidBody(ground);
  }

  // Add both the hands
  {
    btCollisionShape* colShape = new btSphereShape(btScalar(hand_radius));
    collision_shapes.push_back(colShape);

    /// Create Dynamic Objects
    btTransform startTransform;
    startTransform.setIdentity();

    btScalar  mass(0.f);

    //rigidbody is dynamic if and only if mass is non zero, otherwise static
    bool isDynamic = (mass != 0.f);

    startTransform.setOrigin(btVector3(0,0,0));
    btVector3 localInertia(0,0,0);
      
    //using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
    btDefaultMotionState* myMotionStateLeft = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfoLeft(mass, myMotionStateLeft, colShape, localInertia);
    
    left_hand = new btRigidBody(rbInfoLeft);
    dynamics_world->addRigidBody(left_hand);

    left_hand->setCollisionFlags( left_hand->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    left_hand->setActivationState(DISABLE_DEACTIVATION);
    left_hand->activate(true);

    btDefaultMotionState* myMotionStateRight = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfoRight(mass, myMotionStateRight, colShape, localInertia);
    
    right_hand = new btRigidBody(rbInfoRight);
    dynamics_world->addRigidBody(right_hand);

    right_hand->setCollisionFlags( right_hand->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    right_hand->setActivationState(DISABLE_DEACTIVATION);
    right_hand->activate(true); 
  }

  running_ = true;

}


void PhantomPhysics::SharedObject::ExitPhysics() {

  running_ = false;
  update_mutex.lock();
  ball_orients.clear();
  
  for (size_t i =0 ; i < balls.size(); ++i ){
    dynamics_world->removeRigidBody(balls[i]);
    delete balls[i]->getMotionState();
    delete balls[i];
  }

  balls.clear();
  
  dynamics_world->removeRigidBody(left_hand);
  delete left_hand->getMotionState();
  delete left_hand;

  dynamics_world->removeRigidBody(right_hand);
  delete right_hand->getMotionState();
  delete right_hand;
 
  dynamics_world->removeRigidBody(ground);
  delete ground->getMotionState();
  delete ground;

   //delete collision shapes
  for (size_t i=0; i < collision_shapes.size(); ++i) {
    btCollisionShape* shape = collision_shapes[i];
    delete shape;
  }

  collision_shapes.clear();

  delete dynamics_world;
  delete solver;
  delete collision_configuration;
  delete dispatcher;
  delete broadphase;

  update_mutex.unlock();

}