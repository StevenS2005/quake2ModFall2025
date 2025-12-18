// from this website https://web.archive.org/web/20051227025942/http://www.planetquake.com/qdevels/quake2/5_1_98.html


void fire_bfg(edict_t* self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius);
void fire_rocket(edict_t* self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius, int radius_damage);

//Sword weapon

void fire_sword(edict_t* self, vec3_t start, vec3_t aimdir, int damage, int kick, int range)
{
	trace_t tr;
	vec3_t end;

	VectorMA(start, range, aimdir, end);

	tr = gi.trace(self->s.origin, NULL, NULL, end, self, MASK_SHOT);

	if (!((tr.surface) && (tr.surface->flags & SURF_SKY)))
	{
		if (tr.fraction < 1.0)
		{
			if (tr.ent->takedamage)
			{
				T_Damage(tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, 0, MOD_UNKNOWN);
				gi.sound(self, CHAN_AUTO, gi.soundindex("misc/fhit3.wav"), 1, ATTN_NORM, 0);
			}
			else
			{
				// effect when hit a wall
				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(TE_SPARKS);
				gi.WritePosition(tr.endpos);
				gi.WriteDir(tr.plane.normal);
				gi.multicast(tr.endpos, MULTICAST_PVS);
				gi.sound(self, CHAN_AUTO, gi.soundindex("weapons/hgrenlb1b.wav"), 1, ATTN_NORM, 0);
			}
		}
	}
}

void sword_attack(edict_t* ent, vec3_t g_offset, int damage, int kick, int range)
{
	vec3_t  forward, right, start, offset;

	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 24, 8, ent->viewheight - 8);
	VectorAdd(offset, g_offset, offset);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale(forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	fire_sword(ent, start, forward, damage, kick, range);
}

void Weapon_Sword_Fire(edict_t* ent)
{
	// base stats
	int damage = 35;
	int kick = 200;
	int range = 45;

	if (ent->client->pers.sword_level >= 1) damage = 100;  // Level 1: Damage
	if (ent->client->pers.sword_level >= 2) kick = 800;   // Level 2: Kick
	if (ent->client->pers.sword_level >= 3) range = 200;   // Level 3: Range

	sword_attack(ent, vec3_origin, damage, kick, range);
	ent->client->ps.gunframe++;
}

void Weapon_Sword(edict_t* ent)
{
	static int pause_frames[] = { 19, 32, 0 };
	static int fire_frames[] = { 5, 0 };
	Weapon_Generic(ent, 4, 8, 52, 55, pause_frames, fire_frames, Weapon_Sword_Fire);
}

//magic staff
void Weapon_MagicStaff_Fire(edict_t* ent)
{
	vec3_t offset, start, forward, right, aim_dir;
	int damage = 80;
	int shots = 1;

	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorScale(forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	// Dual Shots
	if (ent->client->pers.staff_level >= 3) shots = 2;

	for (int i = 0; i < shots; i++)
	{
		VectorCopy(forward, aim_dir);

		if (shots == 2) {
			// Split balls
			if (i == 0) VectorMA(aim_dir, 0.15, right, aim_dir);  // Right
			if (i == 1) VectorMA(aim_dir, -0.15, right, aim_dir); // Left
		}

		VectorSet(offset, 8, 8, ent->viewheight - 8);
		P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

		// fire ball
		if (ent->client->pers.staff_level == 1)
		{
			fire_rocket(ent, start, aim_dir, 100, 650, 120, 100);
		}
		else if (ent->client->pers.staff_level == 2)
		{
			// ice; just different color i think and very fast
			fire_blaster(ent, start, aim_dir, 80, 1500, EF_BLUEHYPERBLASTER, true);
		}
		else
		{
			//base
			fire_bfg(ent, start, aim_dir, 80, 600, 200);
		}
	}

	ent->client->ps.gunframe++;

	// should play diff sounds based on type
	if (ent->client->pers.staff_level == 1)
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/rocklf1a.wav"), 1, ATTN_NORM, 0); 
	else if (ent->client->pers.staff_level == 2)
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/hyprbf1a.wav"), 1, ATTN_NORM, 0); 
	else
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/bfg__f1y.wav"), 1, ATTN_NORM, 0); 
}


void Weapon_MagicStaff(edict_t* ent)
{
	static int pause_frames[] = { 56, 0 };
	static int fire_frames[] = { 4, 0 };
	Weapon_Generic(ent, 3, 18, 56, 61, pause_frames, fire_frames, Weapon_MagicStaff_Fire);
}

// bow 

void arrow_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf)
{
	if (other == self->owner)
		return; // dont hit me
	if (surf && (surf->flags & SURF_SKY)) { 
		G_FreeEdict(self); // if hits the sky, make it disappear to keep the illusion
		return; 
	}

	//Explosion
	if (self->owner->client->pers.bow_level == 3) {
		T_RadiusDamage(self, self->owner, 100, other, 120, MOD_R_SPLASH);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_EXPLOSION1);
		gi.WritePosition(self->s.origin);
		gi.multicast(self->s.origin, MULTICAST_PVS);
	}

	// poision (more like just increased cause thats what it is since didnt have time to do like actual poision effect)
	if (self->owner->client->pers.bow_level >= 2) {
		self->dmg += 30;
	}

	if (other->takedamage) {
		T_Damage(other, self, self->owner, self->velocity, self->s.origin, plane->normal, self->dmg, 0, 0, MOD_UNKNOWN);
		gi.sound(self, CHAN_AUTO, gi.soundindex("misc/fhit3.wav"), 1, ATTN_NORM, 0);
	}
	else {
		gi.sound(self, CHAN_AUTO, gi.soundindex("weapons/grenlb1b.wav"), 1, ATTN_NORM, 0);
	}
	G_FreeEdict(self);
}

void fire_arrow(edict_t* self, vec3_t start, vec3_t dir, int damage, int speed)
{
	edict_t* arrow;

	arrow = G_Spawn();
	VectorCopy(start, arrow->s.origin);
	VectorCopy(dir, arrow->movedir);
	vectoangles(dir, arrow->s.angles);
	VectorScale(dir, speed, arrow->velocity);

	arrow->movetype = MOVETYPE_TOSS;
	arrow->clipmask = MASK_SHOT;
	arrow->solid = SOLID_BBOX;
	arrow->s.modelindex = gi.modelindex("models/objects/laser/tris.md2");
	arrow->owner = self;
	arrow->touch = arrow_touch;
	arrow->dmg = damage;
	arrow->classname = "arrow";
	arrow->nextthink = level.time + 10;
	arrow->think = G_FreeEdict; // if 10 seconds without hitting something free the arrow (i think)
	gi.linkentity(arrow);
}

void Weapon_Bow_Fire(edict_t* ent)
{
	vec3_t offset, start, forward, right, aim_dir;
	int damage = 50;
	int arrows = 1;

	AngleVectors(ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 8, 8, ent->viewheight - 8);
	P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);

	//triple shot
	if (ent->client->pers.bow_level >= 1) 
		arrows = 3;

	for (int i = 0; i < arrows; i++)
	{
		VectorCopy(forward, aim_dir);

		if (i == 1) VectorMA(aim_dir, 0.05, right, aim_dir); // Right
		if (i == 2) VectorMA(aim_dir, -0.05, right, aim_dir); // Left

		fire_arrow(ent, start, aim_dir, damage, 1200);
	}

	ent->client->ps.gunframe++;
	gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/rocklf1a.wav"), 1, ATTN_NORM, 0);
}

void Weapon_Bow(edict_t* ent)
{
	static int  pause_frames[] = { 22, 28, 34, 0 };

	static int  fire_frames[] = { 9, 0 };

	Weapon_Generic(ent, 7, 18, 36, 39, pause_frames, fire_frames, Weapon_Bow_Fire);
}