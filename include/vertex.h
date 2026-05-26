#ifndef VERTEX_H
#define VERTEX_H

#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Vertex_c
{
public:
    struct Position
    {
        float x, y, z;
    };

    struct Color
    {
        float r, g, b;
    };

    struct Texture
    {
        float s, t, r;
    };

    struct Normal
    {
        float x, y, z;
    };

    Position position;
    Color color;
    Texture texture;
    Normal normal;

    Position SavePosition(float x, float y, float z){
        this->position.x = x;
        this->position.y = y;
        this->position.z = z;
        return this->position;
    }

    Position SavePosition(glm::vec3 xyz){
        this->position.x = xyz.x;
        this->position.y = xyz.y;
        this->position.z = xyz.z;
        return this->position;
    }

    Normal SaveNormal(float x, float y, float z){
        this->normal.x = x;
        this->normal.y = y;
        this->normal.z = z;
        return this->normal;
    }

    Normal SaveNormal(glm::vec3 xyz){
        this->normal.x = xyz.x;
        this->normal.y = xyz.y;
        this->normal.z = xyz.z;
        return this->normal;
    }

    void CreateVertices();

    void DestroyVertices();
};

extern Vertex_c vertex;
extern std::vector<std::vector<Vertex_c>> vertices;

#endif