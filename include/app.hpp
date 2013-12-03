/*
* @brief PhantomLimb Application Header
* @file app.hpp
* @author Benjamin Blundell <oni@section9.co.uk>
* @date 02/12/2013
*
*/

#ifndef MD5APP_HPP
#define MD5APP_HPP

#include "s9/common.hpp"
#include "s9/file.hpp"
#include "s9/camera.hpp"
#include "s9/shapes.hpp"
#include "s9/node.hpp"
#include "s9/gl/drawable.hpp"
#include "s9/gl/shader.hpp"
#include "s9/gl/glfw_app.hpp"
#include "s9/image.hpp"
#include "s9/gl/texture.hpp"
#include "s9/md5.hpp"
#include "s9/composite_shapes.hpp"
#include "s9/oculus/oculus.hpp"
#include "s9/openni/openni.hpp"

#include "anttweakbar/AntTweakBar.h"

 
namespace s9 {

	/*
 	 * Phantom Limb main Oculus 3D Application
 	 */

	class PhantomLimb : public WindowApp, public WindowResponder {
	public:
		~PhantomLimb();
		
		void init();
		void display(double_t dt);
		void update(double_t dt);

		void processEvent(MouseEvent e);
		void processEvent(KeyboardEvent e);
		void processEvent(ResizeEvent e);

		
	protected:

		// Geometry
		Quad quad_;

		// Textures
		gl::Texture texture_;

		// Cameras
		Camera camera_;
		Camera ortho_camera_;
		
		// Global Nodes

		Node node_model_;
		Node node_depth_;
    Node node_colour_;

		
		// Model Classes
		MD5Model md5_;
		SkeletonShape skeleton_shape_;

		// Oculus Rift
		oculus::OculusBase oculus_;
		glm::quat oculus_rotation_dt_;
		glm::quat oculus_rotation_prev_;

		// OpenNI
		s9::oni::OpenNIBase openni_;
    s9::oni::OpenNISkeleton openni_skeleton_tracker_;

		// Shaders

		gl::Shader shader_skinning_;
		gl::Shader shader_quad_;
		gl::Shader shader_colour_;

		// Temp bytes for data off the cameras

		byte_t *camera_buffer_;

		float rotation_;
		
	};
}

#endif
