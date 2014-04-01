/**
* @brief Phantomlimb Main class
* @file app.cpp
* @author Benjamin Blundell <oni@section9.co.uk>
* @date 29/10/2013
*
*/

#include "app.hpp"
#include <signal.h>


using namespace std;
using namespace s9;
using namespace s9::gl;
using namespace s9::oni;

PhantomLimb* PhantomLimb::pp_;


PhantomLimb::PhantomLimb(XMLSettings &settings) : file_settings_(settings) {
  window_manager_.AddListener(this);

  // AntTweakBar
  tweakbar_ = TwNewBar("PhantomLimb Controls");
 
  // Create a GL Window, passing functions to that window

  std::function<void()> initZero = std::bind(&PhantomLimb::InitSecondDisplay, this);
  std::function<void(double_t)> drawZero = std::bind(&PhantomLimb::DrawSecondDisplay, this, std::placeholders::_1);
  const GLWindow &windowZero = window_manager_.CreateWindow("PhantomLimb Controls",800,600, initZero, drawZero );

  std::function<void()> initOne = std::bind(&PhantomLimb::InitOculusDisplay, this);
  std::function<void(double_t)> drawOne = std::bind(&PhantomLimb::DrawOculusDisplay, this, std::placeholders::_1);
  const GLWindow &windowOne = window_manager_.CreateWindow("PhantomLimb Oculus Window", 0, 0, initOne, drawOne, true, file_settings_["oculus_display"].Value().c_str(), -1, -1);

  pp_ = this;

};

/*
 * Called when the control window is created
 */

void PhantomLimb::InitSecondDisplay() {

  camera_second_ = Camera(glm::vec3(0.0f,0.0f,1.0f));
  camera_second_.set_orthographic(true);
  
  quad_controls_ = Quad(1,1);

  TwWindowSize(800, 600);

  controls_shader_ = Shader( s9::File("./data/quad_texture.vert"), s9::File("./data/quad_texture.frag"));

  scale_speed_ =  FromStringS9<float_t>( *file_settings_["game/speed/factor"]);
  scale_width_ =  FromStringS9<float_t>( *file_settings_["game/width"]);

  node_controls_.Add(quad_controls_).Add(controls_shader_).Add(camera_second_);

  TwAddButton(tweakbar_, "Fire Ball", on_button_fire_clicked, NULL, " label='Fire a ball.' ");
  TwAddButton(tweakbar_, "Reset Game", on_button_reset_clicked, NULL, " label='Reset the game.' ");
  TwAddButton(tweakbar_, "Reset Tracking", on_button_tracking_clicked, NULL, " label='Reset the skeleton tracking.' ");
  TwAddButton(tweakbar_, "Start / Stop Game", on_button_auto_game_clicked, NULL, " label='Start or Stop the game.' ");
  TwAddButton(tweakbar_, "Reset Rift", on_button_oculus_clicked, NULL, " label='Reset the Oculus View.'");
  TwAddButton(tweakbar_, "Quit", on_button_quit_clicked, NULL, " label='Quit.' ");
  TwAddVarRW(tweakbar_, "Arm Emphasis", TW_TYPE_BOOL8, &arm_emphasis_, "label='Toggle Arm Emphasis'");
  
  TwAddVarRW(tweakbar_, "Scale Width", TW_TYPE_FLOAT, &scale_width_, "label='Scale the width of the ball spawn point' min=0 max=10 step=0.01 ");
  TwAddVarRW(tweakbar_, "Scale Speed", TW_TYPE_FLOAT, &scale_speed_, "label='Scale the Speed of the Balls' min=0 max=10 step=0.01 ");

  TwEnumVal armsEV[] = { {BOTH_ARMS, "Both Arms Free"}, {LEFT_ARM_COPY, "Left Arm Copy"}, {LEFT_ARM_RIGHT_MIRROR, "Left Arm Right Mirror"}, 
    {LEFT_ARM_RIGHT_FROZEN, "Left Arm, Right Frozen"}, {RIGHT_ARM_COPY, "Right Arm Copy"},
    {RIGHT_ARM_LEFT_MIRROR, "Right Arm, Left Mirror"}, {RIGHT_ARM_LEFT_FROZEN, "Right Arm, Left Frozen"}};


  TwType armsType;
 
  armsType = TwDefineEnum("ArmsType", armsEV, 7);
  // Adding season to bar
  TwAddVarRW(tweakbar_, "Arm Behaviour", armsType, &arm_state_, NULL);

}


/*
 * Called when the oculus window is created
 */

 void PhantomLimb::InitOculusDisplay(){

  std::srand(std::time(0)); 

  // File Load

  shader_skinning_ = Shader(s9::File("./data/skinning.glsl"));
  shader_quad_= Shader( s9::File("./data/quad_texture.vert"), s9::File("./data/quad_texture.frag"));
  shader_colour_ = Shader(s9::File("./data/solid_colour.glsl"));

  shader_warp_ = Shader( s9::File("./data/barrel.vert"), s9::File("./data/barrel.frag"));

  shader_room_ = Shader( s9::File("./data/basic_mesh.vert"),  s9::File("./data/textured_mesh.frag"));

  // Oculus Rift Setup

  oculus_ = oculus::OculusBase(0.01f, 100.0f);

  // OpenNI
  openni_ = OpenNIBase(openni::ANY_DEVICE);
  openni_skeleton_tracker_ = OpenNISkeleton(openni_);

  // Virtual Cameras
  // Calibrated for the Sintel Model
  camera_= Camera( glm::vec3(0.0f,1.59f,-0.04),  glm::vec3(0.0,1.59f,-4.0f));
  camera_.set_update_on_node_draw(false);
  camera_.set_near(0.01f);
  camera_.Resize(1280,800);

  camera_left_ = Camera(glm::vec3(0.0f,0.0f,0.0f));
  camera_right_ = Camera(glm::vec3(0.0f,0.0f,0.0f));

  camera_left_.set_update_on_node_draw(false);
  camera_right_.set_update_on_node_draw(false);

  camera_ortho_ = Camera(glm::vec3(0.0f,0.0f,0.1f));
  camera_ortho_.set_near(0.01f);
  camera_ortho_.set_far(1.0f);
  camera_ortho_.set_orthographic(true);

  // MD5 Model Load

  md5_ = MD5Model( s9::File("./data/tracksuit/tracksuit.md5mesh") ); 
  //md5_.set_geometry_cast(WIREFRAME);

  // Nodes

  node_model_.Add(md5_).Add(shader_skinning_);

  model_base_mat_ = glm::rotate(glm::mat4(), 180.0f, glm::vec3(0.0f, 1.0f, 0.0f));
  model_base_mat_ = glm::rotate(model_base_mat_, -90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
  //mat = glm::scale(mat, glm::vec3(0.1f,0.1f,0.1f));
  
  node_model_.set_matrix(model_base_mat_);

  quad_ = Quad(320,240);
  node_depth_.Add(quad_).Add(shader_quad_).Add(camera_ortho_);
  node_colour_.Add(quad_).Add(shader_quad_).Add(camera_ortho_);

  hand_left_colour_ = glm::vec4(1.0f,0.0f,0.0f,1.0f);
  hand_right_colour_ = glm::vec4(0.0f,1.0f,0.0f,1.0f);

  // Hands - calibrated in model co-ordinates for Tracksuit
  Sphere s(0.1f, 20);

  hand_pos_left_ = glm::vec4(0.55f, -0.07f,1.06f, 1.0f);
  hand_pos_right_ = glm::vec4(-0.55f, -0.07f,1.06f, 1.0f);

  node_hands_.Add(shader_colour_).Add(node_left_hand_).Add(node_right_hand_);

  node_left_hand_.Add(s).Add(gl::ShaderClause<glm::vec4,1>("uColour", hand_left_colour_));
  node_right_hand_.Add(s).Add(gl::ShaderClause<glm::vec4,1>("uColour", hand_right_colour_));

  //node_model_.Add(node_hands_);

  // Physics Ball
  ball_radius_ = 0.25f;
  Sphere ball(ball_radius_, 30);
  ball_colour_ = glm::vec4(1.0f,0.0f,1.0f,1.0f);
  node_ball_.Add(shader_colour_).Add(ball).Add(gl::ShaderClause<glm::vec4,1>("uColour", ball_colour_));

  // Skeleton Shape

  skeleton_shape_ = SkeletonShape(md5_.skeleton());
  //skeleton_shape_.set_geometry_cast(WIREFRAME);
  //skeleton_shape_.Add(shader_colour_).Add(camera_);
  //node_model_.Add(skeleton_shape_);

  // Room
  room_ = ObjMesh(s9::File("./data/room/Design_room.obj"));
  room_.set_matrix(glm::scale(glm::mat4(1.0f), glm::vec3(0.02f,0.02f,0.02f)));
  room_.Add(shader_room_);

  //node_left_.Add(camera_left_).Add(node_model_).Add(node_hands_).Add(room_);
  //node_right_.Add(camera_right_).Add(node_model_).Add(node_hands_).Add(room_);

  node_left_.Add(camera_left_).Add(node_model_).Add(room_);
  node_right_.Add(camera_right_).Add(node_model_).Add(room_);


  // Final Splatting

  quad_final_ = Quad(1,1);
  node_final_left_.Add(quad_final_).Add(camera_ortho_);
  node_final_right_.Add(quad_final_).Add(camera_ortho_);
  
  // Game stuff

  arm_state_ = BOTH_ARMS;
  playing_game_ = false;
  arm_emphasis_ = FromStringS9<bool>(*file_settings_["game/emphasis"]);
  last_shot_ = 0;

  // Physics

  physics_ = PhantomPhysics( FromStringS9<float_t>( *file_settings_["game/gravity"]), 
    FromStringS9<float_t>(*file_settings_["game/hand_radius"]));

  CXGLERROR

  // OpenGL Defaults

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

}

// Moved onto the main thread as GCC gets upset on quitting the thread with OpenNI

void PhantomLimb::ThreadMainLoop(double_t dt) { 

  oculus_.Update(dt);

  // update the skeleton positions
  md5_.skeleton().Update();

  // Now copy over the positions of the captured skeleton to the MD5

  if (openni_.ready()) {
    openni_skeleton_tracker_.Update();
    OpenNISkeleton::User user = openni_skeleton_tracker_.GetUserByID(1);
    if (user.IsTracked()){

      // Returned matrices are rotated 180 which is annoying but thats the OpenNI Way
      // In addition, the MD5 model may not have rest orientations that are anywhere near the same :S

      glm::quat ry = glm::angleAxis(180.0f, glm::vec3(0.0f,1.0f,0.0f));
      glm::quat ryi = glm::inverse(ry);

      glm::quat rx = glm::angleAxis(180.0f, glm::vec3(1.0f,0.0f,0.0f));
      glm::quat rxi = glm::inverse(rx);

      glm::quat rz = glm::angleAxis(180.0f, glm::vec3(0.0f,0.0f,1.0f));
      glm::quat rzi = glm::inverse(rz);

      glm::quat rys = glm::angleAxis(-90.0f, glm::vec3(0.0f,1.0f,0.0f));
      glm::quat rysi = glm::inverse(rys);

      glm::quat rzs = glm::angleAxis(-90.0f, glm::vec3(0.0f,0.0f,1.0f));
      glm::quat rzsi = glm::inverse(rzs);


      glm::quat nrys = glm::angleAxis(90.0f, glm::vec3(0.0f,1.0f,0.0f));
      glm::quat nrysi = glm::inverse(nrys);

      glm::quat nrzs = glm::angleAxis(90.0f, glm::vec3(0.0f,0.0f,1.0f));
      glm::quat nrzsi = glm::inverse(nrzs);



      /*Bone * luparm = md5_.skeleton().bone("luparm");
      luparm->set_rotation_relative( ry * user.skeleton().bone("Left Shoulder")->rotation() * ryi );

      Bone * lloarm = md5_.skeleton().bone("lloarm");
      lloarm->set_rotation_relative( ry * user.skeleton().bone("Left Elbow")->rotation() * ryi);

      Bone * ruparm = md5_.skeleton().bone("ruparm");
      ruparm->set_rotation_relative( ry * user.skeleton().bone("Right Shoulder")->rotation() * ryi * rxi );

      Bone * rloarm = md5_.skeleton().bone("rloarm");
      rloarm->set_rotation_relative( rx * ry * user.skeleton().bone("Right Elbow")->rotation() * ryi * rxi );
      */

      // Sintel's & tracksuits bones have different alignments in the arms due to some blender issues it seems
      ///\todo we should always get a consistent postion for bones
 
      // LEFT Model Arm Upper

      Bone * luparm = md5_.skeleton().GetBone("upper_arm.L");
      if (luparm != nullptr) {
        
        glm::quat final_rotation;

        // Deal with the mirroring of arms
        switch (arm_state_) {
          case BOTH_ARMS:
          case LEFT_ARM_RIGHT_MIRROR:
          case LEFT_ARM_COPY:
            final_rotation = user.skeleton().GetBone("Left Shoulder")->rotation();
          break;  
	       
          case LEFT_ARM_RIGHT_FROZEN:
            final_rotation = glm::angleAxis(90.0f,0.0f,0.0f,1.0f);  
          break;

          case RIGHT_ARM_LEFT_MIRROR:
          case RIGHT_ARM_LEFT_FROZEN: {
            glm::quat tq = user.skeleton().GetBone("Right Shoulder")->rotation();
            float_t angle = glm::angle(tq);
            glm::vec3 axis = glm::axis(tq);
            final_rotation = glm::angleAxis(-angle, -axis.x,axis.y,axis.z);
            break;
          }

          case RIGHT_ARM_COPY:
            glm::quat tq = user.skeleton().GetBone("Right Shoulder")->rotation();
            glm::quat tr = glm::angleAxis(-180.0f, 0.0f, 1.0f, 0.0f);
            float_t angle = glm::angle(tq);
            glm::vec3 axis = glm::axis(tq);
            final_rotation = glm::angleAxis(angle, -axis.x,axis.y,axis.z) * tr; 
          break;
        }

        luparm->set_rotation_relative( rys * rzs * final_rotation   * rzsi * rysi );

      }

      // LEFT Model Arm Lower

      Bone * lloarm = md5_.skeleton().GetBone("lower_arm.L");
      if (lloarm != nullptr) {
        glm::quat final_rotation;

        switch (arm_state_) {
          case BOTH_ARMS:
          case LEFT_ARM_RIGHT_MIRROR:
          case LEFT_ARM_COPY:
		        final_rotation = user.skeleton().GetBone("Left Elbow")->rotation();
          break;  
	       
          case LEFT_ARM_RIGHT_FROZEN:
		        final_rotation = glm::quat();
          break;

          case RIGHT_ARM_LEFT_MIRROR:
          case RIGHT_ARM_COPY:
          case RIGHT_ARM_LEFT_FROZEN:{
            glm::quat tq =user.skeleton().GetBone("Right Elbow")->rotation();
            float_t angle = glm::angle(tq);
            glm::vec3 axis = glm::axis(tq);
            final_rotation = glm::angleAxis(-angle, -axis.x,axis.y,axis.z);
            break;
          }


        }

        lloarm->set_rotation_relative(  rys * rzs * final_rotation * rzsi * rysi);
      }
        
      // RIGHT Arm Upper

      Bone * ruparm = md5_.skeleton().GetBone("upper_arm.R");
      if (ruparm != nullptr) {
        glm::quat final_rotation;

        switch (arm_state_) {
          case BOTH_ARMS:
          case RIGHT_ARM_LEFT_MIRROR:
          case RIGHT_ARM_COPY:
            final_rotation = user.skeleton().GetBone("Right Shoulder")->rotation();
          break;
	       
          case RIGHT_ARM_LEFT_FROZEN: {
            final_rotation = glm::angleAxis(90.0f,0.0f,0.0f,1.0f); // user.skeleton().GetBone("Right Shoulder")->rotation();
            break;
          }

          case LEFT_ARM_RIGHT_MIRROR:
          case LEFT_ARM_RIGHT_FROZEN: {
            glm::quat tq = user.skeleton().GetBone("Left Shoulder")->rotation();
            float_t angle = glm::angle(tq);
            glm::vec3 axis = glm::axis(tq);
            final_rotation = glm::angleAxis(-angle, -axis.x, axis.y, axis.z);
            break;
          }

          case LEFT_ARM_COPY:
            glm::quat tq = user.skeleton().GetBone("Left Shoulder")->rotation();
            glm::quat tr = glm::angleAxis(-180.0f, 0.0f, 1.0f, 0.0f);
            float_t angle = glm::angle(tq);
            glm::vec3 axis = glm::axis(tq);
            final_rotation = glm::angleAxis(angle, -axis.x,axis.y,axis.z) * tr; 
          break;

        }

        ruparm->set_rotation_relative( nrys * nrzs *  final_rotation * nrzsi * nrysi  );

      }

      // RIGHT Arm Lower

      Bone * rloarm = md5_.skeleton().GetBone("lower_arm.R");
      if (rloarm != nullptr) {
        glm::quat final_rotation;

        switch (arm_state_) {
          case BOTH_ARMS:
          case RIGHT_ARM_LEFT_MIRROR:
          case RIGHT_ARM_COPY:
            final_rotation = user.skeleton().GetBone("Right Elbow")->rotation();
          break;
          
          case RIGHT_ARM_LEFT_FROZEN:
            final_rotation = glm::quat();  
          break;

          case LEFT_ARM_RIGHT_MIRROR:
          case LEFT_ARM_COPY:
          case LEFT_ARM_RIGHT_FROZEN:{
            glm::quat tq = user.skeleton().GetBone("Left Elbow")->rotation();
            float_t angle = glm::angle(tq);
            glm::vec3 axis = glm::axis(tq);
            final_rotation = glm::angleAxis(-angle, -axis.x, axis.y, axis.z);
            break;
          }

        
        }

        rloarm->set_rotation_relative(  nrys * nrzs  * final_rotation * nrzsi * nrysi );
      }

    }
  }


  // set the hit targets for physics as spheres where the hands are
  // This is done in model space so the actual positions, we need to move to world space

  // Dependent on Arm state

  Bone * lloarm = md5_.skeleton().GetBone("lower_arm.L");
  if (lloarm != nullptr){

    glm::vec4 lp = glm::inverse(model_base_mat_) * lloarm->skinned_matrix() * hand_pos_left_;

    hand_pos_left_final_ = glm::vec3(lp.x,lp.y,lp.z);
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), hand_pos_left_final_);
    node_left_hand_.set_matrix(trans);

    physics_.MoveLeftHand(hand_pos_left_final_);
  }


  Bone * rloarm = md5_.skeleton().GetBone("lower_arm.R");
  if (rloarm != nullptr){
    glm::vec4 lp =  glm::inverse(model_base_mat_) * rloarm->skinned_matrix() * hand_pos_right_;

    hand_pos_right_final_ = glm::vec3(lp.x,lp.y,lp.z);

    glm::mat4 trans = glm::translate(glm::mat4(1.0f), hand_pos_right_final_);
    node_right_hand_.set_matrix(trans);

    physics_.MoveRightHand(hand_pos_right_final_);
  }

}


void PhantomLimb::DrawSecondDisplay(double_t dt){
  glClearBufferfv(GL_COLOR, 0, &glm::vec4(0.1f, 0.9f, 0.9f, 1.0f)[0]);
  GLfloat depth = 1.0f;
  glClearBufferfv(GL_DEPTH, 0, &depth );


  if (fbo_){
    fbo_.colour().Bind();
    node_controls_.Draw();
    fbo_.colour().Unbind();
  }

  TwDraw();

  CXGLERROR
}




/*
 * Called as fast as possible. Not set FPS wise but dt is passed in
 */

 void PhantomLimb::DrawOculusDisplay(double_t dt){

  GLfloat depth = 1.0f;

  // Update Physics
  physics_.Update(dt);

   // Update game state
  if (playing_game_){
    last_shot_ += dt ;
    if (last_shot_ > FromStringS9<float_t>( *file_settings_["game/time"])) {
      last_shot_ = 0;
      FireBall();
    }
  }


  // Create the FBO and setup the cameras
  if (!fbo_ && oculus_.Connected()){
    
      glm::vec2 s = oculus_.fbo_size();

      s = glm::vec2(2048,1024); // Override for the POT es 3 (even though 300 claims NPOT :S )

      fbo_ = FBO(static_cast<size_t>(s.x), static_cast<size_t>(s.y)); 

      camera_left_.Resize(static_cast<size_t>(s.x / 2.0f), static_cast<size_t>(s.y ));
      camera_right_.Resize(static_cast<size_t>(s.x / 2.0f), static_cast<size_t>(s.y ),static_cast<size_t>(s.x / 2.0f) );

      camera_.Resize(static_cast<size_t>(s.x ), static_cast<size_t>(s.y ));

      camera_left_.set_projection_matrix(oculus_.left_projection());
      camera_right_.set_projection_matrix(oculus_.right_projection());

      camera_ortho_.Resize(oculus_.screen_resolution().x, oculus_.screen_resolution().y);
      
  }

  if (fbo_){
    fbo_.Bind();
    // Clear
    glClearBufferfv(GL_COLOR, 0, &glm::vec4(0.9f, 0.9f, 0.9f, 1.0f)[0]);
    glClearBufferfv(GL_DEPTH, 0, &depth );

    // Grab Textures
    openni_.Update(); // While thread safe, its best to put this immediately before the update_textures
    
    //openni_.update_textures(); ///\todo this is causing errors

    // Alter camera with the oculus
    glm::quat q = glm::inverse(oculus_.orientation());
    oculus_rotation_dt_ = glm::inverse(oculus_rotation_prev_) * q;
    oculus_rotation_prev_ =  q;
    camera_.Rotate(oculus_rotation_dt_);
    camera_.Update();

    camera_left_.set_view_matrix( camera_.view_matrix() * oculus_.left_inter() );
    camera_right_.set_view_matrix(  camera_.view_matrix() * oculus_.right_inter() );

    // Draw Balls left and right

    node_ball_.Add(camera_left_);
    for (glm::mat4 mat : physics_.ball_orients()){
      node_ball_.set_matrix(mat);
      node_ball_.Draw();
    }
    node_ball_.Remove(camera_left_);


    node_ball_.Add(camera_right_);
    for (glm::mat4 mat : physics_.ball_orients()){
      node_ball_.set_matrix(mat);
      node_ball_.Draw();
    }
    node_ball_.Remove(camera_right_);

    // Draw Model
    node_left_.Draw();
    node_right_.Draw();

    // Draw the hand collision units


    fbo_.Unbind();
    //CXGLERROR

    // Draw to main screen - this cheats and uses a geometry shader

    // Be wary here that we are messing with the polygon mode up the chain

    glClearBufferfv(GL_COLOR, 0, &glm::vec4(0.9f, 0.9f, 0.9f, 1.0f)[0]);
    glClearBufferfv(GL_DEPTH, 0, &depth );

    //glViewport(0,0, camera_ortho_.width(), camera_ortho_.height());

    shader_warp_.Bind();
    fbo_.colour().Bind();

    shader_warp_.s("uTexSampler0",0);
    shader_warp_.s("uDistortionOffset", oculus_.distortion_xcenter_offset()); // Can change with future headsets apparently
    shader_warp_.s("uDistortionScale", 1.0f/oculus_.distortion_scale());
    shader_warp_.s("uChromAbParam", oculus_.chromatic_abberation());
    shader_warp_.s("uHmdWarpParam",oculus_.distortion_parameters() );
    shader_warp_.s("uScale", glm::vec2(0.25f,0.5f));
    shader_warp_.s("uScaleIn", glm::vec2(4.0f,2.0f));
    shader_warp_.s("uHmdWarpParam",glm::vec4(1.0f,0.22f,0.24f,0.0f));


    glm::vec2 screenCenter = glm::vec2(0.5,0.5);
    glm::vec2 lensCenter = glm::vec2(0.5 + oculus_.distortion_xcenter_offset() * 0.25, 0.5);

    shader_warp_.s("sLensCenter", lensCenter);
    shader_warp_.s("sScreenCenter", screenCenter);
    shader_warp_.s("uModelMatrix", node_final_left_.matrix());
    shader_warp_.s("uViewMatrix", camera_ortho_.view_matrix());
    shader_warp_.s("uProjectionMatrix", camera_ortho_.projection_matrix());

    node_final_left_.Draw();

    screenCenter = glm::vec2(0.5,0.5);
    lensCenter = glm::vec2(0.5 + oculus_.distortion_xcenter_offset() * 0.25, 0.5);

    shader_warp_.s("sLensCenter", lensCenter);
    shader_warp_.s("sScreenCenter", screenCenter);
    shader_warp_.s("uModelMatrix", node_final_right_.matrix());

    node_final_right_.Draw();

    fbo_.colour().Unbind();
    shader_warp_.Unbind();

    CXGLERROR
  }
}

/// Fire a ball into the scene
void PhantomLimb::FireBall() {

  float_t rval0 = static_cast<float_t>(std::rand()) /  RAND_MAX;
  float_t rval1 = static_cast<float_t>(std::rand()) /  RAND_MAX;

  float_t game_width = FromStringS9<float_t>(*file_settings_["game/width"]);
  float_t speed_min = FromStringS9<float_t>(*file_settings_["game/speed/min"]);
  float_t speed_factor = FromStringS9<float_t>(*file_settings_["game/speed/factor"]);

  float_t height_min = FromStringS9<float_t>(*file_settings_["game/height/min"]);
  float_t height_factor = FromStringS9<float_t>(*file_settings_["game/height/factor"]);

  float_t xpos = -game_width + (rval0 * 2.0f * game_width);

  if (arm_emphasis_){
    switch (arm_state_) {
      case BOTH_ARMS:
      case LEFT_ARM_RIGHT_MIRROR:
      case RIGHT_ARM_LEFT_MIRROR:
      break;
           
      case LEFT_ARM_COPY:
      case LEFT_ARM_RIGHT_FROZEN:
        xpos = (-game_width * 0.1) + (rval0 * 1.1 * game_width);
      break;

     
      case RIGHT_ARM_LEFT_FROZEN: 
      case RIGHT_ARM_COPY:
        xpos = -game_width + (rval0 * 1.1f * game_width);
      break;
    }

  }


  physics_.AddBall(ball_radius_, glm::vec3( xpos , height_min + (rval1 * height_factor), -4.0f), 
      glm::vec3(0.0f, (4.0f - speed_min + rval1 ) * 0.5f + rval0, speed_factor * rval1 + speed_min));
}


PhantomLimb::~PhantomLimb() {   

}

void PhantomLimb::Close() {

  // Shutdown a few things first - order is important

  // Write back file settings
  file_settings_["game/speed/factor"].SetValue(scale_speed_);
  file_settings_["game/width"].SetValue(scale_width_);

  // shutdown systems
  openni_.Shutdown();
  Shutdown();
}


void PhantomLimb::ProcessEvent( const GLWindow &window, CloseWindowEvent e) {
  cout << "Phantom Limb Shutting down." << endl;
  Close();
}

/*
 * This is called by the wrapper function when an event is fired
 */

 void PhantomLimb::ProcessEvent( const GLWindow &window, MouseEvent e){

 }

/*
 * Called when the window is resized. You should set cameras here
 */

 void PhantomLimb::ProcessEvent( const GLWindow &window, ResizeEvent e){



  if ( window.window_name().compare("PhantomLimb Oculus Window") == 0) {
    camera_ortho_.Resize(e.w,e.h);
    glm::mat4 model_matrix =   glm::translate(glm::mat4(1.0f), glm::vec3(e.w * 0.25, e.h/2.0f, 0.0f)); 
    model_matrix = glm::scale(model_matrix, glm::vec3(e.w * 0.5, e.h,1.0f));
    node_final_left_.set_matrix(model_matrix);


    model_matrix =   glm::translate(glm::mat4(1.0f), glm::vec3(e.w * 0.75, e.h/2.0f, 0.0f)); 
    model_matrix = glm::scale(model_matrix, glm::vec3(e.w * 0.5, e.h,1.0f));
    node_final_right_.set_matrix(model_matrix);
  } else {

    glm::mat4 model_matrix =   glm::translate(glm::mat4(1.0f), glm::vec3(e.w / 2.0f, e.h/2.0f, 0.0f)); 
    model_matrix = glm::scale(model_matrix, glm::vec3(e.w * 0.9, e.h * 0.9,1.0f));
    TwWindowSize(e.w,e.h);
    camera_second_.Resize(e.w,e.h);
    node_controls_.set_matrix( model_matrix );

     //node_controls_.set_matrix(glm::translate(glm::mat4(1.0f), glm::vec3(e.w / 2.0f, e.h/2.0f, 0.0f)));
  }
}

void PhantomLimb::ProcessEvent( const GLWindow &window, KeyboardEvent e){
  cout << "Key Pressed: " << e.key << endl;
}


void PhantomLimb::MainLoop(double_t dt){
  window_manager_.MainLoop();
}



void TW_CALL PhantomLimb::on_button_fire_clicked(void * ) {
  cout << "Firing Ball" << endl;
  pp_->FireBall();
}


void TW_CALL PhantomLimb::on_button_reset_clicked(void * ) {
  cout << "Reset Physics" << endl;
  pp_->ResetPhysics();
}

void TW_CALL PhantomLimb::on_button_tracking_clicked(void * ) {
  cout << "Restarting Tracking" << endl;
  pp_->RestartTracking();
}


void TW_CALL PhantomLimb::on_button_auto_game_clicked(void * ) {
  cout << "Auto Game Clicked" << endl;
  pp_->PlayGame(!pp_->playing_game());
}

void TW_CALL PhantomLimb::on_button_oculus_clicked(void * ) {
  cout << "Resetting Oculus View" << endl;
  pp_->ResetOculus();
}

void TW_CALL PhantomLimb::on_button_quit_clicked(void * ) {
  cout << "Quitting PhantomLimb" << endl;
  pp_->Close();
}


/*
 * Main function - uses boost to parse program arguments
 */

int main (int argc, const char * argv[]) {

  XMLSettings settings;

  if (!settings.LoadFile(s9::File("./data/settings.xml"))){
    cerr << "PhantomLimb: Could not find data/settings.xml. Aborting." << endl;
    return -1;
  }

  PhantomLimb b(settings);

  b.Run();

  settings.SaveFile(s9::File("./data/settings.xml"));

  return EXIT_SUCCESS;

}
