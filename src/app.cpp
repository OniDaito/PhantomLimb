/**
* @brief GLFW Window based solution
* @file app.hpp
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

 void PhantomLimb::init(){

  // File Load

  shader_skinning_ = Shader(s9::File("./data/skinning.glsl"));
  shader_quad_= Shader( s9::File("./data/quad_texture.vert"), s9::File("./data/quad_texture.frag"));
  shader_colour_ = Shader(s9::File("./data/solid_colour.glsl"));

   shader_warp_ = Shader( s9::File("./data/barrel.vert"), 
        s9::File("./data/barrel.frag"),
        s9::File("./data/barrel.geom"));

  addWindowListener(this);

  // Oculus Rift Setup

  oculus_ = oculus::OculusBase(true);

  // OpenNI
  openni_ = OpenNIBase(openni::ANY_DEVICE);
  openni_skeleton_tracker_ = OpenNISkeleton(openni_);


  // Virtual Cameras

  camera_= Camera( glm::vec3(0.0f,12.5f,-1.5f),  glm::vec3(0.0,12.5f,-4.0f));
  camera_.set_update_on_node_draw(false);
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

  md5_ = MD5Model( s9::File("./data/hellknight/hellknight.md5mesh") ); 
  //md5_.set_geometry_cast(WIREFRAME);

  // Nodes

  node_model_.add(md5_).add(shader_skinning_);

  glm::mat4 mat = glm::rotate(glm::mat4(), 180.0f, glm::vec3(0.0f, 1.0f, 0.0f));
  mat = glm::rotate(mat, -90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
  mat = glm::scale(mat, glm::vec3(0.1f,0.1f,0.1f));
  
  node_model_.setMatrix(mat);

  quad_ = Quad(320,240);
  node_depth_.add(quad_).add(shader_quad_).add(camera_ortho_);
  node_colour_.add(quad_).add(shader_quad_).add(camera_ortho_);

  // Skeleton Shape

  skeleton_shape_ = SkeletonShape(md5_.skeleton());
  //skeleton_shape_.set_geometry_cast(WIREFRAME);
  //skeleton_shape_.add(shader_colour_).add(camera_);
  //node_model_.add(skeleton_shape_);

  node_left_.add(camera_left_).add(node_model_);
  node_right_.add(camera_right_).add(node_model_);

  CXGLERROR

  // OpenGL Defaults

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

}

void PhantomLimb::update(double_t dt) {

  // update the skeleton positions
  md5_.skeleton().update();

  // Update Oculus - take the difference
  oculus_.update(dt);

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

      Bone * luparm = md5_.skeleton().bone("luparm");
      luparm->set_rotation_relative( ry * user.skeleton().bone("Left Shoulder")->rotation() * ryi );

      Bone * lloarm = md5_.skeleton().bone("lloarm");
      lloarm->set_rotation_relative( ry * user.skeleton().bone("Left Elbow")->rotation() * ryi);

      Bone * ruparm = md5_.skeleton().bone("ruparm");
      ruparm->set_rotation_relative( ry * user.skeleton().bone("Right Shoulder")->rotation() * ryi * rxi );

      Bone * rloarm = md5_.skeleton().bone("rloarm");
      rloarm->set_rotation_relative( rx * ry * user.skeleton().bone("Right Elbow")->rotation() * ryi * rxi );

    }
  }
}


/*
 * Called as fast as possible. Not set FPS wise but dt is passed in
 */

 void PhantomLimb::display(double_t dt){

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

    // Draw Model

    node_left_.draw();
    node_right_.draw();

    // Draw textures from the camera
    /*
    glm::mat4 mat = glm::translate(glm::mat4(1.0f), glm::vec3(160,120,0));
    node_depth_.setMatrix(mat);
    openni_.texture_depth().bind();
    node_depth_.draw();
    openni_.texture_depth().unbind();

    mat = glm::translate(glm::mat4(1.0f), glm::vec3(480,120,0));
    node_colour_.setMatrix(mat);
    openni_.texture_colour().bind();
    node_colour_.draw();
    openni_.texture_colour().unbind();

    */

    fbo_.unbind();
    //CXGLERROR
  }


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


PhantomLimb::~PhantomLimb() {   

}


/*
 * This is called by the wrapper function when an event is fired
 */

 void PhantomLimb::processEvent(MouseEvent e){}

/*
 * Called when the window is resized. You should set cameras here
 */

void PhantomLimb::processEvent(ResizeEvent e){
  camera_ortho_.resize(e.w,e.h);
}

void PhantomLimb::processEvent(KeyboardEvent e){}

/*
 * Main function - uses boost to parse program arguments
 */

 int main (int argc, const char * argv[]) {

  PhantomLimb b;

#ifdef _SEBURO_OSX
  GLFWApp a(b, 1280, 800, false, argc, argv, "Phantom Limb", 3, 2);
#else
  GLFWApp a(b, 1280, 800, false, argc, argv, "Phantom Limb");
#endif

  return EXIT_SUCCESS;

}