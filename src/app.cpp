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

  // Load settings
  file_settings_.LoadFile(s9::File("./data/settings.xml"));

  std::srand(std::time(0)); 

  // File Load

  shader_skinning_ = Shader(s9::File("./data/skinning.glsl"));
  shader_quad_= Shader( s9::File("./data/quad_texture.vert"), s9::File("./data/quad_texture.frag"));
  shader_colour_ = Shader(s9::File("./data/solid_colour.glsl"));

  shader_warp_ = Shader( s9::File("./data/barrel.vert"), 
        s9::File("./data/barrel.frag"),
        s9::File("./data/barrel.geom"));

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
  
  // Game stuff

  arm_state_ = BOTH_ARMS;
  playing_game_ = false;
  last_shot_ = 0;

  // Physics

  physics_ = PhantomPhysics( FromStringS9<float_t>(file_settings_["game/gravity"]), 
    FromStringS9<float_t>(file_settings_["game/hand_radius"]));

  CXGLERROR

  // OpenGL Defaults

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

}

// Moved onto the main thread as GCC gets upset on quitting the thread with OpenNI

void PhantomLimb::UpdateMainThread(double_t dt) { 

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
 
      Bone * luparm = md5_.skeleton().GetBone("upper_arm.L");
      if (luparm != nullptr) {
        
        glm::quat final_rotation;

        // Deal with the mirroring of arms
        switch (arm_state_) {
          case BOTH_ARMS:
        final_rotation = user.skeleton().GetBone("Left Shoulder")->rotation();
break;  
	case LEFT_ARM:
            
        	final_rotation = glm::angleAxis(90.0f,0.0f,0.0f,1.0f);  
	break;

          case RIGHT_ARM:
            // Convert back to Euler, reverse then rebuild - not the best way I suspect :S
            glm::quat tq = user.skeleton().GetBone("Right Shoulder")->rotation();

            float_t angle = glm::angle(tq);
            glm::vec3 axis = glm::axis(tq);
            final_rotation = glm::angleAxis(-angle, -axis.x,axis.y,axis.z);

          break;
        }

        luparm->set_rotation_relative( rys * rzs * final_rotation   * rzsi * rysi );

      }


      Bone * lloarm = md5_.skeleton().GetBone("lower_arm.L");
      if (lloarm != nullptr) {
        glm::quat final_rotation;

        switch (arm_state_) {
          case BOTH_ARMS:
		final_rotation = user.skeleton().GetBone("Left Elbow")->rotation();

        break;  
	case LEFT_ARM:
		final_rotation = glm::quat();
            break;

          case RIGHT_ARM:
            // Convert back to Euler, reverse then rebuild - not the best way I suspect :S
            glm::quat tq =user.skeleton().GetBone("Right Elbow")->rotation();
            float_t angle = glm::angle(tq);
            glm::vec3 axis = glm::axis(tq);
            
            final_rotation = glm::angleAxis(-angle, -axis.x,axis.y,axis.z);
        
          break;
        }

        lloarm->set_rotation_relative(  rys * rzs * final_rotation * rzsi * rysi);
      }
        

      Bone * ruparm = md5_.skeleton().GetBone("upper_arm.R");
      if (ruparm != nullptr){
        glm::quat final_rotation;

        switch (arm_state_) {
          case BOTH_ARMS:
	final_rotation = user.skeleton().GetBone("Right Shoulder")->rotation();
        break;
	  case RIGHT_ARM:
            final_rotation = glm::angleAxis(90.0f,0.0f,0.0f,1.0f); // user.skeleton().GetBone("Right Shoulder")->rotation();
          break;

          case LEFT_ARM:
            // Convert back to Euler, reverse then rebuild - not the best way I suspect :S
            glm::quat tq = user.skeleton().GetBone("Left Shoulder")->rotation();

            float_t angle = glm::angle(tq);
            glm::vec3 axis = glm::axis(tq);
            final_rotation = glm::angleAxis(-angle, -axis.x, axis.y, axis.z);

          break;
        }

        ruparm->set_rotation_relative( nrys * nrzs *  final_rotation * nrzsi * nrysi  );

      }

      Bone * rloarm = md5_.skeleton().GetBone("lower_arm.R");
      if (rloarm != nullptr) {
        glm::quat final_rotation;

        switch (arm_state_) {
          case BOTH_ARMS:
	  final_rotation = user.skeleton().GetBone("Right Elbow")->rotation();

	break;
          case RIGHT_ARM:
        	final_rotation = glm::quat();  
	 break;

          case LEFT_ARM:
            // Convert back to Euler, reverse then rebuild - not the best way I suspect :S
            glm::quat tq = user.skeleton().GetBone("Left Elbow")->rotation();

            float_t angle = glm::angle(tq);
            glm::vec3 axis = glm::axis(tq);
            final_rotation = glm::angleAxis(-angle, -axis.x, axis.y, axis.z);

          break;
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

void PhantomLimb::Update(double_t dt) {

   // Update Oculus - take the difference
  oculus_.Update(dt);


 

}


/*
 * Called as fast as possible. Not set FPS wise but dt is passed in
 */

 void PhantomLimb::Display(GLFWwindow* window, double_t dt){

  GLfloat depth = 1.0f;

  // Update Physics
  physics_.Update(dt);

   // Update game state
  if (playing_game_){
    last_shot_ += dt ;
    if (last_shot_ > FromStringS9<float_t>(file_settings_["game/time"])) {
      last_shot_ = 0;
      FireBall();
    }
  }

  UpdateMainThread(dt);

  // Create the FBO and setup the cameras
  if (!fbo_ && oculus_.Connected()){
    
      glm::vec2 s = oculus_.fbo_size();
      fbo_ = FBO(static_cast<size_t>(s.x), static_cast<size_t>(s.y)); 

      camera_left_.Resize(static_cast<size_t>(s.x / 2.0f), static_cast<size_t>(s.y ));
      camera_right_.Resize(static_cast<size_t>(s.x / 2.0f), static_cast<size_t>(s.y ),static_cast<size_t>(s.x / 2.0f) );

      camera_.Resize(static_cast<size_t>(s.x ), static_cast<size_t>(s.y ));

      camera_left_.set_projection_matrix(oculus_.left_projection());
      camera_right_.set_projection_matrix(oculus_.right_projection());

      camera_ortho_.Resize(oculus_.screen_resolution().x, oculus_.screen_resolution().y);

      glGenVertexArrays(1, &(null_VAO_));
      
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

    // Draw textures from the camera
    /*
    glm::mat4 mat = glm::translate(glm::mat4(1.0f), glm::vec3(160,120,0));
    node_depth_.set_matrix(mat);
    openni_.texture_depth().Bind();
    node_depth_.Draw();
    openni_.texture_depth().Unbind();

    mat = glm::translate(glm::mat4(1.0f), glm::vec3(480,120,0));
    node_colour_.set_matrix(mat);
    openni_.texture_colour().Bind();
    node_colour_.Draw();
    openni_.texture_colour().Unbind();

    */

    fbo_.Unbind();
    //CXGLERROR

    // Draw to main screen - this cheats and uses a geometry shader

    // Be wary here that we are messing with the polygon mode up the chain

    glClearBufferfv(GL_COLOR, 0, &glm::vec4(0.9f, 0.9f, 0.9f, 1.0f)[0]);
    glClearBufferfv(GL_DEPTH, 0, &depth );

    glBindVertexArray(null_VAO_);

    glViewport(0,0, camera_ortho_.width(), camera_ortho_.height());

    shader_warp_.Bind();
    fbo_.colour().Bind();

    shader_warp_.s("uDistortionOffset", oculus_.distortion_xcenter_offset()); // Can change with future headsets apparently
    shader_warp_.s("uDistortionScale", 1.0f/oculus_.distortion_scale());
    shader_warp_.s("uChromAbParam", oculus_.chromatic_abberation());
    shader_warp_.s("uHmdWarpParam",oculus_.distortion_parameters() );

    glDrawArrays(GL_POINTS, 0, 1);

    fbo_.colour().Unbind();
    shader_warp_.Unbind();

    glBindVertexArray(0);

    //CXGLERROR -  annoyingly there is an error
  }
}

/// Fire a ball into the scene
void PhantomLimb::FireBall() {

  float_t rval0 = static_cast<float_t>(std::rand()) /  RAND_MAX;
  float_t rval1 = static_cast<float_t>(std::rand()) /  RAND_MAX;

  float_t game_width = FromStringS9<float_t>(file_settings_["game/width"]);
  float_t speed_min = FromStringS9<float_t>(file_settings_["game/speed/min"]);
  float_t speed_factor = FromStringS9<float_t>(file_settings_["game/speed/factor"]);

  float_t height_min = FromStringS9<float_t>(file_settings_["game/height/min"]);
  float_t height_factor = FromStringS9<float_t>(file_settings_["game/height/factor"]);

  float_t xpos = -game_width +  (rval0 * 2.0f * game_width);

  physics_.AddBall(ball_radius_, glm::vec3( xpos , height_min + (rval1 * height_factor), -4.0f), 
      glm::vec3(0.0f, 1.0f + rval1, speed_factor * rval1 + speed_min));
}


PhantomLimb::~PhantomLimb() {   

}


/*
 * This is called by the wrapper function when an event is fired
 */

void PhantomLimb::ProcessEvent(MouseEvent e, GLFWwindow* window){}

/*
 * Called when the window is Resized. You should set cameras here
 */

void PhantomLimb::ProcessEvent(ResizeEvent e, GLFWwindow* window){
  camera_ortho_.Resize(e.w,e.h);
}

void PhantomLimb::ProcessEvent(KeyboardEvent e, GLFWwindow* window){}



/*
 * The UX window Class
 */

#ifdef _SEBURO_LINUX


///\todo passing in the gtk_app is a bit naughty I think
UXWindow::UXWindow(gl::WithUXApp &gtk_app, PhantomLimb &app) : gtk_app_(gtk_app), app_(app)  {  

  //Glib::RefPtr< Screen > screen =  Gdk::Screen::get_default();
  //Glib::RefPtr<Display> display = screen->get_display();

  maximize();
  fullscreen();

  // Sets the border width of the window.
  set_border_width(10);

  // When the button receives the "clicked" signal, it will call the
  // on_button_clicked() method defined below.
  button_fire_ = new Gtk::Button("Fire Ball");
  button_fire_->signal_clicked().connect(sigc::mem_fun(*this, &UXWindow::on_button_fire_clicked));
  button_fire_->set_hexpand(true);
  button_fire_->set_vexpand(true);


  button_reset_ = new Gtk::Button("Reset Game");
  button_reset_->signal_clicked().connect(sigc::mem_fun(*this, &UXWindow::on_button_reset_clicked));
  button_reset_->set_hexpand(true);
  button_reset_->set_vexpand(true);

  button_auto_game_ = new Gtk::Button("Auto Game Start / Stop");
  button_auto_game_->signal_clicked().connect(sigc::mem_fun(*this, &UXWindow::on_button_auto_game_clicked));
  button_auto_game_->set_hexpand(true);
  button_auto_game_->set_vexpand(true);

  button_oculus_ = new Gtk::Button("Reset Oculus View");
  button_oculus_->signal_clicked().connect(sigc::mem_fun(*this, &UXWindow::on_button_oculus_clicked));
  button_oculus_->set_hexpand(true);
  button_oculus_->set_vexpand(true);

  button_tracking_ = new Gtk::Button("Restart Tracking");
  button_tracking_->signal_clicked().connect(sigc::mem_fun(*this, &UXWindow::on_button_tracking_clicked));
  button_tracking_->set_hexpand(true);
  button_tracking_->set_vexpand(true);


  button_quit_ = new Gtk::Button("Quit");
  button_quit_->signal_clicked().connect(sigc::mem_fun(*this, &UXWindow::on_button_quit_clicked));
  button_quit_->set_hexpand(true);
  button_quit_->set_vexpand(true);

  signal_delete_event().connect(sigc::mem_fun(*this, &UXWindow::on_window_closed));

  // Combo Box

  combo_arms_.append("Both");
  combo_arms_.append("Left Arm Dominate");
  combo_arms_.append("Right Arm Dominate");
  combo_arms_.set_active_text("Both");
  combo_arms_.signal_changed().connect(sigc::mem_fun(*this, &UXWindow::on_combo_arms_changed));

  // This packs the button into the Window (a container).
  grid_.attach(*button_fire_,0,0,1,1);
  grid_.attach(*button_reset_,0,1,1,1);
  grid_.attach(*button_auto_game_,0,2,1,1);
  grid_.attach(*button_tracking_,0,3,1,1);
  grid_.attach(*button_quit_,0,4,1,1);

  grid_.attach(*button_oculus_,1,0,1,1);
  grid_.attach(combo_arms_,1,1,1,1);

  grid_.set_hexpand();
  grid_.set_vexpand();

  add(grid_);
  show_all();

}

UXWindow::~UXWindow() {
  // Need to signal that we are done here
  delete button_fire_;
  delete button_reset_;
  delete button_auto_game_;
  delete button_tracking_;
  delete button_oculus_;
  delete button_quit_;

}

void UXWindow::on_button_fire_clicked() {
  cout << "Firing Ball" << endl;
  app_.FireBall();
}

void UXWindow::on_button_reset_clicked() {
  cout << "Reset Physics" << endl;
  app_.ResetPhysics();
}

void UXWindow::on_button_tracking_clicked() {
  cout << "Restarting Tracking" << endl;
  app_.RestartTracking();
}


void UXWindow::on_button_auto_game_clicked() {
  cout << "Auto Game Clicked" << endl;
  app_.PlayGame(!app_.playing_game());
}

void UXWindow::on_button_oculus_clicked() {
  cout << "Resetting Oculus View" << endl;
  app_.ResetOculus();
}

void UXWindow::on_button_quit_clicked() {
  cout << "Quitting PhantomLimb" << endl;
  gtk_app_.Shutdown();
}

bool UXWindow::on_window_closed(GdkEventAny* event) {
  cout << "Quitting PhantomLimb" << endl;
  gtk_app_.Shutdown();
  return false;
}

void UXWindow::on_combo_arms_changed() {
  Glib::ustring selected = combo_arms_.get_active_text();
  if (selected.compare( Glib::ustring("Both")) == 0){
    app_.SetHanded(BOTH_ARMS);
  } else if (selected.compare( Glib::ustring("Left Arm Dominate")) == 0){
    app_.SetHanded(LEFT_ARM);
  } else if (selected.compare( Glib::ustring("Right Arm Dominate")) == 0) {
    app_.SetHanded(RIGHT_ARM);
  }
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

  XMLSettings settings;
  if (!settings.LoadFile(s9::File("./data/settings.xml"))){
    cerr << "PhantomLimb: Could not find data/settings.xml. Aborting." << endl;
    return -1;
  }

  a.CreateWindowFullScreen("Oculus", 0, 0, settings["oculus_display"].c_str());
  
  //a.CreateWindow("Oculus", 1280, 800);

  UXWindow ux(a,b);

  a.Run(ux);

  // Call shutdown once the GTK Run loop has quit. This makes GLFW quit cleanly
  //a.Shutdown();

  return EXIT_SUCCESS;

#endif

}
