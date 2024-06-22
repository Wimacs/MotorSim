//
// Created by yilong on 2024/6/2.
//

#include <iostream>
#include <vector>
#include "settings.h"
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

struct stator
{
    b2Body* coil[2];
    b2Fixture* coilFixture[2];
    int isConduction = 0;
};
class bldc : public Test
{
public:

    enum
    {
        e_count = 50
    };

    bldc()
    {
        b2Body* ground = NULL;
        {
            b2BodyDef bd;
            ground = m_world->CreateBody(&bd);

            b2EdgeShape shape;
            shape.SetTwoSided(b2Vec2(-40.0f, 0.0f), b2Vec2(40.0f, 0.0f));
            ground->CreateFixture(&shape, 0.0f);
        }

        m_world->SetGravity(b2Vec2(0.0f, 0.0f));

        m_enableLimit = true;
        m_enableMotor = false;
        m_motorSpeed = 10.0f;

        {
            b2CircleShape shape;
            shape.m_radius = 2.0f;

            b2BodyDef bd;
            bd.type = b2_dynamicBody;
            bd.position.Set(0.0f, 15.0f);
            bd.allowSleep = false;
            bd.angularDamping =100;
            b2Body* body = m_world->CreateBody(&bd);
            body->CreateFixture(&shape, 5.0f);


            b2WheelJointDef jd;

            // Horizontal
            jd.Initialize(ground, body, bd.position, b2Vec2(0.0f, 1.0f));

            jd.enableMotor = m_enableMotor;
            jd.lowerTranslation = -0.0f;
            jd.upperTranslation = 0.0f;
            jd.enableLimit = m_enableLimit;
            //b2LinearStiffness(jd.stiffness, jd.damping, hertz, dampingRatio, ground, body);

            m_joint = (b2WheelJoint*)m_world->CreateJoint(&jd);


            b2PolygonShape coilShape;
            coilShape.SetAsBox(0.5f, 0.5f);

            float triC = 2.82842712474619f;
            float triA = 2.0f;
            float triB = 2.0f;

        	b2BodyDef bdcoil;
            bdcoil.type = b2_staticBody;
            bdcoil.position.Set(0.0f, 15.0f + triC);
            bdcoil.angle = 0.5f * b2_pi;
			b2Body* coil0 = m_world->CreateBody(&bdcoil);
            bdcoil.position.Set(0.0f + triA, 15.0f + triB);
            bdcoil.angle = 0.25 * b2_pi;
            b2Body* coil1 = m_world->CreateBody(&bdcoil);
            bdcoil.position.Set(0.0f + triC, 15.0f);
            bdcoil.angle = 0.0f * b2_pi;
            b2Body* coil2 = m_world->CreateBody(&bdcoil);
            bdcoil.position.Set(0.0f - triA, 15.0f + triB);
            bdcoil.angle = 1.75f * b2_pi;
            b2Body* coil3 = m_world->CreateBody(&bdcoil);
            bdcoil.position.Set(0.0f - triC, 15.0f);
            bdcoil.angle = 0.0f * b2_pi;
            b2Body* coil4 = m_world->CreateBody(&bdcoil);
            bdcoil.position.Set(0.0f - triA, 15.0f - triB);
            bdcoil.angle = 0.25f * b2_pi;
            b2Body* coil5 = m_world->CreateBody(&bdcoil);
            bdcoil.position.Set(0.0f, 15.0f - triC);
            bdcoil.angle = 0.5f * b2_pi;
            b2Body* coil6 = m_world->CreateBody(&bdcoil);
            bdcoil.position.Set(0.0f + triA, 15.0f - triB);
            bdcoil.angle = 1.75f * b2_pi;
            b2Body* coil7 = m_world->CreateBody(&bdcoil);

			b2Fixture* coilFixture0 = coil0->CreateFixture(&coilShape, 5.0f);
            b2Fixture* coilFixture1 = coil1->CreateFixture(&coilShape, 5.0f);
            b2Fixture* coilFixture2 = coil2->CreateFixture(&coilShape, 5.0f);
            b2Fixture* coilFixture3 = coil3->CreateFixture(&coilShape, 5.0f);
            b2Fixture* coilFixture4 = coil4->CreateFixture(&coilShape, 5.0f);
            b2Fixture* coilFixture5 = coil5->CreateFixture(&coilShape, 5.0f);
            b2Fixture* coilFixture6 = coil6->CreateFixture(&coilShape, 5.0f);
            b2Fixture* coilFixture7 = coil7->CreateFixture(&coilShape, 5.0f);

            m_rotor = body;
            stator statorPair;
            statorPair.coil[0] = coil0;
            statorPair.coil[1] = coil6;
            statorPair.coilFixture[0] = coilFixture0;
            statorPair.coilFixture[1] = coilFixture6;
            Stators.push_back(statorPair);
            statorPair.coil[0] = coil1;
            statorPair.coil[1] = coil5;
            statorPair.coilFixture[0] = coilFixture1;
            statorPair.coilFixture[1] = coilFixture5;
            Stators.push_back(statorPair);
            statorPair.coil[0] = coil2;
            statorPair.coil[1] = coil4;
            statorPair.coilFixture[0] = coilFixture2;
            statorPair.coilFixture[1] = coilFixture4;
            Stators.push_back(statorPair);
            statorPair.coil[0] = coil3;
            statorPair.coil[1] = coil7;
            statorPair.coilFixture[0] = coilFixture3;
            statorPair.coilFixture[1] = coilFixture7;
            Stators.push_back(statorPair);
        }

        {

        }

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
			uniform float mu;
			uniform int numPos;
			const float k = 8.0;

			float dot2(vec2 v) { return dot(v, v); }

			// Calculate the signed distance from point p to triangle defined by points a, b, c
			float sdTriangle(vec2 p, vec2 a, vec2 b, vec2 c) {
			    vec2 ba = b - a; vec2 pa = p - a;
			    vec2 cb = c - b; vec2 pb = p - b;
			    vec2 ac = a - c; vec2 pc = p - c;
			    
			    vec2 nor = vec2(ba.y, -ba.x);
			    
			    // Determine whether point is inside the triangle using edge signs
			    float signPA = sign(dot(nor, pa));
			    nor = vec2(cb.y, -cb.x);
			    float signPB = sign(dot(nor, pb));
			    nor = vec2(ac.y, -ac.x);
			    float signPC = sign(dot(nor, pc));
			    
			    // If the point is inside the triangle, all signs will be positive
			    bool isInside = (signPA > 0.0) && (signPB > 0.0) && (signPC > 0.0);

			    // Compute distance to the edges
			    float d = min(min(
			        dot2(pa - ba * clamp(dot(pa, ba) / dot2(ba), 0.0, 1.0)),
			        dot2(pb - cb * clamp(dot(pb, cb) / dot2(cb), 0.0, 1.0))),
			        dot2(pc - ac * clamp(dot(pc, ac) / dot2(ac), 0.0, 1.0)));
			    
			    d = sqrt(d);
			    
			    // Return the distance with correct sign
			    return isInside ? -d : d;
			}

			void main()
			{
				float offset =1.5;
			    vec2 B = vec2(0.0, 0.0);
                vec2 pixPos = (inversProjectionMatrix * vec4(fragCoord, 0.0, 1.0)).xy;
			    for(int i = 0; i < numPos; ++i)
			    {
			        vec2 dir = pixPos - positions[i];
			        float distance = length(dir);
					vec2 NIK = (1.0/distance) * dir;
					if (distance > 0.001)
					{
			        B +=
			        1.0f / (distance * distance * distance) *
			        (3.0f * dot(NIK, moments[i]) * NIK - moments[i]);
					}
			    }
				B = B * mu;
				float magnitude = length(B);
				vec2 NormB = B / magnitude;
			    vec4 fieldColor = vec4(0.0, 0.0, magnitude * 1000, 0.2);
			    color = fieldColor;


				vec2 indicatorPixelLoc = pixPos - vec2(mod(pixPos.x, offset), mod(pixPos.y, offset)) + vec2(offset, offset) * 0.5;
				B = vec2(0.0, 0.0);
			    for(int i = 0; i < numPos; ++i)
			    {
			        vec2 dir = indicatorPixelLoc - positions[i];
			        float distance = length(dir);
					vec2 NIK = (1.0/distance) * dir;
					if (distance > 0.001)
					{
			        B +=
			        1.0f / (distance * distance * distance) *
			        (3.0f * dot(NIK, moments[i]) * NIK - moments[i]);
					}
			    }
				magnitude = length(B);
				NormB = B / magnitude;
                
                vec2 triDir = indicatorPixelLoc + NormB * 0.8;
                vec2 triB0  = indicatorPixelLoc + vec2(NormB.y, -NormB.x) * 0.1;
                vec2 triB1  = indicatorPixelLoc - vec2(NormB.y, -NormB.x)* 0.1;
                float indicatorArrowDistanceField = sdTriangle(pixPos, triB0, triB1,triDir);

                if (indicatorArrowDistanceField < 0)
                    color = vec4(vec3(1.) - fieldColor.xyz, 0.2);
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

    ~bldc()
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
        ImGui::SetNextWindowSize(ImVec2(200.0f, 100.0f));
        ImGui::Begin("Joint Controls", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);


        ImGui::SliderInt("stator pair 0", &Stators[0].isConduction, 0, 2);
        ImGui::SliderInt("stator pair 1", &Stators[1].isConduction, 0, 2);
        ImGui::SliderInt("stator pair 2", &Stators[2].isConduction, 0, 2);
        ImGui::SliderInt("stator pair 3", &Stators[3].isConduction, 0, 2);


        m_motorState += 1;

        for (auto& stator : Stators)
            stator.isConduction = false;


        if (m_motorState <= 10)
            Stators[0].isConduction = 1;
        else if (m_motorState > 10 && m_motorState <= 20)
            Stators[1].isConduction = 1;
        else if (m_motorState > 20 && m_motorState <= 30)
            Stators[2].isConduction = 1;
        else if (m_motorState > 30 && m_motorState <= 40)
            Stators[3].isConduction = 1;
        else if (m_motorState > 40 && m_motorState <= 50)
            Stators[0].isConduction = 2;
        else if (m_motorState > 50 && m_motorState <= 60)
            Stators[1].isConduction = 2;
        else if (m_motorState > 60 && m_motorState <= 70)
            Stators[2].isConduction = 2;
        else if (m_motorState > 70 && m_motorState <= 80)
            Stators[3].isConduction = 2;

        if (m_motorState > 80)
            m_motorState = 0;

        if (ImGui::Checkbox("Motor", &m_enableMotor))
        {
            m_joint->EnableMotor(m_enableMotor);
        }

        if (ImGui::SliderFloat("Speed", &m_motorSpeed, -100.0f, 100.0f, "%.0f"))
        {
            m_joint->SetMotorSpeed(m_motorSpeed);
        }
        ImGui::End();

        for (auto stator : Stators)
        {
	        if (stator.isConduction)
	        {
                b2PolygonShape* poly = (b2PolygonShape*)stator.coilFixture[0]->GetShape();
                int32 vertexCount = poly->m_count;
                b2Assert(vertexCount <= b2_maxPolygonVertices);
                b2Vec2 vertices[b2_maxPolygonVertices];

                for (int32 i = 0; i < vertexCount; ++i)
                {
                    vertices[i] = b2Mul(stator.coil[0]->GetTransform(), poly->m_vertices[i]);
                }

                g_debugDraw.DrawSolidPolygon(vertices, vertexCount, b2Color(1.0, 0.0, 1.0, 1.0));

                poly = (b2PolygonShape*)stator.coilFixture[1]->GetShape();
				vertexCount = poly->m_count;
                b2Assert(vertexCount <= b2_maxPolygonVertices);

                for (int32 i = 0; i < vertexCount; ++i)
                {
                    vertices[i] = b2Mul(stator.coil[1]->GetTransform(), poly->m_vertices[i]);
                }

                g_debugDraw.DrawSolidPolygon(vertices, vertexCount, b2Color(1.0, 0.0, 1.0, 1.0));
	        }
        }


    }

    void Step(Settings& settings) override
    {
        Test::Step(settings);

        b2Vec2 PosK = m_rotor->GetPosition();
        b2Vec2 MomentK = b2Mul(m_rotor->GetTransform().q, b2Vec2(1.0f, 0.0f));

        float totalTorque = 0;
        for (auto stator : Stators)
        {
            if (stator.isConduction == 1)
            {
                b2Vec2 PosI = stator.coil[0]->GetPosition();
                b2Vec2 MomentI = b2Mul(stator.coil[0]->GetTransform().q, b2Vec2(1.0f, 0.0f));
                totalTorque += ComputeMagetTorque(PosK, PosI, MomentK, MomentI);

            	PosI = stator.coil[1]->GetPosition();
                MomentI = b2Mul(stator.coil[1]->GetTransform().q, b2Vec2(1.0f, 0.0f));
                totalTorque += ComputeMagetTorque(PosK, PosI, MomentK, MomentI);
            }
            else if (stator.isConduction == 2)
            {
                b2Vec2 PosI = stator.coil[0]->GetPosition();
                b2Vec2 MomentI = b2Mul(stator.coil[0]->GetTransform().q, b2Vec2(-1.0f, 0.0f));
                totalTorque += ComputeMagetTorque(PosK, PosI, MomentK, MomentI);

                PosI = stator.coil[1]->GetPosition();
                MomentI = b2Mul(stator.coil[1]->GetTransform().q, b2Vec2(-1.0f, 0.0f));
                totalTorque += ComputeMagetTorque(PosK, PosI, MomentK, MomentI);
            }
        }
        totalTorque *= m_nu / 4.0f * 3.14159f;
        if (m_rotor)
			m_rotor->ApplyTorque(totalTorque, true);
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
        return new bldc;
    }

    b2Body* m_bodies[e_count];
    std::vector<stator> Stators;
    b2Body* m_rotor;
    float m_force;
    float m_nu = 2000000;

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

    b2WheelJoint* m_joint;
    float m_motorSpeed;
    bool m_enableMotor;
    bool m_enableLimit;

    int m_motorState = 0;

};

static int testIndex = RegisterTest("Magnets", "bldc", bldc::Create);
