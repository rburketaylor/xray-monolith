////////////////////////////////////////////////////////////////////////////
//	Module 		: sound_collection_storage.h
//	Created 	: 13.10.2005
//  Modified 	: 13.10.2005
//	Author		: Dmitriy Iassenev
//	Description : sound collection storage
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "sound_player.h"

class CSoundCollectionStorage
{
public:
	typedef CSoundPlayer::CSoundCollectionParams CSoundCollectionParams;
	typedef CSoundPlayer::CSoundCollection CSoundCollection;
	typedef std::pair<CSoundCollectionParams, CSoundCollection*> SOUND_COLLECTION_PAIR;
	typedef xr_vector<SOUND_COLLECTION_PAIR> OBJECTS;

private:
	// the storage lives for the whole process and only grows, so the linear
	// search over it was repeated for every sound a spawning object registers
	struct CKey
	{
		shared_str m_sound_prefix;
		shared_str m_sound_player_prefix;
		u32 m_max_count;
		ESoundTypes m_type;

		IC CKey(const CSoundCollectionParams& params) :
			m_sound_prefix(params.m_sound_prefix),
			m_sound_player_prefix(params.m_sound_player_prefix),
			m_max_count(params.m_max_count),
			m_type(params.m_type)
		{
		}

		IC bool operator==(const CKey& other) const
		{
			return (m_sound_prefix == other.m_sound_prefix) &&
				(m_sound_player_prefix == other.m_sound_player_prefix) &&
				(m_max_count == other.m_max_count) &&
				(m_type == other.m_type);
		}
	};

	struct CHash
	{
		IC size_t operator()(const CKey& key) const
		{
			size_t hash = std::hash<shared_str>()(key.m_sound_prefix);
			hash ^= std::hash<shared_str>()(key.m_sound_player_prefix) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
			hash ^= size_t(key.m_max_count) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
			hash ^= size_t(key.m_type) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
			return hash;
		}
	};

	typedef xr_unordered_map<CKey, size_t, CHash> LOOKUP;

	OBJECTS m_objects;
	LOOKUP m_lookup;

public:
	virtual ~CSoundCollectionStorage();
	const SOUND_COLLECTION_PAIR& object(const CSoundCollectionParams& params);
};

extern CSoundCollectionStorage* g_sound_collection_storage;

IC CSoundCollectionStorage& sound_collection_storage();

#include "sound_collection_storage_inline.h"
