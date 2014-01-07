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


#include <gtkmm.h>
 
namespace s9 {


#ifdef _SEBURO_LINUX

	class PhantomLimb;

	/*
 	 * UX Window for the Application. Runs on the main screen
 	 */

	class UXWindow : public Gtk::Window {

	public:
	  UXWindow(PhantomLimb &app);
	  virtual ~UXWindow();

	protected:
	  //Signal handlers:
	  void on_button_clicked();

	  Gtk::Button button_;

	  PhantomLimb& app_;

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

		// OpenNI
		s9::oni::OpenNIBase openni_;
    s9::oni::OpenNISkeleton openni_skeleton_tracker_;

		// Shaders

		gl::Shader shader_skinning_;
		gl::Shader shader_quad_;
		gl::Shader shader_colour_;
		gl::Shader 		shader_warp_;

		// Temp bytes for data off the cameras

		byte_t *camera_buffer_;

		float rotation_;

		
	};
}

#endif
