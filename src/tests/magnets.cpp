//
// Created by yilong on 2024/6/2.
//

#include "scene.h"
#include "imgui/imgui.h"

// This shows how to use sensor shapes. Sensors don't have collision, but report overlap events.
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

        ImGui::SliderFloat("mu", &m_nu, 0.1f, 200000.0f, "%.0f");

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
        ImGui::End();
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
};

static int testIndex = RegisterTest("Magnets", "Magnets", Magnets::Create);
