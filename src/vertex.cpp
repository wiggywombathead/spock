#include <vertex.h>

// TODO: make position a vec3
Vertex::Vertex(glm::vec2 position, glm::vec3 color, glm::vec2 texCoord) {
	this->position = position;
	this->color = color;
	this->texCoord = texCoord;
}

