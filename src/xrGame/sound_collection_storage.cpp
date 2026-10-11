////////////////////////////////////////////////////////////////////////////
//	Module 		: sound_collection_storage.cpp
//	Created 	: 13.10.2005
//  Modified 	: 13.10.2005
//	Author		: Dmitriy Iassenev
//	Description : sound collection storage
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "sound_collection_storage.h"
#include "object_broker.h"

CSoundCollectionStorage* g_sound_collection_storage = 0;

CSoundCollectionStorage::~CSoundCollectionStorage()
{
	delete_data(m_objects);
}

const CSoundCollectionStorage::SOUND_COLLECTION_PAIR&
CSoundCollectionStorage::object(const CSoundCollectionParams& params)
{
	const CKey key(params);

	LOOKUP::const_iterator I = m_lookup.find(key);
	if (I != m_lookup.end())
		return (m_objects[I->second]);

	m_objects.push_back(std::make_pair(params, xr_new<CSoundCollection>(params)));
	m_lookup.emplace(key, m_objects.size() - 1);
	return (m_objects.back());
}
