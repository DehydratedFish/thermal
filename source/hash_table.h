#ifndef INCLUDE_GUARD_HASH_TABLE_H
#define INCLUDE_GUARD_HASH_TABLE_H

#include "memory.h"


template<typename KeyType, typename HashType = u32>
using HashTableHashFunc = HashType(KeyType key);

template<typename KeyType, typename ValueType, typename HashType, HashTableHashFunc<KeyType, HashType> *hash_func>
struct HashTable {
	struct Entry {
		KeyType   key;
		ValueType value;
		HashType  hash;
	};

	Allocator allocator;

	Entry *entries;
	s32 buckets;
	s32 alloc;
	s32 max_load;
};

template<typename Type>
bool equal(Type const &lhs, Type const &rhs) {
    return lhs == rhs;
}

template<typename KeyType, typename ValueType, typename HashType, HashTableHashFunc<KeyType, HashType> *hash_func>
void init(HashTable<KeyType, ValueType, HashType, hash_func> *table, s32 size) {
	typedef typename HashTable<KeyType, ValueType, HashType, hash_func>::Entry Entry;
	r32 const default_max_load = 0.6f;

	assert((size & (size - 1)) == 0); // NOTE: table size needs to be a power of two

    if (table->allocator.allocate == 0) table->allocator = default_allocator();

	table->entries  = (Entry*)allocate(table->allocator, size * sizeof(Entry));
	table->buckets  = 0;
	table->alloc    = size;
	table->max_load = (s32)(size * default_max_load);
}

template<typename KeyType, typename ValueType, typename HashType, HashTableHashFunc<KeyType, HashType> *hash_func>
ValueType *insert(HashTable<KeyType, ValueType, HashType, hash_func> *table, KeyType key, u32 hash, ValueType value) {
	typedef typename HashTable<KeyType, ValueType, HashType, hash_func>::Entry Entry;

	if (table->alloc == 0) {
		u32 const default_size = 128;
		init(table, default_size);
	}

	if (table->buckets == table->max_load) {
		grow(table);
	}

	if (hash == 0) hash += 1;

	u32 mask = table->alloc - 1;
	s32 bucket = hash & mask;
    Entry *entries = table->entries;

    while (entries[bucket].hash && entries[bucket].key != key) {
        bucket = (bucket + 1) & mask;
    }

    if (entries[bucket].hash) {
        entries[bucket].value = value;
    } else {
        entries[bucket].hash  = hash;
        entries[bucket].key   = key;
        entries[bucket].value = value;

        table->buckets += 1;
    }

	return &entries[bucket].value;
}

template<typename KeyType, typename ValueType, typename HashType, HashTableHashFunc<KeyType, HashType> *hash_func>
ValueType *insert(HashTable<KeyType, ValueType, HashType, hash_func> *table, KeyType key, ValueType value) {
	return insert(table, key, hash_func(key), value);
}

template<typename KeyType, typename ValueType, typename HashType, HashTableHashFunc<KeyType, HashType> *hash_func>
ValueType *find(HashTable<KeyType, ValueType, HashType, hash_func> *table, u32 hash, KeyType key) {
	typedef typename HashTable<KeyType, ValueType, HashType, hash_func>::Entry Entry;

	if (table->buckets == 0) return 0;
	
	if (hash == 0) hash += 1;

	u32 mask = table->alloc - 1;
	s32 bucket = hash & mask;
    Entry *entries = table->entries;

    while (entries[bucket].hash && entries[bucket].key != key) {
        bucket = (bucket + 1) & mask;
    }

    if (entries[bucket].hash == 0) return 0;

    return &entries[bucket].value;
}

template<typename KeyType, typename ValueType, typename HashType, HashTableHashFunc<KeyType, HashType> *hash_func>
ValueType *find(HashTable<KeyType, ValueType, HashType, hash_func> *table, KeyType key) {
	return find(table, hash_func(key), key);
}

template<typename KeyType, typename ValueType, typename HashType, HashTableHashFunc<KeyType, HashType> *hash_func>
bool remove(HashTable<KeyType, ValueType, HashType, hash_func> *table, KeyType key) {
    typedef typename HashTable<KeyType, ValueType, HashType, hash_func>::Entry Entry;

    if (table->buckets == 0) return 0;

    u32 hash = hash_func(key);
    if (hash == 0) hash += 1;

    u32 mask = table->alloc - 1;
    s32 bucket = hash & mask;
    Entry *entries = table->entries;

    while (entries[bucket].hash && entries[bucket].key != key) {
        bucket = (bucket + 1) & mask;
    }

    if (entries[bucket].hash == 0) return false;
    INIT_STRUCT(&entries[bucket]);
    table->buckets -= 1;

    u32 bucket2 = bucket;

    // TODO: As the hash is saved could I just check if the original hashes bucket is higher than the deleted hash?
    //
    // source: https://en.wikipedia.org/wiki/Open_addressing
    for (;;) {
        bucket2 = (bucket2 + 1) & mask;
        if (entries[bucket2].hash == 0) return true;

        HashType original_bucket_hash = hash_func(entries[bucket2].key);
        if (original_bucket_hash == 0) original_bucket_hash += 1;
        u32 original_bucket = original_bucket_hash & mask;

        if (bucket <= bucket2) {
            if ((bucket < original_bucket) && (original_bucket <= bucket2)) continue;
        } else {
            if ((original_bucket <= bucket2) || (bucket < original_bucket)) continue;
        }

        entries[bucket] = entries[bucket2];
        INIT_STRUCT(&entries[bucket2]);

        bucket = bucket2;
    }

    return false;
}

template<typename KeyType, typename ValueType, typename HashType, HashTableHashFunc<KeyType, HashType> *hash_func>
void grow(HashTable<KeyType, ValueType, HashType, hash_func> *table) {
	assert(table->alloc);

	u32 const GrowFactor = 2;

	HashTable<KeyType, ValueType, HashType, hash_func> new_table = {};
    new_table.allocator = table->allocator;

	init(&new_table, table->alloc * GrowFactor);

	for (s32 i = 0; i < table->alloc; i += 1) {
		if (table->entries[i].hash != 0) {
			insert(&new_table, table->entries[i].key, table->entries[i].hash, table->entries[i].value);
		}
	}

	destroy(table);
	*table = new_table;
}

template<typename KeyType, typename ValueType, typename HashType, HashTableHashFunc<KeyType, HashType> *hash_func>
void destroy(HashTable<KeyType, ValueType, HashType, hash_func> *table) {
	typedef typename HashTable<KeyType, ValueType, HashType, hash_func>::Entry Entry;

	deallocate(table->allocator, table->entries, table->alloc * sizeof(Entry));

	INIT_STRUCT(table);
}




// NOTE: curtasy of https://nullprogram.com/blog/2023/09/30/

template<typename KeyType>
using ArenaHashTableHashFunc = u64(KeyType key);

inline u64 DefaultStringArenaHash(String str) {
    u64 h = 0x100;

    for (s64 i = 0; i < str.size; i += 1) {
        h ^= str[i];
        h *= 1111111111111111111u;
    }

    return h;
}

template<typename KeyType, typename ValueType, ArenaHashTableHashFunc<KeyType> *hash_func>
struct ArenaHashTable {
    ArenaHashTable *child[4];
    KeyType key;
    ValueType value;
};

template<typename KeyType, typename ValueType, ArenaHashTableHashFunc<KeyType> *hash_func>
ValueType *upsert(ArenaHashTable<KeyType, ValueType, *hash_func> **table, KeyType key, MemoryArena *arena, b32 *inserted) {
    assert(arena && inserted);

    u64 hash = hash_func(key);

    for (; *table; hash <<= 2) {
        if (equal(key, (*table)->key)) {
            *inserted = false;
            return &(*table)->value;
        }
        table = &(*table)->child[hash >> 62];
    }

    typedef ArenaHashTable<KeyType, ValueType, *hash_func> TableType;
    *table = ALLOCATE(arena, TableType, 1);
    (*table)->key = key;

    *inserted = true;
    return &(*table)->value;
}

#endif // INCLUDE_GUARD_HASH_TABLE_H

