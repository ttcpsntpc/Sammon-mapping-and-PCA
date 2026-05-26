#ifndef OBJECT_H
#define OBJECT_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include "Vertex.h"

class Object_c
{
public:
    unsigned int VAO_;
    unsigned int VBO_;
    unsigned int EBO_;
    unsigned int size;

    Object_c()
    {
    }

    ~Object_c()
    {
        glDeleteVertexArrays(1, &VAO_);
        glDeleteBuffers(1, &VBO_);
        if (EBO_ != 0)
        {
            glDeleteBuffers(1, &EBO_);
        }
    }

    void CreateObject(const std::vector<Vertex_c> &vertices, const std::vector<unsigned int> &indices)
    {
        glGenVertexArrays(1, &VAO_);
        glGenBuffers(1, &VBO_);

        glBindVertexArray(VAO_);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex_c), vertices.data(), GL_STATIC_DRAW);

        size = vertices.size();

        if (!indices.empty())
        {
            glGenBuffers(1, &EBO_);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        }

        // attributes
        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_c), 0);
        glEnableVertexAttribArray(0);

        // color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_c), (void *)sizeof(Vertex_c::Position));
        glEnableVertexAttribArray(1);

        // texture attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex_c), (void *)(sizeof(Vertex_c::Position) + sizeof(Vertex_c::Color)));
        glEnableVertexAttribArray(2);

        // normal attribute
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_c), (void *)(sizeof(Vertex_c::Position) + sizeof(Vertex_c::Color) + sizeof(Vertex_c::Texture)));
        glEnableVertexAttribArray(3);
    }

    void RenewObject(const std::vector<Vertex_c> &vertices)
    {
        glDeleteBuffers(1, &VBO_);
        glBindVertexArray(VAO_);
        glGenBuffers(1, &VBO_);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex_c), vertices.data(), GL_DYNAMIC_DRAW);

        size = vertices.size();
        
        // attributes
        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_c), 0);

        // color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_c), (void *)sizeof(Vertex_c::Position));

        // texture attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex_c), (void *)(sizeof(Vertex_c::Position) + sizeof(Vertex_c::Color)));

        // normal attribute
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_c), (void *)(sizeof(Vertex_c::Position) + sizeof(Vertex_c::Color) + sizeof(Vertex_c::Texture)));
    }
};

#endif