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

#ifndef SPINE_SDL
#define SPINE_SDL

#include <string>
#include <map>
#include <d3d9.h>
#include <spine.h>

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)

struct DX9VEC2
{
	FLOAT x, y;
};

struct DX9COLOR
{
	BYTE r, g, b, a;
};

struct DX9VERTEX {
	DX9VEC2 position;
	DX9COLOR color;
	DX9VEC2 uv;
};

struct CUSTOMVERTEX
{
	float x, y, z, rhw;
	DWORD color;
	float u, v;
};

namespace spine
{
	class SkeletonDrawable
	{
	public:
		SkeletonDrawable(SkeletonData *skeletonData, AnimationStateData *animationStateData = nullptr);
		~SkeletonDrawable();

		void update(float delta, Physics physics);
		void draw(LPDIRECT3DDEVICE9 d3ddev);

		bool setSkin(std::string skinName);

		Skeleton* getSkeleton() { return m_pSkeleton; }
		AnimationState* getAnimationState() { return m_pAnimationState; }

		void setX(float fX) { m_pSkeleton->setX(fX); }
		float getX() { return m_pSkeleton->getX(); }
		void setY(float fY) { m_pSkeleton->setY(fY); }
		float getY() { return m_pSkeleton->getY(); }

	protected:
		void renderGeometry(LPDIRECT3DDEVICE9 d3ddev, LPDIRECT3DTEXTURE9 pTexture, DX9VERTEX* pvVertices, size_t vertexCnt, int* pvIndices, int indexCnt);

	private:
		SkeletonData* m_pSkeletonData;
		Skeleton* m_pSkeleton;
		AnimationState* m_pAnimationState;
		CUSTOMVERTEX* m_ptmpVetrtex;
		bool m_ownsAnimationStateData;
		Vector<DX9VERTEX> m_vVertices;
		Vector<int> m_vIndices;
	};

	class DX9TextureLoader : public spine::TextureLoader
	{
		struct TextureData
		{
			LPDIRECT3DTEXTURE9 texture;
			int width;
			int height;
			int refCnt;

			TextureData()
				: texture(nullptr)
				, width(0)
				, height(0)
				, refCnt(1)
			{
			}
		};

		LPDIRECT3DDEVICE9 m_d3ddev;
		std::map<std::string, TextureData> m_mapTextures;

	public:
		DX9TextureLoader(LPDIRECT3DDEVICE9 d3ddev)
			: m_d3ddev(d3ddev)
		{
		}

		void load(AtlasPage &page, const String &path);

		void unload(void* pTexture);
	};
}// namespace spine

#endif
