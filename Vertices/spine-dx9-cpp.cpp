/******************************************************************************
 * Spine Runtimes License Agreement
 * Last updated July 28, 2023. Replaces all prior versions.
 *
 * Copyright (c) 2013-2023, Esoteric Software LLC
 *
 * Integration of the Spine Runtimes into software or otherwise creating
 * derivative works of the Spine Runtimes is permitted under the terms and
 * conditions of Section 2 of the Spine Editor License Agreement:
 * http://esotericsoftware.com/spine-editor-license
 *
 * Otherwise, it is permitted to integrate the Spine Runtimes into software or
 * otherwise create derivative works of the Spine Runtimes (collectively,
 * "Products"), provided that each user of the Products must obtain their own
 * Spine Editor license and redistribution of the Products in any form must
 * include this license and copyright notice.
 *
 * THE SPINE RUNTIMES ARE PROVIDED BY ESOTERIC SOFTWARE LLC "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ESOTERIC SOFTWARE LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES,
 * BUSINESS INTERRUPTION, OR LOSS OF USE, DATA, OR PROFITS) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THE
 * SPINE RUNTIMES, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/


#define STB_IMAGE_IMPLEMENTATION

#include <strsafe.h>
#include "spine-dx9-cpp.h"
#include <d3dx9.h>
#pragma warning( disable : 4996 ) // disable deprecated warning 
#include <strsafe.h>
#pragma warning( default : 4996 )

using namespace spine;

SkeletonRenderer* skeletonRenderer = nullptr;

SkeletonDrawable::SkeletonDrawable(SkeletonData *skeletonData, AnimationStateData *animationStateData)
	: m_pSkeletonData(skeletonData)
{
	m_ptmpVetrtex = new CUSTOMVERTEX[8192];

	Bone::setYDown(true);
	m_pSkeleton = new (__FILE__, __LINE__) Skeleton(skeletonData);

	m_ownsAnimationStateData = animationStateData == 0;
	if (m_ownsAnimationStateData) animationStateData = new (__FILE__, __LINE__) AnimationStateData(skeletonData);
	m_pAnimationState = new (__FILE__, __LINE__) AnimationState(animationStateData);
}

SkeletonDrawable::~SkeletonDrawable()
{
	if (m_pAnimationState)
	{
		if (m_ownsAnimationStateData) delete m_pAnimationState->getData();
		delete m_pAnimationState;
		m_pAnimationState = 0x0;
	}
	if (m_pSkeleton)
	{
		delete m_pSkeleton;
		m_pSkeleton = 0x0;
	}
	if (m_ptmpVetrtex)
	{
		delete m_ptmpVetrtex;
		m_ptmpVetrtex = 0x0;
	}
}

void SkeletonDrawable::update(float delta, Physics physics)
{
	m_pAnimationState->update(delta);
	m_pAnimationState->apply(*m_pSkeleton);
	m_pSkeleton->update(delta);
	m_pSkeleton->updateWorldTransform(physics);
}

inline void toDX9Color(uint32_t color, DX9COLOR *dx9Color)
{
	dx9Color->a = (color >> 24) & 0xFF;
	dx9Color->r = (color >> 16) & 0xFF;
	dx9Color->g = (color >> 8) & 0xFF;
	dx9Color->b = color & 0xFF;
}

void SkeletonDrawable::draw(LPDIRECT3DDEVICE9 d3ddevice)
{
	if (!skeletonRenderer) skeletonRenderer = new (__FILE__, __LINE__) SkeletonRenderer();
	RenderCommand *command = skeletonRenderer->render(*m_pSkeleton);
	while (command)
	{
		float* positions = command->positions;
		float* uvs = command->uvs;
		uint32_t* colors = command->colors;
		m_vVertices.clear();
		for (int ii = 0; ii < command->numVertices << 1; ii += 2)
		{
			DX9VERTEX vertex;
			vertex.position.x = positions[ii];
			vertex.position.y = positions[ii + 1];
			vertex.uv.x = uvs[ii];
			vertex.uv.y = uvs[ii + 1];
			toDX9Color(colors[ii >> 1], &vertex.color);
			m_vVertices.add(vertex);
		}
		m_vIndices.clear();
		uint16_t* indices = command->indices;
		for (int ii = 0; ii < command->numIndices; ii++)
		{
			m_vIndices.add(indices[ii]);
		}

		BlendMode blendMode = command->blendMode;
		LPDIRECT3DTEXTURE9 pTexture = (LPDIRECT3DTEXTURE9)command->texture;
		switch (blendMode)
		{
		case BlendMode_Normal:
			// 設定 Blend 狀態
			d3ddevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE); // Src = 1
			d3ddevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA); // Dest = 1 - SrcAlpha
			d3ddevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD); // 顏色加法
			break;
		case BlendMode_Multiply:
			d3ddevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCCOLOR); // Src = 1
			d3ddevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_DESTCOLOR); // Dest = 1 - SrcAlpha
			d3ddevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_MAX); // 顏色加法
			break;
		case BlendMode_Additive:
			d3ddevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCCOLOR); // Src = 1
			d3ddevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_DESTCOLOR); // Dest = 1 - SrcAlpha
			break;
		case BlendMode_Screen:
			d3ddevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE); // Src = 1
			d3ddevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE); // Dest = 1 - SrcAlpha
			break;
		}

		d3ddevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
		d3ddevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		d3ddevice->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_ONE);
		d3ddevice->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA);
		d3ddevice->SetRenderState(D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD);

		renderGeometry(d3ddevice, pTexture, m_vVertices.buffer(), m_vVertices.size(), m_vIndices.buffer(), command->numIndices);
		command = command->next;
	}
}

bool SkeletonDrawable::setSkin(std::string skinName)
{
	spine::Skin* pSkin = m_pSkeletonData->findSkin(skinName.c_str());
	if (!pSkin) return false;
	m_pSkeletonData->setDefaultSkin(pSkin);
	return true;
}

void SkeletonDrawable::renderGeometry(LPDIRECT3DDEVICE9 d3ddev, LPDIRECT3DTEXTURE9 pTexture, DX9VERTEX* pvVertices, size_t vertexCnt, int* pvIndices, int indexCnt)
{
	d3ddev->SetTexture(0, pTexture);

	for (int i = 0; i < vertexCnt; i++)
	{
		m_ptmpVetrtex[i].x = pvVertices[i].position.x;
		m_ptmpVetrtex[i].y = pvVertices[i].position.y;
		m_ptmpVetrtex[i].z = 0.0f; // z coordinate is not used in 2D rendering
		m_ptmpVetrtex[i].rhw = 1.0f; // reciprocal of homogeneous w coordinate
		m_ptmpVetrtex[i].color = D3DCOLOR_ARGB(pvVertices[i].color.a, pvVertices[i].color.r, pvVertices[i].color.g, pvVertices[i].color.b);
		m_ptmpVetrtex[i].u = pvVertices[i].uv.x;
		m_ptmpVetrtex[i].v = pvVertices[i].uv.y;
	}

	d3ddev->SetFVF(D3DFVF_CUSTOMVERTEX);
	d3ddev->DrawIndexedPrimitiveUP(
		D3DPT_TRIANGLELIST,
		0,
		vertexCnt,
		indexCnt/3,
		pvIndices,
		D3DFMT_INDEX32,
		m_ptmpVetrtex,
		sizeof(CUSTOMVERTEX)
	);
}

void DX9TextureLoader::load(AtlasPage &page, const String &path)
{
	LPDIRECT3DTEXTURE9 pTexture = NULL;
	std::map<std::string, TextureData>::iterator it = m_mapTextures.find(path.buffer());
	if (it != m_mapTextures.end())
	{
		it->second.refCnt++;
		pTexture = it->second.texture;
	}
	else
	{
		// Use D3DX to create a texture from a file based image
		if (SUCCEEDED(D3DXCreateTextureFromFileA(m_d3ddev, path.buffer(), &pTexture)))
		{
			TextureData textureData;
			textureData.texture = pTexture;
			textureData.refCnt++;
			m_mapTextures[path.buffer()] = textureData;
		}
	}
	page.texture = pTexture;
}

void DX9TextureLoader::unload(void* pTexture)
{
	std::map<std::string, TextureData>::iterator it = m_mapTextures.begin();
	for (; it != m_mapTextures.end(); ++it)
	{
		if (it->second.texture == pTexture)
		{
			it->second.refCnt--;
			if (it->second.refCnt <= 0)
			{
				it->second.texture->Release();
				m_mapTextures.erase(it);
			}
			return;
		}
	}
}

SpineExtension *spine::getDefaultExtension()
{
	return new DefaultSpineExtension();
}
