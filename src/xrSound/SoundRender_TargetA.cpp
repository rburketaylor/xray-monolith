#include "stdafx.h"
#pragma hdrstop

#include "soundrender_TargetA.h"
#include "soundrender_emitter.h"
#include "soundrender_source.h"

xr_vector<u8> g_target_temp_data;
xr_vector<u8> g_target_temp_data_16;

CSoundRender_TargetA::CSoundRender_TargetA(): CSoundRender_Target()
{
	cache_gain = 0.f;
	cache_pitch = 1.f;
	// mirrors of the OpenAL source state for the rarely changing
	// parameters; they are (re)applied to the source in _initialize()
	cache_min_dist = 1.f;
	cache_max_dist = flt_max;
	cache_rolloff = 1.f;
	cache_position.set(0.f, 0.f, 0.f);
	cache_velocity.set(0.f, 0.f, 0.f);
	cache_2d = FALSE;
	pSource = 0;
	Slot = u32(-1);
}

CSoundRender_TargetA::~CSoundRender_TargetA()
{
}

void CSoundRender_TargetA::SetSlot(ALuint NewSlot)
{
	Slot = NewSlot;
}

BOOL CSoundRender_TargetA::_initialize()
{
	inherited::_initialize();
	// initialize buffer
	A_CHK(alGenBuffers (sdef_target_count, pBuffers));
	alGenSources(1, &pSource);
	ALenum error = alGetError();
	if (AL_NO_ERROR == error)
	{
		A_CHK(alSourcei (pSource, AL_LOOPING, AL_FALSE));
		A_CHK(alSourcef (pSource, AL_MIN_GAIN, 0.f));
		A_CHK(alSourcef (pSource, AL_MAX_GAIN, 1.f));
		A_CHK(alSourcef (pSource, AL_GAIN, cache_gain));
		A_CHK(alSourcef (pSource, AL_PITCH, cache_pitch));
		// restore the cached source parameters on the new source (change
		// detection in fill_parameters would otherwise skip them)
		A_CHK(alSourcef (pSource, AL_REFERENCE_DISTANCE, cache_min_dist));
		A_CHK(alSourcef (pSource, AL_MAX_DISTANCE, cache_max_dist));
		A_CHK(alSource3f(pSource, AL_POSITION, cache_position.x, cache_position.y, cache_position.z));
		A_CHK(alSource3f(pSource, AL_VELOCITY, cache_velocity.x, cache_velocity.y, cache_velocity.z));
		A_CHK(alSourcei (pSource, AL_SOURCE_RELATIVE, cache_2d));
		A_CHK(alSourcef (pSource, AL_ROLLOFF_FACTOR, cache_rolloff));
		return TRUE;
	}
	else
	{
		Msg("! sound: OpenAL: Can't create source. Error: %s.", (LPCSTR)alGetString(error));
		return FALSE;
	}
}

void CSoundRender_TargetA::_destroy()
{
	// clean up target
	if (alIsSource(pSource))
		alDeleteSources(1, &pSource);
	A_CHK(alDeleteBuffers (sdef_target_count, pBuffers));
	inherited::_destroy();
}

void CSoundRender_TargetA::_restart()
{
	_destroy();
	_initialize();
}

void CSoundRender_TargetA::start(CSoundRender_Emitter* E)
{
	inherited::start(E);

	// Calc storage
	buf_block = sdef_target_block * E->source()->m_wformat.nAvgBytesPerSec / 1000;
	g_target_temp_data.resize(buf_block);
	g_target_temp_data_16.resize(buf_block * 2);
}

void CSoundRender_TargetA::render()
{
	for (u32 buf_idx = 0; buf_idx < sdef_target_count; buf_idx++)
		fill_block(pBuffers[buf_idx]);

	A_CHK(alSourceQueueBuffers(pSource, sdef_target_count, pBuffers));
	if (Slot != u32(-1) && !m_pEmitter->bIntro)
	{
		A_CHK(alSource3i(pSource, AL_AUXILIARY_SEND_FILTER, Slot, 0, AL_FILTER_NULL));
	}
	// demonized: explicitly disable effects by sending sounds to null slot, ie. not sending
	else
	{
		A_CHK(alSource3i(pSource, AL_AUXILIARY_SEND_FILTER, AL_EFFECTSLOT_NULL, 0, NULL));
	}
	A_CHK(alSourcePlay(pSource));

	inherited::render();
}

void CSoundRender_TargetA::stop()
{
	if (rendering)
	{
		A_CHK(alSourceStop(pSource));
		A_CHK(alSourcei (pSource, AL_BUFFER, NULL));
		A_CHK(alSourcei (pSource, AL_SOURCE_RELATIVE, TRUE));
		cache_2d = TRUE; // keep the cache in sync with the forced value above
	}
	inherited::stop();
}

void CSoundRender_TargetA::rewind()
{
	inherited::rewind();

	A_CHK(alSourceStop(pSource));
	A_CHK(alSourcei (pSource, AL_BUFFER, NULL));
	for (u32 buf_idx = 0; buf_idx < sdef_target_count; buf_idx++)
		fill_block(pBuffers[buf_idx]);
	A_CHK(alSourceQueueBuffers (pSource, sdef_target_count, pBuffers));
	A_CHK(alSourcePlay (pSource));
}

void CSoundRender_TargetA::update()
{
	inherited::update();

	ALint processed;
	// Get status
	A_CHK(alGetSourcei(pSource, AL_BUFFERS_PROCESSED, &processed));

	if (processed > 0)
	{
		while (processed)
		{
			ALuint BufferID;
			A_CHK(alSourceUnqueueBuffers(pSource, 1, &BufferID));
			fill_block(BufferID);
			A_CHK(alSourceQueueBuffers(pSource, 1, &BufferID));
			--processed;
		}

		ALint state;
		A_CHK(alGetSourcei(pSource, AL_SOURCE_STATE, &state));
		if (state == AL_STOPPED)
		A_CHK(alSourcePlay(pSource));
	}
	else
	{
		// processed == 0
		// check play status -- if stopped then queue is not being filled fast enough
		ALint state;
		A_CHK(alGetSourcei(pSource, AL_SOURCE_STATE, &state));
		if (state != AL_PLAYING)
		{
			//			Log		("Queuing underrun detected.");
			A_CHK(alSourcePlay(pSource));
		}
	}
}

void CSoundRender_TargetA::fill_parameters()
{
	CSoundRender_Emitter* SE = m_pEmitter;
	VERIFY(SE);

	inherited::fill_parameters();

	// 3D params
	VERIFY2(m_pEmitter, SE->source()->file_name());
	if (!fsimilar(m_pEmitter->p_source.min_distance, cache_min_dist, 0.01f))
	{
		cache_min_dist = m_pEmitter->p_source.min_distance;
		A_CHK(alSourcef(pSource, AL_REFERENCE_DISTANCE, cache_min_dist));
	}

	VERIFY2(m_pEmitter, SE->source()->file_name());
	if (!fsimilar(m_pEmitter->p_source.max_distance, cache_max_dist, 0.01f))
	{
		cache_max_dist = m_pEmitter->p_source.max_distance;
		A_CHK(alSourcef(pSource, AL_MAX_DISTANCE, cache_max_dist));
	}

	VERIFY2(m_pEmitter, SE->source()->file_name());
	Fvector _pos = m_pEmitter->p_source.position;
	_pos.z = -_pos.z;
	if (cache_position.x != _pos.x || cache_position.y != _pos.y || cache_position.z != _pos.z)
	{
		cache_position.set(_pos);
		A_CHK(alSource3f(pSource, AL_POSITION, cache_position.x, cache_position.y, cache_position.z));
	}

	VERIFY2(m_pEmitter, SE->source()->file_name());
	Fvector _vel = m_pEmitter->p_source.velocity;
	_vel.z = -_vel.z;
	if (cache_velocity.x != _vel.x || cache_velocity.y != _vel.y || cache_velocity.z != _vel.z)
	{
		cache_velocity.set(_vel);
		A_CHK(alSource3f(pSource, AL_VELOCITY, cache_velocity.x, cache_velocity.y, cache_velocity.z));
	}

	VERIFY2(m_pEmitter, SE->source()->file_name());
	if (cache_2d != m_pEmitter->b2D)
	{
		cache_2d = m_pEmitter->b2D;
		A_CHK(alSourcei(pSource, AL_SOURCE_RELATIVE, cache_2d));
	}

	if (!fsimilar(psSoundRolloff, cache_rolloff, 0.01f))
	{
		cache_rolloff = psSoundRolloff;
		A_CHK(alSourcef(pSource, AL_ROLLOFF_FACTOR, cache_rolloff));
	}

	VERIFY2(m_pEmitter, SE->source()->file_name());
	float _gain = m_pEmitter->smooth_volume;
	clamp(_gain, EPS_S, 1.f);
	if (!fsimilar(_gain, cache_gain, 0.01f))
	{
		cache_gain = _gain;
		A_CHK(alSourcef (pSource, AL_GAIN, _gain));
	}

	VERIFY2(m_pEmitter, SE->source()->file_name());
	float _pitch = m_pEmitter->p_source.freq;
	clamp(_pitch, EPS_L, 2.f);

	if (!fsimilar(cache_pitch, _pitch * psSpeedOfSound))
	{
		cache_pitch = _pitch * psSpeedOfSound;

		// Only update time to stop for non-looped sounds
		if (!m_pEmitter->iPaused && (m_pEmitter->m_current_state == CSoundRender_Emitter::stStarting || m_pEmitter->m_current_state == CSoundRender_Emitter::stPlaying || m_pEmitter->m_current_state == CSoundRender_Emitter::stSimulating))
			m_pEmitter->fTimeToStop = SoundRender->fTimer_Value + ((m_pEmitter->get_length_sec() - (SoundRender->fTimer_Value - m_pEmitter->fTimeStarted)) / cache_pitch);

		A_CHK(alSourcef(pSource, AL_PITCH, cache_pitch));
	}
	VERIFY2(m_pEmitter, SE->source()->file_name());
}

void CSoundRender_TargetA::fill_block(ALuint BufferID)
{
	R_ASSERT(m_pEmitter);
	ALuint format = (m_pEmitter->source()->m_wformat.nChannels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	if (format == AL_FORMAT_MONO16)
	{
		m_pEmitter->fill_block(&g_target_temp_data.front(), g_target_temp_data.size());
		A_CHK(alBufferData(BufferID, format, &g_target_temp_data.front(), g_target_temp_data.size(), m_pEmitter->source()->m_wformat.nSamplesPerSec));
	}
	else
	{
		m_pEmitter->fill_block(&g_target_temp_data_16.front(), g_target_temp_data_16.size());
		A_CHK(alBufferData(BufferID, format, &g_target_temp_data_16.front(), g_target_temp_data_16.size(), m_pEmitter->source()->m_wformat.nSamplesPerSec));
	}
}

void CSoundRender_TargetA::source_changed()
{
	dettach();
	attach();
}
