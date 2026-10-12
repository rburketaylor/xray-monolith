#ifndef	dx10SamplerStateCache_included
#define	dx10SamplerStateCache_included
#pragma once


class dx10SamplerStateCache
{
public:
	enum
	{
		hInvalidHandle = 0xFFFFFFFF,
		hUnknownHandle = 0xFFFFFFFE	//	device binding is unknown (see ResetDeviceState)
	};

	//	State handle
	typedef u32 SHandle;
	typedef xr_vector<SHandle> HArray;
public:
	dx10SamplerStateCache();
	~dx10SamplerStateCache();

	void ClearStateArray();

	SHandle GetState(D3D_SAMPLER_DESC& desc);

	void VSApplySamplers(HArray& samplers);
	void PSApplySamplers(HArray& samplers);
	void GSApplySamplers(HArray& samplers);
#ifdef USE_DX11
	void HSApplySamplers(HArray& samplers);
	void DSApplySamplers(HArray& samplers);
	void CSApplySamplers(HArray& samplers);
#endif

	void SetMaxAnisotropy(u32 uiMaxAniso);
	void SetMipLODBias(float uiMipLODBias);

	//	Marks all device sample as unused
	void ResetDeviceState();

	//	Private declarations
private:
	typedef ID3DSamplerState IDeviceState;
	typedef D3D_SAMPLER_DESC StateDecs;

	struct StateRecord
	{
		u32 m_crc;
		IDeviceState* m_pState;				//	currently active variant, used by Apply
		IDeviceState* m_pStateAniso1;		//	variant created with MaxAnisotropy == 1
		IDeviceState* m_pStateAnisoMax;		//	variant created with MaxAnisotropy == m_uiAnisoMax
		u32 m_uiAnisoMax;					//	MaxAnisotropy value of m_pStateAnisoMax
		//	Base desc both variants are created from: MaxAnisotropy is normalised
		//	to 1 and ValidateState() has been applied. It is not part of the
		//	state lookup (neither the CRC nor the comparison use anisotropy).
		StateDecs m_desc;
	};

private:
	void CreateState(StateDecs desc, IDeviceState** ppIState);
	SHandle FindState(const StateDecs& desc, u32 StateCRC);

	//	Returns the state variant built for uiMaxAniso, creating it on demand.
	IDeviceState* GetAnisoState(StateRecord& rec, u32 uiMaxAniso);

	//	Resolves the changed slots only. Returns false when the device samplers
	//	already match the state, in which case uiMin/uiMax are undefined.
	bool PrepareSamplerStates(
		HArray& samplers,
		ID3DSamplerState* pSS[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT],
		SHandle pCurrentState[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT],
		u32& uiMin,
		u32& uiMax
	) const;

	//	Private data
private:
	//	This must be cleared on device destroy
	xr_vector<StateRecord> m_StateArray;

	SHandle m_aPSSamplers[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT];
	SHandle m_aVSSamplers[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT];
	SHandle m_aGSSamplers[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT];
#ifdef USE_DX11
	SHandle m_aHSSamplers[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT];
	SHandle m_aDSSamplers[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT];
	SHandle m_aCSSamplers[D3D_COMMONSHADER_SAMPLER_SLOT_COUNT];
#endif

	u32 m_uiMaxAnisotropy;
	float m_uiMipLODBias;	
};

extern dx10SamplerStateCache SSManager;

#endif	//	dx10SamplerStateCache_included
