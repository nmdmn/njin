#include "program.h"

#include <fstream>
#include <sstream>
#include <exception>
#include <vector>
#include <algorithm>

using namespace std;

namespace njin {
program::program(const initializer_list<const filesystem::path> &vertex_file_paths)
	: id_{glCreateProgram()} {
	if(!id_) throw runtime_error("glCreateProgram() failed");

	vector<GLuint> shader_ids;
	ranges::for_each(vertex_file_paths, [&](const auto &vertex_file_path) {
		auto shader_type = get_shader_type(vertex_file_path);
		auto shader_source = load_shader_file(vertex_file_path);
		auto shader_id = compile_shader(shader_source, shader_type);

		glAttachShader(id_, shader_id);
		shader_ids.push_back(shader_id);
	});

	glLinkProgram(id_);

	GLint result = GL_FALSE;
	glGetProgramiv(id_, GL_LINK_STATUS, &result);
	if(GL_FALSE == result) {
		GLint length;
		glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &length);
		unique_ptr<char[]> errorMessage = make_unique<char[]>(static_cast<size_t>(length));
		glGetProgramInfoLog(id_, length, NULL, errorMessage.get());
		throw invalid_argument("program linker failed: " + string(errorMessage.get()));
	}

	ranges::for_each(shader_ids, [&](auto shader_id) {
		glDetachShader(id_, shader_id);
		glDeleteShader(shader_id);
	});
}

auto program::on() const -> void {
	glUseProgram(id_);
}

auto program::off() const -> void {
	glUseProgram(0);
}

auto program::load_shader_file(const filesystem::path &shader_file_path) const -> string {
	ifstream shader_file{shader_file_path};
	if(!shader_file.is_open())
		throw invalid_argument("missing shader file: " + shader_file_path.string());
	stringstream shader_source_buffer;
	shader_source_buffer << shader_file.rdbuf();
	return shader_source_buffer.str();
}

auto program::compile_shader(const string &shader_source, const GLenum &shader_type) const
	-> GLuint {
	GLuint shader_id = glCreateShader(shader_type);
	if(!shader_id) throw runtime_error("glCreateShader() failed");

	const GLchar *rawSource = shader_source.c_str();
	glShaderSource(shader_id, 1, &rawSource, NULL);
	glCompileShader(shader_id);

	GLint result = GL_FALSE;
	glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);
	if(GL_FALSE == result) {
		GLint length;
		glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &length);
		unique_ptr<char[]> errorMessage = make_unique<char[]>(static_cast<size_t>(length));
		glGetShaderInfoLog(shader_id, length, NULL, errorMessage.get());
		throw invalid_argument("shader compile failed: " + string(errorMessage.get()));
	}
	return shader_id;
}

auto program::get_shader_type(const filesystem::path &shader_file_path) const -> GLenum {
	auto extension = shader_file_path.extension().string();
	if(extension == ".vs" || extension == ".vert")
		return GL_VERTEX_SHADER;
	else if(extension == ".fs" || extension == ".frag")
		return GL_FRAGMENT_SHADER;
	else
		throw invalid_argument("unkonw shader file extension: " + extension);
}

auto program::to_string() const -> string {
	GLint size;
	GLenum type;
	const GLsizei bufSize = 16;
	GLchar name[bufSize];
	GLsizei length;

	GLint count;
	glGetProgramiv(id_, GL_ACTIVE_ATTRIBUTES, &count);
	stringstream msg_stream;
	msg_stream << "program info:" << endl << "attributes: " << ::to_string(count) << endl;
	for(GLint index = 0; index < count; index++) {
		glGetActiveAttrib(id_, static_cast<GLuint>(index), bufSize, &length, &size, &type, name);

		msg_stream << "\t id=" << ::to_string(index) << ", type=" << ::to_string(type)
				   << ", name=\"" << name << "\"" << endl;
	}

	glGetProgramiv(id_, GL_ACTIVE_UNIFORMS, &count);
	msg_stream << "uniforms: " << ::to_string(count) << endl;

	for(GLint index = 0; index < count; index++) {
		glGetActiveUniform(id_, static_cast<GLuint>(index), bufSize, &length, &size, &type, name);

		msg_stream << "\t id=" << ::to_string(index) << ", type=" << ::to_string(type)
				   << ", name=\"" << name << "\"" << endl;
	}

	return msg_stream.str();
}
} // namespace njin
