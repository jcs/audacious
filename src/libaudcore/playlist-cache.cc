/*
 * playlist-cache.h
 * Copyright 2016 John Lindgren
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    provided with the distribution.
 *
 * This software is provided "as is" and without any warranty, express or
 * implied. In no event shall the authors be liable for any damages arising from
 * the use of this software.
 */

#include "playlist-internal.h"
#include "mainloop.h"
#include "multihash.h"
#include "threads.h"

static SimpleHash<String, PlaylistAddItem> cache;
static aud::mutex mutex;
static QueuedFunc clear_timer;

EXPORT void Playlist::cache_selected () const
{
    auto mh = mutex.take ();
    int entries = n_entries ();

    for (int i = 0; i < entries; i ++)
    {
        if (! entry_selected (i))
            continue;

        String filename = entry_filename (i);
        Tuple tuple = entry_tuple (i, NoWait);
        PluginHandle * decoder = entry_decoder (i, NoWait);

        if (tuple.valid () || decoder)
            cache.add (filename, {filename, std::move (tuple), decoder});
    }

    clear_timer.queue (30000, playlist_cache_clear, nullptr);
}

void playlist_cache_load (Index<PlaylistAddItem> & items)
{
    auto mh = mutex.take ();

    if (! cache.n_items ())
        return;

    for (auto & item : items)
    {
        if (item.tuple.valid () && item.decoder)
            continue;

        auto node = cache.lookup (item.filename);
        if (! node)
            continue;

        if (! item.tuple.valid () && node->tuple.valid ())
            item.tuple = node->tuple.ref ();
        if (! item.decoder && node->decoder)
            item.decoder = node->decoder;
    }
}

void playlist_cache_clear (void *)
{
    auto mh = mutex.take ();

    cache.clear ();
    clear_timer.stop ();
}
