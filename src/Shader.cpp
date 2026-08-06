#include "Shader.hpp"

// |=====================================================
// |---[Get all contents in the File]--------------------
// |=====================================================
std::string get_file_contents(const char* filename)
{
	std::ifstream in(filename, std::ios::binary);
	if (in)
	{
		std::string contents;
		// Jump to the end to find the file size, then read it all in one go
		in.seekg(0, std::ios::end);
		contents.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return(contents);
	}
	throw std::runtime_error(std::string("Could not open file: ") + filename);
}

// |=====================================================
// |---[Constructor]-------------------------------------
// |=====================================================
Shader::Shader(const char* vertexFile, const char* fragmentFile)
{
    // Load the raw GLSL source from both shader files from disk
    std::string vertexCode = get_file_contents(vertexFile);
    std::string fragmentCode = get_file_contents(fragmentFile);
    const char* vertexSource = vertexCode.c_str();
    const char* fragmentSource = fragmentCode.c_str();

    // ===[Compile the vertex shader]===
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);
	compileErrors(vertexShader, "VERTEX", vertexFile);

	// ===[Compile the fragment shader]===
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);
	compileErrors(fragmentShader, "FRAGMENT", fragmentFile);

	// ===[Link both stages into a single usable program]===
	ID = glCreateProgram();
	glAttachShader(ID, vertexShader);
	glAttachShader(ID, fragmentShader);
	glLinkProgram(ID);
	compileErrors(ID, "PROGRAM", (vertexFilePath + " / " + fragmentFilePath).c_str());

	// Once linked into the program, shader objs are not needed
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

// |=====================================================
// |---[Use and Delete Shader]---------------------------
// |=====================================================
void Shader::Use()
{
    glUseProgram(ID);
}
void Shader::Delete()
{
    glDeleteProgram(ID);
}

// |=====================================================
// |---[Check for Compiler errors]-----------------------
// |=====================================================
void Shader::compileErrors(unsigned int shader, const char* type, const char* filePath)
{
    // Stores status of compilation
	int hasCompiled;
	// Character array to store error message in
	char infoLog[1024];

	// Individual shader stages and the final linked program report errors differently
	if (strcmp(type, "PROGRAM") != 0)
	{
		glGetShaderiv(shader, GL_COMPILE_STATUS, &hasCompiled);
		if (hasCompiled == GL_FALSE)
		{
			glGetShaderInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "SHADER_COMPILATION_ERROR [" << type << "] in: "
                << filePath << "\n" << infoLog << '\n';
		}
	}
	else
	{
		glGetProgramiv(shader, GL_LINK_STATUS, &hasCompiled);
		if (hasCompiled == GL_FALSE)
		{
			glGetProgramInfoLog(shader, 1024, NULL, infoLog);
			std::cout << "SHADER_LINKING_ERROR [PROGRAM] files: "
                << filePath << "\n" << infoLog << '\n';
		}
	}
}
