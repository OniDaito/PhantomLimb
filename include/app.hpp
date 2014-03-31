/*
* @brief PhantomLimb Application Header
* @file app.hpp
* @author Benjamin Blundell <oni@section9.co.uk>
* @date 02/12/2013
*
*/

#ifndef PHANTOM_LIMB_HPP
#define PHANTOM_LIMB_HPP

#include "s9/application.hpp"
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
#include "s9/obj_mesh.hpp"

#include <AntTweakBar/AntTweakBar.h>
#include "physics.hpp"
 
namespace s9 {


#ifdef _SEBURO_LINUX

	// Type for selecting which arms to use
	typedef enum {
		BOTH_ARMS,
		LEFT_ARM_RIGHT_FROZEN,
		RIGHT_ARM_LEFT_FROZEN,
		LEFT_ARM_RIGHT_MIRROR,
		RIGHT_ARM_LEFT_MIRROR,
		LEFT_ARM_COPY,
		RIGHT_ARM_COPY
	}ArmState;

	class PhantomLimb;


#endif

	/*
 	 * Phantom Limb main Oculus 3D Application
 	 */

	class PhantomLimb : public Application, public WindowListener<gl::GLWindow>  {
	public:
		
		PhantomLimb (XMLSettings &settings );
		~PhantomLimb();

		
		void InitOculusDisplay();
		void InitSecondDisplay();
		void DrawOculusDisplay(double_t dt);
		void DrawSecondDisplay(double_t dt);
		void ThreadMainLoop(double_t dt);
		void MainLoop(double_t dt);

		
		// Event handling - you can choose which to override
		void ProcessEvent(const gl::GLWindow & window, MouseEvent e);
		void ProcessEvent(const gl::GLWindow & window, KeyboardEvent e);
		void ProcessEvent(const gl::GLWindow & window, ResizeEvent e);
		void ProcessEvent(const gl::GLWindow & window, CloseWindowEvent e);


		// UX Interface functions
		void FireBall();
		void ResetPhysics() { physics_.Reset(); }
		void PlayGame(bool b) { playing_game_ = b; last_shot_ = 0; }
		void RestartTracking() { openni_skeleton_tracker_.RestartTracking(); }
		void ResetOculus() { oculus_.ResetView(); }
		void SetHanded(ArmState a) { arm_state_ = a; }

		bool playing_game() {return playing_game_; }

		void UpdateMainThread(double_t dt);

		void set_arm_emphasis(bool b) { arm_emphasis_ = b; }


	protected:

		void Close();

		// Window Manager
		gl::GLFWWindowManager window_manager_;

		// Geometry
		Quad quad_, quad_controls_;

		// Textures
		gl::Texture texture_;

		// Cameras
		Camera camera_;
		Camera camera_ortho_;
		Camera camera_left_;
		Camera camera_right_;
		
		Camera camera_second_;

		// Global Nodes

		Node node_model_;
		Node node_depth_;
    Node node_colour_;
    Node node_hands_;
    Node node_left_hand_;
    Node node_right_hand_;

    Node 					node_left_;
		Node 					node_right_;

		Node node_controls_;
		
		// Model Classes
		MD5Model md5_;
		SkeletonShape skeleton_shape_;

		ObjMesh room_;

		// Tweakbar
		TwBar *tweakbar_;

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
		gl::Shader shader_room_;
		gl::Shader controls_shader_;

		// Colours

		glm::vec4 hand_left_colour_, hand_right_colour_;

		// Temp bytes for data off the cameras

		byte_t *camera_buffer_;

		float rotation_;

		// Physics

		PhantomPhysics physics_;

		// Game

		bool playing_game_;
		bool arm_emphasis_;
		double_t last_shot_;
		float_t scale_speed_;
		float_t scale_width_;

		ArmState arm_state_;
		
		// Read settings

		XMLSettings &file_settings_;


		// AntTweakbar Functions for the UX
		// Annoyingly, they need to be static

		static PhantomLimb* pp_;
		static void TW_CALL on_button_fire_clicked(void * );
		static void TW_CALL on_button_reset_clicked(void * );
		static void TW_CALL on_button_tracking_clicked(void * );
		static void TW_CALL on_button_auto_game_clicked(void * );
		static void TW_CALL on_button_oculus_clicked(void * );
		static void TW_CALL on_button_quit_clicked(void * );
 

	};
}

#endif
