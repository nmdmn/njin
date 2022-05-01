#pragma once

#include <initializer_list>
#include <filesystem>

#include <glad/glad.h>

namespace njin {
class program {
public:
	program(const std::initializer_list<const std::filesystem::path> &shader_file_paths);
	auto on() const -> void;
	auto off() const -> void;

	template <typename T>
	friend auto operator<<(T &stream, const program &program) -> T & {
		return stream << program.to_string() << std::endl;
	}

private:
	auto load_shader_file(const std::filesystem::path &shader_file_path) const -> std::string;
	auto compile_shader(const std::string &shader_source, const GLenum &shader_type) const
		-> GLuint;
	auto get_shader_type(const std::filesystem::path &shader_file_path) const -> GLenum;
	auto to_string() const -> std::string;

	GLuint id_;
};

} // namespace njin
