#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SOIL/SOIL.h> 
#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <list>

#include "model.h"
#include <random>
#include <cstdlib>  
#include <ctime>

glm::mat4 projection = glm::perspective(glm::radians(45.0f), 900.0f / 900.0f, 0.1f, 100.0f);

glm::vec3 cameraPos = glm::vec3(0, 60, 80);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.5f, -0.8f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

GLuint snowVBO;

GLuint snowVAO;

GLuint snowTexture;

GLuint ProgramFong;

glm::vec3 lightPos(0.0f, 0.0f, 0.0f);
glm::vec3 lightDirection(-1.0f, -24.0f, -60.0f);
glm::vec3 lightness(1.5f, 1.5f, 1.5f);
float conus = 20.0f;

glm::vec3 airshipPos(0.0f, 15.0f, 30.0f);
vector<glm::vec3> positionCloudsAndBalloons;
float numCloudsAndBalloons = 10;
bool turnLight = true;

const char* VertexShaderFong = R"(
    #version 330 core

    layout (location = 0) in vec3 coord_pos;
    layout (location = 1) in vec3 normal_in;
    layout (location = 2) in vec2 tex_coord_in;

    out vec2 coord_tex;
	out vec3 normal;
	out vec3 frag_pos;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    void main() 
    { 
        gl_Position = projection * view * model * vec4(coord_pos, 1.0);
        coord_tex = tex_coord_in;
		frag_pos = vec3(model * vec4(coord_pos, 1.0));
		normal =  mat3(transpose(inverse(model))) * normal_in;
    }
    )";

const char* FragShaderFong = R"(
    #version 330 core

	struct Light {
		vec3 position;
		vec3 direction; 
		vec3 directionSpot; 
  
		vec3 ambient;
		vec3 diffuse;
		vec3 specular;

		float constant;
		float linear;
		float quadratic;

		float cutOff;
		float outerCutOff;
	};

	uniform Light light;  

    in vec2 coord_tex;
    in vec3 frag_pos;
    in vec3 normal;

	out vec4 frag_color;

    uniform sampler2D texture_diffuse1;
	uniform vec3 viewPos;
	uniform vec3 airshipPos;
	uniform int shininess;
	uniform int turnLight;

    void main()  
    {
		vec3 norm = normalize(normal);
		vec3 lightDir;

		lightDir = normalize(-light.direction);
		
		vec3 ambient = light.ambient * texture(texture_diffuse1, coord_tex).rgb; 

		float diff = max(dot(norm, lightDir), 0.0);
		vec3 diffuse = light.diffuse * (diff * texture(texture_diffuse1, coord_tex).rgb); 

		vec3 viewDir = normalize(viewPos - frag_pos);
		vec3 reflectDir = reflect(-lightDir, norm);  

		float spec = pow(max(dot(viewDir, reflectDir), 0.0), 1);
		vec3 specular = light.specular * (spec * texture(texture_diffuse1, coord_tex).rgb); 

		vec3 result = (ambient * 2 + diffuse + specular);

		if(turnLight == 1)
		{
			lightDir = normalize(airshipPos - frag_pos);

			diff = max(dot(norm, lightDir), 0.0);
			diffuse = vec3(1.0f, 0.4f, 0.7f) * (diff * texture(texture_diffuse1, coord_tex).rgb); 

		    reflectDir = reflect(-lightDir, norm);  
			spec = pow(max(dot(viewDir, reflectDir), 0.0), 1);
			specular = vec3(0.8f, 0.0f, 0.7f) * (spec * texture(texture_diffuse1, coord_tex).rgb);

			float distance    = length(airshipPos - frag_pos);
			float attenuation = 1.0 / (light.constant + light.linear * distance 
								+ light.quadratic * (distance * distance));
				
			diffuse  *= attenuation;
			specular *= attenuation;  
			
			float theta = dot(lightDir, normalize(-light.directionSpot)); 
			float epsilon   = light.cutOff - light.outerCutOff;
			float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

			diffuse  *= intensity;
			specular *= intensity;

			result +=  diffuse * 5 + specular;
		}

		frag_color = vec4(result, 1.0);
    } 
)";

void CloudsAndBalloons();

void checkOpenGLerror()
{
	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
	{
		std::cout << "OpenGL error " << err << std::endl;
	}
}

void ShaderLog(unsigned int shader)
{
	int infologLen = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLen);
	GLint vertex_compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &vertex_compiled);

	if (infologLen > 1)
	{
		int charsWritten = 0;
		std::vector<char> infoLog(infologLen);
		glGetShaderInfoLog(shader, infologLen, &charsWritten, infoLog.data());
		std::cout << "InfoLog: " << infoLog.data() << std::endl;
	}

	if (vertex_compiled != GL_TRUE)
	{
		GLsizei log_length = 0;
		GLchar message[1024];
		glGetShaderInfoLog(shader, 1024, &log_length, message);
		std::cout << "InfoLog2: " << message << std::endl;
	}
}

void InitShader()
{
	GLuint vShaderFong = glCreateShader(GL_VERTEX_SHADER); 
	glShaderSource(vShaderFong, 1, &VertexShaderFong, NULL); 
	glCompileShader(vShaderFong);
	std::cout << "vertex shader f\n";
	ShaderLog(vShaderFong); 

	GLuint fShaderFong = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fShaderFong, 1, &FragShaderFong, NULL);
	glCompileShader(fShaderFong);
	std::cout << "fragment shader f\n";
	ShaderLog(fShaderFong);
	
	int link_ok;
	
	ProgramFong = glCreateProgram();
	glAttachShader(ProgramFong, vShaderFong);
	glAttachShader(ProgramFong, fShaderFong);

	glLinkProgram(ProgramFong);

	glGetProgramiv(ProgramFong, GL_LINK_STATUS, &link_ok);

	if (!link_ok)
	{
		std::cout << "error attach shaders \n";
		return;
	}

	checkOpenGLerror();
}

void Plane()
{
	float plane[] = {
		 25.0f, 0.0f,  25.0f,  0.0f, 1.0f, 0.0f,  5.0f,  0.0f,
		-25.0f, 0.0f,  25.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
		-25.0f, 0.0f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 5.0f,

		 25.0f, 0.0f,  25.0f,  0.0f, 1.0f, 0.0f,  5.0f,  0.0f,
		-25.0f, 0.0f, -25.0f,  0.0f, 1.0f, 0.0f,   0.0f, 5.0f,
		 25.0f, 0.0f, -25.0f,  0.0f, 1.0f, 0.0f,  5.0f, 5.0f
	};

	glGenVertexArrays(1, &snowVAO);
	glGenBuffers(1, &snowVBO);
	glBindVertexArray(snowVAO);

	glBindBuffer(GL_ARRAY_BUFFER, snowVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(plane), plane, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

	glBindVertexArray(0);

	glGenTextures(1, &snowTexture);

	glBindTexture(GL_TEXTURE_2D, snowTexture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height;
	unsigned char* image = SOIL_load_image("snow.png", &width, &height, 0, SOIL_LOAD_RGB);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);

	glBindTexture(GL_TEXTURE_2D, 0);
}

void Init()
{
	InitShader();
	glEnable(GL_DEPTH_TEST);
}

void Draw(sf::Clock clock, vector<Model> models, GLint shader, int count)
{
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 model = glm::mat4(1.0f);

	glUseProgram(shader); 

	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

	projection = glm::perspective(glm::radians(60.0f), 900.0f / 900.0f, 0.1f, 1000.0f);

	glUniform3f(glGetUniformLocation(shader, "airshipPos"), airshipPos.x, airshipPos.y, airshipPos.z);
	glUniform3f(glGetUniformLocation(shader, "light.position"), lightPos.x, lightPos.y, lightPos.z);

	glUniform3f(glGetUniformLocation(shader, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	glUniform3f(glGetUniformLocation(shader, "light.ambient"), 0.2f, 0.2f, 0.2f);
	glUniform3f(glGetUniformLocation(shader, "light.diffuse"), 0.9f, 0.9f, 0.9f);
	glUniform3f(glGetUniformLocation(shader, "light.specular"), 1.0f, 1.0f, 1.0f);
	glUniform1i(glGetUniformLocation(shader, "turnLight"), turnLight);
	glUniform1i(glGetUniformLocation(shader, "shininess"), 16);

	glUniform3f(glGetUniformLocation(shader, "light.direction"), lightDirection.x, lightDirection.y, lightDirection.z);
	glUniform3f(glGetUniformLocation(shader, "light.directionSpot"), 0.0f, -28.0f, -20.0f);

	glUniform1f(glGetUniformLocation(shader, "light.constant"), 1.0f);
	glUniform1f(glGetUniformLocation(shader, "light.linear"), 0.045f);
	glUniform1f(glGetUniformLocation(shader, "light.quadratic"), 0.0075f);

	glUniform1f(glGetUniformLocation(shader, "light.cutOff"), glm::cos(glm::radians(conus)));
	glUniform1f(glGetUniformLocation(shader, "light.outerCutOff"), glm::cos(glm::radians(conus * 1.4f)));

	glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	glUniform1i(glGetUniformLocation(shader, "shininess"), 2);

	model = glm::scale(model, glm::vec3(1.5f, 1.0f, 1.5f));

	glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, snowTexture);
	glUniform1i(glGetUniformLocation(shader, "texture_diffuse1"), 0);
	glBindVertexArray(snowVAO);

	glDrawArrays(GL_TRIANGLES, 0, 36);

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 30.0f, -35.0f));
	model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));

	glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(airshipPos.x, airshipPos.y, airshipPos.z));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(0.015f, 0.015f, 0.015f));
	model = glm::translate(model, glm::vec3(0.0f, 15.0f, 0.0f));

	glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));

	models[0].Draw(shader, count);

	model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(10.0f, 10.0f, 10.0f));

	glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));

	models[1].Draw(shader, count);

	for (int i = 0; i < positionCloudsAndBalloons.size() - 5; i++)
	{
		model = glm::mat4(1.0f);
		model = glm::translate(model, positionCloudsAndBalloons[i]);
		model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));

		glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
		models[2].Draw(shader, count);
	}

	for (int i = 5; i < positionCloudsAndBalloons.size(); i++)
	{
		model = glm::mat4(1.0f);
		model = glm::translate(model, positionCloudsAndBalloons[i]);
		model = glm::scale(model, glm::vec3(0.6f, 0.6f, 0.6f));

		glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
		models[3].Draw(shader, count);
	}

	glUseProgram(0);

	checkOpenGLerror();
}

void CloudsAndBalloons() 
{
	std::srand(static_cast<unsigned int>(std::time(0)));

	for (int i = 0; i < numCloudsAndBalloons; ++i) 
	{
		float x = static_cast<float>(rand() % 61 - 30);
		float z = static_cast<float>(rand() % 61 - 30);
		float y = 30.0f;
		positionCloudsAndBalloons.push_back(glm::vec3(x, y, z));
	}
}

void ReleaseVBO()
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ReleaseShader()
{
	glUseProgram(0);
	glDeleteProgram(ProgramFong);
}

void Release()
{
	ReleaseShader();
	ReleaseVBO();
}

int main()
{
	std::setlocale(LC_ALL, "Russian");

	sf::Window window(sf::VideoMode(900, 900), "Postal Airship", sf::Style::Default, sf::ContextSettings(24));
	window.setVerticalSyncEnabled(true);
	window.setActive(true);
	
	glewInit();
	glGetError(); 

	sf::Clock clock;
	vector<Model> models;

	Model airship("airship/airship.obj");
	models.push_back(airship);

	Model tree("tree/tree.obj");
	models.push_back(tree);

	Model cloud("cloud/cloud.obj");
	models.push_back(cloud);

	Model balloon("balloon/balloon.obj");
	models.push_back(balloon);

	Init();
	Plane();
	CloudsAndBalloons();

	while (window.isOpen())
	{
		sf::Event event;

		while (window.pollEvent(event))
		{
			float speed = 1.0f;
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.code == sf::Keyboard::Escape)
				{
					window.close();
					break;
				}
				if (event.key.code == sf::Keyboard::W)
				{
					cameraPos -= speed * glm::vec3(0.0f, 0.0f, 1.0f); 
																			
					airshipPos -= glm::vec3(0.0f, 0.0f, speed);
				}
				if (event.key.code == sf::Keyboard::S)
				{
					cameraPos += speed * glm::vec3(0.0f, 0.0f, 1.0f);
					airshipPos += glm::vec3(0.0f, 0.0f, speed);
				}
				if (event.key.code == sf::Keyboard::A)
				{
					cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
					airshipPos -= glm::vec3(speed, 0.0f, 0.0f);
				}
				if (event.key.code == sf::Keyboard::D)
				{
					cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
					airshipPos += glm::vec3(speed, 0.0f, 0.0f);
				}
				if (event.key.code == sf::Keyboard::Q)
				{
					cameraPos += speed * cameraUp;
					airshipPos += glm::vec3(0.0f, speed, 0.0f);
				}
				if (event.key.code == sf::Keyboard::E)
				{
					cameraPos -= speed * cameraUp;
					airshipPos -= glm::vec3(0.0f, speed, 0.0f);
				}
				if (event.key.code == sf::Keyboard::F)
				{
					turnLight = !turnLight;
				}
			}
			else if (event.type == sf::Event::Resized)
				glViewport(0, 0, event.size.width, event.size.height);
		}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		Draw(clock, models, ProgramFong, 1);

		window.display();
	}

	Release();
	return 0;
}