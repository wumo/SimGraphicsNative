#pragma once
#include "sim/graphics/renderer/default/model/camera.h"
#include "sim/graphics/renderer/default/model/light.h"
#include "syntactic_sugar.h"
#include "sim/graphics/util/materials.h"
#include "sim/graphics/compiledShaders/intersection/sphere_rint.h"
#include "sim/graphics/compiledShaders/intersection/infinite_plane_rint.h"

using namespace sim;
using namespace sim::graphics;
using namespace sim::graphics::renderer::defaultRenderer;
using namespace material;
auto buildScene(Camera &camera, Light &light, ModelManager &mm) {
  camera.location = {0, 20, -100};
  camera.focus = {0, 0, 0};
  light.location = {100, 100, 100, 0};
  //  lights[1].location = {0, 10, 10, 0};
  auto whiteTex = mm.newTex(White);
  auto redTex = mm.newTex(Red);
  auto greenTex = mm.newTex(Green);
  auto pbr2Tex = mm.newTex(PBR{0.1f, 0.7f});
  auto pbrTex = mm.newTex(PBR{0.1f, 0.7f});
  auto iceTex = mm.newTex(Refractive{1.31f});
  auto diamondTex = mm.newTex(Refractive{2.417f, 0.5f});
  Material mat{whiteTex, pbrTex};
  mat.refractive = iceTex;
  mat.flag = MaterialFlag ::eRefractive;
  auto refractiveMat = mm.newMaterial(mat);
  mat.color = greenTex;
  auto refractiveGreenMat = mm.newMaterial(mat);
  mat.color = whiteTex;
  mat.flag = MaterialFlag ::eReflective;
  auto reflectMat = mm.newMaterial(mat);

  auto testModel = mm.newModel([&](ModelBuilder &builder) {
    auto mesh = mm.newMeshes([](MeshBuilder &builder) {
      builder.cone({10, 0, 0}, {0, 2, 0}, 2);
      builder.newMesh();
      builder.cylinder({14, 0, 0}, {0, 2, 0}, 2);
      builder.newMesh();
      builder.sphere({18, 0, 0}, 2);
    });
    auto root = builder.addNode();
    builder.addMeshPart(root, mesh[0], mm.newMaterial(Red));
    builder.addMeshPart(root, mesh[1], mm.newMaterial(Green));
    builder.addMeshPart(root, mesh[2], mm.newMaterial(Blue));
  });
  auto testInstacne = mm.newModelInstance(testModel);

  auto axisModel = mm.newModel([&](ModelBuilder &builder) {
    auto mesh = mm.newMeshes([](MeshBuilder &builder) {
      builder.line({0, 0, 0}, {10, 0, 0});
      builder.newMesh();
      builder.line({0, 0, 0}, {0, 10, 0});
      builder.newMesh();
      builder.line({0, 0, 0}, {0, 0, 10});
    });
    auto root = builder.addNode();
    builder.addMeshPart(root, mesh[0], mm.newMaterial(Red));
    builder.addMeshPart(root, mesh[1], mm.newMaterial(Green));
    builder.addMeshPart(root, mesh[2], mm.newMaterial(Blue));
  });
  mm.newModelInstance(axisModel);

  auto sphereModel = mm.newModel([&](ModelBuilder &builder) {
    auto mesh = mm.newProceduralMesh(
      sphere_rint, __ArraySize__(sphere_rint), {AABB{{-1, -1, -1}, {1, 1, 1}}});
    builder.addMeshPart(mesh, refractiveMat);
  });
  auto sphere = mm.newModelInstance(sphereModel);
  auto ball2 = mm.newModelInstance(sphereModel, Transform{{-5, 1.0, 5}});
  ball2->node(0).meshParts[0].material = refractiveGreenMat;
  mm.newModelInstance(sphereModel, Transform{{-0, 1.0, 5}});
  mm.newModelInstance(sphereModel, Transform{{5, 1.0, 5}});

  auto sunModel = mm.newModel([&](ModelBuilder &builder) {
    auto mesh = mm.newMeshes([](MeshBuilder &builder) { builder.sphere({0, 0, 0}, 1); });
    builder.addMeshPart(
      mesh[0], mm.newMaterial(White, PBR{0.3f, 0.4f}, MaterialFlag::eNone));
  });
  mm.newModelInstance(sunModel, Transform{glm::vec3(light.location)});

  auto Box2Model = mm.newModel([&](ModelBuilder &builder) {
    auto mesh = mm.newMeshes([](MeshBuilder &builder) {
      builder.box({0, 0, 0}, {1, 0, 0}, {0, 0, 1}, 2);
    });
    builder.addMeshPart(
      mesh[0], mm.newMaterial(White, PBR{0.3f, 0.4f}, MaterialFlag::eNone));
  });
  auto b0 = mm.newModelInstance(Box2Model, Transform{{20, 10, 0}});

  //transparent color
  auto blueTrans = mm.newTex(glm::vec4{0, 0, 1, 0.5});
  auto wallModel = mm.newModel([&](ModelBuilder &builder) {
    auto mesh = mm.newMeshes([](MeshBuilder &builder) {
      builder.rectangle({-50, 0, 0}, {20, 0, 0}, {0, 20, 0});
    });
    builder.addMeshPart(
      mesh[0], mm.newMaterial(blueTrans, pbrTex, MaterialFlag::eTranslucent));
  });
  mm.newModelInstance(wallModel);

  auto gridTex = mm.newTex("assets/private/models/grid.png", {}, true, -1);
  auto planeModel = mm.newModel([&](ModelBuilder &builder) {
    auto mesh = mm.newProceduralMesh(
      infinite_plane_rint, __ArraySize__(infinite_plane_rint),
      {AABB{{-1e30f, -0.1f, -1e30f}, {1e30f, 0.1f, 1e30f}}});
    builder.addMeshPart(
      mesh, mm.newMaterial(gridTex, pbr2Tex, MaterialFlag ::eReflective));
  });
  mm.newModelInstance(planeModel);

  auto boxModel = mm.newModel([&](ModelBuilder &builder) {
    auto mesh = mm.newMeshes([](MeshBuilder &builder) {
      builder.box({0, 0, 0}, {1, 0, 0}, {0, 0, 1}, 2);
    })[0];
    mat.color = greenTex;
    mat.refractive = diamondTex;
    mat.flag = MaterialFlag ::eRefractive;
    builder.addMeshPart(mesh, mm.newMaterial(mat));
  });
  mm.newModelInstance(boxModel, Transform{{2, 2.1, -2}});

  auto mirrorModel = mm.newModel([&](ModelBuilder &builder) {
    auto mesh = mm.newMeshes([](MeshBuilder &builder) {
      builder.rectangle({0, 0, 0}, {0, 0, 5}, {0, 2, 0});
    })[0];
    ;
    builder.addMeshPart(mesh, reflectMat);
  });
  mm.newModelInstance(mirrorModel, Transform{{5, 2, 0}});
  mm.newModelInstance(mirrorModel, Transform{{-5, 2, 0}});

  auto b = mm.newModelInstance(
    sphereModel, Transform{{2.5, 1.0, 6},
                           {1, 2, 2},
                           glm::angleAxis(glm::radians(30.f), glm::vec3{0.f, 0.f, 1.f})});
  b->node(0).meshParts[0].material = mm.newMaterial(Red);
  b = mm.newModelInstance(sphereModel.get(), Transform{{-2.5, 1.0, 6}});
  b->node(0).meshParts[0].material = reflectMat;
  auto m = mm.loadGLTF("assets/private/models/jets2.glb", {{0.1, 0.1, 0.1}});
  auto tp = mm.newTransform(
    Transform{{-10, 3, 0},
              {0.5, 0.5, 0.5},
              glm::angleAxis(glm::radians(-60.f), glm::vec3{0.f, 0.f, 1.f})});
  auto jet = mm.newModelInstance(m, tp);
  auto num = 10;
  for(int x = 0; x < num; ++x)
    for(int y = 0; y < num; ++y) {
      auto t = mm.newModelInstance(
        m, mm.newTransform(
             Transform{{(x + 2) * -10, 3, y * 10},
                       {0.5, 0.5, 0.5},
                       glm::angleAxis(glm::radians(-60.f), glm::vec3{0.f, 0.f, 1.f})}));
      for(auto i = 0u; i < t->numNodes(); ++i)
        for(auto &meshPart: t->node(i).meshParts) {
          meshPart.material->color = redTex;
          meshPart.material->flag = MaterialFlag ::eReflective;
        }
    }
  auto ballModel = mm.newModel([&](ModelBuilder &builder) {
    auto mesh = mm.newMeshes([](MeshBuilder &builder) { builder.sphere({0, 0, 0}, 2); });
    auto root = builder.addNode();
    builder.addMeshPart(root, mesh[0], reflectMat);
  });
  num = 100;
  for(int x = 0; x < num; ++x)
    for(int y = 0; y < num; ++y) {
      auto t = mm.newModelInstance(ballModel, Transform{{(x + 2) * 10, 3, y * 10}});
      t->node(0).meshParts[0].material = mm.newMaterial(greenTex, pbrTex);
    }

  for(auto i = 0u; i < jet->numNodes(); ++i)
    for(auto &meshPart: jet->node(i).meshParts) {
      meshPart.material->color = redTex;
      meshPart.material->flag = MaterialFlag ::eReflective;
    }

  //  auto m2 = mm.loadObj("assets/private/models/fake_whitted/fake_whitted.obj");
  auto m2 = mm.loadGLTF("assets/private/models/whitted.glb");
  auto w = mm.newModelInstance(m2);
  for(auto &meshPart: w->node(0).meshParts) {
    meshPart.material->pbr = pbrTex;
    meshPart.material->refractive = iceTex;
    meshPart.material->flag = MaterialFlag ::eRefractive;
  }

  for(auto &meshPart: w->node(1).meshParts) {
    meshPart.material->color = redTex;
    meshPart.material->pbr = pbrTex;
    meshPart.material->refractive = iceTex;
    meshPart.material->flag = MaterialFlag ::eRefractive;
  }
  auto mmPtr = &mm;
  return [=](float elapsedDuration) {
    auto angle = glm::radians(elapsedDuration * 20);
    auto quat = glm::angleAxis(angle, glm::vec3{0.f, 1.f, 0.f});
    auto j = jet;
    auto &t = j->transform().get();
    auto &ts = t.translation;
    ts = quat * ts;
    t.rotation = quat * t.rotation;
    //    j->node(0).transform->translation.x+=1;
    sphere.get().transform()->translation.x += elapsedDuration;
    testInstacne.get().transform()->rotation =
      quat * testInstacne.get().transform()->rotation;

    mmPtr->transformChange(j->transform().index());
    mmPtr->transformChange(sphere.get().transform().index());
    mmPtr->transformChange(testInstacne.get().transform().index());
  };
}