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
		rec.m_desc = desc;
		//	The base desc is aniso-independent: the variants differ only in this field.
		rec.m_desc.MaxAnisotropy = 1;
		rec.m_pStateAniso1 = 0;
		rec.m_pStateAnisoMax = 0;
		rec.m_uiAnisoMax = 0;
		rec.m_pState = GetAnisoState(rec, m_uiMaxAnisotropy);
		hResult = m_StateArray.size();
		m_StateArray.push_back(rec);
	}

	return hResult;
}

ID3DSamplerState* dx10SamplerStateCache::GetAnisoState(StateRecord& rec, u32 uiMaxAniso)
{
	if (uiMaxAniso > 1)
	{
		StateDecs desc = rec.m_desc;
		desc.MaxAnisotropy = uiMaxAniso;
		dx10StateUtils::ValidateState(desc);

		//	ValidateState() resets MaxAnisotropy to 1 for filter modes which do not
		//	support it: those records always use the 1x variant.
		if (desc.MaxAnisotropy > 1)
		{
			if ((rec.m_pStateAnisoMax == 0) || (rec.m_uiAnisoMax != uiMaxAniso))
			{
				_RELEASE(rec.m_pStateAnisoMax);
				CreateState(desc, &rec.m_pStateAnisoMax);
				rec.m_uiAnisoMax = uiMaxAniso;
			}

			return rec.m_pStateAnisoMax;
		}
	}

	if (rec.m_pStateAniso1 == 0)
		CreateState(rec.m_desc, &rec.m_pStateAniso1);

	return rec.m_pStateAniso1;
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
		//	m_pState is an alias of one of the two variants.
		_RELEASE(m_StateArray[i].m_pStateAniso1);
		_RELEASE(m_StateArray[i].m_pStateAnisoMax);
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

	bool bStateChanged = false;

	for (u32 i = 0; i < m_StateArray.size(); ++i)
	{
		StateRecord& rec = m_StateArray[i];
		IDeviceState* pOldState = rec.m_pState;

		//	Both variants are kept per record: switching between them does not
		//	create or destroy any device state once they are built. Records whose
		//	filter mode cannot use anisotropy always resolve to the 1x variant.
		rec.m_pState = GetAnisoState(rec, m_uiMaxAnisotropy);

		if (rec.m_pState != pOldState)
			bStateChanged = true;
	}

	//	The cached handles are unchanged, but the device states they resolve to
	//	may differ: force the next Apply to rebind every sampler slot.
	if (bStateChanged)
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

        rec.m_desc.MipLODBias = m_uiMipLODBias;
        dx10StateUtils::ValidateState(rec.m_desc);

        //	Both variants embed the LOD bias: drop them and rebuild the active
        //	one. The other one is recreated on demand by SetMaxAnisotropy().
        _RELEASE(rec.m_pStateAniso1);
        _RELEASE(rec.m_pStateAnisoMax);
        rec.m_uiAnisoMax = 0;
        rec.m_pState = GetAnisoState(rec, m_uiMaxAnisotropy);
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
