#pragma once

#include <glew/glew.h>

#include <string>

namespace coral {

	class ShaderFactory {
	public:

		/// @brief Create a new shader by reading files. Returns a new shader
		/// program if successful or 0 if program failed to compile
		/// successfully.
		///
		/// @param vertex_path Path to the file containing the vertex shader
		/// @param fragment_path Path to the file containing the fragment shader
		/// @param log_output Log information about compiling and linking to stdout.
		/// @return A new shader program name on success, or 0 if failure.
		static GLuint create_shader(const std::string& vertex_path, const std::string& fragment_path, bool log_output);

	};

	/// @brief Encapsulates creation and destruction of a shader.
	class Shader {

		GLuint program = 0u;

	public:

		// ============
		// Construction
		// ============

		/// @brief Create a new shader from a .vert and .frag file.
		/// @param path Path to the base file without the extension.
		/// @return A shader if successful or throws std::exception if any step of compilation fails.
		Shader(const std::string& path);
		~Shader();

		Shader(const Shader&) = delete;
		Shader& operator=(const Shader&) = delete;

		Shader(Shader&&) = default;
		Shader& operator=(Shader&&) = default;

		// =========
		// Interface
		// =========

		[[nodiscard]] GLuint get_program() noexcept
		{
			return program;
		}

		void use();

	};

} // namespace coral

