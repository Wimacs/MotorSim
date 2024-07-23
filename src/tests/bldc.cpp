//
// Created by yilong on 2024/6/2.
//

#include <iostream>
#include <vector>

#include "settings.h"
#include "scene.h"
#include "imgui/imgui.h"
#include "implot/implot.h"

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
        ImPlot::CreateContext();
        b2Body* ground = NULL;
        {
            b2BodyDef bd;
            ground = m_world->CreateBody(&bd);
        }

        m_world->SetGravity(b2Vec2(0.0f, 0.0f));

        m_enableLimit = true;
        m_enableMotor = false;
        m_motorSpeed = 10.0f;

        {
            b2CircleShape shape;
            shape.m_radius = 5.0f;

            b2BodyDef bd;
            bd.type = b2_dynamicBody;
            bd.position.Set(0.0f, 20.0f);
            bd.allowSleep = false;
            bd.angularDamping = 45.0f;
            b2Body* body = m_world->CreateBody(&bd);
            body->CreateFixture(&shape, 0.005f);


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
            coilShape.SetAsBox(0.6f, 1.2f);


            const float pi = 3.14159265358979323846f;
            const float radius = 6.0f; // triC
            b2BodyDef bdcoil;
            std::vector<b2Body*> coilArray;
            bdcoil.type = b2_staticBody;
            float centerX = 0.0f;
            float centerY = 20.0f;


            for (int i = 0; i < 3; ++i) {
                float angle = i * (pi / 3); // 60 degrees in radians
                float x = centerX + radius * cos(angle);
                float y = centerY + radius * sin(angle);

                // Create the first box
                bdcoil.position.Set(x, y);
                bdcoil.angle = angle;
                m_world->CreateBody(&bdcoil);
                coilArray.push_back(m_world->CreateBody(&bdcoil));

                // Create the opposite box
                bdcoil.position.Set(centerX - (x - centerX), centerY - (y - centerY));
                bdcoil.angle = angle; // add 180 degrees to the angle for the opposite box
                m_world->CreateBody(&bdcoil);
                coilArray.push_back(m_world->CreateBody(&bdcoil));

            }

			b2Fixture* coilFixture0 = coilArray[0]->CreateFixture(&coilShape, 5.0f);
            b2Fixture* coilFixture1 = coilArray[1]->CreateFixture(&coilShape, 5.0f);
            b2Fixture* coilFixture2 = coilArray[2]->CreateFixture(&coilShape, 5.0f);
            b2Fixture* coilFixture3 = coilArray[3]->CreateFixture(&coilShape, 5.0f);
            b2Fixture* coilFixture4 = coilArray[4]->CreateFixture(&coilShape, 5.0f);
            b2Fixture* coilFixture5 = coilArray[5]->CreateFixture(&coilShape, 5.0f);

            m_rotor = body;
            stator statorPair;
            statorPair.coil[0] = coilArray[0];
            statorPair.coil[1] = coilArray[1];
            statorPair.coilFixture[0] = coilFixture0;
            statorPair.coilFixture[1] = coilFixture3;
            Stators.push_back(statorPair);
            statorPair.coil[0] = coilArray[2];
            statorPair.coil[1] = coilArray[3];
            statorPair.coilFixture[0] = coilFixture1;
            statorPair.coilFixture[1] = coilFixture4;
            Stators.push_back(statorPair);
            statorPair.coil[0] = coilArray[4];
            statorPair.coil[1] = coilArray[5];
            statorPair.coilFixture[0] = coilFixture2;
            statorPair.coilFixture[1] = coilFixture5;
            Stators.push_back(statorPair);

        }
        glInit();
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

    // utility structure for realtime plot
    struct ScrollingBuffer {
        int MaxSize;
        int Offset;
        ImVector<ImVec2> Data;
        ScrollingBuffer(int max_size = 2000) {
            MaxSize = max_size;
            Offset = 0;
            Data.reserve(MaxSize);
        }
        void AddPoint(float x, float y) {
            if (Data.size() < MaxSize)
                Data.push_back(ImVec2(x, y));
            else {
                Data[Offset] = ImVec2(x, y);
                Offset = (Offset + 1) % MaxSize;
            }
        }
        void Erase() {
            if (Data.size() > 0) {
                Data.shrink(0);
                Offset = 0;
            }
        }
    };

    // utility structure for realtime plot
    struct RollingBuffer {
        float Span;
        ImVector<ImVec2> Data;
        RollingBuffer() {
            Span = 10.0f;
            Data.reserve(2000);
        }
        void AddPoint(float x, float y) {
            float xmod = fmodf(x, Span);
            if (!Data.empty() && xmod < Data.back().x)
                Data.shrink(0);
            Data.push_back(ImVec2(xmod, y));
        }
    };

    void UpdateUI() override
    {
        t += ImGui::GetIO().DeltaTime;
        static float history = 10.0f;
        ImGui::SliderFloat("History", &history, 1, 30, "%.1f s");
        rdata1.Span = history;
        rdata2.Span = history;

        static ImPlotAxisFlags flags = ImPlotAxisFlags_NoTickLabels;

        if (ImPlot::BeginPlot("##Scrolling", ImVec2(-1, 150))) {
            ImPlot::SetupAxes(NULL, NULL, flags, flags);
            ImPlot::SetupAxisLimits(ImAxis_X1, t - history, t, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1);
            ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
            ImPlot::PlotShaded("Mouse X", &sdata1.Data[0].x, &sdata1.Data[0].y, sdata1.Data.size(), -INFINITY, 0, sdata1.Offset, 2 * sizeof(float));
            ImPlot::PlotLine("Mouse Y", &sdata2.Data[0].x, &sdata2.Data[0].y, sdata2.Data.size(), 0, sdata2.Offset, 2 * sizeof(float));
            ImPlot::EndPlot();
        }
        if (ImPlot::BeginPlot("##Rolling", ImVec2(-1, 150))) {
            ImPlot::SetupAxes(NULL, NULL, flags, flags);
            ImPlot::SetupAxisLimits(ImAxis_X1, 0, history, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1);
            ImPlot::PlotLine("Mouse X", &rdata1.Data[0].x, &rdata1.Data[0].y, rdata1.Data.size(), 0, 0, 2 * sizeof(float));
            ImPlot::PlotLine("Mouse Y", &rdata2.Data[0].x, &rdata2.Data[0].y, rdata2.Data.size(), 0, 0, 2 * sizeof(float));
            ImPlot::EndPlot();
        }        ImGui::SetNextWindowPos(ImVec2(10.0f, 100.0f));
        ImGui::SetNextWindowSize(ImVec2(200.0f, 100.0f));
        ImGui::Begin("Joint Controls", nullptr);
        ImGui::SliderInt("stator pair 0", &Stators[0].isConduction, 0, 2);
        ImGui::SliderInt("stator pair 1", &Stators[1].isConduction, 0, 2);
        ImGui::SliderInt("stator pair 2", &Stators[2].isConduction, 0, 2);


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
        sdata1.AddPoint(t, totalTorque * 0.0005f);
        rdata1.AddPoint(t, totalTorque * 0.0005f);
        sdata2.AddPoint(t, totalTorque * 0.0005f);
        rdata2.AddPoint(t, totalTorque * 0.0005f);

        if (m_motorState >= 70)
            m_motorState = 0;
        m_motorState += 1;

        for (auto& stator : Stators)
            stator.isConduction = false;


        if (m_motorState <= 10)
            Stators[0].isConduction = 1;
        else if (m_motorState > 10 && m_motorState <= 20)
            Stators[1].isConduction = 1;
        else if (m_motorState > 20 && m_motorState <= 30)
            Stators[2].isConduction = 1;
        else if (m_motorState > 40 && m_motorState <= 50)
            Stators[0].isConduction = 2;
        else if (m_motorState > 50 && m_motorState <= 60)
            Stators[1].isConduction = 2;
        else if (m_motorState > 60 && m_motorState <= 70)
            Stators[2].isConduction = 2;

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

    void glInit()
    {
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

    static Test* Create()
    {
        return new bldc;
    }

    b2Body* m_bodies[e_count];
    std::vector<stator> Stators;
    b2Body* m_rotor;
    float m_force;
    float m_nu = 1000000.0f;

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

    int m_motorState = 1;
	ScrollingBuffer sdata1, sdata2;
	RollingBuffer   rdata1, rdata2;
     float t = 0;
};

static int testIndex = RegisterTest("Magnets", "bldc", bldc::Create);
