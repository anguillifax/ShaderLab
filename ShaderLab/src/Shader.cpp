#include "Shader.h"

#include "Util.h"

#include <glew/glew.h>

#include <iostream>

namespace coral {

	// ==============
	// Shader Factory
	// ==============

	namespace {

		void intl_create_shader(const std::string& path, GLuint shader_name)
		{
			std::string source = Util::read_file(path);

			const GLchar* src[] = { source.data() };
			GLint length[] = { static_cast<GLint>(source.length()) };

			glShaderSource(shader_name, 1, src, length);
			glCompileShader(shader_name);
		}

	}

	GLuint ShaderFactory::create_shader(const std::string& vertex_path, const std::string& fragment_path, bool log_output)
	{
		GLuint program = NULL;

		GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
		intl_create_shader(vertex_path, vertex_shader);

		GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
		intl_create_shader(fragment_path, frag_shader);

		// Create and compile program

		program = glCreateProgram();
		glObjectLabel(GL_PROGRAM, program, 0, "coral::shader");
		glAttachShader(program, vertex_shader);
		glAttachShader(program, frag_shader);

		glLinkProgram(program);

		// Cleanup shaders

		glDetachShader(program, vertex_shader);
		glDeleteShader(vertex_shader);

		glDetachShader(program, frag_shader);
		glDeleteShader(frag_shader);

		// Log linking output if enabled

		if (log_output) {
			GLchar out_buf[1024];
			GLint length;
			glGetProgramInfoLog(program, sizeof(out_buf), &length, out_buf);

			if (length > 0) {
				std::cout << "Program linking output:\n";
				std::cout << out_buf << '\n';
			} else {
				std::cout << "Program linked successfully with no output.\n";
			}
		}

		// Check if compilation was successful and return

		GLint success;
		glGetProgramiv(program, GL_LINK_STATUS, &success);

		if (!success) {
			glDeleteProgram(program);
			return NULL;
		} else {
			return program;
		}
	}

	// ============
	// Shader Class
	// ============

	Shader::Shader(const std::string& path)
	{
		Util::print_divider("SHADER BEGIN");

		program = ShaderFactory::create_shader(path + ".vert", path + ".frag", true);

		Util::print_divider("SHADER END");

		if (program == 0u) {
			throw std::exception("Failed to create shader.");
		}
	}

	Shader::~Shader()
	{
		glDeleteProgram(program);
	}

	void Shader::use()
	{
		glUseProgram(program);
	}

} // namespace coral
