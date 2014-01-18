/*
* @brief PhantomLimb Application Header
* @file app.hpp
* @author Benjamin Blundell <oni@section9.co.uk>
* @date 02/12/2013
*
*/

#ifndef PHANTOM_LIMB_HPP
#define PHANTOM_LIMB_HPP

#include "s9/common.hpp"
#include "s9/file.hpp"
#include "s9/camera.hpp"
#include "s9/shapes.hpp"
#include "s9/node.hpp"
#include "s9/gl/drawable.hpp"
#include "s9/gl/shader.hpp"
#include "s9/gl/glfw.hpp"
#include "s9/image.hpp"
#include "s9/gl/texture.hpp"
#include "s9/gl/fbo.hpp"
#include "s9/md5.hpp"
#include "s9/composite_shapes.hpp"
#include "s9/oculus/oculus.hpp"
#include "s9/openni/openni.hpp"
#include "s9/xml_parse.hpp"

#include "physics.hpp"

#include <gtkmm.h>
 
namespace s9 {


#ifdef _SEBURO_LINUX

	// Type for selecting which arms to use
	typedef enum {
		BOTH_ARMS,
		LEFT_ARM,
		RIGHT_ARM
	}ArmState;

	class PhantomLimb;

	/*
 	 * UX Window for the Application. Runs on the main screen
 	 */

	class UXWindow : public Gtk::Window {

	public:
	  UXWindow(gl::WithUXApp &gtk_app, PhantomLimb &app);
	  virtual ~UXWindow();

	protected:
	  //Signal handlers:
	  void on_button_fire_clicked();
	  void on_button_reset_clicked();
	  void on_button_auto_game_clicked();
	  void on_button_quit_clicked();
	  void on_button_tracking_clicked();
	  void on_button_oculus_clicked();
	  bool on_window_closed(GdkEventAny* event);
	  void on_combo_arms_changed();

	  // Layout

	  Gtk::Grid		grid_;

	  // Buttons
	  Gtk::Button* button_fire_;
	  Gtk::Button* button_auto_game_;
	  Gtk::Button* button_reset_;
	  Gtk::Button* button_oculus_;
	  Gtk::Button* button_tracking_;
	  Gtk::Button* button_quit_;

	  Gtk::ComboBoxText combo_arms_;

	  PhantomLimb& app_;
	  gl::WithUXApp& gtk_app_;

	};

#endif

	/*
 	 * Phantom Limb main Oculus 3D Application
 	 */

	class PhantomLimb : public WindowApp<GLFWwindow*> {
	public:
		~PhantomLimb();
		
		void Init();
		void Display(GLFWwindow* window, double_t dt);
		void Update(double_t dt);

		void ProcessEvent(MouseEvent e, GLFWwindow* window);
		void ProcessEvent(KeyboardEvent e, GLFWwindow* window);
		void ProcessEvent(ResizeEvent e, GLFWwindow* window);

		// UX Interface functions
		void FireBall();
		void ResetPhysics() { physics_.Reset(); }
		void PlayGame(bool b) { playing_game_ = b; last_shot_ = 0; }
		void RestartTracking() { openni_skeleton_tracker_.RestartTracking(); }
		void ResetOculus() { oculus_.ResetView(); }
		void SetHanded(ArmState a) { arm_state_ = a; }

		bool playing_game() {return playing_game_; }

	protected:

		// Geometry
		Quad quad_;

		// Textures
		gl::Texture texture_;

		// Cameras
		Camera camera_;
		Camera camera_ortho_;
		Camera 				camera_left_;
		Camera 				camera_right_;
		
		// Global Nodes

		Node node_model_;
		Node node_depth_;
    Node node_colour_;
    Node node_hands_;
    Node node_left_hand_;
    Node node_right_hand_;

    Node 					node_left_;
		Node 					node_right_;
		
		// Model Classes
		MD5Model md5_;
		SkeletonShape skeleton_shape_;

		// Oculus Rift
		oculus::OculusBase oculus_;
		glm::quat oculus_rotation_dt_;
		glm::quat oculus_rotation_prev_;

		gl::FBO				fbo_;
		GLuint				null_VAO_;

		glm::mat4 model_base_mat_;

		// Hands
		glm::vec4 hand_pos_left_;
		glm::vec4 hand_pos_right_;

		glm::vec3 hand_pos_left_final_;
		glm::vec3 hand_pos_right_final_;

		// Balls for Physics
		Node 	node_ball_;
		glm::vec4 ball_colour_;
		float_t ball_radius_;

		// OpenNI
		s9::oni::OpenNIBase openni_;
    s9::oni::OpenNISkeleton openni_skeleton_tracker_;

		// Shaders

		gl::Shader shader_skinning_;
		gl::Shader shader_quad_;
		gl::Shader shader_colour_;
		gl::Shader shader_warp_;

		// Colours

		glm::vec4 hand_left_colour_, hand_right_colour_;

		// Temp bytes for data off the cameras

		byte_t *camera_buffer_;

		float rotation_;

		// Physics

		PhantomPhysics physics_;

		// Game

		bool playing_game_;
		double_t last_shot_;

		ArmState arm_state_;
		
		// Read settings

		XMLSettings file_settings_;

	};
}

#endif
