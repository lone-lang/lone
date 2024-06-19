/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/types.h>

#include <lone/hash.h>
#include <lone/hash/fnv_1a.h>

void lone_hash_initialize(struct lone_system *system, struct lone_bytes random)
{
	lone_hash_fnv_1a_initialize(system, random);
}
