//
// Created by yilong on 2024/6/2.
//

#include <iostream>
#include <vector>

#include "scene.h"
#include "imgui/imgui.h"

// This shows how to use sensor shapes. Sensors don't have collision, but report overlap events.
static void sPrintLog(GLuint object)
{
    GLint log_length = 0;
    if (glIsShader(object))
        glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
    else if (glIsProgram(object))
        glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);
    else
    {
        fprintf(stderr, "printlog: Not a shader or a program\n");
        return;
    }

    char* log = (char*)malloc(log_length);

    if (glIsShader(object))
        glGetShaderInfoLog(object, log_length, NULL, log);
    else if (glIsProgram(object))
        glGetProgramInfoLog(object, log_length, NULL, log);

    fprintf(stderr, "%s", log);
    free(log);
}


static GLuint sCreateShaderFromString(const char* source, GLenum type)
{
    GLuint res = glCreateShader(type);
    const char* sources[] = { source };
    glShaderSource(res, 1, sources, NULL);
    glCompileShader(res);
    GLint compile_ok = GL_FALSE;
    glGetShaderiv(res, GL_COMPILE_STATUS, &compile_ok);
    if (compile_ok == GL_FALSE)
    {
        fprintf(stderr, "Error compiling shader of type %d!\n", type);
        sPrintLog(res);
        glDeleteShader(res);
        return 0;
    }

    return res;
}
static GLuint sCreateShaderProgram(const char* vs, const char* fs)
{
    GLuint vsId = sCreateShaderFromString(vs, GL_VERTEX_SHADER);
    GLuint fsId = sCreateShaderFromString(fs, GL_FRAGMENT_SHADER);
    assert(vsId != 0 && fsId != 0);

    GLuint programId = glCreateProgram();
    glAttachShader(programId, vsId);
    glAttachShader(programId, fsId);
    glBindFragDataLocation(programId, 0, "color");
    glLinkProgram(programId);

    glDeleteShader(vsId);
    glDeleteShader(fsId);

    GLint status = GL_FALSE;
    glGetProgramiv(programId, GL_LINK_STATUS, &status);
    assert(status != GL_FALSE);

    return programId;
}
class Magnets : public Test
{
public:

    enum
    {
        e_count = 50
    };

    Magnets()
    {
        {
            b2BodyDef bd;
            b2Body* ground = m_world->CreateBody(&bd);
            m_world->SetGravity(b2Vec2(0.0f, -100.0f));

            {
                b2EdgeShape shape;
                shape.SetTwoSided(b2Vec2(-80.0f, 0.0f), b2Vec2(80.0f, 0.0f));
                ground->CreateFixture(&shape, 1.0f);
                shape.SetTwoSided(b2Vec2(-80.0f, 80.0f), b2Vec2(80.0f, 80.0f));
                ground->CreateFixture(&shape, 1.0f);
                shape.SetTwoSided(b2Vec2(-80.0f, 0.0f), b2Vec2(-80, 80.0f));
                ground->CreateFixture(&shape, 1.0f);
                shape.SetTwoSided(b2Vec2(80.0f, 0.0f), b2Vec2(80.0f, 80.0f));
                ground->CreateFixture(&shape, 1.0f);


            }


        }

        {
            b2CircleShape shape;
            shape.m_radius = 1.0f;
            for (int32 i = 0; i < e_count; ++i)
            {
                b2BodyDef bd;
                bd.type = b2_dynamicBody;
                float x = RandomFloat(-50, 50);
                float y = RandomFloat(0, 80);
                bd.position.Set(x, y);
                bd.userData.pointer = i;
                bd.angularDamping = 10.0f;


                m_bodies[i] = m_world->CreateBody(&bd);

                m_bodies[i]->CreateFixture(&shape, 1.0f);
            }
        }

        m_force = 100.0f;

        //gl stuff
        glGenFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

        glGenTextures(1, &m_tex);
        glBindTexture(GL_TEXTURE_2D, m_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_camera.m_width, g_camera.m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);


        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_tex, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "fuck!";
            exit(1);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        {
            const char* vertexShaderSource = R"(
			#version 330 core
			layout(location = 0) in vec2 position;
			out vec2 fragCoord;
			void main()
			{
			    fragCoord = position;
			    gl_Position = vec4(position, 0.0, 1.0);
			}
			)";

            const char* fragmentShaderSource = R"(
			#version 330 core
			in vec2 fragCoord;
			out vec4 color;
			uniform mat4 projectionMatrix;
			uniform mat4 inversProjectionMatrix;
			uniform vec2 positions[100];
			uniform vec2 moments[100];
			uniform int numPos;
			const float k = 8.0;

			void main()
			{
			    vec2 E = vec2(0.0, 0.0);
			    for(int i = 0; i < numPos; ++i)
			    {
			        vec2 r = (inversProjectionMatrix * vec4(fragCoord, 0.0, 1.0)).xy - positions[i];
			        float distance = length(r);
			        if(distance > 0.01)
			        {
			            vec2 E_i = k * 1.0 * r / (distance * distance * distance);
			            E += E_i;
			        }
			    }

			    float magnitude = length(E);
			    vec4 fieldColor = vec4(0.5 + 0.5 * E / magnitude, 1.0 - magnitude / 1e5, 0.1);
			    color = fieldColor;
			}
			)";

            shaderProgram = sCreateShaderProgram(vertexShaderSource, fragmentShaderSource);
        	glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);

            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
        }
    }

    ~Magnets()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shaderProgram);
    }

    // Implement contact listener.
    void BeginContact(b2Contact* contact) override
    {
    }

    // Implement contact listener.
    void EndContact(b2Contact* contact) override
    {
    }

    void UpdateUI() override
    {
        ImGui::SetNextWindowPos(ImVec2(10.0f, 100.0f));
        ImGui::SetNextWindowSize(ImVec2(200.0f, 60.0f));
        ImGui::Begin("Sensor Controls", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui::SliderFloat("mu", &m_nu, 0.1f, 200000.0f, "%.0f");

        //glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        //glBindTexture(GL_TEXTURE_2D, m_tex);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    	for (int i = -40; i < 40; i++)
        {
            for (int j = 0; j < 40; j++)
            {
                b2Vec2 PosK = b2Vec2(float(i), float(j));
                b2Vec2 B = b2Vec2(0.0f, 0.0f);
                for (int32 ii = 0; ii < e_count; ++ii)
                {
                    b2Body* body = m_bodies[ii];
                    b2Vec2 PosI = body->GetPosition();
                    b2Vec2 MomentI =  b2Mul(body->GetTransform().q, b2Vec2(1.0f, 0.0f));
                    B += ComputeMagetField(PosK, PosI, MomentI);
                }
                B = m_nu/4.0f * 3.14159f * B;
                b2Vec2 NormB = (1.0f / B.Length()) * B;
                float ScaleB = B.Length();
                g_debugDraw.DrawSegment(PosK, PosK + 1.0f * NormB, b2Color(NormB.x,NormB.y,0,0));
            }
        }
        positionsFlat.clear();
        momentFlat.clear();
        for (int32 ii = 0; ii < e_count; ++ii)
        {
            b2Body* body = m_bodies[ii];
            b2Vec2 PosI = body->GetPosition();
            b2Vec2 MomentI = b2Mul(body->GetTransform().q, b2Vec2(1.0f, 0.0f));
            positionsFlat.push_back(PosI.x);
            positionsFlat.push_back(PosI.y);
            momentFlat.push_back(MomentI.x);
            momentFlat.push_back(MomentI.y);
        }
        g_debugDraw.Flush();
        //glBindFramebuffer(GL_FRAMEBUFFER, 0);

        ImVec2 pos = ImGui::GetCursorScreenPos();
        const float window_width = ImGui::GetContentRegionAvail().x;
        const float window_height = ImGui::GetContentRegionAvail().y;


        ImGui::End();

        //glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);
        glUseProgram(shaderProgram);

        GLint positionsLoc = glGetUniformLocation(shaderProgram, "positions");
        GLint momentsLoc = glGetUniformLocation(shaderProgram, "moments");
        GLint numPosLoc = glGetUniformLocation(shaderProgram, "numPos");
        GLint projectionLoc = glGetUniformLocation(shaderProgram, "projectionMatrix");
        GLint invprojectionLoc = glGetUniformLocation(shaderProgram, "inversProjectionMatrix");

        float proj[16] = { 0.0f };
        float invproj[16] = { 0.0f };
        g_camera.BuildProjectionMatrix(proj, 0.0f);
        g_camera.BuildInverseProjectionMatrix(invproj, 0.0f);
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, proj);
        glUniformMatrix4fv(invprojectionLoc, 1, GL_FALSE, invproj);


        glUniform2fv(positionsLoc, e_count, positionsFlat.data());
        glUniform2fv(momentsLoc, e_count, momentFlat.data());

        glUniform1i(numPosLoc, e_count);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        GLenum errCode = glGetError();
        if (errCode != GL_NO_ERROR)
        {
            fprintf(stderr, "OpenGL error = %d\n", errCode);
            assert(false);
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glUseProgram(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //ImGui::GetWindowDrawList()->AddImage(
        //    reinterpret_cast<void*>(m_tex),
        //    ImVec2(pos.x, pos.y),
        //    ImVec2(pos.x + window_width, pos.y + window_height),
        //    ImVec2(0, 1),
        //    ImVec2(1, 0)
        //);
    }

    void Step(Settings& settings) override
    {
        Test::Step(settings);

        // Traverse the contact results. Apply a force on shapes
        // that overlap the sensor.
        for (int32 i = 0; i < e_count; ++i)
        {
            b2Body* body = m_bodies[i];
            b2Vec2 PosK = body->GetPosition();
            b2Vec2 MomentK =  b2Mul(body->GetTransform().q, b2Vec2(1.0f, 0.0f));
            b2Vec2 magnetForce = b2Vec2(0.0f, 0.0f);
            float magnetTorque = 0.0f;

            for (int32 j = 0; j < e_count; ++j)
            {
                if (i == j)
                    continue;
                b2Body* otherBody = m_bodies[j];
                b2Vec2 PosI = otherBody->GetPosition();
                b2Vec2 MomentI =  b2Mul(otherBody->GetTransform().q, b2Vec2(1.0f, 0.0f));
                magnetForce += ComputeMagetForce(PosK, PosI, MomentK, MomentI);
                magnetTorque += ComputeMagetTorque(PosK, PosI, MomentK, MomentI);
            }

            magnetForce = m_nu/4.0f * 3.14159f * magnetForce;
            magnetTorque *= m_nu/4.0f * 3.14159f;
            body->ApplyForceToCenter(magnetForce, true);
            body->ApplyTorque(magnetTorque, true);
        }
    }

    b2Vec2 ComputeMagetForce(b2Vec2 PosK, b2Vec2 PosI, b2Vec2 MomentK, b2Vec2 MomentI)
    {
        b2Vec2 dir = PosK - PosI;
        b2Vec2 NIK = 1.0f/dir.Length() * dir;
        b2Vec2 MagnetForce =
                1.0f / (dir.LengthSquared() * dir.LengthSquared()) *
                ((-15.0f * ((b2Dot(MomentK, NIK) * b2Dot(MomentI, NIK))) * NIK) +
                3.0f * b2Dot(MomentK, MomentI) * NIK +
                3.0f * (b2Dot(MomentI, NIK) * MomentK + b2Dot(MomentK, NIK) * MomentI)) ;
        return MagnetForce;
    }

    float ComputeMagetTorque(b2Vec2 PosK, b2Vec2 PosI, b2Vec2 MomentK, b2Vec2 MomentI)
    {
        b2Vec2 dir = PosK - PosI;
        b2Vec2 NIK = 1.0f/dir.Length() * dir;
        float MagnetTorque =
        (3 * b2Cross(MomentK, NIK) * b2Dot(MomentI, NIK) -
        b2Cross(MomentK, MomentI)) /
        (dir.Length() * dir.Length() * dir.Length());
        return MagnetTorque;
    }

    b2Vec2 ComputeMagetField(b2Vec2 PosK, b2Vec2 PosI, b2Vec2 MomentI)
    {
        b2Vec2 dir = PosK - PosI;
        b2Vec2 NIK = 1.0f/dir.Length() * dir;
        b2Vec2 B =
        1.0f / (dir.Length() * dir.Length() * dir.Length()) *
        (3.0f * b2Dot(NIK, MomentI) * NIK - MomentI);

        return B;
    }

    static Test* Create()
    {
        return new Magnets;
    }

    b2Body* m_bodies[e_count];
    float m_force;
    float m_nu = 20000;

    std::vector<float> positionsFlat;
    std::vector<float> momentFlat;
    float vertices[8] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f
    };
    GLuint VAO, VBO;
    GLuint shaderProgram;
};

static int testIndex = RegisterTest("Magnets", "Magnets", Magnets::Create);
