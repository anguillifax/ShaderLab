#include "Util.h"
#include "Shader.h"

#include <sdl/SDL.h>
#include <glew/glew.h>

#include <memory>
#include <iomanip>
#include <iostream>
#include <functional>

static void GLAPIENTRY gl_message_callback(GLenum, GLenum type, GLuint, GLenum severity, GLsizei, const GLchar* message, const void*)
{
	using namespace coral;

	bool is_error = type == GL_DEBUG_TYPE_ERROR;
	std::ostream& output = (is_error ? std::cerr : std::cout);

	if (is_error) {
		Util::set_color(AnsiColor::RED);
	}
	output << "GL Callback: " << (is_error ? "** GL Error **" : "Non Error") << " - Severity: " << severity << '\n';
	output << message << '\n';
	if (is_error) {
		Util::clear_color();
	}
}

namespace coral {

	struct Vector3 {
		GLfloat x;
		GLfloat y;
		GLfloat z;
	};

	struct Viewport {
		int x;
		int y;
		int width;
		int height;
	};

	class TimeManager {

		const double TIME_DELTA;

		double total_time = 0;
		float time_scale = 1;
		bool hit_zero = false;

	public:

		TimeManager(double time_delta)
			: TIME_DELTA(time_delta)
		{
		}

		void update(const std::vector<SDL_Event>& events)
		{
			for (const SDL_Event& event : events) {
				if (event.type != SDL_KEYDOWN) {
					continue;
				}

				switch (event.key.keysym.scancode) {
					case SDL_SCANCODE_H:
						time_scale = 1;
						total_time = 0;
						std::cout << "Restarting playback\n";
						break;

					case SDL_SCANCODE_SPACE:
						time_scale = 1;
						print_time_scale();
						break;

					case SDL_SCANCODE_L:
						time_scale += 1.0f;
						if (time_scale == 0) {
							time_scale += 1;
						}
						print_time_scale();
						break;

					case SDL_SCANCODE_J:
						hit_zero = false;
						time_scale -= 1.0f;
						if (time_scale == 0) {
							time_scale -= 1;
						}
						print_time_scale();
						break;

					case SDL_SCANCODE_K:
						time_scale = 0.0;
						print_time_scale();
						break;

					case SDL_SCANCODE_U:
						time_scale = 0.0;
						total_time -= TIME_DELTA;
						if (total_time < 0) {
							total_time = 0;
						}
						print_step();
						break;

					case SDL_SCANCODE_O:
						time_scale = 0.0;
						total_time += TIME_DELTA;
						print_step();
						break;

					case SDL_SCANCODE_I:
						total_time = 0.0;
						std::cout << "Reset to time 0\n";
						break;

					case SDL_SCANCODE_P:
						total_time = 10'000;
						std::cout << "Jump to time 10,000s\n";
						break;
				}
			}

			if (time_scale != 0) {
				total_time += time_scale * TIME_DELTA;
			}
			if (total_time < 0) {
				total_time = 0;
				if (!hit_zero) {
					puts("Hit time 0");
					hit_zero = true;
				}
			}
		}

		[[nodiscard]] float get_time() const noexcept
		{
			return static_cast<float>(total_time);
		}

		void set_time_scale(float timescale)
		{
			this->time_scale = timescale;
		}

		[[nodiscard]] float get_time_scale() const noexcept
		{
			return time_scale;
		}

		void set_time(float time)
		{
			this->total_time = time;
		}

	private:

		void print_time_scale() const
		{
			std::cout << "Timescale: " << time_scale << "x\n";
		}

		void print_step() const
		{
			std::cout << "Step: " << total_time << 's';
		}

	};

	class Program {

		static constexpr unsigned int SWAP_DELAY = 1000u / 60u;
		static constexpr double TIME_DELTA = static_cast<double>(SWAP_DELAY) / 1000.0;
		static constexpr int VIEWPORT_INSET = 0;

		SDL_Window* window = nullptr;
		SDL_GLContext context = nullptr;

		GLuint program = 0u;
		GLuint vao = 0u;
		GLuint vbo = 0u;

		bool quit = false;
		bool skip_render = false;
		SDL_Event cur_event{};
		std::vector<SDL_Event> event_queue{};

		TimeManager time{ TIME_DELTA };

		Viewport viewport{ VIEWPORT_INSET, VIEWPORT_INSET, 0, 0 };

		static constexpr Vector3 vertices[] = {
			//{ -0.5f, +0.5f, 0.5f },
			//{ -0.5f, -0.5f, 0.5f },
			//{ +0.5f, +0.5f, 0.5f },
			//{ +0.5f, -0.5f, 0.5f },
			{ -1.0f, +1.0f, 0.5f },
			{ -1.0f, -1.0f, 0.5f },
			{ +1.0f, +1.0f, 0.5f },
			{ +1.0f, -1.0f, 0.5f },
		};

		enum UniformLocation {
			UNIFORM_WINDOW_SIZE = 0,
			UNIFORM_TIME = 1,
			UNIFORM_MOUSE_POSITION = 2,
		};

	public:

		// ============
		// Construction
		// ============

		Program()
		{
			window = SDL_CreateWindow("SDL + OpenGL",
				SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720,
				SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
			if (window == nullptr) {
				throw std::exception(SDL_GetError());
			}

			static constexpr int CHANNEL_SIZE = 8;
			SDL_GL_SetAttribute(SDL_GL_RED_SIZE, CHANNEL_SIZE);
			SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, CHANNEL_SIZE);
			SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, CHANNEL_SIZE);
			SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, CHANNEL_SIZE);
			SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, CHANNEL_SIZE * 4);
			SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

			context = SDL_GL_CreateContext(window);
			if (context == nullptr) {
				throw std::exception(SDL_GetError());
			}

			if (glewInit() != GLEW_OK) {
				throw std::exception("GLEW failed to init");
			}

			glEnable(GL_DEBUG_OUTPUT);
			glDebugMessageCallback(gl_message_callback, nullptr);

			SDL_SetWindowSize(window, 1280, 720);
		}

		~Program()
		{
			glDeleteVertexArrays(1, &vao);
			glDeleteBuffers(1, &vbo);
			glDeleteProgram(program);

			SDL_GL_DeleteContext(context);
			SDL_DestroyWindow(window);
		}

		// ==============
		// Implementation
		// ==============

		void run()
		{
			create_shader();
			create_buffer();

			glPointSize(4.0f);
			glPolygonMode(GL_BACK, GL_LINE);

			while (!quit) {
				handle_events();
				time.update(event_queue);

				glClearColor(0.01f, 0.01f, 0.01f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT);

				if (program != NULL) {
					glUseProgram(program);
					update_uniforms();

					glBindVertexArray(vao);
					glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
					glBindVertexArray(NULL);

					glUseProgram(NULL);
				}

				SDL_GL_SwapWindow(window);

				SDL_Delay(SWAP_DELAY);
			}
		}

	private:

		void update_uniforms()
		{
			glUniform1f(UNIFORM_TIME, time.get_time());

			int x, y;
			SDL_GetMouseState(&x, &y);
			int height;
			SDL_GetWindowSize(window, nullptr, &height);
			glUniform2i(UNIFORM_MOUSE_POSITION, x, height - y);
		}

		void update_window_size_uniform()
		{
			glUniform2i(UNIFORM_WINDOW_SIZE, viewport.width, viewport.height);
		}

		void log_info()
		{
			Util::print_divider("INFO BEGIN");

			std::cout << glGetString(GL_VERSION) << '\n';
			std::cout << glGetString(GL_VENDOR) << '\n';
			std::cout << glGetString(GL_RENDERER) << '\n';
			std::cout << glGetString(GL_SHADING_LANGUAGE_VERSION) << '\n';

			GLint max;
			glGetIntegerv(GL_MAX_LABEL_LENGTH, &max);
			std::cout << "Max Label Length: " << max << '\n';

			Util::print_divider("INFO END");
		}

		void create_shader()
		{
			static const std::string path = "../shaders/first";

			if (program != NULL) {
				glDeleteProgram(program);
				program = NULL;
			}
			program = ShaderFactory::create_shader(path + ".vert", path + ".frag", true);

			if (program != NULL) {
				glUseProgram(program);
				update_uniforms();
				update_window_size_uniform();
				glUseProgram(NULL);
			} else {
				Util::set_color(AnsiColor::RED);
				std::cout << "Error compiling recompiling shader.\n";
				Util::clear_color();
			}
		}

		void create_buffer()
		{
			// VAO
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);
			glObjectLabel(GL_VERTEX_ARRAY, vao, 0, "agfx::vao");

			// VBO
			glGenBuffers(1, &vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glObjectLabel(GL_BUFFER, vbo, 0, "agfx::vao");

			glBufferStorage(GL_ARRAY_BUFFER, sizeof(vertices), vertices, 0u);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), reinterpret_cast<void*>(0));

			// Cleanup
			glBindBuffer(GL_ARRAY_BUFFER, NULL);
			glBindVertexArray(NULL);
		}

		void clear_console()
		{
			for (size_t i = 0; i < 100u; i++) {
				puts("");
			}
		}

		void handle_events()
		{
			event_queue.clear();

			while (SDL_PollEvent(&cur_event)) {
				event_queue.push_back(cur_event);

				switch (cur_event.type) {

					case SDL_QUIT:
						quit = true;
						break;

					case SDL_KEYDOWN:
						switch (cur_event.key.keysym.scancode) {

							case SDL_SCANCODE_F4:
								quit = true;
								break;

							case SDL_SCANCODE_R:
								puts("");
								Util::set_color(AnsiColor::GREEN);
								puts("Hot reloading shaders...");
								Util::clear_color();

								create_shader();

								time.set_time(0);
								time.set_time_scale(1);
								break;

							case SDL_SCANCODE_C:
								clear_console();
								break;

							case SDL_SCANCODE_F1:
								log_info();
								break;
						}
						break;

					case SDL_WINDOWEVENT:
						switch (cur_event.window.event) {
							case SDL_WINDOWEVENT_SIZE_CHANGED:
								Sint32 width = cur_event.window.data1;
								Sint32 height = cur_event.window.data2;

								Util::set_color(AnsiColor::GREEN);
								std::cout << "Window resized to " << width << " x " << height << '\n';
								Util::clear_color();

								viewport.width = width - 2 * VIEWPORT_INSET;
								viewport.height = height - 2 * VIEWPORT_INSET;
								glViewport(viewport.x, viewport.y, viewport.width, viewport.height);

								if (program != NULL) {
									glUseProgram(program);
									update_window_size_uniform();
									glUseProgram(NULL);
								}

								break;
						}
						break;

				}
			}
		}

	};

} // namespace coral

int main(int, char**)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cerr << "SDL Failed to init\n";
	}

	try {
		coral::Program().run();
	} catch (std::exception& e) {
		std::cerr << "Program panicked:\n" << e.what() << '\n';
		SDL_Quit();
		return 1;
	}

	SDL_Quit();
	return 0;
}