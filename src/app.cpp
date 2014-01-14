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

/*
 * Called when the mainloop starts, just once
 */

 void PhantomLimb::Init(){

  // File Load

  shader_skinning_ = Shader(s9::File("./data/skinning.glsl"));
  shader_quad_= Shader( s9::File("./data/quad_texture.vert"), s9::File("./data/quad_texture.frag"));
  shader_colour_ = Shader(s9::File("./data/solid_colour.glsl"));

   shader_warp_ = Shader( s9::File("./data/barrel.vert"), 
        s9::File("./data/barrel.frag"),
        s9::File("./data/barrel.geom"));

  // Oculus Rift Setup

  oculus_ = oculus::OculusBase(0.01f, 100.0f);

  // OpenNI
  openni_ = OpenNIBase(openni::ANY_DEVICE);
  openni_skeleton_tracker_ = OpenNISkeleton(openni_);

  // Virtual Cameras
  // Calibrated for the Sintel Model
  camera_= Camera( glm::vec3(0.0f,1.51f,-0.02),  glm::vec3(0.0,1.51f,-4.0f));
  camera_.set_update_on_node_draw(false);
  camera_.set_near(0.01f);
  camera_.resize(1280,800);

  camera_left_ = Camera(glm::vec3(0.0f,0.0f,0.0f));
  camera_right_ = Camera(glm::vec3(0.0f,0.0f,0.0f));

  camera_left_.set_update_on_node_draw(false);
  camera_right_.set_update_on_node_draw(false);

  camera_ortho_ = Camera(glm::vec3(0.0f,0.0f,0.1f));
  camera_ortho_.set_near(0.01f);
  camera_ortho_.set_far(1.0f);
  camera_ortho_.set_orthographic(true);

  // MD5 Model Load

  md5_ = MD5Model( s9::File("./data/sintel_lite/sintel_lite.md5mesh") ); 
  //md5_.set_geometry_cast(WIREFRAME);

  // Nodes

  node_model_.add(md5_).add(shader_skinning_);

  model_base_mat_ = glm::rotate(glm::mat4(), 180.0f, glm::vec3(0.0f, 1.0f, 0.0f));
  model_base_mat_ = glm::rotate(model_base_mat_, -90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
  //mat = glm::scale(mat, glm::vec3(0.1f,0.1f,0.1f));
  
  node_model_.set_matrix(model_base_mat_);

  quad_ = Quad(320,240);
  node_depth_.add(quad_).add(shader_quad_).add(camera_ortho_);
  node_colour_.add(quad_).add(shader_quad_).add(camera_ortho_);

  hand_left_colour_ = glm::vec4(1.0f,0.0f,0.0f,1.0f);
  hand_right_colour_ = glm::vec4(0.0f,1.0f,0.0f,1.0f);

  // Hands - calibrated in model co-ordinates for Sintel
  Sphere s(0.1f, 20);

  hand_pos_left_ = glm::vec4(0.63f, 0.03f,1.32f, 1.0f);
  hand_pos_right_ = glm::vec4(-0.63f, 0.03f,1.32f, 1.0f);

  node_hands_.add(shader_colour_).add(node_left_hand_).add(node_right_hand_);

  node_left_hand_.add(s).add(gl::ShaderClause<glm::vec4,1>("uColour", hand_left_colour_));
  node_right_hand_.add(s).add(gl::ShaderClause<glm::vec4,1>("uColour", hand_right_colour_));

  node_model_.add(node_hands_);

  // Physics Ball
  ball_colour_ = glm::vec4(1.0f,0.0f,1.0f,1.0f);
  node_ball_.add(shader_colour_).add(s).add(gl::ShaderClause<glm::vec4,1>("uColour", ball_colour_));

  // Skeleton Shape

  skeleton_shape_ = SkeletonShape(md5_.skeleton());
  //skeleton_shape_.set_geometry_cast(WIREFRAME);
  //skeleton_shape_.add(shader_colour_).add(camera_);
  //node_model_.add(skeleton_shape_);

  node_left_.add(camera_left_).add(node_model_);
  node_right_.add(camera_right_).add(node_model_);


  // Physics

  physics_ = PhantomPhysics(-10.0f, 0.1f);

  CXGLERROR

  // OpenGL Defaults

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

}

void PhantomLimb::Update(double_t dt) {

  // update the skeleton positions
  md5_.skeleton().update();

  // Update Oculus - take the difference
  oculus_.update(dt);


  // Update Physics
  physics_.Update(dt);

  // Now copy over the positions of the captured skeleton to the MD5

  if (openni_.ready()) {
    openni_skeleton_tracker_.update();
    OpenNISkeleton::User user = openni_skeleton_tracker_.user(1);
    if (user.isTracked()){

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

      // Sintel's bones have different alignments in the arms due to some blender issues it seems
      ///\todo we should always get a consistent postion for bones
 
      Bone * luparm = md5_.skeleton().bone("upper_arm.L");
      if (luparm != nullptr)
        luparm->set_rotation_relative( rys * rzs *  user.skeleton().bone("Left Shoulder")->rotation()  * rzsi * rysi );

      Bone * lloarm = md5_.skeleton().bone("lower_arm.L");
      if (lloarm != nullptr)
        lloarm->set_rotation_relative(  rys * rzs * user.skeleton().bone("Left Elbow")->rotation() * rzsi * rysi);

      Bone * ruparm = md5_.skeleton().bone("upper_arm.R");
      if (ruparm != nullptr)
        ruparm->set_rotation_relative( nrys * nrzs *  user.skeleton().bone("Right Shoulder")->rotation() * nrzsi * nrysi  );

      Bone * rloarm = md5_.skeleton().bone("lower_arm.R");
      if (rloarm != nullptr)
        rloarm->set_rotation_relative(  nrys * nrzs  * user.skeleton().bone("Right Elbow")->rotation() * nrzsi * nrysi );

    }
  }


  // set the hit targets for physics as spheres where the hands are
  // This is done in model space so the actual positions, we need to move to world space

  Bone * lloarm = md5_.skeleton().bone("lower_arm.L");
  if (lloarm != nullptr){
    glm::vec4 lp = lloarm->skinned_matrix() * hand_pos_left_;
    glm::vec3 tp = glm::vec3(lp.x, lp.y, lp.z);
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), tp);
    node_left_hand_.set_matrix(trans);
    hand_pos_left_final_ = tp;
  }


  Bone * rloarm = md5_.skeleton().bone("lower_arm.R");
  if (rloarm != nullptr){
    glm::vec4 lp = rloarm->skinned_matrix() * hand_pos_right_;
    glm::vec3 tp = glm::vec3(lp.x, lp.y, lp.z);
    glm::mat4 trans = glm::translate(glm::mat4(1.0f), tp);
    node_right_hand_.set_matrix(trans);
    hand_pos_right_final_ = tp;
  }


}


/*
 * Called as fast as possible. Not set FPS wise but dt is passed in
 */

 void PhantomLimb::Display(GLFWwindow* window, double_t dt){

  GLfloat depth = 1.0f;

  // Create the FBO and setup the cameras
  if (!fbo_ && oculus_.connected()){
    
      glm::vec2 s = oculus_.fbo_size();
      fbo_ = FBO(static_cast<size_t>(s.x), static_cast<size_t>(s.y)); 

      camera_left_.resize(static_cast<size_t>(s.x / 2.0f), static_cast<size_t>(s.y ));
      camera_right_.resize(static_cast<size_t>(s.x / 2.0f), static_cast<size_t>(s.y ),static_cast<size_t>(s.x / 2.0f) );

      camera_.resize(static_cast<size_t>(s.x ), static_cast<size_t>(s.y ));

      camera_left_.set_projection_matrix(oculus_.left_projection());
      camera_right_.set_projection_matrix(oculus_.right_projection());

      camera_ortho_.resize(oculus_.screen_resolution().x, oculus_.screen_resolution().y);

      glGenVertexArrays(1, &(null_VAO_));
      
  }

  if (fbo_){
    fbo_.bind();
    // Clear
    glClearBufferfv(GL_COLOR, 0, &glm::vec4(0.9f, 0.9f, 0.9f, 1.0f)[0]);
    glClearBufferfv(GL_DEPTH, 0, &depth );

    // Grab Textures
    openni_.update(); // While thread safe, its best to put this immediately before the update_textures
    
    //openni_.update_textures(); ///\todo this is causing errors

    // Alter camera with the oculus
    glm::quat q = glm::inverse(oculus_.orientation());
    oculus_rotation_dt_ = glm::inverse(oculus_rotation_prev_) * q;
    oculus_rotation_prev_ =  q;
    camera_.rotate(oculus_rotation_dt_);
    camera_.update();

    camera_left_.set_view_matrix( camera_.view_matrix() * oculus_.left_inter() );
    camera_right_.set_view_matrix(  camera_.view_matrix() * oculus_.right_inter() );

    // Draw Balls left and right

    node_ball_.add(camera_left_);
    for (glm::mat4 mat : physics_.ball_orients()){
      node_ball_.set_matrix(mat);
      node_ball_.draw();
    }
    node_ball_.remove(camera_left_);


    node_ball_.add(camera_right_);
    for (glm::mat4 mat : physics_.ball_orients()){
      node_ball_.set_matrix(mat);
      node_ball_.draw();
    }
    node_ball_.remove(camera_right_);

    // Draw Model

    node_left_.draw();
    node_right_.draw();

    // Draw the hand collision units

    // Draw textures from the camera
    /*
    glm::mat4 mat = glm::translate(glm::mat4(1.0f), glm::vec3(160,120,0));
    node_depth_.set_matrix(mat);
    openni_.texture_depth().bind();
    node_depth_.draw();
    openni_.texture_depth().unbind();

    mat = glm::translate(glm::mat4(1.0f), glm::vec3(480,120,0));
    node_colour_.set_matrix(mat);
    openni_.texture_colour().bind();
    node_colour_.draw();
    openni_.texture_colour().unbind();

    */

    fbo_.unbind();
    //CXGLERROR

    // Draw to main screen - this cheats and uses a geometry shader

    // Be wary here that we are messing with the polygon mode up the chain

    glClearBufferfv(GL_COLOR, 0, &glm::vec4(0.9f, 0.9f, 0.9f, 1.0f)[0]);
    glClearBufferfv(GL_DEPTH, 0, &depth );

    glBindVertexArray(null_VAO_);

    glViewport(0,0, camera_ortho_.width(), camera_ortho_.height());

    shader_warp_.bind();
    fbo_.colour().bind();

    shader_warp_.s("uDistortionOffset", oculus_.distortion_xcenter_offset()); // Can change with future headsets apparently
    shader_warp_.s("uDistortionScale", 1.0f/oculus_.distortion_scale());
    shader_warp_.s("uChromAbParam", oculus_.chromatic_abberation());
    shader_warp_.s("uHmdWarpParam",oculus_.distortion_parameters() );

    glDrawArrays(GL_POINTS, 0, 1);

    fbo_.colour().unbind();
    shader_warp_.unbind();

    glBindVertexArray(0);

    //CXGLERROR -  annoyingly there is an error
  }
}

/// Fire a ball into the scene
void PhantomLimb::FireBall() {
  physics_.AddBall(0.1f, glm::vec3(0.0f, 20.0f, -1.0f), glm::vec3(0,0,0));
}


PhantomLimb::~PhantomLimb() {   

}


/*
 * This is called by the wrapper function when an event is fired
 */

void PhantomLimb::ProcessEvent(MouseEvent e, GLFWwindow* window){}

/*
 * Called when the window is resized. You should set cameras here
 */

void PhantomLimb::ProcessEvent(ResizeEvent e, GLFWwindow* window){
  camera_ortho_.resize(e.w,e.h);
}

void PhantomLimb::ProcessEvent(KeyboardEvent e, GLFWwindow* window){}



/*
 * The UX window Class
 */

#ifdef _SEBURO_LINUX


UXWindow::UXWindow(PhantomLimb &app) : app_(app)  {   // creates a new button with label "Hello World".
  // Sets the border width of the window.
  set_border_width(10);

  // When the button receives the "clicked" signal, it will call the
  // on_button_clicked() method defined below.
  button_fire_ = new Gtk::Button("Fire Ball");
  button_fire_->signal_clicked().connect(sigc::mem_fun(*this, &UXWindow::on_button_fire_clicked));

  button_reset_ = new Gtk::Button("Reset Game");
  button_reset_->signal_clicked().connect(sigc::mem_fun(*this, &UXWindow::on_button_reset_clicked));

  // This packs the button into the Window (a container).
  grid_.attach(*button_fire_,0,0,1,1);
  grid_.attach(*button_reset_,0,1,1,1);

  add(grid_);
  show_all();

}

UXWindow::~UXWindow() {
  // Need to signal that we are done here
  delete button_fire_;
  delete button_reset_;
}

void UXWindow::on_button_fire_clicked() {
  cout << "Firing Ball" << endl;
  app_.FireBall();
}

void UXWindow::on_button_reset_clicked() {
  cout << "Reset Physics" << endl;
  app_.ResetPhysics();
}

#endif


/*
 * Main function - uses boost to parse program arguments
 */

int main (int argc, const char * argv[]) {

  PhantomLimb b;

#ifdef _SEBURO_OSX
  WithUXApp a(b,argc,argv,3,2);
#else
  WithUXApp a(b,argc,argv);
#endif

#ifdef _SEBURO_LINUX
  // Change HDMI-0 to whatever is listed in the output for the GLFW Monitor Screens
  //a.CreateWindowFullScreen("Oculus", 0, 0, "HDMI-0");
  
  a.CreateWindow("Oculus", 1280, 800);

  UXWindow ux(b);

  a.Run(ux);

  // Call shutdown once the GTK Run loop has quit. This makes GLFW quit cleanly
  a.Shutdown();

  return EXIT_SUCCESS;

#endif

}