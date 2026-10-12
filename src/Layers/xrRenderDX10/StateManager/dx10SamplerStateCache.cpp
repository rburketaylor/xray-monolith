#include "stdafx.h"
#include "dx10SamplerStateCache.h"

#include "../dx10StateUtils.h"

using dx10StateUtils::operator==;

dx10SamplerStateCache SSManager;

dx10SamplerStateCache::dx10SamplerStateCache():
	m_uiMaxAnisotropy(1), m_uiMipLODBias(0.0f)
{
	static const int iMaxRSStates = 10;
	m_StateArray.reserve(iMaxRSStates);
	ResetDeviceState();
}

dx10SamplerStateCache::~dx10SamplerStateCache()
{
	ClearStateArray();
}

dx10SamplerStateCache::SHandle dx10SamplerStateCache::GetState(D3D_SAMPLER_DESC& desc)
{
	SHandle hResult;

	//	MaxAnisitropy is reset by ValidateState if not aplicable
	//	to the filter mode used.
	desc.MaxAnisotropy = m_uiMaxAnisotropy;
	desc.MipLODBias = m_uiMipLODBias;
	dx10StateUtils::ValidateState(desc);

	u32 crc = dx10StateUtils::GetHash(desc);

	hResult = FindState(desc, crc);

	if (hResult == hInvalidHandle)
	{
		StateRecord rec;
		rec.m_crc = crc;
		CreateState(desc, &rec.m_pState);
		hResult = m_StateArray.size();
		m_StateArray.push_back(rec);
	}

	return hResult;
}

void dx10SamplerStateCache::CreateState(StateDecs desc, IDeviceState** ppIState)
{
	CHK_DX(HW.pDevice->CreateSamplerState( &desc, ppIState));
}

dx10SamplerStateCache::SHandle dx10SamplerStateCache::FindState(const StateDecs& desc, u32 StateCRC)
{
	u32 res = 0xffffffff;
	u32 i = 0;
	for (; i < m_StateArray.size(); ++i)
	{
		if (m_StateArray[i].m_crc == StateCRC)
		{
			StateDecs descCandidate;
			m_StateArray[i].m_pState->GetDesc(&descCandidate);
			if (descCandidate == desc)
				//return i;
				//	TEST
			{
				//return i;
				res = i;
				break;
			}
			//else
			//{
			//	VERIFY(0);
			//}
		}
	}

	return res != 0xffffffff ? i : (u32)hInvalidHandle;
}

void dx10SamplerStateCache::ClearStateArray()
{
	for (u32 i = 0; i < m_StateArray.size(); ++i)
	{
		_RELEASE(m_StateArray[i].m_pState);
	}

	m_StateArray.clear_not_free();
}

bool dx10SamplerStateCache::PrepareSamplerStates(
	HArray& samplers,
	ID3DSamplerState* pSS[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT],
	SHandle pCurrentState[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT],
	u32& uiMin,
	u32& uiMax
) const
{
	//	It seems that sizeof pSS is 4 wor win32!
	ZeroMemory(pSS, sizeof(pSS[0])*D3D_COMMONSHADER_SAMPLER_SLOT_COUNT);

	uiMin = 0;
	uiMax = 0;

	u32 uiSlots = (u32)samplers.size();
	if (uiSlots > D3D_COMMONSHADER_SAMPLER_SLOT_COUNT)
		uiSlots = D3D_COMMONSHADER_SAMPLER_SLOT_COUNT;

	bool bChanged = false;

	for (u32 i = 0; i < uiSlots; ++i)
	{
		if (samplers[i] != hInvalidHandle)
		{
			VERIFY(samplers[i]<m_StateArray.size());
			pSS[i] = m_StateArray[samplers[i]].m_pState;
		}
		//	else pSS[i] stays NULL: the slot is explicitly unbound.

		if (samplers[i] == pCurrentState[i])
			continue;

		pCurrentState[i] = samplers[i];

		if (!bChanged)
		{
			bChanged = true;
			uiMin = i;
			uiMax = i;
		}
		else
		{
			uiMin = _min(uiMin, i);
			uiMax = _max(uiMax, i);
		}
	}

	//	Slots the state does not describe at all must be unbound as well,
	//	otherwise a sampler bound by a previously applied state stays in the pipeline.
	for (u32 i = uiSlots; i < D3D_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
	{
		if (pCurrentState[i] == (SHandle)hInvalidHandle)
			continue;

		pCurrentState[i] = (SHandle)hInvalidHandle;

		if (!bChanged)
		{
			bChanged = true;
			uiMin = i;
			uiMax = i;
		}
		else
		{
			uiMin = _min(uiMin, i);
			uiMax = _max(uiMax, i);
		}
	}

	return bChanged;
}

void dx10SamplerStateCache::VSApplySamplers(HArray& samplers)
{
	ID3DSamplerState* pSS[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT];
	u32 uiMin;
	u32 uiMax;
	if (PrepareSamplerStates(samplers, pSS, m_aVSSamplers, uiMin, uiMax))
		HW.pContext->VSSetSamplers(uiMin, uiMax - uiMin + 1, &pSS[uiMin]);
}

void dx10SamplerStateCache::PSApplySamplers(HArray& samplers)
{
	ID3DSamplerState* pSS[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT];
	u32 uiMin;
	u32 uiMax;
	if (PrepareSamplerStates(samplers, pSS, m_aPSSamplers, uiMin, uiMax))
		HW.pContext->PSSetSamplers(uiMin, uiMax - uiMin + 1, &pSS[uiMin]);
}

void dx10SamplerStateCache::GSApplySamplers(HArray& samplers)
{
	ID3DSamplerState* pSS[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT];
	u32 uiMin;
	u32 uiMax;
	if (PrepareSamplerStates(samplers, pSS, m_aGSSamplers, uiMin, uiMax))
		HW.pContext->GSSetSamplers(uiMin, uiMax - uiMin + 1, &pSS[uiMin]);
}

#ifdef USE_DX11
void dx10SamplerStateCache::HSApplySamplers(HArray& samplers)
{
	ID3DSamplerState* pSS[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT];
	u32 uiMin;
	u32 uiMax;
	if (PrepareSamplerStates(samplers, pSS, m_aHSSamplers, uiMin, uiMax))
		HW.pContext->HSSetSamplers(uiMin, uiMax - uiMin + 1, &pSS[uiMin]);
}

void dx10SamplerStateCache::DSApplySamplers(HArray& samplers)
{
	ID3DSamplerState* pSS[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT];
	u32 uiMin;
	u32 uiMax;
	if (PrepareSamplerStates(samplers, pSS, m_aDSSamplers, uiMin, uiMax))
		HW.pContext->DSSetSamplers(uiMin, uiMax - uiMin + 1, &pSS[uiMin]);
}

void dx10SamplerStateCache::CSApplySamplers(HArray& samplers)
{
	ID3DSamplerState* pSS[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT];
	u32 uiMin;
	u32 uiMax;
	if (PrepareSamplerStates(samplers, pSS, m_aCSSamplers, uiMin, uiMax))
		HW.pContext->CSSetSamplers(uiMin, uiMax - uiMin + 1, &pSS[uiMin]);
}
#endif


void dx10SamplerStateCache::SetMaxAnisotropy(u32 uiMaxAniso)
{
	clamp(uiMaxAniso, (u32)1, (u32)16);

	if (m_uiMaxAnisotropy == uiMaxAniso)
		return;

	m_uiMaxAnisotropy = uiMaxAniso;

	for (u32 i = 0; i < m_StateArray.size(); ++i)
	{
		StateRecord& rec = m_StateArray[i];
		StateDecs desc;

		if (!rec.m_pState)
			continue;

		rec.m_pState->GetDesc(&desc);

		//	MaxAnisitropy is reset by ValidateState if not aplicable
		//	to the filter mode used.
		//	Reason: all checks for aniso applicability are done
		//	in ValidateState.
		desc.MaxAnisotropy = m_uiMaxAnisotropy;
		dx10StateUtils::ValidateState(desc);

		//	This can cause fragmentation if called too often
		rec.m_pState->Release();
		CreateState(desc, &rec.m_pState);
	}

	//	The cached handles are unchanged, but the device states they resolve to
	//	were recreated: force the next Apply to rebind every sampler slot.
	ResetDeviceState();
}

void dx10SamplerStateCache::SetMipLODBias(float uiMipLODBias)
{
    if (m_uiMipLODBias == uiMipLODBias)
        return;

    m_uiMipLODBias = uiMipLODBias;

    for (u32 i = 0; i < m_StateArray.size(); ++i)
    {
        StateRecord& rec = m_StateArray[i];
        StateDecs desc;

        rec.m_pState->GetDesc(&desc);

        desc.MipLODBias = m_uiMipLODBias;
        dx10StateUtils::ValidateState(desc);

        // This can cause fragmentation if called too often
        rec.m_pState->Release();
        CreateState(desc, &rec.m_pState);
    }

    //	Same as SetMaxAnisotropy: the handles stay, the device states change.
    ResetDeviceState();
}


void dx10SamplerStateCache::ResetDeviceState()
{
	for (int i = 0; i < sizeof(m_aPSSamplers) / sizeof(m_aPSSamplers[0]); ++i)
	{
		m_aPSSamplers[i] = (SHandle)hUnknownHandle;
		m_aVSSamplers[i] = (SHandle)hUnknownHandle;
		m_aGSSamplers[i] = (SHandle)hUnknownHandle;
#ifdef USE_DX11
		m_aHSSamplers[i] = (SHandle)hUnknownHandle;
		m_aDSSamplers[i] = (SHandle)hUnknownHandle;
		m_aCSSamplers[i] = (SHandle)hUnknownHandle;
#endif
	}
}
