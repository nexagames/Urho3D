//
// Copyright (c) 2008-2014 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "Camera.h"
#include "CollisionBox2D.h"
#include "CollisionCircle2D.h"
#include "CoreEvents.h"
#include "DebugRenderer.h"
#include "Drawable2D.h"
#include "Engine.h"
#include "Font.h"
#include "Graphics.h"
#include "Input.h"
#include "Octree.h"
#include "PhysicsWorld2D.h"
#include "Renderer.h"
#include "ResourceCache.h"
#include "RigidBody2D.h"
#include "Scene.h"
#include "SceneEvents.h"
#include "Sprite2D.h"
#include "StaticSprite2D.h"
#include "Text.h"
#include "Urho2DPhysics.h"

#include "DebugNew.h"

DEFINE_APPLICATION_MAIN(Urho2DPhysics)

static const unsigned NUM_OBJECTS = 100;

Urho2DPhysics::Urho2DPhysics(Context* context) : Sample(context)
{    
}

void Urho2DPhysics::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update events
    SubscribeToEvents();
}

void Urho2DPhysics::CreateScene()
{
    scene_ = new Scene(context_);
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();
    // Create camera node
    cameraNode_ = scene_->CreateChild("Camera");
    // Set camera's position
    cameraNode_->SetPosition(Vector3(0.0f, 0.0f, -10.0f));
    
    Camera* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetOrthographic(true);

    Graphics* graphics = GetSubsystem<Graphics>();
    float width = (float)graphics->GetWidth();
    float height = (float)graphics->GetHeight();
    camera->SetOrthoSize(Vector2(width, height) * PIXEL_SIZE);

    // Create 2D physics world component
    PhysicsWorld2D* physicsWorld = scene_->CreateComponent<PhysicsWorld2D>();
    
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    Sprite2D* boxSprite = cache->GetResource<Sprite2D>("Urho2D/Box.png");
    Sprite2D* ballSprite = cache->GetResource<Sprite2D>("Urho2D/Ball.png");

    // Create ground.
    Node* groundNode = scene_->CreateChild("Ground");
    groundNode->SetPosition(Vector3(0.0f, -3.0f, 0.0f));
    groundNode->SetScale(Vector3(200.0f, 1.0f, 0.0f));
    
    // Create 2D rigid body for gound
    RigidBody2D* groundBody = groundNode->CreateComponent<RigidBody2D>();
    
    StaticSprite2D* groundSprite = groundNode->CreateComponent<StaticSprite2D>();
    groundSprite->SetSprite(boxSprite);

    // Create box collider for ground
    CollisionBox2D* groundShape = groundNode->CreateComponent<CollisionBox2D>();
    // Set box size
    groundShape->SetSize(Vector2(0.32f, 0.32f));
    // Set friction
    groundShape->SetFriction(0.5f);

    for (unsigned i = 0; i < NUM_OBJECTS; ++i)
    {
        Node* node  = scene_->CreateChild("RigidBody");
        node->SetPosition(Vector3(Random(-0.1f, 0.1f), 5.0f + i * 0.4f, 0.0f));

        // Create rigid body
        RigidBody2D* body = node->CreateComponent<RigidBody2D>();
        body->SetBodyType(BT_DYNAMIC);

        StaticSprite2D* staticSprite = node->CreateComponent<StaticSprite2D>();

        if (i % 2 == 0)
        {
            staticSprite->SetSprite(boxSprite);

            // Create box
            CollisionBox2D* box = node->CreateComponent<CollisionBox2D>();
            // Set size
            box->SetSize(Vector2(0.32f, 0.32f));
            // Set density
            box->SetDensity(1.0f);
            // Set friction
            box->SetFriction(0.5f);
            // Set restitution
            box->SetRestitution(0.1f);
        }
        else
        {
            staticSprite->SetSprite(ballSprite);

            // Create circle
            CollisionCircle2D* circle = node->CreateComponent<CollisionCircle2D>();
            // Set radius
            circle->SetRadius(0.16f);
            // Set density
            circle->SetDensity(1.0f);
            // Set friction.
            circle->SetFriction(0.5f);
            // Set restitution
            circle->SetRestitution(0.1f);
        }
    }
}

void Urho2DPhysics::CreateInstructions()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    UI* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    Text* instructionText = ui->GetRoot()->CreateChild<Text>();
    instructionText->SetText("Use WASD keys to move, use PageUp PageDown keys to zoom.");
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void Urho2DPhysics::SetupViewport()
{
    Renderer* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void Urho2DPhysics::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    Input* input = GetSubsystem<Input>();

    // Movement speed as world units per second
    const float MOVE_SPEED = 4.0f;

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    // Use the TranslateRelative() function to move relative to the node's orientation. Alternatively we could
    // multiply the desired direction with the node's orientation quaternion, and use just Translate()
    if (input->GetKeyDown('W'))
        cameraNode_->TranslateRelative(Vector3::UP * MOVE_SPEED * timeStep);
    if (input->GetKeyDown('S'))
        cameraNode_->TranslateRelative(Vector3::DOWN* MOVE_SPEED * timeStep);
    if (input->GetKeyDown('A'))
        cameraNode_->TranslateRelative(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown('D'))
        cameraNode_->TranslateRelative(Vector3::RIGHT * MOVE_SPEED * timeStep);
    
    if (input->GetKeyDown(KEY_PAGEUP))
    {
        Camera* camera = cameraNode_->GetComponent<Camera>();
        camera->SetZoom(camera->GetZoom() * 1.01f);
    }

    if (input->GetKeyDown(KEY_PAGEDOWN))
    {
        Camera* camera = cameraNode_->GetComponent<Camera>();
        camera->SetZoom(camera->GetZoom() * 0.99f);
    }
}

void Urho2DPhysics::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, HANDLER(Urho2DPhysics, HandleUpdate));
}

void Urho2DPhysics::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}
