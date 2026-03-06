#pragma once

#include "MaterialStandard.h"
#include "Application.h"
#include "ModuleResources.h"
#include "ResourceTexture.h"
#include "Shader.h"
#include "glad/glad.h"

MaterialStandard::MaterialStandard(MaterialType type) : Material(type)
{

}

MaterialStandard::~MaterialStandard()
{
    auto* resModule = Application::GetInstance().resources.get();
    if (albedoMapUID != 0) resModule->ReleaseResource(albedoMapUID);
    albedoMapUID = 0;
    if (normalMapUID != 0) resModule->ReleaseResource(normalMapUID);
    normalMapUID = 0;
    if (heightMapUID != 0) resModule->ReleaseResource(heightMapUID);
    heightMapUID = 0;
    if (metallicMapUID != 0) resModule->ReleaseResource(metallicMapUID);
    metallicMapUID = 0;
    if (occlusionMapUID != 0) resModule->ReleaseResource(occlusionMapUID);
    occlusionMapUID = 0;
}

void MaterialStandard::Bind(Shader* shader)
{
    if (!shader) return;

    shader->SetVec4("uColor", color);
    shader->SetFloat("uMetallic", metallic);
    shader->SetFloat("uRoughness", roughness);
    shader->SetFloat("uHeightScale", heightScale);
    shader->SetVec2("uTiling", tiling);
    shader->SetVec2("uOffset", offset);

    glActiveTexture(GL_TEXTURE0);
    if (albedoMap && albedoMap->IsLoadedToMemory()) {
        glBindTexture(GL_TEXTURE_2D, albedoMap->GetGPU_ID());
        shader->SetInt("uAlbedoMap", 0);
    }
    
    glActiveTexture(GL_TEXTURE1);
    if (metallicMap && metallicMap->IsLoadedToMemory()) {
        glBindTexture(GL_TEXTURE_2D, metallicMap->GetGPU_ID());
        shader->SetInt("uMetallicMap", 1);
    }

    glActiveTexture(GL_TEXTURE2);
    if (normalMap && normalMap->IsLoadedToMemory()) {
        glBindTexture(GL_TEXTURE_2D, normalMap->GetGPU_ID());
        shader->SetInt("uNormalMap", 2);
    }

    glActiveTexture(GL_TEXTURE3);
    if (occlusionMap && occlusionMap->IsLoadedToMemory()) {
        glBindTexture(GL_TEXTURE_2D, occlusionMap->GetGPU_ID());
        shader->SetInt("uOcclusionMap", 3);
    }

    glActiveTexture(GL_TEXTURE4);
    if (heightMap && heightMap->IsLoadedToMemory()) {
        glBindTexture(GL_TEXTURE_2D, heightMap->GetGPU_ID());
        shader->SetInt("uHeightMap", 4);
    }

    glActiveTexture(GL_TEXTURE0);
}

void MaterialStandard::SetAlbedoMap(UID uid) {

    if (albedoMapUID != 0) {
        Application::GetInstance().resources->ReleaseResource(albedoMapUID);
        albedoMapUID = 0;
    }

    albedoMap = (ResourceTexture*)Application::GetInstance().resources.get()->RequestResource(uid);

    if (albedoMap) albedoMapUID = uid;
}

void MaterialStandard::SetHeightMap(UID uid) {

    if (heightMapUID != 0) {
        Application::GetInstance().resources->ReleaseResource(heightMapUID);
        heightMapUID = 0;
    }

    heightMap = (ResourceTexture*) Application::GetInstance().resources.get()->RequestResource(uid);

    if (heightMap) heightMapUID = uid;
}

void MaterialStandard::SetNormalMap(UID uid) {

    if (normalMapUID != 0) {
        Application::GetInstance().resources->ReleaseResource(normalMapUID);
        normalMapUID = 0;
    }

    normalMap = (ResourceTexture*) Application::GetInstance().resources.get()->RequestResource(uid);

    if (normalMap) normalMapUID = uid;
}

void MaterialStandard::SetMetallicMap(UID uid) {

    if (metallicMapUID != 0) {
        Application::GetInstance().resources->ReleaseResource(metallicMapUID);
        metallicMapUID = 0;
    }

    metallicMap = (ResourceTexture*) Application::GetInstance().resources.get()->RequestResource(uid);

    if (metallicMap) metallicMapUID = uid;
}

void MaterialStandard::SetOcclusionMap(UID uid) {

    if (occlusionMapUID != 0) {
        Application::GetInstance().resources->ReleaseResource(occlusionMapUID);
        occlusionMapUID = 0;
    }

    occlusionMap = (ResourceTexture*) Application::GetInstance().resources.get()->RequestResource(uid);

    if (occlusionMap) occlusionMapUID = uid;
}

